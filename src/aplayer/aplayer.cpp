/*
 Hive Audio Player
 Copyright (c) 2008-2017 Hive Solutions Lda.

 This file is part of Hive Audio Player.

 Hive Audio Player is free software: you can redistribute it and/or modify
 it under the terms of the Apache License as published by the Apache
 Foundation, either version 2.0 of the License, or (at your option) any
 later version.

 Hive Audio Player is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 Apache License for more details.

 You should have received a copy of the Apache License along with
 Hive Audio Player. If not, see <http://www.apache.org/licenses/>.

 __author__    = João Magalhães <joamag@hive.pt>
 __version__   = 1.0.0
 __revision__  = $LastChangedRevision$
 __date__      = $LastChangedDate$
 __copyright__ = Copyright (c) 2008-2017 Hive Solutions Lda.
 __license__   = Apache License, Version 2.0
*/

#include "stdafx.h"

#include "aplayer.h"

ao_device *init_ao(AVCodecContext *codec_ctx) {
    int driver = ao_default_driver_id();

    ao_sample_format sformat;
    AVSampleFormat sample_format = codec_ctx->sample_fmt;
    switch(sample_format) {
        case AV_SAMPLE_FMT_U8:
            sformat.bits = 8;
            break;

        case AV_SAMPLE_FMT_S16:
            sformat.bits = 16;
            break;

        case AV_SAMPLE_FMT_S32:
            sformat.bits = 32;
            break;

        default:
            sformat.bits = 8;
            break;
    }

    sformat.channels = codec_ctx->channels;
    sformat.rate = codec_ctx->sample_rate;
    sformat.byte_format = AO_FMT_NATIVE;
    sformat.matrix = 0;

    ao_device *device = ao_open_live(driver, &sformat, NULL);
    return device;
}

int register_aplayer() {
    // starts the audio relates structures, allowing
    // sound to be "played"
    ao_initialize();

    // starts the registration of the audio subsystem
    // by registering all the resources
    av_register_all();

#ifdef _DEBUG
    av_log_set_level(AV_LOG_DEBUG);
#else
    av_log_set_level(AV_LOG_ERROR);
#endif

    return 0;
}

int unregister_aplayer() {
    // shutdowns the audio related structures, this should
    // avoid leaks in that subsystem
    ao_shutdown();

    return 0;
}

int open_aplayer(const char *filename, struct aplayer_t *player) {
    // creates a new format (file container) context to be
    // used to detect the kind of file in use
    AVFormatContext *container = avformat_alloc_context();
    if(avformat_open_input(&container, filename, NULL, NULL) < 0) {
        WARN("Could not open file");
    }
    if(avformat_find_stream_info(container, NULL) < 0) {
        WARN("Could not find file info");
    }

#ifdef _DEBUG
    // dumps the format information to the standard outpu
    // this should print information on the container file
    av_dump_format(container, 0, filename, false);
#endif

    // starts the (audio) stream id with an invalid value and then
    // iterates over the complete set of stream to find one that complies
    // with the audio "interface"
    int stream_id = -1;
    for(unsigned int index = 0; index < container->nb_streams; index++) {
        // retrieves the current stram and checks if the
        // codec type is of type audio in case it's not
        // continues the loop (nothing to be done)
        AVStream *stream = container->streams[index];
        if(stream->codec->codec_type != AVMEDIA_TYPE_AUDIO) { continue; }

        // sets the current index as the stream identifier
        // to be used from now on
        stream_id = index;
        break;
    }
    if(stream_id == -1) { WARN("Could not find Audio Stream"); }

    // retrieves the codec context associted with the audio stream
    // that was just discovered
    AVCodecContext *codec_ctx = container->streams[stream_id]->codec;

    // tries to find the codec for the current codec context and
    // opens it for the current execution
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if(codec == NULL) { WARN("Cannot find codec"); }
    if(avcodec_open2(codec_ctx, codec, NULL) < 0) { WARN("Codec cannot be found"); }

    // initializes the ao structure creating the device associated
    // with the created structures this is going to be used
    ao_device *device = init_ao(codec_ctx);

    // allocates the buffer to be used in the packet for the
    // unpacking of the various packets
    uint8_t buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];

    // creates the packet structure to be used and initializes
    // it, this is going to be the payload for each iteration
    // then sets its data and size
    av_init_packet(&player->packet);
    player->packet.data = buffer;
    player->packet.size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;

    // allocates a new frame structure to be used for the audio
    // frames in iteration
    AVFrame *frame = avcodec_alloc_frame();

    // updates the player structure with all the attributes that
    // were retrieved for the current context
    player->device = device;
    player->stream_id = stream_id;
    player->frame = frame;
    player->container = container;
    player->codec_ctx = codec_ctx;

    return 0;
}

void close_aplayer(struct aplayer_t *player) {
    // unpacks the various attributes from the player
    // structure that are going to be used
    ao_device *device = player->device;
    AVFrame *frame = player->frame;
    AVFormatContext *container = player->container;
    AVCodecContext *codec_ctx = player->codec_ctx;

    // releases the structure that holds the frame
    // no need to used it anymore
    av_free(frame);

    // closes the av associated structures including
    // the codec and the container file
    avcodec_close(codec_ctx);
    avformat_close_input(&container);

    // closes the ao related structures that includes
    // the device and the global structures
    ao_close(device);
}

int play_aplayer(struct aplayer_t *player) {
    // unpacks the various attributes from the player
    // structure that are going to be used
    ao_device *device = player->device;
    int stream_id = player->stream_id;
    AVFrame *frame = player->frame;
    AVFormatContext *container = player->container;
    AVCodecContext *codec_ctx = player->codec_ctx;

    // sets the running flag so that the player starts
    // in that state
    player->running = 1;

    // initializes the flag indicating if the frame processing
    // has been finished and then iterates over the various packets
    // to try to decode the various frames
    int frame_finished = 0;

    for(int i = 0; i < 300; i++) {
    //while(player->running) {
        // reads a frame from the container file and check
        // if a valid one was returned in case not breaks
        // the loop (end of the file)
        int result = av_read_frame(container, &player->packet);
        if(result < 0) { break; }

        // checks if the stream index of the current packet
        // is the same as the just detected audio stream
        if(player->packet.stream_index != stream_id) { continue; }

        // decodes the current packet as an audio packed with
        // the current codec context and in case the frame is
        // not finished continues the loop, otherwise plays the
        // frame using the ao library
        avcodec_decode_audio4(codec_ctx, frame, &frame_finished, &player->packet);
        if(!frame_finished) { continue; }
        ao_play(device, (char *) frame->extended_data[0], frame->linesize[0]);
        av_free_packet(&player->packet);
    }

    return 0;
}

int pause_aplayer(struct aplayer_t *player) {
    player->running = 0;
    return 0;
}

int stop_aplayer(struct aplayer_t *player) {
    player->running = 0;
    av_seek_frame(player->container, player->stream_id, 0, AVSEEK_FLAG_BACKWARD);
    return 0;
}

int seek_aplayer(struct aplayer_t *player, int64_t timestamp) {
    AVRational rational = { 1, AV_TIME_BASE };
    AVStream *stream = player->container->streams[player->stream_id];

    timestamp = av_rescale_q(
        timestamp * AV_TIME_BASE, rational, stream->time_base
    );

    av_seek_frame(player->container, player->stream_id, timestamp, AVSEEK_FLAG_ANY);
    return 0;
}

int main(int argc, char **argv) {
    // in case not enough arguments have been provided
    // must exit the process immediately
    if(argc < 2) { DIE("A file name must be provided"); }

    // retrieves the name of the file to be executed
    // as the first argument from the command line
    const char *input_filename = argv[1];

    // registers the aplayer structures, starting both
    // the hardware and logical "items"
    register_aplayer();

    // starts the play operation using the provided
    // input filename as the file path to be used
    struct aplayer_t player;
    open_aplayer(input_filename, &player);
    play_aplayer(&player);
    seek_aplayer(&player, 160);
    play_aplayer(&player);
    close_aplayer(&player);

    // unregisters th aplayer structures, stopping both
    // the hardware and logical "items"
    unregister_aplayer();

    return 0;
}

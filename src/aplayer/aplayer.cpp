/*
 Hive Audio Player
 Copyright (C) 2008-2012 Hive Solutions Lda.

 This file is part of Hive Audio Player.

 Hive Audio Player is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Hive Audio Player is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Hive Audio Player. If not, see <http://www.gnu.org/licenses/>.

 __author__    = João Magalhães <joamag@hive.pt>
 __version__   = 1.0.0
 __revision__  = $LastChangedRevision$
 __date__      = $LastChangedDate$
 __copyright__ = Copyright (c) 2008-2012 Hive Solutions Lda.
 __license__   = GNU General Public License (GPL), Version 3
*/

#include "stdafx.h"

void die(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

int main(int argc, char **argv) {
	// in case not enough arguments have been provided
	// must exit the process immediately
	if(argc < 2) { die("A file name must be provided"); }

	// retrieves the name of the file to be executed
	// as the first argument from the command line
    const char *input_filename = argv[1];

	// starts the registration of the audio subsystem
	// by registering all the resources
	av_register_all();
	av_log_set_level(AV_LOG_ERROR);

	// creates a new format (file container) context to be
	// used to detect the kind of file in use
    AVFormatContext *container = avformat_alloc_context();
    if(avformat_open_input(&container, input_filename, NULL, NULL) < 0) {
        die("Could not open file");
    }
    if(avformat_find_stream_info(container, NULL) < 0) {
        die("Could not find file info");
    }

#ifdef _DEBUG
	// dumps the format information to the standard outpu
	// this should print information on the container file
    av_dump_format(container, 0, input_filename, false);
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
	if(stream_id == -1) { die("Could not find Audio Stream"); }

    AVCodecContext *codec_ctx = container->streams[stream_id]->codec;

	// tries to find the codec for the current codec context and
	// opens it for the current execution
    AVCodec *codec = avcodec_find_decoder(codec_ctx->codec_id);
    if(codec == NULL) { die("cannot find codec!"); }
    if(avcodec_open(codec_ctx, codec) < 0) { die("Codec cannot be found"); }

    //initialize AO lib
    ao_initialize();
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

    ao_device *adevice=ao_open_live(driver,&sformat,NULL);
    //end of init AO LIB

	// allocates the buffer to be used in the packet for the
	// unpacking of the various packets
	uint8_t buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];

	// creates the packet structure to be used and initializes
	// it, this is going to be the payload for each iteration
	// then sets its data and size
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = buffer;
    packet.size = AVCODEC_MAX_AUDIO_FRAME_SIZE + FF_INPUT_BUFFER_PADDING_SIZE;

	// allocates a new frame structure to be used for the audio
	// frames in iteration
	AVFrame *frame = avcodec_alloc_frame();

    int frameFinished = 0;
    while(1) {
		// reads a frame from the container file and check
		// if a valid one was returned in case not breaks
		// the loop (end of the file)
		int result = av_read_frame(container, &packet);
		if(result < 0) { break; }

		// checks if the stream index of the current packet
		// is the same as the just detected audion stream
		if(packet.stream_index != stream_id) { continue; }

		// decodes the current packet as an audio packed with
		// the current codec context and in case the frame is
		// not finished continues the loop, otherwise plays the
		// frame using the ao library
        int len = avcodec_decode_audio4(codec_ctx, frame, &frameFinished, &packet);
		if(!frameFinished) { continue; }
		ao_play(adevice, (char *) frame->extended_data[0], frame->linesize[0]);
    }

    av_close_input_file(container);
    ao_shutdown();
    return 0;
}
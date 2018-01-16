/*
 Hive Audio Player
 Copyright (c) 2008-2018 Hive Solutions Lda.

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
 __copyright__ = Copyright (c) 2008-2018 Hive Solutions Lda.
 __license__   = Apache License, Version 2.0
*/

#pragma once

#define DIE(msg) fprintf(stderr, "%s\n", msg);\
    exit(1)

#define WARN(msg) fprintf(stderr, "%s\n", msg);\
    return 1

typedef struct aplayer_t {
    /**
     * The reference to the logical device to be used
     * in the playing of the sound.
     */
    ao_device *device;

    /**
     * The identifier of the "first" stream that contains
     * an audio track.
     */
    int stream_id;

    /**
     * The packet structure value to be used in each of the
     * iterations to store packet information.
     */
    AVPacket packet;

    /**
     * The reference to the frame structure value to be used in
     * each of the iterations to store frame information.
     */
    AVFrame *frame;

    /**
     * The reference to the container structure that contains
     * information on the container file.
     */
    AVFormatContext *container;

    /**
     * The current codec context to be used that stores
     * information on the current status, position and codec.
     */
    AVCodecContext *codec_ctx;

    /**
     * Flag that controlls if the player is currently running
     * note that the changing of this value must be atomic.
     */
    int running;
} aplayer;

int register_aplayer();
int unregister_aplayer();
int open_aplayer(const char *filename, struct aplayer_t *player);
void close_aplayer(struct aplayer_t *player);
int play_aplayer(struct aplayer_t *player);
int pause_aplayer(struct aplayer_t *player);
int stop_aplayer(struct aplayer_t *player);
int seek_aplayer(struct aplayer_t *player, int64_t timestamp);

/*
 * Copyright 2018 Google LLC. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
typedef struct AVX_Decoder      AVX_Decoder;
typedef struct AVX_Video_Frame  AVX_Video_Frame;

typedef struct DATA_Source      DATA_Source;

#define NUM_FRAMES_BUFFERED     10

/* EXTERNALLY VISIBLE (From WASM) API */
/**
 *  Instantiate an instance of our decoder implementation
 *
 * @return
 *      An instance of the AV1 decoder, NULL on error.
 */
AVX_Decoder     *AVX_Decoder_new(void);
/**
 *  Free all memory and destroy our decoder instance
 */
void            AVX_Decoder_destroy(AVX_Decoder *);
/**
 *  Set the source data stream for the codec
 */
void            AVX_Decoder_set_source(AVX_Decoder *ad, DATA_Source *ds);
/**
 *  Run the decoder - this reads and decodes frames into
 *  internal buffers. Can run for a long time, call with care.
 */
void            AVX_Decoder_run(AVX_Decoder *ad);
/**
 *  Get a frame of video.
 *  Note, this buffers NUM_FRAMES_BUFFERED on initial load, and
 *  after every failed call to 'AVX_Decoder_run()', in normal
 *  operation when the incoming data is keeping up with playback
 *  speed, this should return a valid frame on every call until
 *  end of video, and then return NULL.
 *
 * @return
 *      If we have decoded frames to display return the next one.
 *      NULL if buffering or nothing to return or end of video.
 */
AVX_Video_Frame *AVX_Decoder_get_frame(AVX_Decoder *ad);
/**
 *  Is the video finished (no more frames to display).
 *  If AVX_Decoder_get_frame() is returning NULL and the
 *  video isn't finished, then showing a spinner could
 *  be nice:-)
 *
 * @return
 *      true if the video stream has been completely read,
 *      false if there is more to decode/more frames buffered.
 */
int             AVX_Decoder_video_finished(AVX_Decoder *ad);

int             AVX_Decoder_get_width(AVX_Decoder *ad);
int             AVX_Decoder_get_height(AVX_Decoder *ad);

float           AVX_Video_Frame_get_time(AVX_Video_Frame *avf);
void            *AVX_Video_Frame_get_buffer(AVX_Video_Frame *avf);
size_t          AVX_Video_Frame_get_size(AVX_Video_Frame *avf);

/* INTERNAL API TO MIMIC Stream I/O for raw bytes (e.g. FILE *) */

DATA_Source     *DS_open(const char *what);
size_t          DS_read(DATA_Source *ds, unsigned char *buf, size_t bytes);
int             DS_empty(DATA_Source *ds);
void            DS_close(DATA_Source *ds);
// Helper function for blob support
void            DS_set_blob(DATA_Source *ds, void *buf, size_t len);

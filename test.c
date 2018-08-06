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
#include <stdlib.h>
#include <stdio.h>

#include "decode-av1-priv.h"

static void
dump_raw_frame(AVX_Video_Frame *avf, int id) {
    FILE    *f;
    char    name[256];
    size_t  size;
    void    *buf;

    sprintf(name, "frame%04d.yuv", id);
    if ((f = fopen(name, "wb")) == NULL) {
        return;
    }
    buf = AVX_Video_Frame_get_buffer(avf);
    size = AVX_Video_Frame_get_size(avf);
    fwrite(buf, size, 1, f);
    fclose(f);
}

int
main(int argc, char *argv[]) {
    AVX_Decoder     *ad;

    ad = AVX_Decoder_new();
    if (ad == NULL) {
        exit(1);
    }
    /* Set the source file, if it exists */
    if (argc > 1) {
        DATA_Source     *ds;

        ds = DS_open(argv[1]);
        if (ds != NULL) {
            AVX_Decoder_set_source(ad, ds);
            // Run the decoder at the start to read the headers at least
            AVX_Decoder_run(ad);
            //
            printf("Video has width %d, height %d\n", AVX_Decoder_get_width(ad), AVX_Decoder_get_height(ad));
            // Decode some frames
            while (AVX_Decoder_video_finished(ad) == 0) {
                AVX_Video_Frame     *af;

                if ((af = AVX_Decoder_get_frame(ad)) != NULL) {
                    // emit a few frames as YUV files
                    static int     i = 0;
                    
                    ++i;
                    if (30 <= i && i < 40) {
                        dump_raw_frame(af, i);
                    }
                }
                /*
                 * Run the decoder every time, so that we keep
                 * the number of buffered frames full if there's
                 * more input data to process.
                 */
                AVX_Decoder_run(ad);
            }
        }
    }
    AVX_Decoder_destroy(ad);

    return 0;
}

/* Implementation of the DATA_Source API that reads a file */
DATA_Source *
DS_open(const char *what) {
    return (DATA_Source *)fopen(what, "rb");
}

size_t
DS_read(DATA_Source *ds, unsigned char *buf, size_t bytes) {
    return fread(buf, 1, bytes, (FILE *)ds);
}

int
DS_empty(DATA_Source *ds) {
    return feof((FILE *)ds);
}

void
DS_close(DATA_Source *ds) {
    fclose((FILE *)ds);
}

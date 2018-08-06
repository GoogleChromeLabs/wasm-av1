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
/*
 *  This file contains the interface visible from JS into the Wasm module hosting our
 *  AV1 decoder. The top section contains the API itself. The last section of this
 *  source contains glue code that deals with the encoded data. Note, the AV1
 *  reference source examples assume a FILE as input. Here we instead abstract
 *  a streams like interface to read the compressed data, which can then be
 *  interfaced to a file, blob or an implementation that supports blocking on
 *  a streamed data source (e.g. from the Streams API: https://streams.spec.whatwg.org/)
 */
#include <stdlib.h>
#include <string.h>

#include "aom/aom_decoder.h"
#include "aom/aomdx.h"

#include "aom_ports/mem_ops.h"

#include "decode-av1-priv.h"

#include "video_common.h"

struct AVX_Decoder {
    AvxVideoInfo        ad_Info;        /**< Codec defined video info       */
    aom_codec_iface_t   *ad_Interface;
    aom_codec_ctx_t     ad_Codec;       /**< AV1 internal opaque type       */
    aom_codec_iter_t    ad_Iterator;
    aom_image_t         *ad_Image;
    DATA_Source         *ad_Source;     /**< Hangs on to the video stream   */
    AVX_Video_Frame     *ad_Frames[NUM_FRAMES_BUFFERED];
    AVX_Video_Frame     *ad_LastFrame;  /**< Last returned one, for freeing */
    int                 ad_NumBuffered;
    uint8_t             *ad_InputBuffer;
    size_t              ad_InputBufferSize;
    size_t              ad_FrameSize;
    int                 ad_IsInitialized;
};

struct AVX_Video_Frame {
    float           avf_TimeStamp;  /**< Time for this frame            */
    void            *avf_Buffer;    /**< Pointer to the decoded frame   */
    size_t          avf_BufferSize; /**< Bytes for the stored frame     */
};

/*
 *  The externally visible API starts here
 */
AVX_Decoder *
AVX_Decoder_new(void) {
    AVX_Decoder     *ad;

    if ((ad = malloc(sizeof *ad)) != NULL) {
        memset(ad, 0, sizeof *ad);
    }
    return ad;
}

void
AVX_Decoder_destroy(AVX_Decoder *ad) {
    if (ad) {
        int     i;

        DS_close(ad->ad_Source);
        if (ad->ad_LastFrame != NULL) {     // Strictly speaking, don't need the test
            free(ad->ad_LastFrame);
        }
        for (i = 0; i < ad->ad_NumBuffered; i++) {
            free(ad->ad_Frames[i]);
        }
        (void)aom_codec_destroy(&ad->ad_Codec);
        if (ad->ad_InputBuffer != NULL) {
            free(ad->ad_InputBuffer);
        }
        free(ad);
    }
}

#define ZOF_HEADER  32

static const char *const kIVFSignature = "DKIF";

/**
 *  Helper function to set up the codec when starting to
 *  read the stream.
 */
static void
init_avx(AVX_Decoder *ad) {
    uint8_t     header[ZOF_HEADER];

    // Check we have a data source!
    if (ad->ad_Source == NULL) {
        return;
    }
    // Read the header, and check the signature
    DS_read(ad->ad_Source, header, ZOF_HEADER);
    if (memcmp(kIVFSignature, header, 4) != 0) {    // Check IVF signature
        return;
    }
    if (mem_get_le16(header + 4) != 0) {            // Check version
        return;
    }
    // Fill in AvxVideoInfo from the header
    ad->ad_Info.codec_fourcc = mem_get_le32(header + 8);
    ad->ad_Info.frame_width = mem_get_le16(header + 12);
    ad->ad_Info.frame_height = mem_get_le16(header + 14);
    ad->ad_Info.time_base.numerator = mem_get_le32(header + 16);
    ad->ad_Info.time_base.denominator = mem_get_le32(header + 20);
    // We know we're only looking for the AV1 decoder interface
    ad->ad_Interface = aom_codec_av1_dx();
    // Initialize the codec itself
    if (aom_codec_dec_init(&ad->ad_Codec, ad->ad_Interface, NULL, 0)) {
        return;
    }
    // Mark the decoder as being set up
    ad->ad_IsInitialized = !0;
}

void
AVX_Decoder_set_source(AVX_Decoder *ad, DATA_Source *ds) {
    ad->ad_Source = ds;
}

#define MAX_FRAME_SZ        (256 * 1024 * 1024)
#define ZOF_YUV             3

static void
buffer_frame(AVX_Decoder *ad) {
    AVX_Video_Frame     *avf;
    aom_image_t         *img;
    unsigned char       *p;
    size_t              z;
    int                 plane;

    z = sizeof (AVX_Video_Frame) + ZOF_YUV * ad->ad_Info.frame_width * ad->ad_Info.frame_height;
    if ((avf = (AVX_Video_Frame *)malloc(z)) == NULL) {
        return;
    }
    avf->avf_Buffer = (void *)(avf + 1);
    avf->avf_TimeStamp = 0.0; // TODO
    img = ad->ad_Image;
    p = (unsigned char *)avf->avf_Buffer;
    // Copy the YUV planes into our buffered set
    for (plane = 0; plane < 3; ++plane) {
        const unsigned char *buf = img->planes[plane];
        const int stride = img->stride[plane];
        const int w = aom_img_plane_width(img, plane) *
                      ((img->fmt & AOM_IMG_FMT_HIGHBITDEPTH) ? 2 : 1);
        const int h = aom_img_plane_height(img, plane);
        int y;

        for (y = 0; y < h; ++y) {
            memcpy(p, buf, w);
            p += w;
            buf += stride;
        }
    }
    // Store how much we wrote (sub-sampled YUV will be smaller than the alloc)
    avf->avf_BufferSize = p - (unsigned char *)avf->avf_Buffer;
    // Append the buffered frame
    ad->ad_Frames[ad->ad_NumBuffered] = avf;
    ad->ad_NumBuffered++;
}

void
AVX_Decoder_run(AVX_Decoder *ad) {
    uint8_t     raw_header[IVF_FRAME_HDR_SZ] = { 0 };

    if (ad->ad_IsInitialized == 0) {
        init_avx(ad);
    }
    if (ad->ad_NumBuffered >= NUM_FRAMES_BUFFERED) {
        return;
    }
    // If there's no image from previous calls means we need to read a compressed frame
    if (ad->ad_Image == NULL) {
        size_t frame_size = 0;
        size_t got;

        if (DS_read(ad->ad_Source, raw_header, IVF_FRAME_HDR_SZ) != IVF_FRAME_HDR_SZ) {
            return;
        }
        frame_size = mem_get_le32(raw_header);
        if (frame_size > MAX_FRAME_SZ) {
            // Report error...
            return;
        }
        // Make sure we have memory to store the frame
        if (ad->ad_InputBufferSize < frame_size) {
            ad->ad_InputBuffer = realloc(ad->ad_InputBuffer, 2 * frame_size);
            if (ad->ad_InputBuffer == NULL) {
                ad->ad_InputBufferSize = 0;
                return;
            }
            ad->ad_InputBufferSize = 2 * frame_size;
            ad->ad_FrameSize = frame_size;
        }
        got = DS_read(ad->ad_Source, ad->ad_InputBuffer, frame_size);
        if (got < frame_size) {
            return;
        }
        // Start the decompress of the frame
        if (aom_codec_decode(&ad->ad_Codec, ad->ad_InputBuffer, got, NULL)) {
            return;
        }
    }
    // Try to decode an image from the compressed stream, buffer as needed
    while (ad->ad_NumBuffered < NUM_FRAMES_BUFFERED) {
        ad->ad_Image = aom_codec_get_frame(&ad->ad_Codec, &ad->ad_Iterator);
        if (ad->ad_Image == NULL) {
            break;
        }
        else {
            buffer_frame(ad);
        }
    }
}

AVX_Video_Frame *
AVX_Decoder_get_frame(AVX_Decoder *ad) {
    // free up the memory from any previous frame returned
    if (ad->ad_LastFrame != NULL) {
        free(ad->ad_LastFrame);
        ad->ad_LastFrame = NULL;
    }
    if (ad->ad_NumBuffered > 0) {
        AVX_Video_Frame     *frame;

        frame = ad->ad_Frames[0];
        ad->ad_NumBuffered--;
        if (ad->ad_NumBuffered > 0) {
            memmove(ad->ad_Frames, &ad->ad_Frames[1],
                    ad->ad_NumBuffered * sizeof (AVX_Video_Frame *));
        }
        // Hang on to this refernce so we can free the memory
        ad->ad_LastFrame = frame;
        return frame;
    }
    return NULL;
}

int
AVX_Decoder_video_finished(AVX_Decoder *ad) {
    return DS_empty(ad->ad_Source);
}

int
AVX_Decoder_get_width(AVX_Decoder *ad) {
    if (ad == NULL) {
        return 0;
    }
    return ad->ad_Info.frame_width;
}

int
AVX_Decoder_get_height(AVX_Decoder *ad) {
    if (ad == NULL) {
        return 0;
    }
    return ad->ad_Info.frame_height;
}

float
AVX_Video_Frame_get_time(AVX_Video_Frame *avf) {
    if (avf == NULL) {
        return 0.0;
    }
    return avf->avf_TimeStamp;
}

void *
AVX_Video_Frame_get_buffer(AVX_Video_Frame *avf) {
    if (avf == NULL) {
        return NULL;
    }
    return avf->avf_Buffer;
}

size_t
AVX_Video_Frame_get_size(AVX_Video_Frame *avf)
{
    if (avf == NULL) {
        return 0;
    }
    return avf->avf_BufferSize;
}
/*
 *  Internal glue to the AV1 codec library from here onwards
 */

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
#include <string.h>

#include "decode-av1-priv.h"

/* Implementation of the DATA_Source API that processes a binary blob */
struct DATA_Source {
    void        *ds_Buf;
    size_t      ds_Len;
    size_t      ds_Pos;
};

DATA_Source *
DS_open(const char *what) {
    DATA_Source     *ds;

    ds = malloc(sizeof *ds);
    if (ds != NULL) {
        memset(ds, 0, sizeof *ds);
    }
    return ds;
}

size_t
DS_read(DATA_Source *ds, unsigned char *buf, size_t bytes) {
    if (DS_empty(ds) || buf == NULL) {
        return 0;
    }
    if (bytes > (ds->ds_Len - ds->ds_Pos)) {
        bytes = ds->ds_Len - ds->ds_Pos;
    }
    memcpy(buf, &ds->ds_Buf[ds->ds_Pos], bytes);
    ds->ds_Pos += bytes;

    return bytes;
}

int
DS_empty(DATA_Source *ds) {
    return ds->ds_Pos >= ds->ds_Len;
}

void
DS_close(DATA_Source *ds) {
    free(ds);
}

void
DS_set_blob(DATA_Source *ds, void *buf, size_t len) {
    ds->ds_Buf = buf;
    ds->ds_Len = len;
    ds->ds_Pos = 0;
}

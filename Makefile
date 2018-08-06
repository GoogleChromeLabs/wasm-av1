#
# Copyright 2018 Google LLC. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
SRCS=decode-av1.c

AOMDIR=third_party/aom

LIBDIR=third_party/embuild

INC=-I $(AOMDIR) -I $(LIBDIR)

X86LIBDIR=third_party/build

LIB=aom

LIBNAME=lib$(LIB).a

EMLIBAV1=$(LIBDIR)/$(LIBNAME)

X86LIBAV1=$(X86LIBDIR)/$(LIBNAME)

TARGET=decode-av1.js

TESTTARGET=testavx

DEPS=$(SRCS) Makefile

all: $(TARGET)

$(TARGET): $(DEPS) blob-api.c yuv-to-rgb.c $(EMLIBAV1)
		emcc -o $@ -O3 -s WASM=1 \
				-s ALLOW_MEMORY_GROWTH=1 \
				-s EXPORTED_FUNCTIONS="['_AVX_Decoder_new', \
										'_AVX_Decoder_destroy', \
										'_AVX_Decoder_set_source', \
										'_AVX_Decoder_run', \
										'_AVX_Decoder_get_width', \
										'_AVX_Decoder_get_height', \
										'_AVX_Decoder_video_finished', \
										'_AVX_Decoder_get_frame', \
										'_AVX_Video_Frame_get_buffer', \
										'_AVX_YUV_to_RGB', \
										'_DS_open', \
										'_DS_close', \
										'_DS_set_blob', \
										'_malloc', '_free' \
									   ]" \
				blob-api.c yuv-to-rgb.c $(SRCS) $(INC) -L $(LIBDIR) -l$(LIB)

$(TESTTARGET): test.c $(DEPS) $(X86LIBAV1)
		cc -o $@ -O3 test.c $(SRCS) $(INC) -L $(X86LIBDIR) -l$(LIB)

$(TESTTARGET)g: test.c $(DEPS) $(X86LIBAV1)
		cc -o $@ -g test.c $(SRCS) $(INC) -L $(X86LIBDIR) -l$(LIB)

clean:
		-rm $(TARGET) $(TESTTARGET) $(TESTTARGET)g

$(EMLIBAV1): $(LIBDIR)
		(cd $(LIBDIR); \
			cmake  `pwd | sed 's?$(LIBDIR)?$(AOMDIR)?'` \
		        -DENABLE_CCACHE=1 \
		        -DAOM_TARGET_CPU=generic \
		        -DENABLE_DOCS=0 \
		        -DCONFIG_ACCOUNTING=1 \
		        -DCONFIG_INSPECTION=1 \
		        -DCONFIG_MULTITHREAD=0 \
		        -DCONFIG_RUNTIME_CPU_DETECT=0 \
		        -DCONFIG_UNIT_TESTS=0 \
		        -DCONFIG_WEBM_IO=0 \
		        -DCMAKE_TOOLCHAIN_FILE=`../../get-emcmake.sh`; \
			make \
		)

$(X86LIBAV1): $(X86LIBDIR)
		(cd $(X86LIBDIR); cmake  `pwd | sed 's?$(X86LIBDIR)?$(AOMDIR)?'`; make)

$(LIBDIR):
		mkdir $(LIBDIR)

$(X86LIBDIR):
		mkdir $(X86LIBDIR)

#
# Copyright 2017 Google LLC. All rights reserved.
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
IJG_DIR=third_party/jpeg-7

IJG_SRCS =\
	$(IJG_DIR)/jaricom.c $(IJG_DIR)/jcapimin.c $(IJG_DIR)/jcapistd.c \
	$(IJG_DIR)/jcarith.c $(IJG_DIR)/jccoefct.c $(IJG_DIR)/jccolor.c \
        $(IJG_DIR)/jcdctmgr.c $(IJG_DIR)/jchuff.c $(IJG_DIR)/jcinit.c \
        $(IJG_DIR)/jcmainct.c $(IJG_DIR)/jcmarker.c $(IJG_DIR)/jcmaster.c \
        $(IJG_DIR)/jcomapi.c $(IJG_DIR)/jcparam.c $(IJG_DIR)/jcprepct.c \
        $(IJG_DIR)/jcsample.c $(IJG_DIR)/jctrans.c $(IJG_DIR)/jdapimin.c \
        $(IJG_DIR)/jdapistd.c $(IJG_DIR)/jdarith.c $(IJG_DIR)/jdatadst.c \
        $(IJG_DIR)/jdatasrc.c $(IJG_DIR)/jdcoefct.c $(IJG_DIR)/jdcolor.c \
        $(IJG_DIR)/jddctmgr.c $(IJG_DIR)/jdhuff.c $(IJG_DIR)/jdinput.c \
        $(IJG_DIR)/jdmainct.c $(IJG_DIR)/jdmarker.c $(IJG_DIR)/jdmaster.c \
        $(IJG_DIR)/jdmerge.c $(IJG_DIR)/jdpostct.c $(IJG_DIR)/jdsample.c \
        $(IJG_DIR)/jdtrans.c $(IJG_DIR)/jerror.c $(IJG_DIR)/jfdctflt.c \
        $(IJG_DIR)/jfdctfst.c $(IJG_DIR)/jfdctint.c $(IJG_DIR)/jidctflt.c \
        $(IJG_DIR)/jidctfst.c $(IJG_DIR)/jidctint.c $(IJG_DIR)/jquant1.c \
        $(IJG_DIR)/jquant2.c $(IJG_DIR)/jutils.c $(IJG_DIR)/jmemmgr.c \
        $(IJG_DIR)/jmemansi.c

SRCS=$(IJG_SRCS) jpgglue.c jpgtranscode.c

all: jpgsquash.js

jpgsquash.js: $(SRCS) Makefile
	emcc -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 \
		-s EXPORTED_FUNCTIONS="['_jpg_transcode']" \
		-Wno-shift-negative-value \
		-o jpgsquash.js $(SRCS)

transcode: $(SRCS) main.c
	cc -o transcode $(SRCS) main.c

#!/bin/sh
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
for i in *.yuv
do
    OUTFILE=`echo $i | sed 's/.yuv/.png/'`
    convert -size 1280x720 -sampling-factor 4:2:0 -depth 16 $i $OUTFILE
done

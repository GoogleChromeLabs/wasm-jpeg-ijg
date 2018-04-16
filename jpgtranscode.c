/*
 * Copyright 2017 Google LLC. All rights reserved.
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
 * Convert input JPG to output JPG at set quality - brute force, overwrites original.
 */
#include <stdlib.h>
#include <string.h>

#include "jpgtranscode-priv.h"

/*
 * NB: This overwrites 'buffer'
 */
int
jpg_transcode(unsigned char *buffer, int len, int quality) {
    void          *out;
    IJG_Private   ip;

    // get sizes
    jpeg_memory_dimensions(buffer, len, &ip.ip_Width, &ip.ip_Height);
    out = malloc(ip.ip_Width * ip.ip_Height * 4);
    if (out == NULL)
      return 0;
    ip.ip_SrcBuf = buffer;
    ip.ip_SrcLen = len;
    ip.ip_DstBuf = out;
    ip.ip_CompBuf = malloc(len);
    ip.ip_CompSize = len;
    if (ip.ip_CompBuf == NULL)
      return 0;

    load_jpeg_data(&ip);
    // We should have a decompressed image in 'out', then recompress into ip_CompBuf
    jpeg_compress(&ip, quality);
    free(out);
    memcpy(buffer, ip.ip_CompBuf, len);
    free(ip.ip_CompBuf);

    return ip.ip_ReCompSize;
}

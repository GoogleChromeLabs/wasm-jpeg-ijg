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
 * NB: derived from IJG example code for filesystem decode/encode, hence refer to
 * the README in the jpeg-7 sub-directory and so the original example code is:
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * Modified 2009 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "third_party/jpeg-7/jpeglib.h"
#include "third_party/jpeg-7/jerror.h"

#include "jpgtranscode-priv.h"

#ifndef SIZEOF
#define SIZEOF(a)        sizeof(a)
#endif

/* Expanded data source object for stdio input */

typedef struct {
  struct  jpeg_source_mgr pub;  /* public fields */

  FILE    *infile;              /* source stream */
  JOCTET  *buffer;              /* start of buffer */
  int     len;
  JOCTET  eoi_buf[2];           /* in case empty buffer passed */
  boolean start_of_data;        /* have we gotten any data yet? */
} my_source_mgr;

typedef my_source_mgr   *my_src_ptr;

#define INPUT_BUF_SIZE  4096    /* choose an efficiently fread'able size */

/*
 * Initialize source --- called by jpeg_read_header
 * before any data is actually read.
 */

METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  /* We reset the empty-input-file flag for each image,
   * but we don't clear the input buffer.
   * This is correct behavior for reading a series of images from one source.
   */
  src->start_of_data = TRUE;
}

/*
 * Fill the input buffer --- called whenever buffer is emptied.
 *
 * In typical applications, this should read fresh data into the buffer
 * (ignoring the current state of next_input_byte & bytes_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been reloaded.  It is not necessary to
 * fill the buffer entirely, only to obtain at least one more byte.
 *
 * There is no such thing as an EOF return.  If the end of the file has been
 * reached, the routine has a choice of ERREXIT() or inserting fake data into
 * the buffer.  In most cases, generating a warning message and inserting a
 * fake EOI marker is the best course of action --- this will allow the
 * decompressor to output however much of the image is there.  However,
 * the resulting error message is misleading if the real problem is an empty
 * input file, so we handle that case specially.
 *
 * In applications that need to be able to suspend compression due to input
 * not being available yet, a FALSE return indicates that no more data can be
 * obtained right now, but more may be forthcoming later.  In this situation,
 * the decompressor will return to its caller (with an indication of the
 * number of scanlines it has read, if any).  The application should resume
 * decompression after it has loaded more data into the input buffer.  Note
 * that there are substantial restrictions on the use of suspension --- see
 * the documentation.
 *
 * When suspending, the decompressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_input_byte & bytes_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point must be rescanned after resumption, so move it to
 * the front of the buffer rather than discarding it.
 */

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  if (src->start_of_data == FALSE)        /* Treat empty input file as fatal error */
  {
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    src->eoi_buf[0] = (JOCTET) 0xFF;
    src->eoi_buf[1] = (JOCTET) JPEG_EOI;
    src->pub.next_input_byte = src->eoi_buf;
    src->pub.bytes_in_buffer = 2;
  }
  else
  {
    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = src->len;
    src->start_of_data = FALSE;
  }

  return TRUE;
}

/*
 * Skip data --- used to skip over a potentially large amount of
 * uninteresting data (such as an APPn marker).
 *
 * Writers of suspendable-input applications must note that skip_input_data
 * is not granted the right to give a suspension return.  If the skip extends
 * beyond the data currently in the buffer, the buffer can be marked empty so
 * that the next read will cause a fill_input_buffer call that can suspend.
 * Arranging for additional bytes to be discarded before reloading the input
 * buffer is the application writer's problem.
 */

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  my_src_ptr src = (my_src_ptr) cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  src->pub.next_input_byte += (size_t) num_bytes;
  src->pub.bytes_in_buffer -= (size_t) num_bytes;
}


/*
 * An additional method that can be provided by data source modules is the
 * resync_to_restart method for error recovery in the presence of RST markers.
 * For the moment, this source module just uses the default resync method
 * provided by the JPEG library.  That method assumes that no backtracking
 * is possible.
 */

/*
 * Terminate source --- called by jpeg_finish_decompress
 * after all data has been read.  Often a no-op.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */

METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

/*
 * Prepare for input from memory.
 */
GLOBAL(void)
jpeg_memory_src (j_decompress_ptr cinfo, void * indata, int len)
{
  my_src_ptr src;

  if (cinfo->src == NULL) {        /* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                  SIZEOF(my_source_mgr));
    src = (my_src_ptr) cinfo->src;
    src->buffer = (JOCTET *)indata;
    src->len = len;
  }

  src = (my_src_ptr) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->pub.next_input_byte = NULL; /* until buffer loaded */
}

//--

// Most of below is (C) Alex Danilo, Abbra pre-Google. modified for demo purposes
struct my_error_mgr
{
  struct jpeg_error_mgr pub;                /* "public" fields */
};

typedef struct my_error_mgr *my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */
METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);
}
/*
 *  No prototype for this in the IJG headers!
 */
void    jpeg_memory_src (j_decompress_ptr cinfo, void * indata, int len);

#define PO_RED        0
#define PO_GREEN      1
#define PO_BLUE       2

/**
 *  Utility routine that decompresses the JPEG image.
 */
void
load_jpeg_data(IJG_Private *ip)
{
    /* This struct contains the JPEG decompression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     */
    struct jpeg_decompress_struct   cinfo;
    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct my_error_mgr             jerr;
    /* More stuff */
    JSAMPARRAY                      buffer;         /* Output row buffer */
    int                             row_stride;     /* physical row width in output buffer */
    unsigned char                   *pRow, *p, *q;
    int                             bitsPerPixel, bytesPerRow;
    unsigned int                    i;

    /*
     *  We need something to decompress!
     */
    if (ip->ip_SrcLen == 0)
        return;
    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source */

    jpeg_memory_src(&cinfo, ip->ip_SrcBuf, ip->ip_SrcLen);

    /* Step 3: read image parameters with jpeg_read_header() */

    jpeg_read_header(&cinfo, TRUE);
    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the memory data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
     */

    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */
    bitsPerPixel = 24;              /* Default to RGB 8 bits/component  */
    /* Step 5: Start decompressor */

    jpeg_start_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the memory data source.
     */

    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available, as well as the output colormap
     * if we asked for color quantization.
     * In this example, we need to make an output work buffer of the right size.
     */ 
    /* JSAMPLEs per row in output buffer */
    bytesPerRow = cinfo.output_width * 3; // STRIDE!!!!! TOCHECK
    pRow = ip->ip_DstBuf;

    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     */
    while (cinfo.output_scanline < cinfo.output_height)
    {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        jpeg_read_scanlines(&cinfo, buffer, 1);
        /* Assume put_scanline_someplace wants a pointer and sample count. */
        if (bitsPerPixel == 8)  //ZZ TODO Remove this? We don't do 8bpp
        {
            memcpy(pRow, buffer[0], cinfo.output_width);
        }
        else
        {
            p = pRow;
            q = buffer[0];
            switch (cinfo.output_components)
            {
            case 1:
                for (i = 0; i < cinfo.output_width; i++, p += 3, q++)
                {
                    p[PO_RED] = q[0];
                    p[PO_GREEN] = q[0];
                    p[PO_BLUE] = q[0];
                }
                break;
            case 3:
                if (PO_RED == 0)    /* Means it's RGB just like the decoded image   */
                {
                    memcpy(p, q, cinfo.output_width * 3);
                }
                else                /* Unpacked JPEG pixels are RGB                 */
                {
                    for (i = 0; i < cinfo.output_width; i++, p += 3, q += 3)
                    {
                        p[PO_RED] = q[0];
                        p[PO_GREEN] = q[1];
                        p[PO_BLUE] = q[2];
                    }
                }
                break;
            }
        }
        pRow += bytesPerRow;
    }

    /* Step 7: Finish decompression */

    jpeg_finish_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the memory data source.
     */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */

    /* And we're done! */
}

/**
 *  Get the native size of a buffered JPEG image
 */
void
jpeg_memory_dimensions(void *indata, int len, int *w, int *h)
{
    struct my_error_mgr             jerr;
    struct jpeg_decompress_struct   cinfo;
    int                             bitsPerPixel;

    if (len == 0)
    {
        *w = *h = 100;  /* This is a hack to comply with CSS defaulting to 100px. */
        return;
    }
    bitsPerPixel = 24;
    *w = *h = 0;
    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */
    jpeg_memory_src(&cinfo, indata, len);

    /* Step 3: read file parameters with jpeg_read_header() */

    jpeg_read_header(&cinfo, TRUE);
    if (cinfo.num_components == 1) // lets not waste space on a mono bitmap
        bitsPerPixel = 8;

    if (bitsPerPixel == 8)  //ZZ TODO can probably chuck this for sizing!
    {
        cinfo.quantize_colors = TRUE;
        cinfo.desired_number_of_colors = 256;
        cinfo.colormap = NULL;
        cinfo.two_pass_quantize = TRUE;
        cinfo.dither_mode = JDITHER_FS;
    }

    /* Step 5: Start decompressor */

    jpeg_start_decompress(&cinfo);

    *w = cinfo.output_width;
    *h = (int)cinfo.output_height;

    jpeg_finish_decompress(&cinfo);

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);
}

static IJG_Private *gIP;

// Destination into memory stuff
static void my_init_destination(j_compress_ptr cinfo)
{
    cinfo->dest->next_output_byte = gIP->ip_CompBuf;
    cinfo->dest->free_in_buffer = gIP->ip_CompSize;
}

static boolean my_empty_output_buffer(j_compress_ptr cinfo)
{
    return 1;
}

static void my_term_destination(j_compress_ptr cinfo)
{
    gIP->ip_ReCompSize = (int)(gIP->ip_CompSize - cinfo->dest->free_in_buffer);
}

static struct jpeg_destination_mgr bah;

static void
jpeg_memory_dst(struct jpeg_compress_struct *cinfo) {
    cinfo->dest = &bah;
    cinfo->dest->init_destination = &my_init_destination;
    cinfo->dest->empty_output_buffer = &my_empty_output_buffer;
    cinfo->dest->term_destination = &my_term_destination;
}

void
jpeg_compress(IJG_Private *ip, int q) {
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr       jerr;
  JDIMENSION                  num_scanlines;
  JSAMPROW                    row_pointer[1];

  gIP = ip;
  /* Initialize the JPEG compression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);

  cinfo.image_width = ip->ip_Width;  
  cinfo.image_height = ip->ip_Height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB; /* arbitrary guess */
  jpeg_set_defaults(&cinfo);

  jpeg_set_quality(&cinfo, q, 1);

  /* Now that we know input colorspace, fix colorspace-dependent defaults */
  jpeg_default_colorspace(&cinfo);

  jpeg_memory_dst(&cinfo);

  /* Start compressor */
  jpeg_start_compress(&cinfo, TRUE);

  /* Process data */
  while (cinfo.next_scanline < cinfo.image_height) {
     row_pointer[0] = &ip->ip_DstBuf[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
     (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
 
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
}


//
// File: MultiPng.cpp
// Purpose: Provides classes for reading/writing 'MultiPng' files, which are
//   simply a sequence of PNG images concatenated back-to-back, with an
//   accompanying index file for random access.
// Author: Vlad Morariu
// Created: 2009-06-25
//
// Copyright (c) 2009-2014 Vlad Morariu
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================
// This code is based on the OpenCV implementation in highgui/src/grfmt_png.cpp.
// The original OpenCV code was released under the following license:
//
///*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/
#include "MultiPng.h"
#include "png.h"
#include "zlib.h"
#include <fstream>

#ifndef strdup_safe
#define strdup_safe(str) ((str) ? (strdup((str))) : NULL)
#endif

inline bool  isBigEndian(void)
{
  return (((const int*)"\0\x1\x2\x3\x4\x5\x6\x7")[0] & 255) != 0;
}

MultiPngWriter::MultiPngWriter(const char * filename) 
{
  m_idx = 0;
  m_filename = strdup_safe(filename);
  m_filename_idx = (char*)malloc(strlen(m_filename)+1+4);
  sprintf(m_filename_idx, "%s.idx", m_filename);  
  m_fp = fopen(m_filename, "wb");
  m_fpi = fopen(m_filename_idx, "w");
  fclose(m_fp);  m_fp = NULL;
  fclose(m_fpi); m_fpi = NULL;
}

MultiPngWriter::~MultiPngWriter() 
{
  free(m_filename);  
  free(m_filename_idx);
}

bool  MultiPngWriter::IsFormatSupported(int depth)
{
  return depth == IPL_DEPTH_8U || depth == IPL_DEPTH_16U;
}

bool  MultiPngWriter::Write(const uchar* data, int step,
                            int width, int height, int depth, int channels)
{
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  png_infop info_ptr = 0;
  FILE* f = 0;
  uchar** buffer = 0;
  int y;
  bool result = false;

  if(depth != IPL_DEPTH_8U && depth != IPL_DEPTH_16U)
    return false;

  if(png_ptr)
  {
    info_ptr = png_create_info_struct(png_ptr);
    
    if(info_ptr)
    {
      if(setjmp(png_jmpbuf(png_ptr)) == 0) // setjmp(png_ptr->jmpbuf) is deprecated
      {
        f = fopen(m_filename, "ab");  
        if(f)
        {
          //fseek(f, 0, SEEK_END);
          //long idx_s = ftell(f);
#ifdef WIN32
          _fseeki64(f, 0, SEEK_END);
          int64 idx_s = _ftelli64(f);
#else
          fseeko(f, 0, SEEK_END);
          int64 idx_s = ftello(f);
#endif
          png_init_io(png_ptr, f);

          png_set_compression_mem_level(png_ptr, MAX_MEM_LEVEL);

          png_set_IHDR(png_ptr, info_ptr, width, height, depth,
              channels == 1 ? PNG_COLOR_TYPE_GRAY :
              channels == 3 ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
              PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
              PNG_FILTER_TYPE_DEFAULT);

          png_write_info(png_ptr, info_ptr);

          png_set_bgr(png_ptr);
          if(!isBigEndian())
            png_set_swap(png_ptr);

          buffer = new uchar*[height];
          for(y = 0; y < height; y++)
            buffer[y] = (uchar*)(data + y*step);

          png_write_image(png_ptr, buffer);
          png_write_end(png_ptr, info_ptr);

          delete[] buffer;
      
          result = true;

//          long idx_e = ftell(f);
#ifdef WIN32
          int64 idx_e = _ftelli64(f);
#else
          int64 idx_e = ftello(f);
#endif
          //FILE * fi = fopen(m_filename_idx, "a");
          //if(fi)
         // {
          //  fprintf(fi, "%i %li %li\n", m_idx, idx_s, idx_e);
          //  fclose(fi);
          //}
          std::ofstream fi(m_filename_idx, std::ios::out | std::ios::app);
          if(!fi)
          {
            printf("Could not open index file %s.\n", m_filename_idx);
          }
          else
          {
            fi << m_idx << " " << idx_s << " " << idx_e << std::endl;
            fi.close();
          }
          m_idx++;
        }
      }
    }
  }

  png_destroy_write_struct(&png_ptr, &info_ptr);

  if(f) fclose(f);

  return result;
}

MultiPngReader::MultiPngReader(const char* filename, int iscolor)
{
  m_iscolor_desired = iscolor;
  m_iscolor = iscolor;
  m_color_type = m_bit_depth = 0;
  m_png_ptr = 0;
  m_info_ptr = m_end_info = 0;
  m_f = 0;
  m_f = fopen(filename, "rb");
  m_filename = strdup_safe(filename);
  m_idx = -1;

  char * filename_idx = (char*)malloc(strlen(filename) + 1 + 4);
  sprintf(filename_idx, "%s.idx", filename);
  //FILE * fi = fopen(filename_idx, "r");
  std::ifstream fi(filename_idx);
  if(!fi)
  {
    printf("Could not open index file %s.\n", filename_idx);
  }
  else
  {
    int frame_idx;
    int64 pos_s, pos_e;
    //long pos_s, pos_e;
    //while(fscanf(fi, "%d %ld %ld\n", &frame_idx, &pos_s, &pos_e) == 3)
    //{
    //  m_indexes[frame_idx] = std::make_pair(pos_s, pos_e);
    //}
    while(!fi.eof())
    {
      fi >> frame_idx >> pos_s >> pos_e;
      m_indexes[frame_idx] = std::make_pair(pos_s, pos_e);
    }

    // set m_idx to frame with smallest index from index file
    if(!m_indexes.empty())
    {
      m_idx = m_indexes.begin()->first;
    }

    fi.close();
    //fclose(fi);
  }
  free(filename_idx);
}

MultiPngReader::~MultiPngReader()
{
  if(m_f)
  {
    fclose(m_f);
    m_f = 0;
  }
  free(m_filename);
}

void  MultiPngReader::Close()
{
  if(m_png_ptr)
  {
    png_structp png_ptr = (png_structp)m_png_ptr;
    png_infop info_ptr = (png_infop)m_info_ptr;
    png_infop end_info = (png_infop)m_end_info;
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    m_png_ptr = m_info_ptr = m_end_info = 0;
  }
}

bool  MultiPngReader::ReadHeader()
{
  bool result = false;

  Close();

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);

  if(png_ptr)
  {
    png_infop info_ptr = png_create_info_struct(png_ptr);
    png_infop end_info = png_create_info_struct(png_ptr);

    m_png_ptr = png_ptr;
    m_info_ptr = info_ptr;
    m_end_info = end_info;

    if(info_ptr && end_info)
    {
      if(setjmp(png_jmpbuf(png_ptr)) == 0) // setjmp(png_ptr->jmpbuf) is deprecated
      {
//        m_f = fopen(m_filename, "rb");
        if(m_f)
        {
          png_uint_32 width, height;
          int bit_depth, color_type;
          
          png_init_io(png_ptr, m_f);
          png_read_info(png_ptr, info_ptr);

          png_get_IHDR(png_ptr, info_ptr, &width, &height,
                        &bit_depth, &color_type, 0, 0, 0);

          m_iscolor = color_type == PNG_COLOR_TYPE_RGB ||
                      color_type == PNG_COLOR_TYPE_RGB_ALPHA ||
                      color_type == PNG_COLOR_TYPE_PALETTE;

          m_width = (int)width;
          m_height = (int)height;
          m_color_type = color_type;
          m_bit_depth = bit_depth;

          result = true;
        }
      }
    }
  }

  if(!result)
    Close();

  return result;
}

bool  MultiPngReader::ReadData(uchar* data, int step, int color)
{
  bool result = false;
  uchar** buffer = 0;

  color = color > 0 || (m_iscolor && color < 0);
  
  if(m_png_ptr && m_info_ptr && m_end_info && m_width && m_height)
  {
    png_structp png_ptr = (png_structp)m_png_ptr;
    png_infop info_ptr = (png_infop)m_info_ptr;
    png_infop end_info = (png_infop)m_end_info;
    
    if(setjmp(png_jmpbuf(png_ptr)) == 0) // setjmp(png_ptr->jmpbuf) is deprecated
    {
      int y;

      if(m_bit_depth > 8 && !m_native_depth)
        png_set_strip_16(png_ptr);
      else if(!isBigEndian())
        png_set_swap(png_ptr);

      ///* observation: png_read_image() writes 400 bytes beyond
      // * end of data when reading a 400x118 color png
      // * "mpplus_sand.png".  OpenCV crashes even with demo
      // * programs.  Looking at the loaded image I'd say we get 4
      // * bytes per pixel instead of 3 bytes per pixel.  Test
      // * indicate that it is a good idea to always ask for
      // * stripping alpha..  18.11.2004 Axel Walthelm
      // 
      png_set_strip_alpha(png_ptr);

      if(m_color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);

      if(m_color_type == PNG_COLOR_TYPE_GRAY && m_bit_depth < 8)
#if (PNG_LIBPNG_VER <= 10237) // somewhere between 1.2.37 and 1.4.0, the function name changed
        png_set_gray_1_2_4_to_8(png_ptr);
#else
        png_set_expand_gray_1_2_4_to_8(png_ptr);
#endif

      if(m_iscolor && color)
        png_set_bgr(png_ptr); // convert RGB to BGR
      else if(color)
        png_set_gray_to_rgb(png_ptr); // Gray->RGB
      else
        png_set_rgb_to_gray(png_ptr, 1, -1, -1); // RGB->Gray

      png_read_update_info(png_ptr, info_ptr);

      buffer = new uchar*[m_height];

      for(y = 0; y < m_height; y++)
        buffer[y] = data + y*step;

      png_read_image(png_ptr, buffer);
      png_read_end(png_ptr, end_info);

      result = true;
    }
  }

  Close();
  delete[] buffer;

  return result;
}

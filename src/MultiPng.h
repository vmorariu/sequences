//
// File: MultiPng.h
// Purpose: Provides classes for reading/writing 'MultiPng' files, which are
//   simply a sequence of PNG images concatenated back-to-back, with an
//   accompanying index file for random access.
// Author: Vlad Morariu
// Created: 2009-06-25
//
// Copyright (c) 2009-2013 Vlad Morariu
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
#ifndef MULTI_PNG_H
#define MULTI_PNG_H

#include <stdio.h>
#include "cv.h"  // needed for IplImage definition, though the code is not 
                 // dependent on it
#include <map>
#include <utility>

// MultiPngWriter and MultiPngReader are based on OpenCV's GrFmtPngWriter
// and GrFmtPngReader; the only changes to the original OpenCV code are those
// allowing more than one image to be read from or written to the same file.

//
// MultiPngWriter
//
class MultiPngWriter
{
public:
  
  MultiPngWriter(const char * filename);
  ~MultiPngWriter();

  bool  IsFormatSupported(int depth);
  bool  Write(const uchar* data, int step,
              int width, int height, int depth, int channels);

  void SetPos(int idx) { m_idx = idx; }
  int GetPos() { return m_idx; }

protected:
  char * m_filename;      // name of data (frames) file
  char * m_filename_idx;  // name of index file
  FILE * m_fp;            // frame file pointer
  FILE * m_fpi;           // index file pointer
  int m_idx;              // index of next frame to be written
};

//
// MultiPngReader
//
class MultiPngReader
{
public:
    
  MultiPngReader(const char* filename, int iscolor=-1);

  ~MultiPngReader();

  // this has kind of odd behavior in terms of how it advances through frames,
  // but I don't have time to deal with it now to make it more intuitive
  IplImage * Read()
  { 
    IplImage * image = NULL;

    std::map< int, std::pair<int64,int64> >::iterator it = m_indexes.end();
    if(m_f != NULL && 
        m_idx != -1 && 
        (it = m_indexes.find(m_idx)) != m_indexes.end())
    {
      // seek to beginning of frame indexed by m_idx
#ifdef WIN32
      _fseeki64(m_f, m_indexes[m_idx].first, SEEK_SET);
#else
      fseeko(m_f, m_indexes[m_idx].first, SEEK_SET);
#endif

      // read image
      if(ReadHeader())
      {
        image = cvCreateImage(cvSize(m_width,m_height), m_bit_depth, (m_iscolor_desired ? 3 : 1));
        if(!ReadData((uchar*)image->imageData, image->widthStep, m_iscolor_desired))
        {
          cvReleaseImage(&image);
          image = NULL;
        }
      }

      // advance frame
      it++;
      if(it != m_indexes.end())
        m_idx = it->first;
      else
        m_idx = -1;
    }

    return image;
  }

  int First()
  {
    if(!m_indexes.empty())
      return m_indexes.begin()->first;
    else
      return -1;
  }

  int Last()
  {
    if(!m_indexes.empty())
      return m_indexes.rbegin()->first;
    else
      return -1;
  }

  int GetLength()
  {
    return (int)m_indexes.size();
  }

  // set the position of the next frame that will be obtained by Read
  void SetNext(int pos)
  {
    if(m_indexes.find(pos) != m_indexes.end() && m_f != NULL)
      m_idx = pos;
    else
      m_idx = -1;
  }

  int Next()
  {
    return m_idx;
  }

protected:

  bool  CheckFormat(const char* signature);
  bool  ReadData(uchar* data, int step, int color);
  bool  ReadHeader();
  void  Close();

  int m_iscolor_desired;
  int m_iscolor;
  int m_width;
  int m_height;
  int m_native_depth;
  int m_bit_depth;

  void* m_png_ptr;  // pointer to decompression structure
  void* m_info_ptr; // pointer to image information structure
  void* m_end_info; // pointer to one more image information structure
  FILE* m_f;
  int   m_color_type;
  char * m_filename;

  std::map< int, std::pair<int64,int64> > m_indexes;
  int m_idx;
};

#ifdef SEQUENCES_HEADER_ONLY
#include "MultiPng.cpp"
#endif

#endif //MULTI_PNG_H

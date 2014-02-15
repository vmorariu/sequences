//
// File: SequenceWriterMultiPng.h
// Purpose: Writer implementation for concatenated png files.
// Author: Vlad Morariu
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
#ifndef SEQUENCE_WRITER_MULTIPNG_H
#define SEQUENCE_WRITER_MULTIPNG_H

#include "SequenceWriter.h"
#include "MultiPng.h"

class SequenceWriterMultiPng : public SequenceWriter
{
public:
  SequenceWriterMultiPng()
    : m_pos(0), m_is_color(-1), m_size(cvSize(0,0)), m_writer(NULL)
  {}

  ~SequenceWriterMultiPng()
  {
    Close();
  }

  void Close()
  {
    if(m_writer != NULL)
      delete m_writer;
    m_pos = 0; 
    m_is_color = -1;
    m_size = cvSize(0,0);
  }

  bool Open(const char * filename, int fourcc, double fps, CvSize frame_size, int is_color=1)
  {
    if((filename == NULL))
      return false;

    // only write to .pngv files
    size_t len;
    if((len = strlen(filename)) < 5 || strcmp(filename + len - 5, ".pngv") != 0)
      return false;

    m_writer = new MultiPngWriter(filename);
    m_is_color = is_color;
    
    return true;
  }

  // the returned image needs to be released by the caller!!
  void Write(CvArr * image, int pos=-1)
  {
    IplImage * img_ipl, img_ipl_stub;
    img_ipl = cvGetImage(image, &img_ipl_stub);
    
    if(pos >= 0)
      m_pos = pos;

    m_writer->SetPos(m_pos++);
    m_writer->Write((uchar*)img_ipl->imageData, img_ipl->widthStep, img_ipl->width, img_ipl->height, 8, img_ipl->nChannels);
  }

  // return the index of the next frame that will be written
  int Next()
  {
    return m_pos;
  }

  CvSize Size()
  {
    return m_size;
  }

  MultiPngWriter * m_writer;
  int m_pos;
  int m_is_color;
  char * m_filename;
  CvSize m_size;
};

#endif // SEQUENCE_WRITER_MULTIPNG_H

//
// File: SequenceWriterMultiFile.h
// Purpose: Writer implementation for sequences consisting of independent files.
// Author: Vlad Morariu
// Created: 2009-10-08
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
#ifndef SEQUENCE_WRITER_MULTI_FILE_H
#define SEQUENCE_WRITER_MULTI_FILE_H

#include "SequenceWriter.h"
#include "highgui.h"

#ifndef strdup_safe
#define strdup_safe(str) ((str) ? (strdup((str))) : NULL)
#endif

class SequenceWriterMultiFile : public SequenceWriter
{
public:
  SequenceWriterMultiFile()
    : m_pos(0), m_is_color(-1), m_filename(NULL), m_size(cvSize(0,0))
  {}

  ~SequenceWriterMultiFile()
  {
    Close();
  }

  void Close()
  {
    free(m_filename);
    m_filename = NULL;
    m_pos = 0; 
    m_is_color = -1;
    m_size = cvSize(0,0);
  }

  bool Open(const char * filename, int fourcc, double fps, CvSize frame_size, int is_color=1)
  {
    if((filename == NULL))
      return false;

    m_filename = strdup_safe(filename);
    m_is_color = is_color;   

    return CheckWriter(frame_size);
  }

  bool CheckWriter(CvSize frame_size)
  {
    // try writing a file
    IplImage * dummy = cvCreateImage(frame_size, 8, 1);
    cvZero(dummy);
    char filename[1024];
    sprintf(filename, m_filename, m_pos);
    try
    {
      cvSaveImage(filename, dummy);
    }
    catch(...)
    {
      cvReleaseImage(&dummy);
      return false;     
    }
    remove(filename);
    cvReleaseImage(&dummy);
    return true;
  }

  void Write(CvArr * image, int pos=-1)
  {
    if(pos >= 0)
      m_pos = pos;
    char filename[1024];
    sprintf(filename, m_filename, m_pos++);
    cvSaveImage(filename, image);    
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

  int m_pos;
  int m_is_color;
  char * m_filename;
  CvSize m_size;
};

#endif // SEQUENCE_WRITER_MULTI_FILE_H

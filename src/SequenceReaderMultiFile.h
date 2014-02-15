//
// File: SequenceReaderMultiFile.h
// Purpose: Multi-file image sequence reader.
// Author: Vlad Morariu
// Created: 2009-05-13
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
#ifndef SEQUENCE_READER_MULTI_FILE_H
#define SEQUENCE_READER_MULTI_FILE_H

#include "SequenceReader.h"
#include "highgui.h"

#ifndef strdup_safe
#define strdup_safe(str) ((str) ? (strdup((str))) : NULL)
#endif

//
// reads an image sequence from a set of files
//
class SequenceReaderMultiFile : public SequenceReader
{
public:
  SequenceReaderMultiFile()
    : m_pos(0), m_first(0), m_last(-1), m_is_color(-1), m_filename(NULL), m_size(cvSize(0,0))
  {}

  ~SequenceReaderMultiFile()
  {
    Close();
  }

  void Close()
  {
    free(m_filename);
    m_filename = NULL;
    m_pos = 0; 
    m_first = -1;
    m_last = -1;
    m_is_color = -1;
    m_size = cvSize(0,0);
  }

  bool Open(const char * filename, int first, int last, int is_color)
  {
    if((filename == NULL) || (first < 0) || (last < first))
      return false;

    m_filename = strdup_safe(filename);
    m_first = first;
    m_last = last;
    m_is_color = is_color;

    char temp_filename[1024];
    sprintf(temp_filename, filename, first);
    IplImage * temp_image = cvLoadImage(temp_filename, m_is_color);    

    if(temp_image == NULL)
      return false;

    m_size = cvGetSize(temp_image);
    cvReleaseImage(&temp_image);

    return true;
  }

  // the returned image needs to be released by the caller!!
  IplImage * Read(int pos)
  {
    IplImage * img = NULL;

    char temp_filename[1024];
    sprintf(temp_filename, m_filename, pos);
    img = cvLoadImage(temp_filename, m_is_color);

    m_pos = pos;
    return img;
  }

  // returns the actual start index
  int First()
  {
    return m_first;
  }
  
  // returns the actual end index
  int Last()
  {
    return m_last; 
  }

  // return the next available frame (or -1 if unknown -- this can happen in the multi-file image sequence case)
  int Next()
  {
    return -1;
  }

  CvSize Size()
  {
    return m_size;
  }

private:
  int m_pos;
  int m_first;
  int m_last;
  int m_is_color;
  bool m_is_video;
  CvCapture * m_video;
  char * m_filename;
  CvSize m_size;
};

#endif // SEQUENCE_READER_MULTI_FILE_H

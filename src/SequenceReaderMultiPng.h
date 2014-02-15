// File: SequenceReaderMultiPng.h
// Author: Vlad Morariu
// Purpose: Reader for concatenated PNG files.
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
#ifndef SEQUENCE_READER_MULTI_PNG_H
#define SEQUENCE_READER_MULTI_PNG_H

#include "SequenceReader.h"
#include "MultiPng.h"

// reads an image sequence either from a video or from a set of files
class SequenceReaderMultiPng : public SequenceReader
{
public:
  SequenceReaderMultiPng()
    : m_pos(0), m_first(-1), m_last(-1), m_is_color(-1), m_reader(NULL), m_size(cvSize(0,0))
  {}

  ~SequenceReaderMultiPng()
  {
    Close();
  }

  void Close()
  {
    if(m_reader)
      delete m_reader;
    m_reader = NULL;

    m_pos = 0; 
    m_first = 0;
    m_last = 0;
    m_is_color = -1;
    m_size = cvSize(0,0);
  }

  bool Open(const char * filename, int first, int last, int is_color)
  {
    if(filename == NULL)
      return false;

    size_t len;
    if((len = strlen(filename)) < 5 || strcmp(filename + len - 5, ".pngv") != 0)
      return false;

    bool open_success = false;

    m_first = MAX(first,0);
    m_last = last;
    m_is_color = is_color;
    
    m_reader = new MultiPngReader(filename, is_color);
    if(m_reader != NULL)
    {
      // set the first frame index if a non-negative first is given
      if(m_first > 0)
        m_reader->SetNext(m_first);
      else
        m_first = m_reader->First();

      // set the ending index (for now, we allow the actual sequence to be
      //   shorter, though to be consistent, we should not)
      m_last = m_reader->Last();
      if(last > 0)
        m_last = MIN(last,m_last);

      // try to open first frame of the video
      IplImage * frame = m_reader->Read();
      if(frame != NULL)
      {
        open_success = true;
        m_size = cvGetSize(frame);
        cvReleaseImage(&frame);
        m_reader->SetNext(m_first);
      } 
      else
        printf("SequenceReaderMultiPng::Open(): could not open first frame.\n");

      // if we were able to get a frame, and all requested frames are available, then
      // the video was opened successfully         
      if(m_last < last)
        printf("WARNING: the requested end index is past the end of the video.\n");
    }

    return open_success;
  }

  // the returned image needs to be released by the caller!!
  IplImage * Read(int pos)
  {
    IplImage * img = NULL;

    if(m_reader)
    {
      // set current frame, if necessary
      if(m_reader->Next() != pos)
        m_reader->SetNext(pos);
      img = m_reader->Read();
      m_pos = pos;
    }

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
    if(m_reader)
      return m_reader->Next();
    else
      return -1;
  }

  CvSize Size()
  {
    return m_size;
  }

  int m_pos;
  int m_first;
  int m_last;
  int m_is_color;
  MultiPngReader * m_reader;
  CvSize m_size;
};

#endif // SEQUENCE_READER_MULTI_PNG_H


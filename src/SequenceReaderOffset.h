//
// File: SequenceReaderOffset.h
// Purpose: Wraps another reader and adjusts the frame index--a quick and dirty
//   way to extract frames starting at index 1 to match the behavior of the
//   ffmpeg executable when it extracts frames from a video.
// Author: Vlad Morariu
// Created: 2012-07-12
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
#ifndef SEQUENCE_READER_OFFSET_H
#define SEQUENCE_READER_OFFSET_H

#include "SequenceReader.h"

class SequenceReaderOffset : public SequenceReader
{
public:

  SequenceReaderOffset() : m_reader(NULL), m_offset(0) {}

  virtual bool Open(const char * filename, int first, int last, int is_color)
  {
    // determine if the filename includes an offset parameter after the filename
    // don't use the offset class if the suffix is zero
    const char * suffix = strstr(filename, "::");
    if(suffix && (m_offset = atoi(suffix + 2)))
    {
      // remove the pattern from the filename of the archive
      char * tmpstr = strdup(filename);
      tmpstr[suffix - filename] = '\0';
      filename = tmpstr;
      if(first != -1)
        first -= m_offset;
      if(last != -1)
        last -= m_offset;
      m_reader = Create(filename, first, last, is_color);
      free((void*)tmpstr);
    }

    return m_reader != NULL;
  }
  
  virtual void Close()
  {
    if(m_reader)
    {
      m_reader->Close();
      delete m_reader;
      m_reader = NULL;
    }
    m_offset = 0;
  }

  virtual IplImage * Read(int pos)
  {
    if(m_reader)
      return m_reader->Read(pos - m_offset);
    return NULL;
  }
  
  virtual int First()
  {
    if(m_reader)
      return m_reader->First() + m_offset;
    return -1;
  }
  
  virtual int Last()
  {
    if(m_reader)
      return m_reader->Last() + m_offset;
    return -1;
  }

  virtual int Next()
  {
    if(m_reader)
      return m_reader->Next() + m_offset;
    return -1;
  }

  virtual CvSize Size()
  {
    if(m_reader)
      return m_reader->Size();
    return cvSize(-1, -1);
  }

  virtual ~SequenceReaderOffset()
  {
    Close();
  }

private:
  SequenceReader * m_reader;
  int m_offset;
};

#endif // SEQUENCE_READER_OFFSET_H

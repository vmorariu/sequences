//
// File: SequenceReaderFfmpeg.h
// Purpose: Implements the SequenceReader interface to read
//   directly from archives (uses libarchive to read tar, tar.gz, ...).
// Author: Vlad Morariu
// Created: 2012-03-08
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
#include "SequenceReader.h"
#include "cv.h"
#include "highgui.h"
#include <stdio.h>

#ifdef WIN32
#define popen _popen
#define pclose _pclose
#define PIPE_STDERR_TO_NULL "2> NUL"
#define POPEN_READ_MODE "rb"  // windows needs the "b" character
#else
#define PIPE_STDERR_TO_NULL "2> /dev/null"
#define POPEN_READ_MODE "r"   // linux fails with the "b" character
#endif


class SequenceReaderFfmpeg : public SequenceReader
{
public:
  SequenceReaderFfmpeg()
  {
    m_fp = NULL;
    m_pos = 0;
    m_first = -1;
    m_last = -1;
    m_image = NULL;
    m_filename = NULL;
  }

  // open a sequence
  virtual bool Open(const char * filename, int first, int last, int is_color)
  {
    m_filename = strdup(filename);

    // determine frame size and video length
    if(!SetSize())
      return false;
    if(!Open())
      return false;
    SetLast();

    m_first = MAX(first, 0);
    if(last > 0)      
      m_last = MIN(last, m_last);

    return true;
  }

  // Extract a frame using ffmpeg and get its size
  bool SetSize()
  {
    char cmd[4096];
    sprintf(cmd, "ffmpeg -i \"%s\" -f image2pipe -vcodec ppm -vframes 1 - "
          PIPE_STDERR_TO_NULL, m_filename);
    FILE * fp = popen(cmd, POPEN_READ_MODE);
    if(fp)
    {
      size_t readsize = 0;
      const size_t buffsize = 4096;
      char buffer[buffsize];
      std::vector<char> data;
      while(readsize = fread(buffer, 1, buffsize, fp))
        data.insert(data.end(), buffer, buffer + readsize);
      if(!data.empty())
      {
        CvMat bufm = cvMat((int)data.size(), 1, CV_8U, (void*)&data[0]);
        m_image = cvDecodeImage(&bufm, 1);
      }
      if(m_image)
        m_size = cvGetSize(m_image);
      pclose(fp);
      return m_image != NULL;
    }
    else
      printf("SequenceReaderFfmpeg::SetSize: could not open pipe (popen).\n");
    return false;
  }

  // Step forward through the video until the end is reached to get the video length
  void SetLast()
  {
    while(ReadNext());
    m_last = m_pos - 1;
  }

  bool Open()
  {
    m_pos = 0;
    if(m_fp)
      pclose(m_fp);
    char cmd[4096];
    sprintf(cmd, "ffmpeg -i \"%s\" -f rawvideo -pix_fmt bgr24 - "
      PIPE_STDERR_TO_NULL, m_filename);
    m_fp = popen(cmd, POPEN_READ_MODE);
    return m_fp != NULL;
  }

  bool ReadNext()
  {
    for(int i = 0; i < m_image->height; i++)
      if(fread(&CV_IMAGE_ELEM(m_image, uchar, i, 0),
        m_image->width*m_image->nChannels, 1, m_fp) != 1)
        return false;
    m_pos++;
    return true;
  }

  // closes the sequence
  virtual void Close()
  {
    if(m_fp)
    {
      pclose(m_fp);
      m_fp = NULL;
    }

    if(m_filename)
    {
      free((void*)m_filename);
      m_filename = NULL;
    }

    cvReleaseImage(&m_image);
  }

  // the returned image needs to be released by the caller!!
  virtual IplImage * Read(int pos)
  {
    if(Seek(pos) && ReadNext())
      return cvCloneImage(m_image);
    return NULL;
  }
  
  // returns the actual start index
  virtual int First()
  {
    return m_first;
  }
  
  // returns the actual end index
  virtual int Last()
  {
    return m_last;
  }

  // return the next available frame (or -1 if unknown -- this can happen in the
  // multi-file image sequence case)
  virtual int Next()
  {
    return m_pos;
  }

  virtual CvSize Size()
  {
    return m_size;
  }

  virtual ~SequenceReaderFfmpeg()
  {
    Close();
  }

  bool Seek(int pos)
  {
    // for non-seekable files, start from the beginning to seek backwards
    if(pos < m_pos)
    {
      m_pos = 0;
      Open();
    }

    // seek forward to current position, if necessary
    while(pos > m_pos && ReadNext());

    // output error message if unsuccessful
    if(pos != m_pos)
    {
      printf("SequenceReaderVideo::Seek: cannot seek to %i.\n", pos);      
      return false;
    }
    return true;
  }

private:
  FILE * m_fp;
  int m_first;
  int m_last;
  int m_pos;
  CvSize m_size;
  IplImage * m_image;
  char * m_filename;
};


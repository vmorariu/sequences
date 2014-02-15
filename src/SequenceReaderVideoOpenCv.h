//
// File: SequenceReaderVideoOpenCv.h
// Purpose: Reader that wraps OpenCV's VideoReader.
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
#ifndef SEQUENCE_READER_VIDEO_OPENCV_H
#define SEQUENCE_READER_VIDEO_OPENCV_H

#include "SequenceReader.h"

#ifndef strdup_safe
#define strdup_safe(str) ((str) ? (strdup((str))) : NULL)
#endif

class SequenceReaderVideo : public SequenceReader
{
public:
  SequenceReaderVideo()
    : m_pos(-1), m_first(-1), m_last(-1), m_is_color(-1), m_video(NULL), m_filename(NULL), m_size(cvSize(0,0))
  {}

  ~SequenceReaderVideo()
  {
    Close();
  }

  void Close()
  {
    cvReleaseCapture(&m_video);
    free(m_filename);
    m_filename = NULL;
    m_pos = -1; 
    m_first = 0;
    m_last = 0;
    m_is_color = -1;
    m_size = cvSize(0,0);
  }

  bool Open(const char * filename, int first, int last, int is_color)
  {
    if(filename == NULL)
      return false;

    bool open_success = false;

    m_filename = strdup_safe(filename);
    m_first = MAX(first,0);
    m_last = last;
    m_is_color = is_color;
    
    m_video = cvCreateFileCapture(m_filename);
    if(m_video != NULL)
    {
      if(m_first > 0)
      {
        cvSetCaptureProperty(m_video, CV_CAP_PROP_POS_FRAMES, m_first);
      }

      // if last is -1 we set m_last using the video property; otherwise
      //    we truncate the video as requested
      //m_last = (int)cvGetCaptureProperty(m_video, CV_CAP_PROP_FRAME_COUNT) - 1;
      // calculate the # of frames by going through the video
      m_last = -1;
      while(cvQueryFrame(m_video))
        m_last++;
      cvReleaseCapture(&m_video);
      m_video = cvCreateFileCapture(m_filename);

      if(last > 0)
        m_last = MIN(last,m_last);

      // try to open a frame of the video
      IplImage * frame = cvQueryFrame(m_video);
      m_pos = 1;
      if(frame == NULL)
        printf("SequenceReader::Open(): cvCreateFileCapture() succeeded, but cvQueryFrame() returned NULL\n");

      // if we were able to get a frame, and all requested frames are available, then
      // the video was opened successfully         
      open_success = (frame != NULL);
      if(m_last < last)
        printf("WARNING: the requested end index is past the end of the video.\n");
      if(open_success)
        m_size = cvGetSize(frame);
      // seek to the first requested frame
      Seek(m_first);
    }

    return open_success;
  }

  void Seek(int pos)
  {
    // set current frame, if necessary
    // disable seeking backwards for opencv
    //if(pos != m_pos + 1)
    //  if(!cvSetCaptureProperty(m_video, CV_CAP_PROP_POS_FRAMES, pos))
    //    return NULL;

    // for non-seekable files, start from the beginning to seek backwards
    if(pos < m_pos)
    {
      m_pos = 0;
      cvReleaseCapture(&m_video); // opencv cannot even seek back to frame 0
      m_video = cvCreateFileCapture(m_filename);
      if(!m_video)
      {
        printf("Could not seek to the beginning of the video...\n");
        exit(1);
      }
    }

    // seek forward to current position, if necessary
    while(pos > m_pos && cvQueryFrame(m_video))
      m_pos++;

    // output error message if unsuccessful
    if(pos != m_pos)
    {
      printf("SequenceReaderVideo::Seek: cannot seek to %i.\n", pos);
      exit(1); // TODO(Vlad): handle errors more nicely, but better to abort now
    }
  }

  // the returned image needs to be released by the caller!!
  IplImage * Read(int pos)
  {
    IplImage * img = NULL;

    if(m_video)
    {
      Seek(pos);
      IplImage * frame = cvQueryFrame(m_video);
      if(frame == NULL)
      {
        printf("SequenceReaderVideo::Read:Could not get frame %i...\n",
          pos);
        return NULL;
      }
      // desired image type is same as video provides
      if(m_is_color == -1 || 
          ((m_is_color == 1) && frame->nChannels == 3) ||
          ((m_is_color == 0) && frame->nChannels == 1))
      {
        img = cvCloneImage(frame);
      }
      // desired image is RGB but video provides GRAY
      if((m_is_color == 1) && (frame->nChannels == 1))
      {
        img = cvCreateImage(cvGetSize(frame), 8, 3);
        cvMerge(frame, frame, frame, NULL, img);
      }
      // desired image is GRAY, but video provides RGB
      if((m_is_color == 0) && (frame->nChannels == 3))
      {
        img = cvCreateImage(cvGetSize(frame), 8, 1);
        cvSplit(frame, img, NULL, NULL, NULL);
      }

      // flip image if necessary
      if(frame->origin == IPL_ORIGIN_BL)
      {
        cvFlip(img, img);
        img->origin = IPL_ORIGIN_TL;
      }

      m_pos = pos + 1;
    }

    return img;
  }

  int First()
  {
    return m_first;
  }
  
  int Last()
  {
    return m_last; 
  }

  int Next()
  {
    return m_pos;
  }

  CvSize Size()
  {
    return m_size;
  }

  int m_pos;
  int m_first;
  int m_last;
  int m_is_color;
  CvCapture * m_video;
  char * m_filename;
  CvSize m_size;
};

#endif // SEQUENCE_READER_VIDEO_OPENCV_H

// File: SequenceReader.cpp
// Author: Vlad Morariu
// Purpose: Implementation of base class static functions.
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
#include "SequenceReaderMultiFile.h"
#include "SequenceReaderArchive.h"
#include "SequenceReaderFfmpeg.h"
#include "SequenceReaderOffset.h"
#ifdef USE_VIDEO_OPENCV  // not frame accurate--use ffmpeg reader instead
#include "SequenceReaderVideoOpenCv.h"
#endif
#ifdef USE_MULTIPNG  // deprecated in favor of archive
#include "SequenceReaderMultiPng.h"
#endif


SequenceReader * SequenceReader::Create(const char * filename, int first, int last, int is_color)
{
  SequenceReader * reader;

  if(!filename)
    return NULL;

  // wrap the sequence in the "offset" wrapper to change indexes if desired
  reader = new SequenceReaderOffset();
  if(reader->Open(filename, first, last, is_color))
    return reader;
  else
    delete reader;

#ifdef USE_MULTIPNG
  reader = new SequenceReaderMultiPng();
  if(reader->Open(filename, first, last, is_color))
    return reader;
  else
    delete reader;
#endif

  // first try the file reader
  reader = new SequenceReaderMultiFile();
  if(reader->Open(filename, first, last, is_color))
    return reader;
  else
    delete reader;

  reader = new SequenceReaderArchive();
  if(reader->Open(filename, first, last, is_color))
    return reader;
  else
    delete reader;

  reader = new SequenceReaderFfmpeg();
  if(reader->Open(filename, first, last, is_color))
    return reader;
  else
    delete reader;

#ifdef USE_VIDEO_OPENCV
  // The opencv video reader is still not frame accurate (as of OpenCV 2.3.1)
  reader = new SequenceReaderVideoOpenCv();
  if(reader->Open(filename, first, last, is_color))
    return reader;
  else
    delete reader;
#endif

  return NULL;
}

void SequenceReader::Destroy(SequenceReader ** reader)
{
  if(reader && *reader)
  {
    delete *reader;
    *reader = NULL;
  }
}


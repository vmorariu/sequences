//
// File: SequenceWriter.cpp
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
#include "SequenceWriter.h"

#ifdef USE_MULTIPNG
#include "SequenceWriterMultiPng.h"
#endif
#include "SequenceWriterMultiFile.h"
#include "SequenceWriterArchive.h"

SequenceWriter * SequenceWriter::Create(const char * filename, int fourcc, double fps, CvSize frame_size, int is_color)
{
  SequenceWriter * writer;

  if(!filename)
    return NULL;

#ifdef USE_MULTIPNG
  writer = new SequenceWriterMultiPng();
  if(writer->Open(filename, fourcc, fps, frame_size, is_color))
    return writer;
  else
    delete writer;
#endif

  writer = new SequenceWriterArchive();
  if(writer->Open(filename, fourcc, fps, frame_size, is_color))
    return writer;
  else
    delete writer;

  writer = new SequenceWriterMultiFile();
  if(writer->Open(filename, fourcc, fps, frame_size, is_color))
    return writer;
  else
    delete writer;

  return NULL;
}

void SequenceWriter::Destroy(SequenceWriter ** writer)
{
  if(writer && *writer)
  {
    delete *writer;
    *writer = NULL;
  }
}


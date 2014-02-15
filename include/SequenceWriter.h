//
// File: SequenceWriter.h
// Purpose: Provides an abstract interface to an image sequence writer.  Its
//   initial purpose was to easily deal with both image sequences and 
//   videos in OpenCV using a unified interface to simplify client code.
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
#ifndef SEQUENCE_WRITER_H
#define SEQUENCE_WRITER_H

#include "SequenceExports.h"
#include "cv.h"

class SEQUENCES_EXPORT SequenceWriter
{
public: 
  static SequenceWriter * Create(const char * filename, int fourcc, double fps, CvSize size, int is_color=1);

  static void Destroy(SequenceWriter ** writer);

  // open a sequence
  virtual bool Open(const char * filename, int fourcc, double fps, CvSize frame_size, int is_color=1)=0;
  
  // closes the sequence
  virtual void Close()=0;

  virtual void Write(CvArr * image, int pos=-1)=0;
  
  virtual int Next()=0;

  virtual CvSize Size()=0;

  virtual ~SequenceWriter(){};
};

#ifdef SEQUENCES_HEADER_ONLY
// include factory source here to make this code headers only 
#include "SequenceWriter.cpp"
#endif

#endif //SEQUENCE_WRITER_H

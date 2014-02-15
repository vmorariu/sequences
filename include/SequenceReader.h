//
// File: SequenceReader.h
// Purpose: Provides an abstract interface to an image sequence reader.  Its
//   initial purpose was to easily deal with both image sequences and 
//   videos in OpenCV using a unified interface to simplify client code.
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
#ifndef SEQUENCE_READER_H
#define SEQUENCE_READER_H

#include "cv.h"
#include "SequenceExports.h"

class SEQUENCES_EXPORT SequenceReader
{
public:
  // static factory function that creates a derived reader that can read "filename"
  static SequenceReader * Create(const char * filename, int first, int last, int is_color);

  // call this to destroy whatever was returned by Create()
  static void Destroy(SequenceReader ** reader);

  // open a sequence
  virtual bool Open(const char * filename, int first, int last, int is_color)=0;
  
  // closes the sequence
  virtual void Close()=0;

  // the returned image needs to be released by the caller!!
  virtual IplImage * Read(int pos)=0;
  
  // returns the actual start index
  virtual int First()=0;
  
  // returns the actual end index
  virtual int Last()=0;

  // return the next available frame (or -1 if unknown -- this can happen in the
  // multi-file image sequence case)
  virtual int Next()=0;

  virtual CvSize Size()=0;

  virtual ~SequenceReader(){};
};

#ifdef SEQUENCES_HEADER_ONLY
// include factory source here to make this code headers only 
#include "SequenceReader.cpp"
#endif

#endif //SEQUENCE_READER_H

// File: main.cpp
// Author: Vlad Morariu
// Purpose: Simple program that tests the library.
// Created: 2013-12-09
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
#include "SequenceWriter.h"

int main(int argc, char * argv[])
{
  if(argc != 3)
  {
    printf("Usage: %s input output\n", argv[0]);
    return -1;
  }

  SequenceReader * reader = SequenceReader::Create(argv[1], -1, -1, 1);
  if(reader == NULL)
  {
    printf("Could not open '%s' for reading...\n", argv[1]);
    return -1;
  }

  SequenceWriter * writer = SequenceWriter::Create(
      argv[2], 0, 30, reader->Size(), 1);
  if(writer == NULL)
  {
    printf("Could not open '%s' for writing...'\n", argv[2]);
    SequenceReader::Destroy(&reader);
    return -1;
  }

  for(int f = reader->First(); f <= reader->Last(); f++)
  {
    IplImage * image = reader->Read(f);
    writer->Write(image, f);
    cvReleaseImage(&image);
  }

  SequenceReader::Destroy(&reader);
  SequenceWriter::Destroy(&writer);

  return 0;
}

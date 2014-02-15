//
// File: SequencesMain.cpp
// Purpose: This is a program for viewing, converting, and merging
//   sequence files.
// Author: Vlad Morariu
// Created: 2009-06-29
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
#include "highgui.h"
#include "SequenceReader.h"
#include "SequenceWriter.h"
#include <vector>

typedef struct MergeStruct
{
  MergeStruct(const char * filename_in, int first_in, int last_in, int step_in)
    : filename(filename_in), first(first_in), last(last_in), step(step_in)
  {}

  const char * filename;
  int first;
  int last;
  int step;
} MergeStruct;

void ParseCmdLineParameters(int argc, char * argv[], 
                             char ** input, 
                             char ** output, 
                             int * start, 
                             int * last, 
                             int * step, 
                             int * is_color,
                             std::vector< MergeStruct > & merge_list)
{
  // parse command line arguments
  bool show_help = false;
  int i = 1;
  while(i < argc)
  {
    show_help = (strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0);
   
    // first argument is assumed to be the input filename    
    if(!show_help && i == 1)
    {
      *input = argv[i];
      i += 1;
      continue;
    }

    // other arguments are the options
    if(strlen(argv[i]) == 2 && argv[i][0] == '-' && !show_help)
    {
      double dval = 0, dval1 = 0, dval2 = 0, dval3 = 0, dval4 = 0;
      int ival = 0, ival1 = 0, ival2 = 0, ival3 = 0;
      int64 ival64 = 0;
      char c = argv[i][1];
      switch(c)
      { 
      case 'c': // output
        if(i+1 >= argc)
          break;
        *is_color = atoi(argv[i+1]);
        i += 2;
        continue;
      case 'o': // output
        if(i+1 >= argc)
          break;
        *output = argv[i+1];
        i += 2;
        continue;
      case 'f': // frames
        if(i+3 >= argc)
          break;
        *start = atoi(argv[i+1]);
        *last = atoi(argv[i+2]);
        *step = atoi(argv[i+3]);
        i += 4;
        continue;
      case 'm': // merge
        if(i + 2 >= argc)
          break;
        {
          int n_merge = atoi(argv[i+1]);
          if(i + 2 + 4*n_merge >= argc)
            break;
          else
          {
            i+=2;
            for(int j = 0; j < n_merge; j++)
            {
              merge_list.push_back(MergeStruct(argv[i], atoi(argv[i+1]), atoi(argv[i+2]), atoi(argv[i+3])));
              i+=4;
            }
          }
        }
        continue;
        break;
      default:
        break;
      }
    }

    // shouldn't get here unless unrecognized option
    if(!show_help)
      printf("Unrecognized(or incorrectly used) option: %s\n", argv[i]);
    printf("Usage: %s input [options]\n", argv[0]);
    printf("   input:     either a frame pattern (which requires -f),\n");
    printf("              the name of a MultiPng file (.pngv), or other\n");
    printf("              video file that can be opened by OpenCV.\n");
    printf("Options:\n");
    printf("   -o output: (optional) write to this output filename; if not\n");
    printf("              provided, display the input sequence.\n");
    printf("   -f first last step: specifies the frame numbers to use for\n");
    printf("              the input sequence (same numbers will be used for output).\n");
    printf("   -m n filename_1 first_1 last_1 step_1 ... filename_n first_n last_n \n");
    printf("              this will append n additional input videos to the output\n");
    printf("              video.  This is option is ignored if -o is not specified.\n");
    printf("   -c is_color (optional) if 0, force to 8-bit single channel; else\n");
    printf("              if 1, force to 24-bit bgr; if -1 auto-select.\n");
    exit(1);
    i++;
  }
}

void ViewSequence(SequenceReader * reader, int step)
{
  int first = reader->First();
  int last = reader->Last();

  int playing = 0; //1;
  int wait_time = 1;
  int exit = 0;
  int redraw = 1;
  int seek = 0, seek_max = (last - first)/step, seek_toolbar = seek;

  // if output is not set, display the image
  const int n_speeds = 21;
  int speed = n_speeds-1;
  int wait_times[n_speeds];
  for(int i = 0; i < n_speeds; i++)
    wait_times[i] = (int)ceil(pow(1000.0, (n_speeds-i-1) / ((double)n_speeds-1)));

  cvNamedWindow("display");
  cvCreateTrackbar("speed", "display", &speed, n_speeds-1, NULL);
  cvSetTrackbarPos("speed", "display", n_speeds-1);
  cvCreateTrackbar("seek", "display", &seek_toolbar, seek_max, NULL);
  cvSetTrackbarPos("seek", "display", 0);

  while(!exit)
  {
    wait_time = wait_times[speed];

    // redraw image, if seek position has changed
    if(redraw)
    {
      cvSetTrackbarPos("seek", "display", seek);
      IplImage * image = reader->Read(first + seek*step);
      cvNamedWindow("display");
      cvShowImage("display", image);
      cvReleaseImage(&image);
      redraw = 0;
    }

    // wait for key to be pressed
    int key = cvWaitKey((playing ? wait_time : 1));

    // check to see if trackbar has moved
    if(cvGetTrackbarPos("seek", "display") != seek)
    {
      playing = 0;
      redraw = 1;
      seek = cvGetTrackbarPos("seek", "display");
    }

    // check what key was pressed, if any
    switch(key)
    {
    case 27:
      exit = 1;
      playing = 0;
      break;
    case 13:
      playing ^= 1;
      break;
    case '>':
    case '.':
      seek = MIN(seek_max, seek+1);
      playing = 0;
      redraw = 1;
      break;
    case '<':
    case ',':
      seek = MAX(0, seek-1);
      playing = 0;
      redraw = 1;
      break;    
    default:
      break;
    }

    // if we're still in playing mode, increment seek position
    if(playing && seek < seek_max)
    {
      seek = MIN(seek_max, seek+1);
      redraw = 1;
    }
  }
}

int main(int argc, char * argv[])
{
  char * input = NULL;
  char * output = NULL;
  int first = -1;
  int last = -1;
  int step = 1;
  std::vector< MergeStruct > merge_list;
  int is_color = -1;

  ParseCmdLineParameters(argc, argv, &input, &output, &first, &last, &step, &is_color, merge_list);

  printf("Input: %s\n", (input ? input : "(NULL)"));
  printf("Frames: %i %i %i\n", first, last, step);
  printf("Output: %s\n", (output ? output : "(NULL)"));
  printf("Merge with: "); 
  for(int merge_i = 0; merge_i < (int)merge_list.size(); merge_i++)
    printf("%s (%i,%i,%i) ", merge_list[merge_i].filename, merge_list[merge_i].first, merge_list[merge_i].last, merge_list[merge_i].step);
  printf("\n");
  fflush(stdout);
  
  SequenceReader * reader = SequenceReader::Create(input, first, last, is_color);
  if(reader == NULL)
  {
    printf("Could not open sequence!\n");
    return 0;
  }

  first = reader->First();
  last = reader->Last();

  // if no output file is specified, display the video
  if(output == NULL)
  {
    ViewSequence(reader, step);
  }

  // create sequence writer
  SequenceWriter * writer = NULL;
  if(output != NULL)
  {
    writer = SequenceWriter::Create(output, 0, 30, reader->Size(), is_color);
    if(writer == NULL)
      printf("Could not create output sequence writer!\n");
  }

  // if the writer was successfully created, write the video and merge any
  // other videos if they exist
  if(writer != NULL)
  {
    // write first video
    for(int frame_i = first; frame_i <= last; frame_i++)
    {
      IplImage * image = reader->Read(frame_i);
      writer->Write(image, frame_i);
      cvReleaseImage(&image);
    }

    // merge with remaining videos
    for(int merge_i = 0; merge_i < (int)merge_list.size(); merge_i++)
    {
      // release previous reader
      if(reader != NULL)
        delete reader;

      // open next sequence
      SequenceReader * reader = SequenceReader::Create(merge_list[merge_i].filename, merge_list[merge_i].first, merge_list[merge_i].last, is_color);
      if(reader == NULL)
      {
        printf("Could not open sequence %i for merging!\n", merge_i);
        break;
      }

      // write next sequence
      first = reader->First();
      last = reader->Last();
      step = merge_list[merge_i].step;
      for(int frame_i = first; frame_i <= last; frame_i+= step)
      {
        IplImage * image = reader->Read(frame_i);
        writer->Write(image, frame_i);
        cvReleaseImage(&image);
      }
    }
  }

  SequenceReader::Destroy(&reader);
 
  if(writer != NULL)
  {
    //writer->Close();
    delete writer;
  }

  return 0;
}


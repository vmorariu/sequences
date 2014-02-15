//
// File: SequenceReaderArchive.h
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
#ifndef SEQUENCE_READER_ARCHIVE_H
#define SEQUENCE_READER_ARCHIVE_H

#include "SequenceReader.h"
#define LIBARCHIVE_STATIC
#include "archive.h"
#include "archive_entry.h"
#include "cv.h"
#include "highgui.h"
#include <vector>
#include <map>

#ifdef WIN32
#define snprintf _snprintf
#include <io.h>
#define lseek _lseeki64
#endif

class SequenceReaderArchive : public SequenceReader
{
public:
  SequenceReaderArchive()
    : m_pos(0), m_apos(0), m_first(-1), m_last(-1),
    m_is_color(-1), m_a(NULL), m_fp(NULL), m_size(cvSize(0,0))
  {}

  ~SequenceReaderArchive()
  {
    Close();
  }

  void Close()
  {
    if(m_a)
      archive_read_free(m_a);
    if(m_fp)
      fclose(m_fp);
    m_a = NULL;
    m_fp = NULL;
    m_pos = 0;
    m_apos = 0;
    m_first = -1;
    m_last = -1;
    m_is_color = -1;
    m_size = cvSize(0,0);
    m_seekable = false;
  }

  int OpenArchive()
  {
    // open the archive
    if(m_a)
      archive_read_free(m_a);
    m_a = archive_read_new();
    archive_read_support_filter_all(m_a);
    archive_read_support_format_all(m_a);
    return archive_read_open_fd(m_a, fileno(m_fp), 10246);
  }

  bool Open(const char * filename, int first, int last, int is_color)
  {
    if(filename == NULL)
      return false;

    // determine if the filename includes the file pattern inside the archive
    const char * pattern = strstr(filename, "::");
    if(pattern)
    {
      // save the pattern
      m_pattern = (pattern + 2);
      // remove the pattern from the filename of the archive
      char * tmpstr = strdup(filename);
      tmpstr[pattern - filename] = '\0';
      filename = tmpstr;
    }

    // open the file that contains the archive
    m_fp = fopen(filename, "rb");
    if(m_fp == NULL)
      return false;

    // initialize libarchive on the newly opened file
    int r = OpenArchive();
    if (r != ARCHIVE_OK)
      return false;

    // only allow fast seeking for tar files with no compression filter
    size_t len = strlen(filename);
    m_seekable = (len >= 4 && strcmp(filename + len - 4, ".tar") == 0);
    int nfilters = archive_filter_count(m_a);
    for(int i = 0; i < nfilters && m_seekable; i++)
      if(strcmp("none", archive_filter_name(m_a, i)) != 0)
        m_seekable = false;

    bool open_success = false;
    m_first = MAX(first, 0);  // we do not allow negative indexes
    m_last = last;         // currently, this could be -1
    m_pos = 0;
    m_apos = 0;
    m_is_color = is_color;

    // create an archive index for seeking and to discover the # of frames
    CreateIndex();
    // create a map from sequence index to archive index, if a filename
    // pattern is provided (i.e., if not all files are in the sequence)
    if(!m_pattern.empty())
      CreateIndexMap();

    // try to open first frame of the video
    IplImage * frame = Read(m_first);
    if(frame != NULL)
    {
      open_success = true;
      m_size = cvGetSize(frame);
      cvReleaseImage(&frame);
    } 
    else
      printf("SequenceReaderArchive::Open: could not open first frame.\n");
    
    // if we extracted a file pattern from the filename, then we 
    // made a copy of the filename
    if(pattern)
      free((void*)filename);

    return open_success;
  }

  // Assumes:
  //   m_first is set correctly (to some non-negative value)
  //   m_last might be -1 (modified by this function)
  void CreateIndex()
  {
    struct archive_entry *entry;
    while (archive_read_next_header(m_a, &entry) == ARCHIVE_OK)
    {
      archive_read_data_skip(m_a);
      m_name_map[archive_entry_pathname(entry)] = (int)m_indexes.size();
      m_indexes.push_back(archive_read_header_position(m_a));
    }
    // update m_last, if it is -1 (i.e., unknown)
    if(m_last == -1)
      m_last = m_first + (int)m_indexes.size() - 1;
    // close and reopen archive from beginning
    rewind(m_fp);
    OpenArchive();
  }

  // Assumes:
  //   m_first is set correctly (to some non-negative value)
  //   m_last is positive, and is an upper bound on the highest index (modified)
  void CreateIndexMap()
  {
    int maxlen = snprintf(NULL, 0, m_pattern.c_str(), m_last) + 1;
    char * tmp = new char[maxlen];
    for(int i = m_first; i <= m_last; i++)
    {
      sprintf(tmp, m_pattern.c_str(), i);
      if(m_name_map.find(tmp) != m_name_map.end())
        m_index_map[i] = m_name_map[tmp];
      else
      {
        m_last = i - 1;
        break;
      }
    }
    delete [] tmp;
  }

  bool Seek(int apos)
  {
    // seek to exact position in file, if the archive is seekable
    if(m_seekable && apos < (int)m_indexes.size() && apos != m_apos)
    {
      m_apos = apos;
      lseek(fileno(m_fp), m_indexes[apos], SEEK_SET);
      OpenArchive(); // close and reopen archive
    }

    // for non-seekable files, start from the beginning to seek backwards
    if(!m_seekable && apos < m_apos)
    {
      m_apos = 0;
      lseek(fileno(m_fp), 0, SEEK_SET);
      OpenArchive(); // close and reopen archive
    }

    // seek forward to current position, if necessary
    struct archive_entry *entry;
    while(apos > m_apos &&
      archive_read_next_header(m_a, &entry) == ARCHIVE_OK)
    {
      archive_read_data_skip(m_a);
      m_apos++;
    }

    // output error message if unsuccessful
    if(apos != m_apos)
    {
      printf("SequenceReaderArchive::Seek: cannot seek to %i.\n", apos);
      return false;
    }
    return true;
  }

  // the returned image needs to be released by the caller!!
  IplImage * Read(int pos)
  {
    IplImage * img = NULL;
    if(m_a)
    {
      // validate position argument
      if((m_index_map.empty() && (pos < 0 || pos >= (int)m_indexes.size())) ||
         (!m_index_map.empty() && m_index_map.find(pos) == m_index_map.end()))
      {
        printf("SequenceReaderArchive::Read: Bad frame position %i...\n", pos);
        return NULL;
      }

      // get the archive pos from the sequence pos
      int apos = m_index_map.empty() ? pos : m_index_map[pos];
      if(!Seek(apos))
        return NULL;

      // assume that the seek was successful
      struct archive_entry * entry;
      int r = archive_read_next_header(m_a, &entry);
      if(r == ARCHIVE_OK)
      {
        size_t size = archive_entry_size(entry);
        const uchar * buf = new uchar[size];
        size_t size_read = archive_read_data(m_a, (void*)buf, size);
        CvMat bufm = cvMat(size, 1, CV_8U, (void*)buf);
        img = cvDecodeImage(&bufm, m_is_color);
        delete [] buf;
        m_pos = pos + 1;
        m_apos = apos + 1;
      }
      else
      {
        printf("SequenceReaderArchive::Read: Header read error!\n");
        printf("%s\n", archive_error_string(m_a));
      }
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

  // return the next available frame index (or -1 if unknown -- this can happen in the multi-file image sequence case)
  int Next()
  {
    return m_pos;
  }

  CvSize Size()
  {
    return m_size;
  }

private:
  int m_apos;
  int m_pos;
  int m_first;
  int m_last;
  int m_is_color;
  bool m_seekable;
  FILE * m_fp;
  struct archive * m_a;
  CvSize m_size;
  std::map<std::string, int> m_name_map;
  std::map<int, int> m_index_map;
  std::vector<int64> m_indexes; // for seeking
  std::string m_pattern;
};

#endif // SEQUENCE_READER_ARCHIVE_H

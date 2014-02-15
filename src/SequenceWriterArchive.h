//
// File: SequenceWriterArchive.h
// Purpose: Write image sequences directly to archives.
// Created: 2012-07-05
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
#ifndef SEQUENCE_WRITER_ARCHIVE_H
#define SEQUENCE_WRITER_ARCHIVE_H

#include "SequenceWriter.h"
#define LIBARCHIVE_STATIC
#include "archive.h"
#include "archive_entry.h"
#include "cv.h"
#include "highgui.h"

class SequenceWriterArchive : public SequenceWriter
{
public:
  SequenceWriterArchive()
    :m_pos(0), m_is_color(-1), m_filename(NULL), m_size(cvSize(0, 0)),
      m_a(NULL)
  {}

  ~SequenceWriterArchive()
  {
    Close();
  }

  void Close()
  {
    if(m_a)
    {
      archive_write_close(m_a);
      archive_write_free(m_a);
    }
    m_a = NULL;
    free(m_filename);
    m_filename = NULL;
    m_pos = 0; 
    m_is_color = -1;
    m_size = cvSize(0, 0);
  }

  bool Open(const char * filename, int fourcc, double fps, CvSize frame_size, int is_color=1)
  {
    if(filename == NULL)
      return false;

    // determine if the filename includes the file pattern inside the archive
    const char * pattern = strstr(filename, "::");
    char * tmpstr = NULL;
    if(pattern)
    {
      // save the pattern
      m_pattern = (pattern + 2);
      // remove the pattern from the filename of the archive
      tmpstr = strdup(filename);
      tmpstr[pattern - filename] = '\0';
      filename = tmpstr;
    }
    else
    {
      if((strcmp(filename + strlen(filename) - 7, ".tar.gz") == 0) ||
         (strcmp(filename + strlen(filename) - 4, ".tgz") == 0) ||
         (strcmp(filename + strlen(filename) - 4, ".tar") == 0))
      {
        fprintf(stderr, 
                "SequenceWriterArchive::Open(): please provide a file"
                " pattern, e.g., /path/to/archive.tar.gz::pattern%%i.png\n");
      }
      return false;
    }

    // open the file that contains the archive
    m_a = archive_write_new();
    if(m_a == NULL)
      return false;

    // currently only support tar and tar.gz files
    if((strcmp(filename + strlen(filename) - 7, ".tar.gz") == 0) ||
       (strcmp(filename + strlen(filename) - 4, ".tgz") == 0))
    {
      archive_write_add_filter_gzip(m_a);
      archive_write_set_format_pax_restricted(m_a);
    }
    else if(strcmp(filename + strlen(filename) - 4, ".tar") == 0)
    {
      archive_write_set_format_pax_restricted(m_a);
    }
    else
      return false;

    if(archive_write_open_filename(m_a, filename) != ARCHIVE_OK)
      return false;
      
    m_filename = strdup_safe(filename);
    m_is_color = is_color;   

    if(tmpstr)
      free((void*)tmpstr);

    return true;
  }

  void Write(CvArr * image, int pos=-1)
  {
    if(pos >= 0)
     m_pos = pos;
    char filename[1024];
    sprintf(filename, m_pattern.c_str(), m_pos++);
    CvMat * data = cvEncodeImage(filename, image, NULL);
    int size = data->cols*data->rows;
    struct archive_entry * entry;
    entry = archive_entry_new();
    archive_entry_set_pathname(entry, filename);
    archive_entry_set_size(entry, size);
    archive_entry_set_filetype(entry, AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    time_t t = time(NULL);
    archive_entry_set_mtime(entry, t, 0);
    archive_entry_set_ctime(entry, t, 0);
    archive_entry_set_atime(entry, t, 0);
    int ret = archive_write_header(m_a, entry);
    if(ret != ARCHIVE_OK)
      printf("SequenceWriterArchive::Write(): "
             "archive_write_header(m_a, entry) != ARCHIVE_OK\n");
    if(archive_write_data(m_a, data->data.ptr, size) != size)
      printf("SequenceWriterArchive::Write(): "
             "archive_write_data(m_a, data->data.ptr, size) != size\n");
    archive_entry_free(entry);
    cvReleaseMat(&data);
  }

  // return the index of the next frame that will be written
  int Next()
  {
    return m_pos;
  }

  CvSize Size()
  {
    return m_size;
  }

  int m_pos;
  int m_is_color;
  char * m_filename;
  std::string m_pattern;
  struct archive * m_a;
  CvSize m_size;
};

#endif // SEQUENCE_WRITER_ARCHIVE_H

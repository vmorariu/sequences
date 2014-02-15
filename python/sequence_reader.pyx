#
# Author: Vlad Morariu
# Created: 2012-03-15
#
# Copyright (c) 2009-2013 Vlad Morariu
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
import numpy as np
cimport numpy as np
from libcpp cimport bool


cdef extern from "cv.h":
    ctypedef struct IplImage
    cdef void cvReleaseImage(IplImage ** image)
    ctypedef struct c_CvSize "CvSize":
        int width
        int height


cdef extern from "Python.h":
    object PyString_FromStringAndSize(char *s, Py_ssize_t len)


from cpython.ref cimport PyObject, Py_XINCREF, Py_XDECREF


cdef extern from "opencv2numpy.h":
    cdef PyObject * pyopencv_from(IplImage * img)


cdef extern from "SequenceReader.h":
    ctypedef struct c_Reader "SequenceReader":
        bool Open(char * filename, int first, int last, int is_color)
        void Close()
        IplImage * Read(int pos)
        int First()
        int Last()
        int Next()
        c_CvSize Size()


cdef extern from "SequenceReader.h" namespace "SequenceReader":
    cdef c_Reader * Create(char * filename, int first,
        int last, int is_color)
    cdef void Destroy(c_Reader ** reader)


cdef class SequenceReader(object):
    cdef c_Reader * thisptr

    def __init__(self, filename, first=-1, last=-1, is_color=-1):
        """Open sequence specified by 'filename'. If first and last are set
        (not -1) then open only the subsequence first:last+1. The is_color
        option is the same as in OpenCV: -1 don't care, 0 no, 1 yes."""
        self.thisptr = Create(filename, first, last, is_color)
        if self.thisptr == NULL:
            raise IOError('Could not open image sequence "%s"' % filename)

    def __cinit__(self):
        self.thisptr = NULL

    def __dealloc__(self):        
        Destroy(&self.thisptr)

    def read(self, index):
        """Read frame at index 'index'."""
        cdef IplImage * frame = self.thisptr.Read(index)
        if frame==NULL:
            raise IndexError('Cannot get frame %i.' % index)
        pyframe = <object>pyopencv_from(frame)
        # the returned pointer already has refcount==1 and the <object> cast
        # increments it to 2; however, pyframe has the only reference
        Py_XDECREF(<PyObject*>pyframe)
        cvReleaseImage(&frame)
        return pyframe

    @property
    def first(self):
        """The index of the first frame."""
        return self.thisptr.First()

    @property
    def last(self):
        """The index of the last frame (inclusive)."""
        return self.thisptr.Last()

    @property
    def shape(self):
        """Frame shape as a (height, width) tuple (currently excludes depth)."""
        cdef c_CvSize size = self.thisptr.Size()
        return (size.height, size.width)

    def __iter__(self):
        for i in range(self.thisptr.First(), self.thisptr.Last() + 1):
            yield i, self.read(i)


#
# Author: Vlad Morariu
# Created: 2012-07-06
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
    ctypedef struct c_CvSize "CvSize":
        int width
        int height
    ctypedef struct c_CvArr "CvArr"
    ctypedef struct c_CvMat "CvMat":
        pass
    cdef void cvReleaseImage(IplImage ** image)
    cdef void cvReleaseMat(c_CvMat ** mat)
    cdef c_CvMat cvMat(int rows, int cols, int type, void * data)
    cdef int IPL2CV_DEPTH(int depth)
    cdef int CV_8UC(int n)


cdef extern from "Python.h":
    object PyString_FromStringAndSize(char *s, Py_ssize_t len)


from cpython.ref cimport PyObject, Py_XINCREF, Py_XDECREF


cdef extern from "opencv2numpy.h":
    cdef PyObject * pyopencv_from(IplImage * img)


cdef extern from "SequenceWriter.h":
    ctypedef struct c_Writer "SequenceWriter":
        bool Open(char * filename, int fourcc, double fps, c_CvSize frame_size, int is_color)
        void Close()
        void Write(c_CvArr * image, int pos)
        int Next()
        #CvSize Size()


cdef extern from "SequenceWriter.h" namespace "SequenceWriter":
    cdef c_Writer * Create(char * filename, int fourcc,
        int fps, c_CvSize size, int is_color)
    cdef void Destroy(c_Writer ** writer)


cdef class SequenceWriter:
    cdef c_Writer * thisptr

    def __init__(self, filename, fourcc=0, fps=30, shape=(0, 0), is_color=1):
        """Initialize the sequence writer. The arguments correspond
           to those found in OpenCV's VideoWriter C interface. The fourcc, fps,
           and shape flags are used only by the OpenCV's VideoWriter.
        """
        cdef c_CvSize csize
        csize.height, csize.width = shape
        self.thisptr = Create(filename, fourcc, fps, csize, is_color)
        if self.thisptr == NULL:
            raise IOError('Could not open image sequence "%s"' % filename)

    def __cinit__(self):
        self.thisptr = NULL

    def __dealloc__(self):        
        Destroy(&self.thisptr)

    def write(self, np.ndarray[np.uint8_t, ndim=3, mode="c"] frame, index=-1):
        """Write a frame at the indended index in the video. If index==-1, then
           the index is set to self.next."""
        if not((frame.shape[2] == 1) or (frame.shape[2] == 3)):
            raise ValueError('the third dimension of \'frame\' should be 1 or 3')
        cdef c_CvMat m = cvMat(frame.shape[0], frame.shape[1],
                             CV_8UC(frame.shape[2]), <void*>frame.data)
        self.thisptr.Write(<c_CvArr*>&m, index)

    @property
    def next(self):
        """The index of the next frame that will be written."""
        return self.thisptr.Next()


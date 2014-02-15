// File: opencv2numpy.cpp
// Purpose: Code excerpted from OpenCV's python/cv2.cpp source (modified slightly)
// to convert OpenCV data to numpy arrays. 
// Author: Vlad Morariu
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
//-------------------------------------------------------------------------------------
//
// The original OpenCV code is provided under the following license:
// IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
// 
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
// 
// 
//                           License Agreement
//                For Open Source Computer Vision Library
// 
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2008-2011, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
// 
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
// 
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
// 
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
// 
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
#include "opencv2numpy.h"
#include <Python.h>
#include "numpy/arrayobject.h"
#include "numpy/ndarrayobject.h"
#include "opencv2/core/core.hpp"
using namespace cv;

static int failmsg(const char *fmt, ...)
{
    char str[1000];
    
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(str, sizeof(str), fmt, ap);
    va_end(ap);
    
    PyErr_SetString(PyExc_TypeError, str);
    return 0;
}


static size_t REFCOUNT_OFFSET = (size_t)&(((PyObject*)0)->ob_refcnt) +
    (0x12345678 != *(const size_t*)"\x78\x56\x34\x12\0\0\0\0\0")*sizeof(int);

static inline PyObject* pyObjectFromRefcount(const int* refcount)
{
    return (PyObject*)((size_t)refcount - REFCOUNT_OFFSET);
}

static inline int* refcountFromPyObject(const PyObject* obj)
{
    return (int*)((size_t)obj + REFCOUNT_OFFSET);
}

class NumpyAllocator : public MatAllocator
{
public:
    NumpyAllocator() {
      import_array(); // Added by Vlad to make PyArray_SimpleNew work.
    }
    ~NumpyAllocator() {}
    
    void allocate(int dims, const int* sizes, int type, int*& refcount,
                  uchar*& datastart, uchar*& data, size_t* step)
    {
        int depth = CV_MAT_DEPTH(type);
        int cn = CV_MAT_CN(type);
        const int f = (int)(sizeof(size_t)/8);
        int typenum = depth == CV_8U ? NPY_UBYTE : depth == CV_8S ? NPY_BYTE :
                      depth == CV_16U ? NPY_USHORT : depth == CV_16S ? NPY_SHORT :
                      depth == CV_32S ? NPY_INT : depth == CV_32F ? NPY_FLOAT :
                      depth == CV_64F ? NPY_DOUBLE : f*NPY_ULONGLONG + (f^1)*NPY_UINT;
        int i;
        npy_intp _sizes[CV_MAX_DIM+1];
        for( i = 0; i < dims; i++ )
            _sizes[i] = sizes[i];
        if( cn > 1 )
        {
            /*if( _sizes[dims-1] == 1 )
                _sizes[dims-1] = cn;
            else*/
                _sizes[dims++] = cn;
        }
        PyObject* o = PyArray_SimpleNew(dims, _sizes, typenum);
        if(!o)
            CV_Error_(CV_StsError, ("The numpy array of typenum=%d, ndims=%d can not be created", typenum, dims));
        refcount = refcountFromPyObject(o);
        npy_intp* _strides = PyArray_STRIDES(o);
        for( i = 0; i < dims - (cn > 1); i++ )
            step[i] = (size_t)_strides[i];
        datastart = data = (uchar*)PyArray_DATA(o);
    }
    
    void deallocate(int* refcount, uchar* datastart, uchar* data)
    {
        if( !refcount )
            return;
        PyObject* o = pyObjectFromRefcount(refcount);
        Py_INCREF(o);
        Py_DECREF(o);
    }
};

NumpyAllocator g_numpyAllocator;
    
enum { ARG_NONE = 0, ARG_MAT = 1, ARG_SCALAR = 2 };

static int pyopencv_to(const PyObject* o, Mat& m, const char* name = "<unknown>", bool allowND=true)
{
    if(!o || o == Py_None)
    {
        if( !m.data )
            m.allocator = &g_numpyAllocator;
        return true;
    }
        
    if( !PyArray_Check(o) )
    {
        failmsg("%s is not a numpy array", name);
        return false;
    }
    
    int typenum = PyArray_TYPE(o);
    int type = typenum == NPY_UBYTE ? CV_8U : typenum == NPY_BYTE ? CV_8S :
               typenum == NPY_USHORT ? CV_16U : typenum == NPY_SHORT ? CV_16S : 
               typenum == NPY_INT || typenum == NPY_LONG ? CV_32S :
               typenum == NPY_FLOAT ? CV_32F :
               typenum == NPY_DOUBLE ? CV_64F : -1;
    
    if( type < 0 )
    {
        failmsg("%s data type = %d is not supported", name, typenum);
        return false;
    }
    
    int ndims = PyArray_NDIM(o);
    if(ndims >= CV_MAX_DIM)
    {
        failmsg("%s dimensionality (=%d) is too high", name, ndims);
        return false;
    }
    
    int size[CV_MAX_DIM+1];
    size_t step[CV_MAX_DIM+1], elemsize = CV_ELEM_SIZE1(type);
    const npy_intp* _sizes = PyArray_DIMS(o);
    const npy_intp* _strides = PyArray_STRIDES(o);
    bool transposed = false;
    
    for(int i = 0; i < ndims; i++)
    {
        size[i] = (int)_sizes[i];
        step[i] = (size_t)_strides[i];
    }
    
    if( ndims == 0 || step[ndims-1] > elemsize ) {
        size[ndims] = 1;
        step[ndims] = elemsize;
        ndims++;
    }
    
    if( ndims >= 2 && step[0] < step[1] )
    {
        std::swap(size[0], size[1]);
        std::swap(step[0], step[1]);
        transposed = true;
    }
    
    if( ndims == 3 && size[2] <= CV_CN_MAX && step[1] == elemsize*size[2] )
    {
        ndims--;
        type |= CV_MAKETYPE(0, size[2]);
    }
    
    if( ndims > 2 && !allowND )
    {
        failmsg("%s has more than 2 dimensions", name);
        return false;
    }
    
    m = Mat(ndims, size, type, PyArray_DATA(o), step);
    
    if( m.data )
    {
        m.refcount = refcountFromPyObject(o);
        m.addref(); // protect the original numpy array from deallocation
                    // (since Mat destructor will decrement the reference counter)
    };
    m.allocator = &g_numpyAllocator;
    
    if( transposed )
    {
        Mat tmp;
        tmp.allocator = &g_numpyAllocator;
        transpose(m, tmp);
        m = tmp;
    }
    return true;
}

static PyObject* pyopencv_from(const Mat& m)
{
    if( !m.data )
        Py_RETURN_NONE;
    Mat temp, *p = (Mat*)&m;
    if(!p->refcount || p->allocator != &g_numpyAllocator)
    {
        temp.allocator = &g_numpyAllocator;
        m.copyTo(temp);
        p = &temp;
    }
    p->addref();
    return pyObjectFromRefcount(p->refcount);
}

PyObject* pyopencv_from(const IplImage * img)
{
  Mat m(img);
  return pyopencv_from(m);
}

// File: opencv2numpy.h
// Purpose: Header file for code excerpted from OpenCV's python/cv2.cpp
// source (modified slightly) to convert OpenCV data to numpy arrays. 
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

#ifndef CV2NUMPY_H
#define CV2NUMPY_H

#include <Python.h>
#include "cv.h"

//int pyopencv_to(const PyObject* o, IplImage * img,
//  const char* name = "<unknown>", bool allowND=true);

PyObject* pyopencv_from(const IplImage * img);


#endif // CV2NUMPY_H

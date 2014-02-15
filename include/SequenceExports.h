// Author: Vlad Morariu
// Created: 2013-12-10
// Purpose: Defines the SEQUENCES_EXPORT flag
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
#ifndef SEQUENCE_EXPORTS_H
#define SEQUENCE_EXPORTS_H

#ifdef WIN32
  #if defined(SEQUENCES_STATIC) || defined(SEQUENCES_HEADER_ONLY)
    #define SEQUENCES_EXPORT
  #else
    #ifdef SEQUENCES_EXPORTS
      #define SEQUENCES_EXPORT __declspec(dllexport)
    #else
      #define SEQUENCES_EXPORT __declspec(dllimport)
    #endif
  #endif
#else
  #define SEQUENCES_EXPORT
#endif

#endif // SEQUENCE_EXPORTS_H

# Author: Vlad Morariu
# Created: 2013-12-09
# Purpose: Targets that test linking with the installed library.
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
cmake_minimum_required(VERSION 2.8)
project(test_sequences_library)

find_package(sequences REQUIRED)
include_directories(${SEQUENCES_INCLUDE_DIRS})

# Use the headers-only version.
add_executable(test_headers main.cpp)
target_link_libraries(test_headers ${SEQUENCES_LIBRARIES})
set_target_properties(test_headers
  PROPERTIES COMPILE_DEFINITIONS SEQUENCES_HEADER_ONLY)

# Link to the static sequences library
add_executable(test_static main.cpp)
target_link_libraries(test_static sequences_static)

# Link to the shared sequences library
add_executable(test_shared main.cpp)
target_link_libraries(test_shared sequences_shared)

# Test the compiled executables using the CTest framework on sample videos
set(ID Chase4_A2_C2_Act1_2_URBAN7_MC_AFTN_48ab8d33-c5af-11df-af3e-e80688cb869a)
set(URL http://s3.amazonaws.com/mindseye-y1-development/${ID}.mov)
set(MOV ${CMAKE_CURRENT_SOURCE_DIR}/../data/${ID}.mov)
set(TGZ ${CMAKE_CURRENT_BINARY_DIR}/${ID}.tar.gz::frame_%06i.png)
set(TAR ${CMAKE_CURRENT_BINARY_DIR}/${ID}.tar::frame_%06i.png)
set(PNG ${CMAKE_CURRENT_BINARY_DIR}/${ID}_frame_%06i.png)
if(NOT EXISTS ${MOV})
  file(DOWNLOAD ${URL} ${MOV})
endif()
if(EXISTS ${MOV})
  enable_testing()
  add_test(NAME test_headers COMMAND test_headers ${MOV} ${TAR})
  add_test(NAME test_shared COMMAND test_shared ${MOV} ${TAR})
  add_test(NAME test_to_tar COMMAND test_static ${MOV} ${TAR})
  add_test(NAME test_to_tgz COMMAND test_static ${MOV} ${TGZ})
  add_test(NAME test_to_png COMMAND test_static ${MOV} ${PNG})
  add_test(NAME test_from_tar COMMAND test_static ${TAR} ${PNG})
  add_test(NAME test_from_tgz COMMAND test_static ${TGZ} ${TAR})
  # add_test(NAME test_from_png COMMAND test_static ${PNG} ${TAR}) # TODO(Vlad): need to auto detect frames to run this test
  if(${BUILD_MULTIPNG})
    set(PNGV ${CMAKE_CURRENT_BINARY_DIR}/${ID}.pngv)
    add_test(NAME test_to_pngv COMMAND test_static ${MOV} ${PNGV})
    add_test(NAME test_from_pngv COMMAND test_static ${PNGV} ${TAR})
  endif()
endif()

# Author: Vlad Morariu
# Created: 2014-01-06
#
# Copyright (c) 2009-2014 Vlad Morariu
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

# Static library
# ==============
add_library(sequences_static STATIC SequenceReader.cpp SequenceWriter.cpp ${MULTIPNG_SRC})
target_link_libraries(sequences_static ${OpenCV_LIBS} ${LibArchive_LIBRARIES})
target_compile_definitions(sequences_static PUBLIC SEQUENCES_STATIC)
set_target_properties(sequences_static PROPERTIES
  PUBLIC_HEADER "SequenceReader.h;SequenceWriter.h")
install(TARGETS sequences_static EXPORT sequences-targets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  ARCHIVE DESTINATION "${INSTALL_LIB_DIR}" COMPONENT dev
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev)

# Shared library
# ==============
add_library(sequences_shared SHARED SequenceReader.cpp SequenceWriter.cpp ${MULTIPNG_SRC})
target_link_libraries(sequences_shared ${OpenCV_LIBS} ${LibArchive_LIBRARIES})
target_compile_definitions(sequences_shared PRIVATE SEQUENCES_EXPORTS)
set_target_properties(sequences_shared PROPERTIES OUTPUT_NAME sequences)
set_target_properties(sequences_shared PROPERTIES
  PUBLIC_HEADER "SequenceReader.h;SequenceWriter.h")
install(TARGETS sequences_shared EXPORT sequences-targets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  LIBRARY DESTINATION "${INSTALL_LIB_DIR}" COMPONENT dev
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev)

# Executable
# ==========
add_executable(sequences SequencesMain.cpp)
target_link_libraries(sequences ${OpenCV_LIBS} ${LibArchive_LIBRARIES})
set_target_properties(sequences PROPERTIES COMPILE_DEFINITIONS SEQUENCES_HEADER_ONLY)
file(GLOB SEQUENCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Sequence*.[ch]*")
file(GLOB MULTIPNG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/MultiPng*.[ch]*")
set(SEQUENCES_FILES "${SEQUENCES_FILES}" "${MULTIPNG_FILES}")
set_target_properties(sequences PROPERTIES
  PUBLIC_HEADER "${SEQUENCES_FILES}")  # TODO(Vlad): add a library target and export headers with it--this should be part of the library target
install(TARGETS sequences EXPORT sequences-targets
  RUNTIME DESTINATION "${INSTALL_BIN_DIR}" COMPONENT bin
  PUBLIC_HEADER DESTINATION "${INSTALL_INCLUDE_DIR}" COMPONENT dev)  # TODO(Vlad): should be part of library target

# Tests
# =====
if(BUILD_TESTS)
  enable_testing()
  add_test(NAME sequences_runs COMMAND sequences)
  set_tests_properties(sequences_runs
    PROPERTIES PASS_REGULAR_EXPRESSION "Could not open sequence!")
endif()

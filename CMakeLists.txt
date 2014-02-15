# Author: Vlad Morariu
# Created: 2013-11-14
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

project(sequences)
set(VERSION 0.1.0)

# Project options
# ===============
option(BUILD_MULTIPNG "Build with MultiPNG reader/writer." OFF)
option(BUILD_PYTHON "Build/install python extension." ON)
option(BUILD_TESTS "Build tests." ON)
option(PYTHON_USER_FLAG "Pass --user flag to distutils to install python extension into the user directory." OFF)

# Build output directories (to keep shared libs and executables together)
# ========================================================================
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Installation directories (the user can override these)
# ======================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include/sequences CACHE PATH
  "Installation directory for header files")
if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/CMake/sequences)
endif()
set(INSTALL_CMAKE_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH
  "Installation directory for CMake files")
# Make relative paths absolute (needed later on)
foreach(p LIB BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

# General dependencies and include directories
# ============================================
find_package(OpenCV REQUIRED core highgui)
find_package(LibArchive REQUIRED)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/src
                    ${LibArchive_INCLUDE_DIRS})
if(${BUILD_MULTIPNG})
  set(MULTIPNG_SRC MultiPng.cpp)
  add_definitions(-DUSE_MULTIPNG)
endif()

# Library and executable.
# =======================
add_subdirectory(src)

# Python extension
# ================
if(BUILD_PYTHON)
  add_subdirectory(python)
endif()

# Export targets to *config*.cmake files so that CMake can find the package.
# ==========================================================================
# Add all targets to the build-tree export set
export(TARGETS sequences FILE "${PROJECT_BINARY_DIR}/sequences-targets.cmake")
# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
#export(PACKAGE sequences)
# Create the sequences-config.cmake and sequences-config-version files
file(RELATIVE_PATH REL_INCLUDE_DIR "${INSTALL_CMAKE_DIR}"
  "${INSTALL_INCLUDE_DIR}")
# ... for the build tree
set(CONF_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
configure_file(sequences-config.cmake.in
  "${PROJECT_BINARY_DIR}/sequences-config.cmake" @ONLY)
# ... for the install tree
set(CONF_INCLUDE_DIRS "\${SEQUENCES_CMAKE_DIR}/${REL_INCLUDE_DIR}")
configure_file(sequences-config.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/sequences-config.cmake" @ONLY)
# ... for both
configure_file(sequences-config-version.cmake.in
  "${PROJECT_BINARY_DIR}/sequences-config-version.cmake" @ONLY)
# Install the sequences-config.cmake and sequences-config-version.cmake
install(FILES
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/sequences-config.cmake"
  "${PROJECT_BINARY_DIR}/sequences-config-version.cmake"
  DESTINATION "${INSTALL_CMAKE_DIR}" COMPONENT dev)
# Install the export set for use with the install-tree
install(EXPORT sequences-targets DESTINATION
  "${INSTALL_CMAKE_DIR}" COMPONENT dev)

# Tests
# =============
if(BUILD_TESTS)
  include(CTest)
  # test sequences-config.cmake file and generated archives/binaries by trying
  # to link to the library in various ways--the following add_subdirectory
  # command needs to come after sequences-config.cmake is generated
  set(sequences_DIR ${PROJECT_BINARY_DIR})
  add_subdirectory(tests)
endif()

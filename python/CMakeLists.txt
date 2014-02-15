# Author: Vlad Morariu
# Created: 2013-11-14
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
find_program(PYTHON "python")

if (PYTHON)
  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
  set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/setup.py")
  set(DEPS "${CMAKE_CURRENT_SOURCE_DIR}/sequence_reader.pyx"
           "${CMAKE_CURRENT_SOURCE_DIR}/sequence_writer.pyx")
  set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/build/timestamp")

  if(PYTHON_USER_FLAG)
    set(USER "--user")
    set(PREFIX "")
  else()
    set(PREFIX "--prefix \"${CMAKE_INSTALL_PREFIX}\"")
  endif()

  get_directory_property(_include_directories INCLUDE_DIRECTORIES)
  configure_file(${SETUP_PY_IN} ${SETUP_PY})

  add_custom_command(OUTPUT ${OUTPUT}
                     COMMAND ${PYTHON} ${SETUP_PY} build
                     COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
                     DEPENDS ${DEPS})

  add_custom_target(sequences_python ALL DEPENDS ${OUTPUT})

  if(BUILD_TESTS)
    enable_testing()
    add_test(test_python ${PYTHON} setup.py test)
  endif()

  install(CODE
    "execute_process(COMMAND ${PYTHON} ${SETUP_PY} install ${PREFIX} ${USER})")
endif()


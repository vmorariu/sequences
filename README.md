Sequences
========= 

Frame-accurate random access is usually achieved by first converting compressed
videos to multi file image sequences, since most codecs are only reliable for
sequential decoding. Unfortunately, long videos lead to thousands of files,
which are slow and cumbersome to deal with on many common file systems.

This C++/python library reads and writes image sequences without requiring
thousands of intermediate files, focusing on frame accuracy, interoperability,
and low overhead:

- frame-accuracy: the code is tested to ensure that random access from
  supported formats is bitwise equivalent regardless of input format (files
  readable by ffmpeg, multi file image sequences, archives of multiple files,
  etc.)
- interoperability: in the absence of the sequences library, all read
  operations can be done with the standard ffmpeg command-line program; all
  output produced by the sequences library can be read by the tar command-line  
  program and standard image readers
- low overhead: the sequences code is a lightweight wrapper around standard
  libraries such as ffmpeg and libarchive. Decoded output is piped by ffmpeg
  (running in a separate process, which takes advantage of multiple cores and
  avoids writing decode frames to disk) directly into the sequences code;
  libarchive uses very little overhead to store images.

In older versions, video files were read via a wrapper around OpenCV's
VideoReader, and intermediate output was written to (or read from) a custom
format equivalent to running 'cat *.png > custom_file.pngv' on an image
sequence. These older readers/writers are now disabled by default, but they can
still be used if enabled.

The library can read:

- any video file readable by ffmpeg
- image sequences using any image format readable by OpenCV
- archives of image sequences using any image format readable by OpenCV and any
  format readable by libarchive (tar and tar.gz have been tested)
- concatenated PNG files (optional)

The library can write to:

- image sequences in any image format writable by OpenCV
- archives of image sequences using any image format writable by OpenCV and any
  archive format writable by libarchive (tar and tar.gz have been tested)
- concatenated PNG files (optional)

The library does not currently:

- provide _fast_ random access for gzipped archives (tar.gz) or ffmpeg files.
  (Currently, reading backwards from these formats means starting from the
  beginning, sacrificing speed for frame-accuracy; fast random access would
  require random access capability from the underlying ffmpeg/gzip decoder)
- write using lossy video compression (e.g., via ffmpeg or the OpenCV writer).
  Lossy compression can be achieved only within each image (e.g., using jpg
  files). As a workaround for lossy compression of output videos, image
  sequences can be converted to videos by a separate ffmpeg call.

Installation
------------

To install, assuming CMake can find compile-time dependencies (OpenCV and
LibArchive) and that the current directory is the base source directory:

    mkdir build
    cd build
    cmake ..
    cmake --build . --config Release
    cmake -P cmake_install.make

### Dependencies

General:
- OpenCV
- LibArchive
- ffmpeg (not linked to directly; ffmpeg is started as a separate process; it
  is assumed that ffmpeg is on the runtime executable PATH)

Python extension:
- Cython
- Numpy

### What if CMake cannot find dependencies?

In the following command examples, replace OPENCV_PATH, LIBARCHIVE_PATH, and
ZLIB_PATH with the actual paths to OpenCV, LibArchive, and zlib.

If OpenCV or its CMake config files are not installed in a standard location,
the appropriate path needs to be provided to CMake. For example, assume the
"OpenCVConfig.cmake" file is in the "OPENCV_PATH/opencv/build" directory, where
"OPENCV_PATH" is some non-standard OpenCV installation path. CMake can be
informed of this path via the -DOpenCV_DIR flag:

    cmake -DOpenCV_DIR="OPENCV_PATH/opencv/build" ..

Similarly, if libarchive and its headers are not installed in the standard
location, you can use the -DLibArchive_INCLUDE_DIR and -DLibArchive_LIBRARY
flags to point CMake to the location of "archive.h" and the shared/static
library (e.g., "libarchive.[a|so|lib]"):

    cmake -DLibArchive_INCLUDE_DIR="LIBARCHIVE_PATH/include" -DLibArchive_LIBRARY="LIBARCHIVE_PATH/lib/libarchive.so" ..

Note that if -DLibArchive_LIBRARY is set to a static library (e.g.,
libarchive.a on Linux or archive_static.lib on Windows), all other static
library needed by libarchive (e.g., zlib) must be provided on the provided,
since a statically linked library does not include its dependencies in the .a
or .lib file. For example:

    cmake -DLibArchive_INCLUDE_DIR="LIBARCHIVE_PATH/include" -DLibArchive_LIBRARY="LIBARCHIVE_PATH/lib/libarchive.a;ZLIB_PATH/libz.a" ..

### Installing to the user local directory or a non-standard path

To install both C/C++ code and python code to a non-standard prefix path (i.e.,
not /usr/ on Linux, not C:/Program Files/ on Windows):

    cmake -DCMAKE_INSTALL_PREFIX='some other path' ..

To install the python extension to the user's local directory (no need for
admin privileges; overrides installation prefix if it is also provided):

    cmake -DPYTHON_USER_FLAG=ON ..

### Installing on Windows

For a 64-bit project on Windows, you may need to add the -G flag to the
configuration command, e.g.,

    cmake -G "Visual Studio 10 Win64" ..

Also, on Windows, if libarchive is compiled only with Zlib support (assuming
you have a static version of zlib named 'zlibstat.lib') you would need to use
the following command:

    cmake -DLibArchive_INCLUDE_DIR="${LIBARCHIVE_PATH}/include" -DLibArchive_LIBRARY="${LIBARCHIVE_PATH}/lib/archive_static.lib;${ZLIB_PATH}/zlibstat.lib;advapi32.lib;user32.lib" ..

The advapi32.lib and user32.lib files are standard system files that also need
to be linked in statically.


Examples
--------

The tests in tests/main.cpp and python/tests.py show how the reader and writer
can be used with C++ and python. The ffmpeg reader assumes that the first frame
has index 0; the multi-file and archive code use the actual index in the
filename as index.

### Python

Include the modules, create the reader/writer, write the frames, and close the
reader/writer (the output file in some cases is finalized only after the writer
is deallocated)

    import sequence_reader
    import sequence_writer 

    # assume that input_fn and output_fn have been set    
    r = sequence_reader.SequenceReader(input_fn)
    w = sequence_writer.SequenceWriter(output_fn, shape=r.shape)
    for index, frame in r:
        w.write(frame, index)
    r, w = None, None  # deallocates the reader/writer (finalizes writing)
    
    # display video (requires cv2 in addition to the sequence reader/writer)
    import cv2
    r = sequence_reader.SequenceReader(input_fn)
    for index, frame in r:
        cv2.imshow('display', frame)
        cv2.waitKey()
    cv2.destroyWindow('display')
   
### C++

Include the reader and write headers, call the Create() functions, which
perform format autodetection, returning NULL on failure, and then call the Read
and Write functions. The image returned by Read must be released by the caller
using the cvReleaseImage() function. The reader and writer are released using
the Destroy() function. First() and Last() return the index of the first and
last frame (inclusive). See the header files for argument descriptions.

Example includes:

    #include "SequenceReader.h"
    #include "SequenceWriter.h"

Example code creating reader/writer and reading/writing frames, and performing
cleanup:

    SequenceReader * reader = SequenceReader::Create(input_fn, -1, -1, 1);
    SequenceWriter * writer = SequenceWriter::Create(
      output_fn, 0, 30, reader->Size(), 1);
    
    for(int f = reader->First(); f <= reader->Last(); f++)
    {
      IplImage * image = reader->Read(f);
      writer->Write(image, f);
      cvReleaseImage(&image);
    }
    
    SequenceReader::Destroy(&reader);
    SequenceWriter::Destroy(&writer);

See tests/CMakeLists.txt for how to link to the library using headers-only,
static, or shared linking using CMake.

### Executable

To view video any file that the reader can read using a simple OpenCV GUI:

    sequences input_file

The executable can also convert and merge video files; see the executable help:

    sequences -h


Author
------

Vlad I. Morariu (morariu@umiacs.umd.edu).


License
-------

This code is covered under [Apache License v2.0](http://www.apache.org/licenses/LICENSE-2.0).


Acknowledgments
---------------

I wrote this code initially for my dissertation while I was a graduate student
at the University of Maryland, and I subsequently used (and improved) it for
other research projects at the University of Maryland. These research projects
were partially supported by US Office of Naval Research surveillance grant
N00014-09-10044 and DARPA Mind's Eye grant W911NF-10-C-0083. The views and
conclusions contained herein are those of the authors and should not be
interpreted as necessarily representing those of the University of Maryland,
ONR, or DARPA.


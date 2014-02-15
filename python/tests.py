#
# File: tests.py
# Purpose: test the python extensions for the video sequence reader/writer
# Author: Vlad Morariu
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
import cv2
import hashlib
import time
import numpy as np
import cPickle as pickle
import os
import subprocess
import shutil
import unittest
from sequence_reader import SequenceReader
from sequence_writer import SequenceWriter


DATA_DIR = '../data'  # this is where videos are downloaded
TMP_DIR = '../data/tmp'  # this is where temporary videos are written


def main():
    unittest.main()


def get_local_filename(filename, local_dir):
    """Download file if it has not already been downloaded. Return filename."""
    local_filename = local_dir + '/' + os.path.basename(filename)
    if not os.path.exists(local_filename):
        import urllib
        print('Downloading %s to %s...' % (filename, local_filename))
        if not os.path.exists(local_dir):
            os.makedirs(local_dir)
        urllib.urlretrieve(filename, local_filename)
    return local_filename


def convert_formats(r, w):
    """Copy all frames from the reader to the writer."""
    for f in range(r.first, r.last + 1):
        w.write(r.read(f), f)


def check_ffmpeg_determinism(fn, tmpdir, frames, iterations, subsample=100):
    """Sometimes ffmpeg 'eats' the first frame, or replaces
    the second frame with a duplicate of the first. This is
    an upstream problem..."""
    hashes = None
    for i in range(iterations):
        print('  iter %i...' % i)
        hashes_ = get_video_hashes_cmd(fn, tmpdir, frames, subsample)
        if (hashes != None) and (hashes_ != hashes):
            show_mismatch(hashes, hashes_)
            mismatch = known_mismatch(hashes, hashes_)
            if mismatch == None:
                raise ValueError('Unknown mismatch...')
            print('Known mismatch (upstream problem): %s' % mismatch)
        hashes = hashes_


def check_formats(inputs, nsamples=20, maxlength=500, seqonly=False):
    """Check that various formats of the same video yield exactly the same
       frames."""
    hashes = []
    times = []
    orders = []

    for fmt, filename, first, last, is_color in inputs:
        print(fmt, filename, first, last, is_color)
        r = SequenceReader(filename, first, last, is_color)
        #display(r)

        # try to access a bad frame index
        try:
            r.read(-2)
            raise Exception('bad index not caught')
        except IndexError:
            pass

        # create up with frame read orders
        if not orders:
            length = r.last - r.first + 1
            length = min(maxlength, length)
            orders.append(('sequential', np.arange(0, length)))
            if not seqonly:
                orders.append(('random', np.random.randint(length, size=nsamples)))
                orders.append(('random_fwd', np.sort(np.random.randint(length, size=nsamples))))
                orders.append(('random_bwd', -np.sort(-np.random.randint(length, size=nsamples))))
                orders.append(('reverse_skip', np.arange(length - 1, -1, -max(1, length / nsamples))))

        # read the frames in the specified order and compute a hash of the frames
        hashes.append([])
        times.append([])
        for desc, order in orders:
            print('fmt: %s, order: %s' % (fmt, desc))
            start = time.time()
            hashes[-1].append((desc, get_video_hashes(r, r.first + order)))
            stop = time.time()
            times[-1].append((desc, stop - start))
        print('')

    match_str = {True: 'match', False: 'mismatch'}
    for (input, h, t) in zip(inputs, hashes, times):
        print(input[0])
        for i, (d, hi) in enumerate(h):
            print('%12s: %s, %.3gs' % (d, match_str[hi == hashes[0][i][1]], t[i][1]))
            if hi != hashes[0][i][1]:
                show_mismatch(hashes[0][i][1], hi)
                mismatch = known_mismatch(hashes[0][i][1], hi)
                if mismatch == None:
                    raise ValueError('Unknown mismatch...')
                print('Known mismatch (upstream problem): %s' % mismatch)
                
        if h != hashes[0]:
            with open('mismatch.pkl', 'wb') as fp:
                pickle.dump((inputs, orders, hashes, times), fp)
        print('')


def get_video_hash(reader, frames, subsample=100):
    """Returns an accumulated hash of the requested frames."""
    sha = hashlib.sha256()
    for f in frames:
        im = reader.read(f)
        sha.update(im.ravel()[::subsample].tostring())
    return sha.hexdigest()


def get_video_hashes(reader, frames, subsample=100):
    """Returns a list of hashes, one for each of the requested frames."""
    hashes = []
    for f in frames:
        im = reader.read(f)
        sha = hashlib.sha256()
        sha.update(im.ravel()[::subsample].tostring())
	hashes.append(sha.hexdigest())
    return hashes


def get_video_hashes_cmd(fn_in, dir_out, frames, subsample=100):
    """Uses ffmpeg on the command-line to create a sequence of pngs and
       compute the frame hash values. NOTE: dir_out is deleted."""
    if not os.path.exists(dir_out):
        os.makedirs(dir_out)
    pat_out = dir_out + '/frames%06d.png'
    cmd = ['ffmpeg', '-i', fn_in, '-vsync', '0', '-vframes', str(max(frames)), pat_out]
    #cmd = ['ffmpeg', '-i', fn_in, '-vframes', str(max(frames)), pat_out]
    devnull = open(os.devnull, 'w')
    subprocess.check_call(cmd, stdout=devnull, stderr=devnull)
    hashes = []
    for f in frames:
        im = cv2.imread(pat_out % f)
        sha = hashlib.sha256()
        sha.update(im.ravel()[::subsample].tostring())
	hashes.append(sha.hexdigest())
    try:
	    shutil.rmtree(dir_out)
    except OSError:
	    pass
    return hashes


def show_mismatch(hashes1, hashes2):
    h2i = {}
    for i, h in enumerate(hashes1):
        h2i.setdefault(h, []).append(i)
    for i, (h1, h2) in enumerate(zip(hashes1, hashes2)):
        if h1 != h2:
            print('mismatch: frame %i of video 2 matches frames %s of video 1' % (
                i, h2i.setdefault(h2, [])))


def known_mismatch(hashes1, hashes2):
    """Returns a string if this is a known mismatch."""
    def frame_0_dup_(h1, h2):  # asymmetric version
       return ((h1[0] == h2[0]) and 
               (h1[2:] == h2[2:]) and
               (h1[1] != h2[1] and h2[1] == h1[0]))
    def frame_0_dup(h1, h2):
       return frame_0_dup_(h1, h2) or frame_0_dup_(h2, h1)
    def frame_0_missing(h1, h2):
       return (h1[1:] == h2[:-1]) or (h2[:1] == h1[:-1])
    for func in [frame_0_dup, frame_0_missing]:
       if func(hashes1, hashes2):
           return func.__name__
    return None


def display(reader):
    for f in range(reader.first, reader.last + 1):
        im = reader.read(f)
        cv2.imshow('display', im)
        if cv2.waitKey(1) == 27:
            break
    cv2.destroyAllWindows()


class TestSequences(unittest.TestCase):

    def test_frame_accuracy(self):
        """Test frame accuracy of sequence reader/writer for multiple formats.
           TODO: Split up into setUp, tearDown, and parameterized tests.
        """
        inputs = [
            ('mov', 'http://s3.amazonaws.com/mindseye-y1-development/COLLIDE2_A1_C1_Act1_PARK_MC_AFTN_b43925a1-07b6-11e0-98f2-e80688cb869a.mov', -1, -1, 1),]
        formats = [
            ('tar', '.tar::frames_%06i.png'),
            ('tgz', '.tar.gz::frames_%06i.png'),
            ('png', '/frames_%06i.png')]
        max_frames_ffmpeg = 10  # max frames for checking ffmpeg determinism 
        check_determinism = False
        iterations = 20  # for checking determinism

        for fmt, filename, first, last, is_color in inputs:
            filename = get_local_filename(filename, DATA_DIR) 
            r = SequenceReader(filename, first, last, is_color)
      
            # convert the input video to all formats of interest
            outputs = []
            for fmt_out, suffix in formats:
                fn_out = (TMP_DIR + '/' + 
                          os.path.splitext(os.path.split(filename)[1])[0] + suffix)
                if not os.path.isdir(os.path.dirname(fn_out)):
                    os.makedirs(os.path.dirname(fn_out))
                print('Converting to \'%s\'...' % fn_out)
                convert_formats(r, SequenceWriter(fn_out, 0, 30, r.shape, 1))
                outputs.append((fmt_out, fn_out, r.first, r.last, is_color))

            # check determinism by repeated reads (ffmpeg cmdline and python readers)
            print('Checking for deterministic ffmpeg reads...')
            check_ffmpeg_determinism(
                filename, TMP_DIR + '/ffmpeg',
                range(1, r.last - r.first + 2)[:max_frames_ffmpeg], iterations)
            if check_determinism:
                print('Checking for deterministic reads...')
                check_formats([(fmt, filename, first, last, is_color),]*iterations, seqonly=True)
                for output in outputs:
                    check_formats([output,]*iterations, seqonly=True)

            # read all videos and compare hashes of all frames for various read orders
            check_formats([(fmt, filename, first, last, is_color),] + outputs)

            # delete output videos
            shutil.rmtree(TMP_DIR)


if __name__ == '__main__':
    main()


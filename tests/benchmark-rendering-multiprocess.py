# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import time
import os
import multiprocessing as mp
import argparse
import sys
import numpy as np
import cv2

from House3D import objrender, create_default_config
from House3D.objrender import Camera, RenderMode

def worker(idx, device, num_iter):
    api = objrender.RenderAPI(args.width, args.height, device=device)
    api.printContextInfo()
    mappingFile = cfg['modelCategoryFile']
    colormapFile = cfg['colorFile']

    assert os.path.isfile(mappingFile) and os.path.isfile(colormapFile)
    api.loadScene(args.obj, mappingFile, colormapFile)
    cam = api.getCamera()

    start = time.time()
    for t in range(num_iter):
        if t % 2 == 1:
            api.setMode(RenderMode.RGB)
        else:
            api.setMode(RenderMode.SEMANTIC)
        mat = np.array(api.render(), copy=False)
    end = time.time()
    print("Worker {}, speed {:.3f} fps".format(idx, num_iter / (end - start)))


if __name__ == '__main__':
    """
    Usage:
    ./benchmark-rendering-multiprocess.py path/to/house.obj --num-proc 9 --num-gpu 3

    Expected per-process speed with house: 00065ecbdd7300d35ef4328ffe871505, 120x90:
    1. Tesla M40, EGL backend, nvidia375.39:
    1proc, 1gpu: 690fps
    3proc, 1gpu: 500x3fps
    5proc, 1gpu: 360x5 should see 99% GPU Utilization here
    8proc, 8gpu: 650x8fps (roughly linear scaling)

    2. GTX 1080Ti, nvidia387.34:
    1proc: 1367fps
    3proc: 1005x3fps
    5proc: 680x5fps

    3. Quadro GP100:
    With EGL backend,nvidia384.81:
    1proc, 1gpu: 700fps
    3proc, 1gpu: 600x3fps
    5proc, 1gpu: 500x5fps
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('obj')
    parser.add_argument('--num-proc', type=int, default=1)
    parser.add_argument('--num-gpu', type=int, default=1)
    parser.add_argument('--width', type=int, default=120)
    parser.add_argument('--height', type=int, default=90)
    parser.add_argument('--num-iter', type=int, default=5000)
    args = parser.parse_args()

    global cfg
    cfg = create_default_config('.')

    procs = []
    for i in range(args.num_proc):
        device = i % args.num_gpu
        procs.append(mp.Process(target=worker, args=(i, device, args.num_iter)))

    for p in procs:
        p.start()

    for p in procs:
        p.join()


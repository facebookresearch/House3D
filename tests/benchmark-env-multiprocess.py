# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import time
import os
import multiprocessing as mp
import argparse
import numpy as np
import random

from House3D import objrender
from House3D import Environment, create_default_config


def worker(idx, house_id, device):
    colormapFile = "../metadata/colormap_coarse.csv"
    api = objrender.RenderAPI(w=args.width, h=args.height, device=device)
    env = Environment(api, house_id, cfg)
    N = 15000
    start = time.time()
    cnt = 0
    env.reset()
    for t in range(N):
        cnt += 1
        env.move_forward(random.random() * 3, random.random() * 3)
        mat = env.render()
        if (cnt % 50 == 0):
            env.reset()
    end = time.time()
    print("Worker {}, speed {:.3f} fps".format(idx, N / (end - start)))


if __name__ == '__main__':
    """
    Usage:
    python benchmark-env-multiprocess.py /path/to/house.obj --num-proc 9 --num-gpu 3

    Speed is expected to be about 20% slower than pure rendering.

    Experiment result on house 00065ecbdd7300d35ef4328ffe871505
    1. GTX 1080, EGL backend
    1proc, 1gpu: 708fps
    3proc, 1gpu: 556x3fps, 52% GPU util
    5proc, 1gpu: 430x5fps, 71% GPU util
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('obj')
    parser.add_argument('--num-proc', type=int, default=1)
    parser.add_argument('--num-gpu', type=int, default=1)
    parser.add_argument('--width', type=int, default=120)
    parser.add_argument('--height', type=int, default=90)
    args = parser.parse_args()

    prefix = os.path.dirname(os.path.dirname(args.obj))
    house_id = os.path.basename(os.path.dirname(args.obj))

    global cfg
    cfg = create_default_config(prefix)

    procs = []
    for i in range(args.num_proc):
        device = i % args.num_gpu
        procs.append(mp.Process(target=worker, args=(i, house_id, device)))

    for p in procs:
        p.start()

    for p in procs:
        p.join()


# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import time
import os
import sys
import tqdm
import numpy as np
import cv2
import argparse

from House3D import objrender, create_default_config
from House3D.objrender import Camera, RenderMode

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('obj')
    parser.add_argument('--width', type=int, default=800)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--device', type=int, default=0)
    parser.add_argument('--interactive', action='store_true',
        help='run interactive rendering (does not work under ssh)')
    args = parser.parse_args()

    cfg = create_default_config('.')

    api = objrender.RenderAPI(args.width, args.height, device=args.device)
    api.printContextInfo()

    api.loadScene(args.obj, cfg['modelCategoryFile'], cfg['colorFile'])
    cam = api.getCamera()

    modes = [RenderMode.RGB, RenderMode.SEMANTIC, RenderMode.INSTANCE, RenderMode.DEPTH]
    for t in tqdm.trange(10000):
        mode = modes[t % len(modes)]
        api.setMode(mode)
        mat = np.array(api.render())
        if mode == RenderMode.DEPTH:
            infmask = mat[:, :, 1]
            mat = mat[:, :, 0] * (infmask == 0)
        else:
            mat = mat[:, :, ::-1]   # cv expects bgr

        if args.interactive:
            cv2.imshow("aaa", mat)
            key = cv2.waitKey(0)
            if key == 27 or key == ord('q'): #esc
                break
            elif key == ord('w'):
                cam.pos += cam.front * 0.5
            elif key == ord('s'):
                cam.pos -= cam.front * 0.5
            elif key == ord('a') or key == 81:
                cam.pos -= cam.right * 0.5
            elif key == ord('d') or key == 83:
                cam.pos += cam.right * 0.5
            elif key == ord('h'):
                cam.yaw -= 5
                # need to call updateDirection to make the change to yaw/pitch
                # take effect
                cam.updateDirection()
            elif key == ord('l'):
                cam.yaw += 5
                cam.updateDirection()
            elif key == 82:
                cam.pos += cam.up * 0.5
            elif key == 84:
                cam.pos -= cam.up * 0.5
            else:
                print("Unknown key:", key)
        else:
            cv2.imwrite("mode={}.png".format(t), mat)
            if t == len(modes) - 1:
                break

# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import tqdm
import cv2
import numpy as np
import os

import pdb

from House3D import objrender, Environment, load_config


if __name__ == '__main__':
    api = objrender.RenderAPI(w=250, h=250, device=0)
    cfg = load_config('config.json')

    houseID = np.random.choice(os.listdir(cfg['prefix']))
    env = Environment(api, houseID, cfg)
    cam = api.getCamera()

    for t in tqdm.trange(1000):
        env.reset()
        mat = env.renderCubeMap()
        cv2.imshow("aaa", mat)

        key = cv2.waitKey(0)
        while key in [ord('h'), ord('l'), ord('s')]:
            if key == ord('h'):
                cam.yaw -= 5
                cam.updateDirection()
            elif key == ord('l'):
                cam.yaw += 5
                cam.updateDirection()
            elif key == ord('s'):
                cv2.imwrite("sample.png", mat)

            mat = env.renderCubeMap()
            cv2.imshow("aaa", mat)
            key = cv2.waitKey(0)

        if not env.keyboard_control(key):
            break

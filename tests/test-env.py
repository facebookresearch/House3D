# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import tqdm
import cv2

from House3D import objrender, Environment, load_config


if __name__ == '__main__':
    api = objrender.RenderAPI(w=600, h=450, device=0)
    cfg = load_config('config.json')

    env = Environment(api, '00065ecbdd7300d35ef4328ffe871505', cfg)

    # fourcc = cv2.VideoWriter_fourcc(*'X264')
    # writer = cv2.VideoWriter('out.avi', fourcc, 30, (1200, 900))
    for t in tqdm.trange(1000):
        if t % 1000 == 0:
            env.reset()
        mat = env.debug_render()
        # writer.write(mat)
        cv2.imshow("aaa", mat)
        key = cv2.waitKey(0)
        if not env.keyboard_control(key):
            break
    # writer.release()

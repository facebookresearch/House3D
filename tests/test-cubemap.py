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

from House3D import objrender, Environment, load_config, House
from House3D.objrender import RenderMode

ROOM_TYPE = 'living_room'
ROBOT_RAD = 3.0
MODES = [RenderMode.RGB, RenderMode.SEMANTIC, RenderMode.INSTANCE]

def create_house(houseID, config, robotRadius=0.1):
    print('Loading house {}'.format(houseID))
    objFile = os.path.join(config['prefix'], houseID, 'house.obj')
    jsonFile = os.path.join(config['prefix'], houseID, 'house.json')
    assert (os.path.isfile(objFile) and os.path.isfile(jsonFile)), '[Environment] house objects not found! objFile=<{}>'.format(objFile)
    cachefile = os.path.join(config['prefix'], houseID, 'cachedmap1k.pkl')
    if not os.path.isfile(cachefile):
        cachefile = None

    house = House(jsonFile, objFile, config["modelCategoryFile"],
                  CachedFile=cachefile, GenRoomTypeMap=True,
                  RobotRadius=robotRadius)
    return house

def get_rand_house(cfg):
    house = None
    houseID = None
    while house is None or not house.hasRoomType(ROOM_TYPE):
        houseID = np.random.choice(os.listdir(cfg['prefix']))
        house = create_house(houseID, cfg, robotRadius=3.0)
        print('Room types available: {}'.format(house.all_roomTypes))
    return (houseID, house)


def create_api():
    return objrender.RenderAPI(w=250, h=250, device=0)


def reset_random(env, house):
    location = house.getRandomLocation(ROOM_TYPE)
    env.reset(*location)


if __name__ == '__main__':
    api = create_api()
    cfg = load_config('config.json')

    houseID, house = get_rand_house(cfg)
    env = Environment(api, house, cfg)
    cam = api.getCamera()
    mode_idx = 0

    for t in tqdm.trange(1000):
        reset_random(env, house)
        mat = cv2.cvtColor(env.render_cube_map(), cv2.COLOR_BGR2RGB)
        cv2.imshow("aaa", mat)

        key = cv2.waitKey(0)
        while key in [ord('h'), ord('l'), ord('s'), ord('n'), ord('i')]:
            if key == ord('h'):
                cam.yaw -= 5
                cam.updateDirection()
            elif key == ord('l'):
                cam.yaw += 5
                cam.updateDirection()
            elif key == ord('s'):
                cv2.imwrite("{}_mode{}.png".format(houseID, mode_idx), mat)
            elif key == ord('i'):
                mode_idx = (mode_idx + 1) % len(MODES)
                env.set_render_mode(MODES[mode_idx])
            elif key == ord('n'):
                houseID, house = get_rand_house(cfg)
                env = Environment(api, house, cfg)
                cam = api.getCamera()
                reset_random(env, house)

            mat = cv2.cvtColor(env.render_cube_map(), cv2.COLOR_BGR2RGB)
            cv2.imshow("aaa", mat)
            key = cv2.waitKey(0)

        if not env.keyboard_control(key):
            break

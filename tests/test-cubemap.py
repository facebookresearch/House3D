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

ROOM_TYPE = 'living_room'
ROBOT_RAD = 3.0

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
    while house is None or not house.hasRoomType(ROOM_TYPE):
        houseID = np.random.choice(os.listdir(cfg['prefix']))
        house = create_house(houseID, cfg, robotRadius=3.0)
        print('Room types available: {}'.format(house.all_roomTypes))
    return house


def create_api():
    return objrender.RenderAPI(w=250, h=250, device=0)


def reset_random(env, house):
    location = house.getRandomLocation(ROOM_TYPE)
    env.reset(*location)


if __name__ == '__main__':
    api = create_api()
    cfg = load_config('config.json')

    house = get_rand_house(cfg)
    env = Environment(api, house, cfg)
    cam = api.getCamera()

    for t in tqdm.trange(1000):
        reset_random(env, house)
        mat = env.render_cube_map()
        cv2.imshow("aaa", mat)

        key = cv2.waitKey(0)
        while key in [ord('h'), ord('l'), ord('s'), ord('n')]:
            if key == ord('h'):
                cam.yaw -= 5
                cam.updateDirection()
            elif key == ord('l'):
                cam.yaw += 5
                cam.updateDirection()
            elif key == ord('s'):
                cv2.imwrite("sample.png", mat)
            elif key == ord('n'):
                house = get_rand_house(cfg)
                env = Environment(api, house, cfg)
                cam = api.getCamera()
                reset_random(env, house)

            mat = env.render_cube_map()
            cv2.imshow("aaa", mat)
            key = cv2.waitKey(0)

        if not env.keyboard_control(key):
            break

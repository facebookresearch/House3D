# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.
import sys, os
import time
import numpy as np
import random
import logging
import six
import cv2
import pickle

import gym
from .house import House
from .objrender import RenderMode

__all__ = ['Environment', 'MultiHouseEnv']

USE_FAST_COLLISION_CHECK = True  # flag for using fast collision check
FAST_COLLISION_CHECK_SAMPLES = 10


def _vec_to_array(pos):
    return np.array([pos.x, pos.y, pos.z])


def create_house(houseID, config, cachefile=None):
    objFile = os.path.join(config['prefix'], houseID, 'house.obj')
    jsonFile = os.path.join(config['prefix'], houseID, 'house.json')
    assert (os.path.isfile(objFile) and os.path.isfile(jsonFile)), '[Environment] house objects not found! objFile=<{}>'.format(objFile)
    if cachefile is None:
        cachefile = os.path.join(config['prefix'], houseID, 'cachedmap1k.pkl')
    if not os.path.isfile(cachefile):
        cachefile = None
    house = House(jsonFile, objFile, config["modelCategoryFile"],
                  CachedFile=cachefile, GenRoomTypeMap=True)
    return house

def local_create_house(h, config):
    if not isinstance(h, House):
        h = create_house(h, config)
    return h

class Environment():
    def __init__(self, api, house, config, seed=None):
        """
        Args:
            api: A RenderAPI or RenderAPIThread instance.
            house: either a house object or a house id
            config: configurations containing path to meta-data files
            seed: if not None, set the seed
        """
        self.config = config
        if not isinstance(house, House):
            house = create_house(house, config)
        self.house = house
        if not hasattr(house, '_id'):
            house._id = 0

        self.cachedLocMap = None

        self.api_mode = RenderMode.RGB
        self.api = api

        if seed is not None:
            np.random.seed(seed)
            random.seed(seed)
        self.viewer = None

        self._load_objects()

    def _load_objects(self):
        # load objects in self.house to GPU
        self.api.loadScene(self.house.objFile, self.house.metaDataFile, self.config['colorFile'])
        self.api.setMode(self.api_mode)
        self.cam = self.api.getCamera()

    def set_render_mode(self, mode):
        """
        Args:
            mode (str or enum): one of 'rgb', 'depth', 'semantic',
                RenderMode.RGB, RenderMode.DEPTH, RenderMode.SEMANTIC
        """
        mappings = {
                'rgb': RenderMode.RGB,
                'depth': RenderMode.DEPTH,
                'semantic': RenderMode.SEMANTIC,
                'instance': RenderMode.INSTANCE }
        if isinstance(mode, six.string_types):
            mode = mode.lower()
            self.api_mode = mappings[mode]
        else:
            assert mode in set(mappings.values())
            self.api_mode = mode
        self.api.setMode(self.api_mode)

    def render(self, mode=None, copy=False):
        """
        Args:
            mode (str or enum or None): If None, use the current mode.

        Returns:
            An image.
        """
        if mode is None:
            return np.array(self.api.render(), copy=copy)
        else:
            backup = self.api_mode
            self.set_render_mode(mode)
            ret = np.array(self.api.render(), copy=copy)
            self.set_render_mode(backup)
            return ret


    def render_cube_map(self, mode=None, copy=False):
        """
        Args:
            mode (str or enum or None): If None, use the current mode.

        Returns:
            An image of resolution 6w * h
        """
        if mode is None:
            return np.array(self.api.renderCubeMap(), copy=copy)
        else:
            backup = self.api_mode
            self.set_render_mode(mode)
            ret = np.array(self.api.renderCubeMap(), copy=copy)
            self.set_render_mode(backup)
            return ret


    @property
    def resolution(self):
        api_resolution = self.api.resolution()
        return (api_resolution.w, api_resolution.h)

    def gen_2dmap(self, x=None, y=None, resolution=None):
        """
        Args:
            x, y: the agent's location. Will use the current camera position by default.
            resolution: (w, h) integer, same as the rendering by default.
        Returns:
            An RGB image of 2d localization, robot locates at (x, y)
        """
        if x is None:
            x, y = self.cam.pos.x, self.cam.pos.z
        if resolution is None:
            resolution = self.resolution
        house = self.house
        n_row = house.n_row

        # TODO move cachedLocMap to House
        if self.cachedLocMap is None:
            locMap = np.zeros((n_row + 1, n_row + 1, 3), dtype=np.uint8)
            for i in range(n_row):  # w
                for j in range(n_row):  # h
                    if house.obsMap[i, j] == 0:
                        locMap[j, i, :] = 255
                    if house.canMove(i, j):
                        locMap[j, i, :2] = 200  # purple
            self.cachedLocMap = locMap.copy()
        else:
            locMap = self.cachedLocMap.copy()

        rad = house.robotRad / house.L_det * house.n_row
        x, y = house.to_grid(x, y)
        locMap = cv2.circle(locMap, (x, y), int(rad), (255, 50, 50), thickness=-1)
        locMap = cv2.resize(locMap, resolution)
        return locMap

    def _check_collision_fast(self, pA, pB, num_samples=5):
        ratio = 1.0 / num_samples
        for i in range(num_samples):
            p = (pB - pA) * (i + 1) * ratio + pA
            gx, gy = self.house.to_grid(p[0], p[2])
            if (not self.house.canMove(gx, gy)) or (not self.house.isConnect(gx, gy)):
                return False
        return True

    def _check_collision(self, pA, pB, num_samples=5):
        if USE_FAST_COLLISION_CHECK:
            return self._check_collision_fast(pA, pB, FAST_COLLISION_CHECK_SAMPLES)
        # pA is always valid
        ratio = 1.0 / num_samples
        for i in range(num_samples):
            p = (pB - pA) * (i + 1) * ratio + pA
            if not self.house.check_occupy(p[0], p[2]):
                return False
        return True

    def move_forward(self, dist_fwd, dist_hor=0):
        """
        Move with `fwd` distance to the front and `hor` distance to the right.
        Both distance are float numbers.

        Returns:
            bool - success or not
        """
        pos = self.cam.pos
        pos = pos + self.cam.front * dist_fwd
        pos = pos + self.cam.right * dist_hor
        return self.move(pos.x, pos.z)

    def rotate(self, deg):
        """
        Change yaw by deg.
        Roll and pitch are currently fixed in this environment.
        """
        self.cam.yaw += deg
        self.cam.updateDirection()

    def move(self, x, y):
        """
        Move to x, y if allowed

        Returns:
            bool - success or not
        """
        state = _vec_to_array(self.cam.pos)
        if self._check_collision(state, np.array([x, state[1], y])):
            self.cam.pos.x = x
            self.cam.pos.z = y
            return True
        return False

    def reset(self, x=None, y=None, yaw=None):
        """
        Reset (teleport) the agent to a new location and pose.
        If no location is given, will use a random valid one.
        Valid means some place in open area.
        """
        if x is None:
            gx, gy = random.choice(self.house.connectedCoors)
            x, y = self.house.to_coor(gx, gy, True)
        if yaw is None:
            yaw = np.random.rand() * 360 - 180
        self.cam.pos.x = x
        self.cam.pos.y = self.house.robotHei
        self.cam.pos.z = y
        self.cam.yaw = yaw
        self.cam.updateDirection()

    def reset_house(self, house_id=None):
        """
        virtual function, no need to switch house here
        """
        pass

    @property
    def num_house(self):
        return 1

    @property
    def info(self):
        loc = (self.cam.pos.x, self.cam.pos.z)
        gx,gy = self.house.to_grid(loc[0],loc[1])
        return dict(house_id=0, loc=loc, yaw=self.cam.yaw, grid=(gx, gy),
                    front=np.array([self.cam.front.x, self.cam.front.z], dtype=np.float32),
                    right=np.array([self.cam.right.x, self.cam.right.z], dtype=np.float32))

    def show(self, img=None, close=False, renderMapLoc=None, storeImage=None, display=True, renderSegment=False):
        """
        When RenderMapLoc is not None, it should be a tuple of reals, the location of robot
        """
        if close:
            if self.viewer is not None:
                self.viewer.close()
            return
        if display and (self.viewer is None):
            from gym.envs.classic_control import rendering
            # TODO: to support visualization of the 2d localization
            self.viewer = rendering.SimpleImageViewer()
        if img is None:
            self.api.setMode(RenderMode.RGB)
            img = np.array(self.api.render(), copy=False)
        if renderSegment:
            self.api.setMode(RenderMode.SEMANTIC)
            segment = np.array(self.api.render(), copy=False)
            if img is not None:
                img = np.concatenate([img, segment], axis=1)
            else:
                img = segment
        if renderMapLoc is not None:
            assert len(renderMapLoc) == 2, 'renderMapLoc must be a tuple of reals, the location of the robot'
            locMap = self.gen_2dmap(x=renderMapLoc[0], y=renderMapLoc[1])
            if img is not None:
                img = np.concatenate([img, locMap], axis=1)
            else:
                img = locMap
        if storeImage is not None:
            with open(storeImage, 'wb') as f:
                pickle.dump(img, f)
        else:
            if display:
                self.viewer.imshow(img)
        return img

    def debug_render(self):
        rgb = self.render(RenderMode.RGB)
        semantic = self.render(RenderMode.SEMANTIC)
        depth = self.render(RenderMode.DEPTH)
        infmask = depth[:, :, 1]
        depth = depth[:, :, 0] * (infmask == 0)
        depth = np.stack([depth] * 3, axis=2)

        map2d = self.gen_2dmap()
        concat1 = np.concatenate((rgb, semantic), axis=1)
        concat2 = np.concatenate((depth, map2d), axis=1)
        ret = np.concatenate((concat1, concat2), axis=0)
        ret = ret[:, :, ::-1]
        return ret


    def keyboard_control(self, key, scale=1.0):
        """
        Args:
            key (int): key returned by cv2.waitKey
            scale (float): scaling factor for the step size

        Returns:
            bool - if False, quit
        """
        if key == 27 or key == ord('q'): #esc
            return False
        elif key == ord('w'):
            self.move_forward(0.1 * scale)
        elif key == ord('s'):
            self.move_forward(-0.1 * scale)
        elif key == ord('a') or key == 81:
            self.move_forward(0, -0.1 * scale)
        elif key == ord('d') or key == 83:
            self.move_forward(0, 0.1 * scale)
        elif key == ord('h'):
            self.rotate(-2 * scale)
        elif key == ord('l'):
            self.rotate(2 * scale)
        else:
            logging.warning("Unknown key: {}".format(key))
        return True


class MultiHouseEnv(Environment):
    def __init__(self, api, houses, config, seed=None):
        """
        Args:
            houses: a list of house id or `House` instance.
        """
        print('Generating all houses ...')
        ts = time.time()
        if not isinstance(houses, list):
            houses = [houses]
        from multiprocessing import Pool
        _args = [(h, config) for h in houses]
        k = len(houses)
        with Pool(k) as pool:
            self.all_houses = pool.starmap(local_create_house, _args)  # parallel version for initialization
        print('  >> Done! Time Elapsed = %.4f(s)' % (time.time() - ts))
        for i, h in enumerate(self.all_houses):
            h._id = i
            h._cachedLocMap = None
        super(MultiHouseEnv, self).__init__(
            api, house=self.all_houses[0], config=config, seed=seed)

    def reset_house(self, house_id=None):
        """
        Reset the scene to a different house.

        Args:
            house_id (int): a integer in range(0, self.num_house).
                If None, will choose a random one.
        """
        if house_id is None:
            self.house = random.choice(self.all_houses)
        else:
            self.house = self.all_houses[house_id]
        self._load_objects()

    def cache_shortest_distance(self):
        # TODO
        for house in self.all_houses:
            house.cache_all_target()

    @property
    def info(self):
        ret = super(MultiHouseEnv, self).info
        ret['house_id'] = self.house._id
        return ret

    @property
    def num_house(self):
        return len(self.all_houses)

    def gen_2dmap(self, x=None, y=None, resolution=None):
        # TODO move cachedLocMap to House
        self.cachedLocMap = self.house._cachedLocMap
        retLocMap = super(MultiHouseEnv, self).gen_2dmap(x, y, resolution)
        if self.house._cachedLocMap is None:
            self.house._cachedLocMap = self.cachedLocMap
        return retLocMap

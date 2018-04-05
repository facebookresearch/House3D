# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import numpy as np
import os
import unittest

from House3D import objrender, Environment, load_config, House
from House3D.objrender import RenderMode

PIXEL_MAX = np.iinfo(np.uint16).max
ROOM_TYPE = 'kitchen'
SIDE = 256
NEAR = 0.3


def depth_of_inverse_depth(inverse_depth):
    '''Calculate float depth from int 16-bit int inverse depth.'''
    assert inverse_depth.dtype == np.uint16
    return NEAR * PIXEL_MAX / inverse_depth.astype(np.float)


def create_house(houseID, config):
    obj_file = os.path.join(config['prefix'], houseID, 'house.obj')
    json_file = os.path.join(config['prefix'], houseID, 'house.json')
    assert (
        os.path.isfile(obj_file) and os.path.isfile(json_file)
    ), '[Environment] house objects not found! obj_file=<{}>'.format(obj_file)
    cache_file = os.path.join(config['prefix'], houseID, 'cachedmap1k.pkl')
    if not os.path.isfile(cache_file):
        cache_file = None
    return House(
        json_file,
        obj_file,
        config["modelCategoryFile"],
        CachedFile=cache_file,
        SetTarget=False)


def find_first_good_house(cfg):
    house_ids = os.listdir(cfg['prefix'])
    for house_id in house_ids:
        house = create_house(house_id, cfg)
        if house.hasRoomType(ROOM_TYPE):
            return (house_id, house)


class TestCubeMap(unittest.TestCase):
    def test_render(self):
        api = objrender.RenderAPI(w=SIDE, h=SIDE, device=0)
        cfg = load_config('config.json')
        houseID, house = find_first_good_house(cfg)
        env = Environment(api, house, cfg)
        location = house.getRandomLocation(ROOM_TYPE)
        env.reset(*location)

        # Check RGB
        env.set_render_mode(RenderMode.RGB)
        rgb = env.render_cube_map()
        self.assertEqual(rgb.shape, (SIDE, SIDE * 6, 3))

        # Check SEMANTIC
        env.set_render_mode(RenderMode.RGB)
        semantic = env.render_cube_map()
        self.assertEqual(semantic.shape, (SIDE, SIDE * 6, 3))

        # Check INSTANCE
        env.set_render_mode(RenderMode.INSTANCE)
        instance = env.render_cube_map()
        self.assertEqual(instance.shape, (SIDE, SIDE * 6, 3))

        # Check DEPTH
        env.set_render_mode(RenderMode.DEPTH)
        depth = env.render_cube_map()
        self.assertEqual(depth.dtype, np.uint8)
        self.assertEqual(depth.shape, (SIDE, SIDE * 6, 2))
        depth_value = depth[0, 0, 0] * 20.0 / 255.0

        # Check INVDEPTH
        env.set_render_mode(RenderMode.INVDEPTH)
        invdepth = env.render_cube_map()
        self.assertEqual(invdepth.dtype, np.uint8)
        self.assertEqual(invdepth.shape, (SIDE, SIDE * 6, 3))

        # Convert to 16 bit and then to depth
        id16 = 256 * invdepth[:, :, 0] + invdepth[:, :, 1]
        depth2 = depth_of_inverse_depth(id16)
        self.assertEqual(depth2.dtype, np.float)
        self.assertEqual(depth2.shape, (SIDE, SIDE * 6))

        # Check that depth value is within 5% of depth from RenderMode.DEPTH
        self.assertAlmostEqual(
            depth2[0, 0], depth_value, delta=depth_value * 0.05)


if __name__ == '__main__':
    unittest.main()

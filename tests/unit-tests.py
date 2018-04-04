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

ROOM_TYPE = 'kitchen'
SIDE = 256


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
        CachedFile=cache_file)


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
        self.assertEqual(depth.shape, (SIDE, SIDE * 6, 2))
        self.assertEqual(depth.dtype, np.uint8)


if __name__ == '__main__':
    unittest.main()

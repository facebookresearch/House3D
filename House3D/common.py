# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import json
import os


def load_config(filename):
    """
    Returns:
        dict
    """
    with open(filename) as file:
        config = json.load(file)

    # back-compat
    if 'csvFile' in config:
        config['modelCategoryFile'] = config['csvFile']
        del config['csvFile']

    required_files = ["prefix", "modelCategoryFile", "colorFile"]
    for f in required_files:
        assert f in config, 'Invalid config! key <{}> is missing!'.format(f)
        assert os.path.exists(
            config[f]), 'Invalid config! path <{}> not exists!'.format(
                config[f])
        if ('File' in f):
            assert os.path.isfile(
                config[f]), 'Invalid config! <{}> is not a valid file!'.format(
                    config[f])

    return config


def create_default_config(prefix, colormap='coarse'):
    assert os.path.isdir(prefix)
    assert colormap in ['coarse', 'fine']

    metadir = os.path.join(os.path.dirname(__file__), 'metadata')
    ret = {
        'colorFile':
        os.path.join(metadir, 'colormap_coarse.csv'
                     if colormap == 'coarse' else 'colormap_fine.csv'),
        'roomTargetFile':
        os.path.join(metadir, 'room_target_object_map.csv'),
        'modelCategoryFile':
        os.path.join(metadir, 'ModelCategoryMapping.csv'),
        'prefix':
        prefix
    }
    return ret

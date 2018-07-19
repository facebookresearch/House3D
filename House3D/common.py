# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import itertools
import sys
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


def detect_nvidia_devices():
    """
    When we are running inside cgroup/container which limits GPU visibility,
    EGL may fail because it can still detect all devices - even those it has no permission to access.
    In this case, users need to call this function and
    only use the device ids that's returned.

    Returns:
        A list of int, the available device id to use for rendering
    """
    assert sys.platform.startswith('linux')
    ret = []
    for k in itertools.count():
        dev = '/dev/nvidia{}'.format(k)
        if os.path.exists(dev):
            try:
                f = open(dev)
                f.close()
            except PermissionError:
                pass
            else:
                ret.append(k)
        else:
            break
    return list(range(len(ret)))

if __name__ == '__main__':
    print(detect_nvidia_devices())

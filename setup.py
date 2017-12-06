# Copyright 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the license found in the
# LICENSE file in the root directory of this source tree.

import setuptools
from setuptools import setup


setup(
    name='House3D',
    version='1.0.0',
    description='An awesome environment',
    include_package_data=True,
    package_data={'House3D': ['objrender.so', 'metadata/*.csv']}
)

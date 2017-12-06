#!/usr/bin/env python
# -*- coding: utf-8 -*-
# File: gen-color-mapping.py

import os, sys
import csv
from six.moves import map
import argparse

from palette import PALETTE_128, PALETTE_256

# class names that's not in ModelCategory
EXTRA_CLASSES = ['Ground', 'Box', 'Ceiling', 'OTHER']


def get_all_classes(model_cat_file, key='coarse_grained_class'):
    classes = set()
    with open(model_cat_file, 'r') as csvfile:
        reader = csv.DictReader(csvfile, delimiter=',', quotechar='|')
        for row in reader:
            classes.add(row[key])
    for c in EXTRA_CLASSES:
        classes.add(c)
    return classes

def generate_colors_custom(n):
    if n <= 128:
        return PALETTE_128[:n]
    elif n <= 256:
        return PALETTE_256[:n]
    raise ValueError(n)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cat', help='modelcategory file')
    parser.add_argument('-o', '--output', help='output csv file')
    parser.add_argument('--key', help='modelcategory key', default='coarse_grained_class')
    args = parser.parse_args()

    classes = get_all_classes(args.cat, args.key)

    colors = generate_colors_custom(len(classes))

    with open(args.output, 'w') as f:
        f.write("name,r,g,b\n")
        for klass, color in zip(classes, colors):
            f.write("{},{},{},{}\n".format(klass, color[0], color[1], color[2]))


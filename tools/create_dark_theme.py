#!/usr/bin/env python3

"""
Bootstraps a dark-mode icon for any light-mode icon
"""

import argparse
import os
import re

# These were quickly chosen as a good starting point based on checking contrast against
# some light- and dark-mode window backgrounds taken from Ubuntu:
# light mode: #FCFCFC
# dark mode:  #484848

COLOR_MAP = {
#    "#ffffff": "#111111",  # white
#    "#fff":    "#111111",  # white (short)
    "#333333": "#E0E0E0",  # off black
    "#333":    "#E0E0E0",  # off black (short)
    "#545454": "#F4EFF3",  # dark grey primary
    "#606060": "#F0EBF0",  # dark grey large area
    "#909090": "#d0d0d0",  # medium grey
    "#b9b9b9": "#8f8f8f",  # light grey 1
    "#c1c1c1": "#999999",  # light grey 2
    "#f3f3f3": "#545454",  # off white 1
    "#f5f5f5": "#545454",  # off white 2
    "#1A81C4": "#42B8EB",  # primary blue
    "#39b4ea": "#1A81C4",  # light blue
    "#bf2641": "#f2647e",  # primary red
}


def process(src_dir, target_dir):
    for entry in os.scandir(src_dir):
        if entry.is_file() and entry.path.endswith(".svg"):
            processOne(os.path.abspath(entry.path), target_dir)

def processOne(src, target_dir):
    #print('Processing {}'.format(src))
    with open(src, 'r') as f:
        svg = f.read()

        for key in COLOR_MAP.keys():
            expr = re.compile(r'({})'.format(key), re.I)
            svg = expr.sub(COLOR_MAP[key], svg)

        target_name = os.path.join(target_dir, os.path.basename(src))

        with open(target_name, 'w') as out:
            out.write(svg)
            print('Wrote {}'.format(target_name))

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("source_dir")
    parser.add_argument("target_dir")
    args = parser.parse_args()

    process(args.source_dir, args.target_dir)

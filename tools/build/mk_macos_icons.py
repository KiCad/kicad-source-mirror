#!/usr/bin/env python

# This script regenerates the .icns files for MacOS application packaging.
# You need to be running on MacOS to use it (for iconutil)
# You also need imagemagick + rsvg installed, with something like:
# brew install imagemagick --with-librsvg

import os
from shutil import copy, rmtree
from subprocess import call

ICON_SOURCES = "../../bitmaps_png/sources/"

ICONS = [
    ("bitmap2component", ("bitmap2component",), "../../bitmap2component"),
    ("cvpcb", ("cvpcb", "cvpcb_doc"), "../../cvpcb"),
    ("eeschema", ("eeschema", "eeschema_doc"), "../../eeschema"),
    ("gerbview", ("gerbview", "gerbview_doc"), "../../gerbview"),
    ("kicad", ("kicad", "kicad_doc"), "../../kicad"),
    ("pagelayout_editor", ("pl_editor", "pl_editor_doc"), "../../pagelayout_editor"),
    ("pcbcalculator", ("pcb_calculator",), "../../pcb_calculator"),
    ("pcbnew", ("pcbnew", "pcbnew_doc"), "../../pcbnew")
]

SIZES = [
    ("!16x16", ("16x16",)),
    ("!32x32", ("16x16@2x", "32x32")),
    ("!64x64", ("32x32@2x",)),
    ("!128x128", ("128x128",)),
    ("!256x256", ("128x128@2x", "256x256")),
    ("!512x512", ("256x256@2x", "512x512")),
    ("!1024x1024", ("512x512@2x",))
]

if __name__ == '__main__':

    for src, dest, finaldir in ICONS:

        src = ICON_SOURCES + "icon_" + src + ".svg"
        iconset = dest[0] + ".iconset"
        os.mkdir(iconset)

        for size, outputs in SIZES:
            dest_path = os.path.join(iconset, "icon_" + outputs[0] + ".png")
            print size, src, dest_path
            call(["convert", "-background", "none", "-density", "5000",
                  "-resize", size, src, dest_path])
            for duplicate in outputs[1:]:
                dupe_path = os.path.join(iconset, "icon_" + duplicate + ".png")
                copy(dest_path, dupe_path)

        call(["iconutil", "-c", "icns", iconset])

        rmtree(iconset)

        for dupe in dest[1:]:
            copy(dest[0] + ".icns", os.path.join(finaldir, dupe + ".icns"))

        copy(dest[0] + ".icns", finaldir)

        os.remove(dest[0] + ".icns")

#!/usr/bin/env python

# Regenerate the icons in resources/linux/mime based on the mime files
# and the original icons in bitmaps_png. Must be run from the scripts
# folder for the relative paths to work.
#
# This script assumes Inkscape is installed and in the PATH

import os, glob
import xml.etree.ElementTree as ET
from shutil import copyfile, rmtree
from subprocess import call

ICON_SOURCES = "../../resources/bitmaps_png/sources/"
DEST_FOLDER = "../../resources/linux/mime"

def icon_sourcename(icon):
    return ICON_SOURCES+"/icon_%s.svg" % icon


# Get a list of the applications we will install, their icons and mimes
app_icons = {}
for desktopfile in glob.glob(DEST_FOLDER+"/applications/*.desktop"):
    icon = None
    mimes = []
    for line in open(desktopfile):
        keypair = map(str.strip, line.split("="))
        if len(keypair) != 2: continue
        key, value = keypair
        if key == "Icon":
            icon = value
        elif key ==  "MimeType":
            mimes = [x.strip() for x in value.split(";") if str.strip(x)]
    if icon is None:
        print "WARNING: file '", desktopfile, "' contains no Icon entry, corrupted?"
        continue
    else:
        app_icons[icon] = mimes;

# Obtain the mime types we provide from the mime package XML
MIME_PACKAGE = DEST_FOLDER+"/mime/packages/kicad-kicad.xml"
mimepkg_root = ET.parse(MIME_PACKAGE).getroot()
mimepkg_mimetypes = [n.attrib['type'] for n in mimepkg_root]

# Reconcile mime types
mime_icons = {}
for mime in mimepkg_mimetypes:
    for icon, mimes in app_icons.iteritems():
        if mime in mimes:
            mime_icons[mime.replace('/','-')] = icon
            break
    else:
        print "WARNING: mimetype'", mime,"' is provided in the package, but no app is associated with it."


RESOLUTIONS = [16,22,24,32,48,64,128]


rmtree(DEST_FOLDER+'/icons')
os.makedirs(DEST_FOLDER+'/icons/hicolor/scalable/apps')
os.makedirs(DEST_FOLDER+'/icons/hicolor/scalable/mimetypes')
for r in RESOLUTIONS:
        os.makedirs(DEST_FOLDER+'/icons/hicolor/%ix%i/apps' % (r,r))
        os.makedirs(DEST_FOLDER+'/icons/hicolor/%ix%i/mimetypes' % (r,r))

for icon in app_icons.keys():
    copyfile(icon_sourcename(icon),
             DEST_FOLDER+"/icons/hicolor/scalable/apps/%s.svg" % icon)
    for r in RESOLUTIONS:
        call(['inkscape', '-f', icon_sourcename(icon),
              '-e', DEST_FOLDER+'/icons/hicolor/%ix%i/apps/%s.png' % (r, r, icon),
              '-w', str(r), '-h', str(r), '--export-area-snap'])


for mime, icon in mime_icons.iteritems():
    copyfile(icon_sourcename(icon),
             DEST_FOLDER+"/icons/hicolor/scalable/mimetypes/%s.svg" % mime)
    for r in RESOLUTIONS:
        call(['inkscape', '-f', icon_sourcename(icon),
              '-e', DEST_FOLDER+'/icons/hicolor/%ix%i/mimetypes/%s.png' % (r, r, mime),
              '-w', str(r), '-h', str(r), '--export-area-snap'])

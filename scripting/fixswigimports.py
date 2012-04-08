#!/usr/bin/env python

# the purpose of this script is rewriting the swig_import_helper
# call so it will not load _xxxxx.so/dso from inside a kicad executable
# because the kicad executable itself sill provide an _xxxxx module
# that's linked inside itself.
#
# for the normal module import it should work the same way with this
# fix in the swig_import_helper
#

from sys import argv,exit

if len(argv)<2:
    print "usage:"
    print "   fixswigimports.py file.py"
    print ""
    print "   will fix the swig import code for working inside KiCad"
    print "   where it happended that the external _pcbnew.so/dll was"
    print "   loaded too -and the internal _pcbnew module was to be used"
    exit(1)


filename = argv[1]

f = open(filename,"rb")
lines = f.readlines()
f.close()

f = open(filename,"wb")

doneOk = False

for l in lines:
    if l.startswith("if version_info >= (2,6,0):"):
        l = l.replace("version_info >= (2,6,0)","False")
        doneOk = True
    elif l.startswith("if False:"):  # it was already patched?
        doneOk = True
    f.write(l)

f.close()

if doneOk:
    print "swig_import_helper fixed for",filename
else:
    print "Error: the swig import helper was not fixed, check",filename
    print "       and fix this script: fixswigimports.py"
    exit(2)

    
exit(0)
    
    


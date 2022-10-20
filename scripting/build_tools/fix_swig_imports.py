#!/usr/bin/env python

# the purpose of this script is rewriting the swig_import_helper
# call added by Swig version < 4.0 in pcbnew.py so it will not load
# _xxxxx.so/dso from inside a kicad executable
# because the kicad executable itself sill provide an _xxxxx module
# that's linked inside itself.
#
# for the normal module import it should work the same way with this
# fix in the swig_import_helper
#
# For Swig version >= 4.0 this method does not exist so nothing useful is made

from __future__ import print_function

from sys import argv,exit


if len(argv)<2:
    print("usage:")
    print("   fix_swig_imports.py file.py")
    print("")
    print("   will fix the swig import code for working inside KiCad")
    print("   where it happened that the external _pcbnew.so/dll was")
    print("   loaded too -and the internal _pcbnew module was to be used")
    exit(1)


filename = argv[1]

f = open(filename,"rb")
lines = f.readlines()
f.close()


doneOk = False

if (len(lines)<4000):
    print("still building")
    exit(0)

txt = b""

for l in lines:
    if l.startswith(b"if _swig_python_version_info < (2, 7, 0):"):     # ok with swig version >= 4.0.0
        l = l.replace(b"_swig_python_version_info < (2, 7, 0)", b"False")
        doneOk = True
    if l.startswith(b"if _swig_python_version_info >= (2, 7, 0):"):     # ok with swig version >= 3.0.10
        l = l.replace(b"_swig_python_version_info >= (2, 7, 0)", b"False")
        doneOk = True
    elif l.startswith(b"elif _swig_python_version_info >= (2, 6, 0):"):  # needed with swig version >= 3.0.10
        l = l.replace(b"_swig_python_version_info >= (2, 6, 0)", b"False")
        doneOk = True
    if l.startswith(b"if version_info >= (2, 7, 0):"):     # ok with swig version >= 3.0.9
        l = l.replace(b"version_info >= (2, 7, 0)", b"False")
        doneOk = True
    elif l.startswith(b"elif version_info >= (2, 6, 0):"):  # needed with swig version >= 3.0.9
        l = l.replace(b"version_info >= (2, 6, 0)", b"False")
        doneOk = True
    elif l.startswith(b"if version_info >= (2,6,0):"):     # ok with swig version <= 3.0.2
        l = l.replace(b"version_info >= (2,6,0)", b"False")
        doneOk = True
    elif l.startswith(b"if version_info >= (2, 6, 0):"): # needed with swig version 3.0.3
        l = l.replace(b"version_info >= (2, 6, 0)", b"False")
        doneOk = True
    elif l.startswith(b"if False:"):  # it was already patched?
        doneOk = True
    txt = txt + l

f = open(filename,"wb")
f.write(txt)
f.close()

if doneOk:
    print("swig_import_helper fixed for", filename)
else:
    print("Error: the swig import helper was not fixed, check", filename)
    print("       and fix this script: fix_swig_imports.py")
    exit(2)


exit(0)




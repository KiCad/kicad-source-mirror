#!/usr/bin/env python
import sys
from pcbnew import *

filename=sys.argv[1]

pcb = LoadBoard(filename)

for module in pcb.GetFootprints():
    if sys.version_info < (3, 0):
        print("* Module: %s" % module.GetReference().encode())
    else:
        print("* Module: %s" % module.GetReference())
    module.Value().SetVisible(False)      # set Value as Hidden
    module.Reference().SetVisible(True)   # set Reference as Visible

SaveBoard("mod_"+filename, pcb)

#
# Example python script to generate an equivalent XML document from XML input
#
# Example: Round robin, XML to XML conversion
#

from __future__ import print_function

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import sys
import pdb


# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
canOpenFile = True
try:
    f = open(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print( __file__, ":", e, sys.stderr)
    f = sys.stdout
    canOpenFile = False

print(net.formatXML(), file=f)

if not canOpenFile:
    sys.exit(1)
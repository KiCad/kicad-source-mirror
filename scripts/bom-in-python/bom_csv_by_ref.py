#
# Example python script to generate a BOM from a KiCad generic netlist
#
"""
    @package
    Generate a BOM list file.
    Components are sorted by ref
    One component per line
    Fields are (if exist)
    Ref, Quantity, value, Part, footprint, 'Description', 'Vendor'
    Fields are separated by tabs
"""

from __future__ import print_function

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import csv
import sys

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
try:
    f = open(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout

# Create a new csv writer object to use as the output formatter, although we
# are created a tab delimited list instead!
out = csv.writer(f, lineterminator='\n', delimiter='\t', quoting=csv.QUOTE_NONE)

# override csv.writer's writerow() to support utf8 encoding:
def writerow( acsvwriter, columns ):
    utf8row = []
    for col in columns:
        txt=str(col);
        utf8row.append( txt.encode('utf-8') )
    acsvwriter.writerow( utf8row )

components = net.getInterestingComponents()

# Output a field delimited header line
writerow( out, ['Source:', net.getSource()] )
writerow( out, ['Date:', net.getDate()] )
writerow( out, ['Tool:', net.getTool()] )
writerow( out, ['Component Count:', len(components)] )
writerow( out, ['Ref', 'Value', 'Part', 'Documentation', 'Description', 'Vendor'] )

# Output all of the component information
for c in components:
    writerow( out, [c.getRef(), c.getValue(), c.getLibName() + ":" + c.getPartName(),
        c.getDatasheet(), c.getDescription(), c.getField("Vendor")])

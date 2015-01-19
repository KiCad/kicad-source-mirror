#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Ungrouped (One component per row) CSV output
#

"""
    @package
    Generate a csv list file.
    Components are sorted by ref and grouped by value
    One component per line
    Fields are (if exist)
    Ref, value, Part, footprint, Datasheet, Manufacturer, Vendor
"""

from __future__ import print_function

# Import the KiCad python helper module
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
    print( __file__, ":", e, sys.stderr )
    f = sys.stdout

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, lineterminator='\n', delimiter=',', quotechar="\"", quoting=csv.QUOTE_ALL)

# override csv.writer's writerow() to support utf8 encoding:
def writerow( acsvwriter, columns ):
    utf8row = []
    for col in columns:
        utf8row.append( str(col).encode('utf8') )
    acsvwriter.writerow( utf8row )

components = net.getInterestingComponents()

# Output a field delimited header line
writerow( out, ['Source:', net.getSource()] )
writerow( out, ['Date:', net.getDate()] )
writerow( out, ['Tool:', net.getTool()] )
writerow( out, ['Component Count:', len(components)] )
writerow( out, ['Ref', 'Value', 'Footprint', 'Datasheet', 'Manufacturer', 'Vendor'] )

# Output all of the component information (One component per row)
for c in components:
    writerow( out, [c.getRef(), c.getValue(), c.getFootprint(), c.getDatasheet(),
        c.getField("Manufacturer"), c.getField("Vendor")])


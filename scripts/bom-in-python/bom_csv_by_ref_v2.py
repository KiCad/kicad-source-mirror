#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Ungrouped (One component per row) CSV output
#

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
    print(__file__, ":", e, file=sys.stderr)
    f = stdout

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, lineterminator='\n', delimiter=',', quotechar="\"", quoting=csv.QUOTE_ALL)

# Output a field delimited header line
out.writerow(['Source:', net.getSource()])
out.writerow(['Date:', net.getDate()])
out.writerow(['Tool:', net.getTool()])
out.writerow(['Component Count:', len(net.components)])
out.writerow(['Ref', 'Value', 'Footprint', 'Datasheet', 'Manufacturer', 'Vendor'])

components = net.getInterestingComponents()

# Output all of the component information (One component per row)
for c in components:
    out.writerow([c.getRef(), c.getValue(), c.getFootprint(), c.getDatasheet(),
        c.getField("Manufacturer"), c.getField("Vendor")])


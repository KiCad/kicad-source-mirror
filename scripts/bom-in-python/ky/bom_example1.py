#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Tab delimited list (The same as std output) Ungrouped
#

# Import the KiCad python helper module and the csv formatter
import ky
import csv
import sys

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = ky.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
try:
    f = open(sys.argv[2], 'w')
except IOError:
    print >> sys.stderr, __file__, ":", e
    f = stdout

# Create a new csv writer object to use as the output formatter, although we
# are created a tab delimited list instead!
out = csv.writer(f, delimiter='\t', quoting=csv.QUOTE_NONE)

# Output a field delimited header line
out.writerow(['Source:', net.getSource()])
out.writerow(['Date:', net.getDate()])
out.writerow(['Tool:', net.getTool()])
out.writerow(['Component Count:', len(net.components)])         
out.writerow(['Ref', 'Value', 'Part', 'Documentation', 'Description', 'Vendor'])

# Output all of the component information
for c in net.components:
    out.writerow([c.getRef(), c.getValue(), c.getLib() + "/" + c.getPart(), 
        c.getDatasheet(), c.getDescription(), c.getField("Vendor")])

#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
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

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, delimiter=',', quotechar='\"', quoting=csv.QUOTE_ALL)

# Output a set of rows for a header providing general information
out.writerow(['Source:', net.getSource()])
out.writerow(['Date:', net.getDate()])
out.writerow(['Tool:', net.getTool()])
out.writerow(['Component Count:', len(net.components)])         
out.writerow(['Ref', 'Qnty', 'Value', 'Part', 'Datasheet', 'Description', 'Vendor'])

# Get all of the components in groups of matching parts + values (see ky.py)
grouped = net.groupComponents()

# Output all of the component information
for group in grouped:
    refs = ""

    # Add the reference of every component in the group and keep a reference 
    # to the component so that the other data can be filled in once per group
    for component in group:
        refs += component.getRef() + ", "
        c = component

    # Fill in the component groups common data
    out.writerow([refs, len(group), c.getValue(), c.getLib() + "/" + c.getPart(), c.getDatasheet(),
        c.getDescription(), c.getField("Vendor")])



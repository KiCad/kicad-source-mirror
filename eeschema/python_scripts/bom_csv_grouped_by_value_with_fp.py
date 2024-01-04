#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
#

"""
    @package
    Output: CSV (comma-separated)
    Grouped By: Value, Footprint, DNP
    Sorted By: Ref
    Fields: Ref, Qnty, Value, Cmp name, Footprint, Description, Vendor, DNP

    Command line:
    python "pathToFile/bom_csv_grouped_by_value_with_fp.py" "%I" "%O.csv"
"""

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import kicad_utils
import csv
import sys

# A helper function to filter/convert a string read in netlist
#currently: do nothing
def fromNetlistText( aText ):
    return aText

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
canOpenFile = True
try:
    f = kicad_utils.open_file_writeUTF8(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout
    canOpenFile = False

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, lineterminator='\n', delimiter=',', quotechar='\"', quoting=csv.QUOTE_ALL)

# Output a set of rows for a header providing general information
out.writerow(['Source:', net.getSource()])
out.writerow(['Date:', net.getDate()])
out.writerow(['Tool:', net.getTool()])
out.writerow( ['Generator:', sys.argv[0]] )
out.writerow(['Component Count:', len(net.components)])
out.writerow(['Ref', 'Qnty', 'Value', 'Cmp name', 'Footprint', 'Description', 'Vendor', 'DNP'])

# subset the components to those wanted in the BOM, controlled
# by <configure> block in kicad_netlist_reader.py
components = net.getInterestingComponents( excludeBOM=True )

# Get all of the components in groups of matching parts + values
# (see ky_generic_netlist_reader.py)
grouped = net.groupComponents(components)

# Output all of the component information
for group in grouped:
    refs = ""
    refs_l = []

    # Add the reference of every component in the group and keep a reference
    # to the component so that the other data can be filled in once per group
    for component in group:
        refs_l.append( fromNetlistText( component.getRef() ) )
        c = component

    refs = ", ".join(refs_l)

    # Fill in the component groups common data
    out.writerow([refs, len(group),
        fromNetlistText( c.getValue() ),
        fromNetlistText( c.getPartName() ),
        fromNetlistText( c.getFootprint() ),
        fromNetlistText( c.getDescription() ),
        fromNetlistText( c.getField("Vendor") ),
        c.getDNPString()])

if not canOpenFile:
    sys.exit(1)

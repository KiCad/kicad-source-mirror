#
# Example python script to generate a BOM from a KiCad generic netlist
#

"""
    @package
    Output: text file (tab-separated)
    Grouped By: ungrouped, one component per line
    Sorted By: Ref
    Fields: Ref, Value, Part, Footprint, Description, Vendor, DNP

    Command line:
    python "pathToFile/bom_txt_sorted_by_ref.py" "%I" "%O.txt"
"""

from __future__ import print_function

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
    f = kicad_utils.open_file_writeUTF8(sys.argv[2], 'w' )
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout
    canOpenFile = False

# Create a new csv writer object to use as the output formatter, although we
# are created a tab delimited list instead!
out = csv.writer(f, lineterminator='\n', delimiter='\t', quotechar="\"", quoting=csv.QUOTE_ALL)

# override csv.writer's writerow() to support utf8 encoding:
def writerow( acsvwriter, columns ):
    utf8row = []
    for col in columns:
        txt=str(col);
        utf8row.append( fromNetlistText(txt) )
    acsvwriter.writerow( utf8row )

components = net.getInterestingComponents( excludeBOM=True )

# Output a field delimited header line
writerow( out, ['Source:', net.getSource()] )
writerow( out, ['Date:', net.getDate()] )
writerow( out, ['Tool:', net.getTool()] )
writerow( out, ['Component Count:', len(components)] )
writerow( out, ['Ref', 'Value', 'Part', 'Footprint', 'Description', 'Vendor', 'DNP'] )

# Output all of the component information
for c in components:
    writerow( out, [c.getRef(), c.getValue(), c.getLibName() + ":" + c.getPartName(),
        c.getFootprint(), c.getDescription(), c.getField("Vendor"), c.getDNPString()])

if not canOpenFile:
    sys.exit(1)
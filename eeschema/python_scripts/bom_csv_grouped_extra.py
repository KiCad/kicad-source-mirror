#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
#

"""
    @package
    Output: CSV (comma-separated)
    Grouped By: Value, Footprint, DNP, specified extra fields
    Sorted By: Reference
    Fields: #, Reference, Qty, Value, Footprint, DNP, specified extra fields

    Outputs components grouped by Value, Footprint, and specified extra fields.
    Extra fields can be passed as command line arguments at the end, one field per argument.

    Command line:
    python "pathToFile/bom_csv_grouped_extra.py" "%I" "%O.csv" "Extra_Field1" "Extra_Field2"
"""

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import kicad_utils
import csv
import sys

# Get extra fields from the command line
extra_fields = sys.argv[3:]

comp_fields = ['Value', 'Footprint', 'DNP'] + extra_fields
header_names = ['#', 'Reference', 'Qty'] + comp_fields

def getComponentString(comp, field_name):
    if field_name == "Value":
        return comp.getValue()
    elif field_name == "Footprint":
        return comp.getFootprint()
    elif field_name == "DNP":
        return comp.getDNPString()
    elif field_name == "Datasheet":
        return comp.getDatasheet()
    else:
        return comp.getField( field_name )

def myEqu(self, other):
    """myEqu is a more advanced equivalence function for components which is
    used by component grouping. Normal operation is to group components based
    on their Value and Footprint.

    In this example of a more advanced equivalency operator we also compare the
    Footprint, Value, DNP and all extra fields passed from the command line. If
    these fields are not used in some parts they will simply be ignored (they
    will match as both will be empty strings).

    """
    result = True
    for field_name in comp_fields:
        if getComponentString(self, field_name) != getComponentString(other, field_name):
            result = False

    return result

# Override the component equivalence operator - it is important to do this
# before loading the netlist, otherwise all components will have the original
# equivalency operator.
kicad_netlist_reader.comp.__eq__ = myEqu

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

# Output a CSV header
out.writerow(header_names)

components = net.getInterestingComponents( excludeBOM=True )

# Get all of the components in groups of matching parts + values
# (see kicad_netlist_reader.py)
grouped = net.groupComponents(components)

# Output all of the component information
index = 1
for group in grouped:
    refs = ""
    refs_l = []

    # Add the reference of every component in the group and keep a reference
    # to the component so that the other data can be filled in once per group
    for component in group:
        refs_l.append( component.getRef() )
        c = component

    refs = ", ".join(refs_l)

    # Fill in the component groups common data
    row = []
    row.append( index )
    row.append( refs )
    row.append( len(group) )

    # Add the values of component-specific data
    for field_name in comp_fields:
        row.append( getComponentString( c, field_name ) )

    out.writerow(row)

    index += 1

if not canOpenFile:
    sys.exit(1)

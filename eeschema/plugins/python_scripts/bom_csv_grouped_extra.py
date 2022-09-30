#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
#

"""
    @package
    Output: CSV (comma-separated)
    Grouped By: Value, Footprint, specified extra fields
    Sorted By: Reference
    Fields: #, Reference, Qty, Value, Footprint, specified extra fields
    
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

def myEqu(self, other):
    """myEqu is a more advanced equivalence function for components which is
    used by component grouping. Normal operation is to group components based
    on their Value and Footprint.

    In this example of a more advanced equivalency operator we also compare the
    Footprint, Value and all extra fields passed from the command line. If 
    these fields are not used in some parts they will simply be ignored (they
    will match as both will be empty strings).

    """
    result = True
    if self.getValue() != other.getValue():
        result = False
    elif self.getFootprint() != other.getFootprint():
        result = False
    else:
        for field_name in extra_fields:
            if self.getField(field_name) != other.getField(field_name):
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
try:
    f = kicad_utils.open_file_writeUTF8(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout

# Create a new csv writer object to use as the output formatter
out = csv.writer(f, lineterminator='\n', delimiter=',', quotechar='\"', quoting=csv.QUOTE_ALL)

# Output a CSV header
out.writerow(['#', 'Reference', 'Qty', 'Value', 'Footprint'] + extra_fields)

# Get all of the components in groups of matching parts + values
# (see kicad_netlist_reader.py)
grouped = net.groupComponents()

# Output all of the component information
index = 1
for group in grouped:
    refs = ""

    # Add the reference of every component in the group and keep a reference
    # to the component so that the other data can be filled in once per group
    for component in group:
        refs += component.getRef() + ", "
        c = component

    # Remove trailing comma
    refs = refs[:-2]
    
    # Fill in the component groups common data
    row = []
    row.append( index )
    row.append( refs )
    row.append( len(group) )
    row.append( c.getValue() )
    row.append( c.getFootprint() )
    
    # Add the values of extra fields
    for field_name in extra_fields:
        row.append( c.getField( field_name ) )

    out.writerow(row)
        
    index += 1

#
# Example python script to generate a BOM from a KiCad generic netlist
# The KiCad generic xml netlist is expected to be encoded UTF-8
#
# Example: Sorted and Grouped HTML BOM with advanced grouping
#

"""
    @package
    Output: HTML
    Grouped By: Value, Part, Footprint, Tolerance, Manufacturer, Voltage, DNP
    Sorted By: Ref
    Fields: Ref, Qnty, Value, Part, Footprint, Description, Vendor, DNP

    Command line:
    python "pathToFile/bom_with_advanced_grouping.py" "%I" "%O.html"
"""

from __future__ import print_function

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import kicad_utils
import sys


# Start with a basic html template
html = """
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
        <title>KiCad BOM Example 5</title>
    </head>
    <body>
    <h1><!--SOURCE--></h1>
    <p><!--DATE--></p>
    <p><!--TOOL--></p>
    <p><!--COMPCOUNT--></p>
    <table>
    <!--TABLEROW-->
    </table>
    </body>
</html>
    """

def myEqu(self, other):
    """myEqu is a more advanced equivalence function for components which is
    used by component grouping. Normal operation is to group components based
    on their Value and Footprint.

    In this example of a more advanced equivalency operator we also compare the
    custom fields Voltage, Tolerance and Manufacturer as well as the assigned
    footprint. If these fields are not used in some parts they will simply be
    ignored (they will match as both will be empty strings).

    """
    result = True
    if self.getValue() != other.getValue():
        result = False
    elif self.getPartName() != other.getPartName():
        result = False
    elif self.getFootprint() != other.getFootprint():
        result = False
    elif self.getField("Tolerance") != other.getField("Tolerance"):
        result = False
    elif self.getField("Manufacturer") != other.getField("Manufacturer"):
        result = False
    elif self.getField("Voltage") != other.getField("Voltage"):
        result = False
    elif self.getDNP() != other.getDNP():
        result = False

    return result

# Override the component equivalence operator - it is important to do this
# before loading the netlist, otherwise all components will have the original
# equivalency operator.
kicad_netlist_reader.comp.__eq__ = myEqu

# Generate an instance of a generic netlist, and load the netlist tree from
# <file>.tmp. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
canOpenFile = True
try:
    f = kicad_utils.open_file_write(sys.argv[2], 'wb')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, file=sys.stderr)
    f = sys.stdout
    canOpenFile = False

# Output a set of rows for a header providing general information
html = html.replace('<!--SOURCE-->', net.getSource())
html = html.replace('<!--DATE-->', net.getDate())
html = html.replace('<!--TOOL-->', net.getTool())
html = html.replace('<!--COMPCOUNT-->', "<b>Component Count:</b>" + \
    str(len(net.components)))

row  = "<tr><th style='width:640px'>Ref</th>" + "<th>Qnty</th>"
row += "<th>Value</th>" + "<th>Part</th>" + "<th>Footprint</th>"
row += "<th>Description</th>" + "<th>Vendor</th>" + "<th>DNP</th></tr>"

html = html.replace('<!--TABLEROW-->', row + "<!--TABLEROW-->")

components = net.getInterestingComponents( excludeBOM=True )

# Get all of the components in groups of matching parts + values
# (see kicad_netlist_reader.py)
grouped = net.groupComponents(components)

# Output all of the component information
for group in grouped:
    refs = ""

    # Add the reference of every component in the group and keep a reference
    # to the component so that the other data can be filled in once per group
    for component in group:
        if len(refs) > 0:
            refs += ", "
        refs += component.getRef()
        c = component

    row = "\n    "
    row += "<tr><td>" + refs +"</td><td>" + str(len(group))
    row += "</td><td>" + c.getValue() + "</td><td>" + c.getLibName() + ":"
    row += c.getPartName() + "</td><td>" + c.getFootprint() + "</td><td>"
    row += c.getDescription() + "</td><td>" + c.getField("Vendor") + "</td><td>"
    row += c.getDNPString()
    row += "</td></tr>"

    html = html.replace('<!--TABLEROW-->', row + "<!--TABLEROW-->")

# Write the formatted html to the file
if sys.version_info[0] < 3:
    f.write(html)
else:
    f.write(html.encode('utf-8'))
f.close

if not canOpenFile:
    sys.exit(1)
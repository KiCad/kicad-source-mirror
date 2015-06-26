#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped HTML BOM with more advanced grouping
#

"""
    @package
    Generate a HTML BOM list.
    Components are sorted and grouped by ref
    Fields are (if exist)
    Ref, Quantity, Value, Symbol, footprint, Description, Vendor
"""


from __future__ import print_function

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import sys

# Start with a basic html template
html = """
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
        <title>KiCad BOM grouped by value and footprint </title>
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
    on their Value, Library source, and Library part.

    In this example of a more advanced equivalency operator we also compare the
    custom fields Voltage, Tolerance and Manufacturer as well as the assigned
    footprint. If these fields are not used in some parts they will simply be
    ignored (they will match as both will be empty strings).

    """
    result = True
    if self.getValue() != other.getValue():
        result = False
    elif self.getLibName() != other.getLibName():
        result = False
    elif self.getPartName() != other.getPartName():
        result = False
    elif self.getFootprint() != other.getFootprint():
        result = False
#    elif self.getField("Tolerance") != other.getField("Tolerance"):
#        result = False
#    elif self.getField("Manufacturer") != other.getField("Manufacturer"):
#        result = False
#    elif self.getField("Voltage") != other.getField("Voltage"):
#        result = False

    return result

# Override the component equivalence operator - it is important to do this
# before loading the netlist, otherwise all components will have the original
# equivalency operator.
kicad_netlist_reader.comp.__equ__ = myEqu

# Generate an instance of a generic netlist, and load the netlist tree from
# video.xml. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write too, if the file cannot be opened output to stdout
# instead
try:
    f = open(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print( __file__, ":", e, sys.stderr )
    f = sys.stdout

components = net.getInterestingComponents()

# Output a set of rows for a header providing general information
html = html.replace('<!--SOURCE-->', net.getSource())
html = html.replace('<!--DATE-->', net.getDate())
html = html.replace('<!--TOOL-->', net.getTool())
html = html.replace('<!--COMPCOUNT-->', "<b>Component Count:</b>" + \
    str(len(components)))

row  = "<tr><th style='width:640px'>Ref</th>" + "<th>Qnty</th>"
row += "<th>Value</th>"
row += "<th>Symbol</th>"
row += "<th>Footprint</th>"
row += "<th>Description</th>"
row += "<th>PartNumber</th>"
row += "<th>Vendor</th></tr>"

html = html.replace('<!--TABLEROW-->', row + "<!--TABLEROW-->")

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

    row = "<tr>"
    row += "<td>" + refs +"</td>"
    row += "<td align=center>" + str(len(group)) + "</td>"
    row += "<td align=center>" + c.getValue() + "</td>"
#    row += "<td align=center>" + c.getLibName() + ":" + c.getPartName() + "</td>"
    row += "<td align=center>" + c.getPartName() + "</td>"
    row += "<td align=center>" + c.getFootprint() + "</td>"
    row += "<td align=center>" + c.getDescription() + "</td>"
    row += "<td align=center>" + c.getField("PartNumber") + "</td>"
    row += "<td align=center>" + c.getField("Vendor") + "</td>"
    row += "</tr>"

    html = html.replace('<!--TABLEROW-->', row + "<!--TABLEROW-->")

# Print the formatted html to output file
print(html, file=f)

#
# Example python script to generate a BOM from a KiCad generic netlist
# The KiCad generic xml netlist is expected to be encoded UTF-8
#
# Example: Sorted and Grouped HTML BOM
#

"""
    @package
    Output: HTML
    Grouped By: Value, DNP
    Sorted By: Ref
    Fields: Ref, Qnty, Value, Part, Datasheet, Description, Vendor, DNP

    Command line:
    python "pathToFile/bom_html_grouped_by_value.py" "%I" "%O.html"
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

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
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

components = net.getInterestingComponents( excludeBOM=True )

# Output a set of rows for a header providing general information
html = html.replace('<!--SOURCE-->', net.getSource())
html = html.replace('<!--DATE-->', net.getDate())
html = html.replace('<!--TOOL-->', net.getTool())
html = html.replace('<!--COMPCOUNT-->', "<b>Component Count:</b>" + \
    str(len(components)))

row =""
row += "<tr><th>Ref</th>"
row += "<th>Qnty</th>"
row += "<th>Value</th>" + "<th>Part</th>" + "<th>Datasheet</th>"
row += "<th>Description</th>" + "<th>Vendor</th>" + "<th>DNP</th></tr>"

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

    row = "<tr><td>" + refs +"</td><td>" + str(len(group))
    row += "</td><td>" + c.getValue()
    row += "</td><td>" + c.getLibName() + ":" + c.getPartName()
    row += "</td><td>" + c.getDatasheet()
    row += "</td><td>" + c.getDescription()
    row += "</td><td>" + c.getField("Vendor")
    row += "</td><td>" + c.getDNPString() + "</td></tr>"
    row += "\n"

    html = html.replace('<!--TABLEROW-->', row + "<!--TABLEROW-->")

# Write the formatted html to the file
if sys.version_info[0] < 3:
    f.write(html)
else:
    f.write(html.encode('utf-8'))
f.close()

if not canOpenFile:
    sys.exit(1)
bom_?.py are some python scripts which read a generic xml netlist from eeschema,
and create a bom.

All examples use ky_netlist_reader.py, which is a python utility to read
and parse this generic xml netlist and create the corresponding data
used to build the bom.

You can modify them to build the bom you want.

to use them, you should install python, and run:
python bom_example?.py <netlist name> <bom list netname>

See Eeschema doc, chapter 14 for info about the generic xml netlist format,
and how to run a script from Eeschema to create a customized netlist or BOM.

If the python comment
"""
    @package
    some comments
"""
is added to the begining of the python script, the comment will be displayed
in Eescheam, in the BOM dialog

For instance:
"""
    @package
    Generate a HTML BOM list.
    Components are sorted and grouped by value
    Fields are (if exist)
    Ref, Quantity, Value, Part, Datasheet, Description, Vendor
"""

displays:
    Generate a HTML BOM list.
    Components are sorted and grouped by value
    Fields are (if exist)
    Ref, Quantity, Value, Part, Datasheet, Description, Vendor
in BOM dialog

#
# Example python script to generate a BOM from a KiCad generic netlist
#

"""
    @package
    Output: Cadstar RINF netlist
    Sorted By: Ref

    Command line:
    python "pathToFile/netlist_form_cadstar.py" "%I" "%O.frp"
"""

from __future__ import print_function

# Import the KiCad python helper module
import kicad_netlist_reader
import sys

# A helper function to convert a UTF8/Unicode/locale string read in netlist
# for python2 or python3 (Windows/unix)
def fromNetlistText( aText ):
    if sys.platform.startswith('win32'):
        try:
            return aText.encode('utf-8').decode('cp1252')
        except UnicodeDecodeError:
            return aText
    else:
        return aText

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
netlist = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
canOpenFile = True
try:
    f = open(sys.argv[2], 'wb')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, sys.stderr)
    f = sys.stdout
    canOpenFile = False

components = netlist.getInterestingComponents( excludeBoard=True )

row =""

''' Netlist header '''
row += ".HEA" + '\n'
''' Generate line .TIM <time> '''
row += '.TIM ' + netlist.getDate() + '\n'
''' Generate line .APP <eeschema version> '''
row += '.APP ' + netlist.getTool() + '\n'
row += '.TYP FULL' + '\n\n'


''' Generate list of component
 for each component  create lines like
    .ADD_COM U3 "74LS541"   (when no footprint name specified)
    .ADD_COM JP1 "CONN_8X2" "pin_array_8x2"   (with a specified footprint name)
'''

for c in components:
    c.getExcludeFromBOM
    row += ".ADD_COM " + " " + c.getRef() + " \"" + c.getValue() + "\""

    fp_name = c.getFootprint( False )

    if fp_name != "":
        row += " \"" + fp_name + "\""

    row += '\n'

'''
generate for each net create lines like
.ADD_TER U3.9 "/PC-RST"
.TER     U3.8
         BUS1.2
.ADD_TER BUS1.14 "/PC-IOR"
.TER     U3.7
'''
nets = netlist.getNets()

row += '\n'

for net in nets:
    # count the number of pads in net. nets with only one pad are skipped
    netitems = net.children
    pad_count = 0

    for node in netitems:
        pad_count += 1

    item_cnt = 1
    netitems = net.children

    if pad_count > 1:
        for node in netitems:

            if item_cnt == 1:
                row += ".ADD_TER " + net.get( "node", "ref" ) + '.' + net.get( "node", "pin" )
                row += " \"" + net.get( "net", "name" ) + '\"\n'

            if item_cnt == 2:
                row += ".TER     "

            if item_cnt > 2:
                row += "         "

            if item_cnt > 1:
                    row += node.get('node','ref') + '.' + node.get('node','pin') + '\n'

            item_cnt += 1


row += '\n.END\n'

f.write(row.encode('utf-8'))
f.close()

if not canOpenFile:
    sys.exit(1)
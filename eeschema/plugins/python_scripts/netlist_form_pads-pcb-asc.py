#
# Example python script to generate a BOM from a KiCad generic netlist
#

"""
    @package
    Output: PADS  format netlist
    Sorted By: Ref

    Command line:
    python "pathToFile/netlist_form_pads-pcb-asc.py" "%I" "%O.net"
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
row += "!PADS-POWERPCB-V2.0-MILS!" + '\n'
row += '*PART*' + '\n'


''' Generate list of component
 for each component  create lines like
 C1 Capacitor_THT:CP_Axial_L18.0mm_D6.5mm_P25.00mm_Horizontal
 C2 Capacitor_THT:CP_Axial_L18.0mm_D6.5mm_P25.00mm_Horizontal
'''

for c in components:
    row += " " + c.getRef()

    fp_name = c.getFootprint( False )

    if fp_name != "":
        row += " " + fp_name

    row += '\n'

'''
generate for each net create lines like
*SIGNAL* /CLOCK-RB6
 P2.27
 P3.39
'''
nets = netlist.getNets()

row += '\n*NET*' + '\n'

for net in nets:
    # count the number of pads in net. nets with only one pad are skipped
    netitems = net.children
    pad_count = 0

    for node in netitems:
        pad_count += 1

    netitems = net.children

    if pad_count > 1:
        row += "*SIGNAL* " + net.get( "net", "name" ) + '\n'

        for node in netitems:
            row += node.get('node','ref') + '.' + node.get('node','pin') + '\n'

row += '*END*\n'

f.write(row.encode('utf-8'))
f.close()

if not canOpenFile:
    sys.exit(1)
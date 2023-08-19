#
# Example python script to generate a BOM from a KiCad generic netlist
#

"""
    @package
    Output: old OrcadPcb2 netlist format
    Sorted By: Ref

    Command line:
    python "pathToFile/netlist_form_OrcadPcb2.py" "%I" "%O.net"
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

''' Netlist header:
( { EESchema Netlist Version 1.1  02/09/2021 11:10:00
Eeschema (5.99.0-12178-ge03257b55f-dirty)}
'''
''' Generate line ( { EESchema Netlist Version 1.1 <time> '''
row += '( { EESchema Netlist Version 1.1 ' + netlist.getDate() + '\n'
''' Generate line Eeschema <eeschema version> '''
row += 'Eeschema ' + netlist.getTool() + ' }\n\n'


'''
    This template read each component and creates lines:
     ( 3EBF7DBD $noname U1 74LS125
      ... pin list ...
      )
    and calls "create_pin_list" template to build the pin list
'''

for c in components:
    fp_name = c.getFootprint( False )

    if fp_name == "":
        fp_name = "$noname"

    row += "( " + c.getTimestamp() + ' ' + fp_name
    row += ' ' + c.getRef()
    row += ' ' + c.getValue()
    row += '\n'

    '''
    generate pin list
    The pin list from library description is something like
          <pins>
            <pin num="1" type="passive"/>
            <pin num="2" type="passive"/>
          </pins>
    Output pin list is ( <pin num> <net name> )
    something like
            ( 1 VCC )
            ( 2 GND )
    for not connected pins:
            ( 3 ? )
    '''
    lib = c.getLibPart()
    pinlist = lib.getPinList()

    for pin in pinlist:
        pin_num = pin.get('pin','num')
        row += "   ( " + pin_num + " "
        netname = c.getPinNetname( pin_num, netlist, True )

        if netname == "":
            netname = "?"

        row += netname
        row += " )\n"

    row += ')\n'

row += '\n)\n'

f.write(row.encode('utf-8'))
f.close()

if not canOpenFile:
    sys.exit(1)
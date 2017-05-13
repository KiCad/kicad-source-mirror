#
# Example python script to generate an equivalent XML document from XML input
#
# Example: Round value robin, XML to XML conversion with partial value monging
#

from __future__ import print_function

# Import the KiCad python helper module and the csv formatter
import kicad_netlist_reader
import sys

def checkvalue(self):
    """Check values, and replace with preferred/consistent values"""
    ref = self.getRef()
    r = ref.split("R")
    c = ref.split("C")
    v = self.getValue()

    # Common to all values - convert decimation if necessary
    dec = v.split(",")
    if (len(dec) == 2):
        newval = dec[0] + "." + dec[1]
        self.setValue(newval)
        v = self.getValue()

    if len(r) == 2 and r[1].isdigit():
        # This is a resistor - make values consistent
        # If the value is a pure value, add R to the end of the value
        if v.isdigit():
            i = int(v)
            if (i > 1000000):
                i = i / 1000000
                v = str(i) + "M"
            if (i > 1000):
                i = i / 1000
                v = str(i) + "K"
            else:
                v = str(i) + "R"

            self.setValue(v)
        else:
            # Get the multiplier character
            multiplier = v[len(v) - 1]
            v = v.strip(multiplier)
            v = v.split(".")
            if (len(v) == 2):
                newval = v[0] + multiplier + v[1]
                self.setValue(newval)
                v = self.getValue()



# Give components a new method for checking the values (this could easily be a
# Company Part Number generator method instead)
kicad_netlist_reader.comp.checkvalue = checkvalue

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = kicad_netlist_reader.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
try:
    f = open(sys.argv[2], 'w')
except IOError:
    e = "Can't open output file for writing: " + sys.argv[2]
    print(__file__, ":", e, file=sys.stderr)
    f = sys.stdout

for c in net.components:
    c.checkvalue()

print(net.formatXML(), file=f)

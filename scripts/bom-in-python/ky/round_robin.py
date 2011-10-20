#
# Example python script to generate an equivalent XML document from XML input
#
# Example: Round robin, XML to XML conversion
#

# Import the KiCad python helper module and the csv formatter
import ky
import sys

# Generate an instance of a generic netlist, and load the netlist tree from
# the command line option. If the file doesn't exist, execution will stop
net = ky.netlist(sys.argv[1])

# Open a file to write to, if the file cannot be opened output to stdout
# instead
try:
    f = open(sys.argv[2], 'w')
except IOError:
    print >> sys.stderr, __file__, ":", e
    f = stdout

print >> f, net.formatXML()

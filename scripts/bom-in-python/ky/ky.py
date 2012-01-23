#
# KiCad python module for interpreting generic netlists which can be used
# to generate Bills of materials, etc.
#
# No string formatting is used on purpose as the only string formatting that
# is current compatible with python 2.4+ to 3.0+ is the '%' method, and that
# is due to be deprecated in 3.0+ soon
#

import sys
import xml.sax as sax


class component():
    """Class for a set of component information"""
    def __init__(self, element):
        self.element = element      
        self.libpart = None
        
        # Set to true when this component is included in a component group
        self.grouped = False
    
    def __eq__(self, other):
        """Equlivalency operator, remember this can be easily overloaded"""
        result = False
        if self.getValue() == other.getValue():
            if self.getLib() == other.getLib():
                if self.getPart() == other.getPart():
                    result = True
        return result

    def setPart(self, part):
        self.libpart = part

    def setValue(self, value):
        """Set the value of this component"""
        v = self.element.getChild("value")
        if v:
            v.setChars(value)
        
    def getValue(self):
        return self.element.get("value")

    def getRef(self):
        return self.element.get("comp", "ref")

    def getFootprint(self):
        return self.element.get("footprint")        

    def getDatasheet(self):
        return self.element.get("datasheet")

    def getLib(self):
        return self.element.get("libsource", "lib")
        
    def getPart(self):
        return self.element.get("libsource", "part") 
    
    def getTimestamp(self):
        return self.element.get("tstamp")
        
    def getDescription(self):
        # When attempting to access the part, we must take care in case the part
        # cannot be found in the netlist
        try:
            d = self.libpart.getDescription()
        except AttributeError:
            d = ""
        return d

    def getDatasheet(self):
        # When attempting to access the part, we must take care in case the part
        # cannot be found in the netlist
        try:
            d = self.libpart.getDatasheet()
        except AttributeError:
            d = ""
        return d
    
    def getField(self, name):
        """Return the value of a field named name. The component is first 
        checked for the field, and then the components library part is checked
        for the field. If the field doesn't exist in either, an empty string is
        returned
        
        Keywords:
        name -- The name of the field to return the value for
        
        """
        field = self.element.get("field", "name", name)
        if field == "":
            try:
                field = self.libpart.getField(name)
            except AttributeError:
                field = ""
        return field


class netlistElement():
    """Generic netlist element. All elements for a netlist tree which can be 
    used to easily generate various output formats by propogating format
    requests to all children
    """
    def __init__(self, name, parent=None):
        self.name = name
        self.attributes = {}
        self.parent = parent
        self.chars = ""
        self.children = []
        self.indent = ""
            
    def __str__(self):
        """String representation of this netlist element
        
        """
        return (self.name + "[" + self.chars + "]" + " attr:" + 
            str(len(self.attributes[a])))            
    
    def formatXML(self, amChild=False):
        """Return this element formatted as XML
        
        Keywords:
        amChild -- If set to True, the start of document is not returned
        
        """
        s = ""

        if not amChild:
            s = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" 
        
        s += self.indent + "<" + self.name
        for a in self.attributes:
            s += " " + a + "=\"" + self.attributes[a] + "\""

        if (len(self.chars) == 0) and (len(self.children) == 0):
            s += "/>"
        else:
            s += ">" + self.chars                                                        
        
        for c in self.children:
            c.indent += self.indent + "    "
            s += "\n"
            s += c.formatXML(True)
                    
        if (len(self.children) > 0):
            s += "\n" + self.indent        
         
        if (len(self.children) > 0) or (len(self.chars) > 0): 
            s += "</" + self.name + ">"        
                  
        return s

    def formatHTML(self, amChild=False):
        """Return this element formatted as HTML
        
        Keywords:
        amChild -- If set to True, the start of document is not returned
        
        """
        s = ""

        if not amChild:
            s = """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
                "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
                <html xmlns="http://www.w3.org/1999/xhtml">
                <head>
                <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
                <title></title>
                </head>
                <body>
                <table>
                """ 

        s += "<tr><td><b>" + self.name + "</b><br>" + self.chars + "</td><td><ul>"
        for a in self.attributes:
            s += "<li>" + a + " = " + self.attributes[a] + "</li>"

        s += "</ul></td></tr>\n"
                                                                
        for c in self.children:
            s += c.formatHTML(True)        
         
        if not amChild:
            s += """</table>
                </body>
                </html>"""        
                  
        return s    
                     
    def addAttribute(self, attr, value):
        """Add an attribute to this element"""
        self.attributes[attr] = value

    def setChars(self, chars):
        """Set the characters for this element"""
        self.chars = chars
                
    def addChars(self, chars):
        """Add characters (textual value) to this element"""
        self.chars += chars
    
    def addChild(self, child):
        """Add a child element to this element"""
        self.children.append(child)
        return self.children[len(self.children) - 1]
        
    def getParent(self):
        """Get the parent of this element (Could be None)"""
        return self.parent

    def setAttribute(self, attr, value):
        """Set an attributes value - in fact does the same thing as add 
        attribute
        
        """
        self.attributes[attr] = value
    
    def getChild(self, name):
        """Returns a child element of name
        
        Keywords:
        name -- The name of the child element to return"""
        for child in self.children:
            if child.name == name:
                return child
    
        return None
                    
    def get(self, element, attribute="", attrmatch=""):
        """Return the data for either an attribute, or else an element"""
        if (self.name == element):
            if attribute != "":
                if attrmatch != "": 
                    if self.attributes[attribute] == attrmatch:
                        return self.chars                
                else:
                    return self.attributes[attribute]
            else:
                return self.chars
        
        for child in self.children:
            if child.get(element, attribute, attrmatch) != "":
                return child.get(element, attribute, attrmatch) 
        
        return ""         


class netlist():
    """ Kicad generic netlist class. Generally loaded from a kicad generic 
    netlist file. Includes several helper functions to ease BOM creating 
    scripts
     
    """
    def __init__(self, fname=""):
        """Initialiser for the genericNetlist class
        
        Keywords:
        fname -- The name of the generic netlist file to open (Optional)
        
        """
        self.design = None
        self.components = []
        self.libparts = []
        self.libraries = []
        self.nets = []
        
        # The entire tree is loaded into self.tree
        self.tree = []

        self._curr_element = None
       
        if fname != "":             
            self.load(fname)
        
    def addChars(self, content):
        """Add characters to the current element"""
        self._curr_element.addChars(content) 

    def addElement(self, name):
        """Add a new kicad generic element to the list"""
        if self._curr_element == None:
            self.tree = netlistElement(name)
            self._curr_element = self.tree             
        else:
            self._curr_element = self._curr_element.addChild( 
                netlistElement(name, self._curr_element))

        # If this element is a component, add it to the components list 
        if self._curr_element.name == "comp":
            self.components.append(component(self._curr_element))

        # Assign the design element
        if self._curr_element.name == "design":
            self.design = self._curr_element

        # If this element is a library part, add it to the parts list
        if self._curr_element.name == "libpart":
            self.libparts.append(part(self._curr_element))

        # If this element is a net, add it to the nets list
        if self._curr_element.name == "net":
            self.nets.append(self._curr_element)
        
        # If this element is a library, add it to the libraries list
        if self._curr_element.name == "library":
            self.libraries.append(self._curr_element)
                
        return self._curr_element        

    def endDocument(self):
        """Called when the netlist document has been fully parsed"""
        # When the document is complete, the library parts must be linked to
        # the components as they are seperate in the tree so as not to
        # duplicate library part information for every component
        for c in self.components:
            for p in self.libparts:
                if p.getPart() == c.getPart() and p.getLib() == c.getLib():
                    c.setPart(p)

    def endElement(self):
        """End the current element and switch to its parent"""
        self._curr_element = self._curr_element.getParent()

    def getDate(self):
        """Return the date + time string generated by the tree creation tool"""
        return self.design.get("date")

    def getSource(self):
        """Return the source string for the design"""
        return self.design.get("source")
    
    def getTool(self):
        """Return the tool string which was used to create the netlist tree"""
        return self.design.get("tool")

    def groupComponents(self):
        """Return a list of component lists. Components are grouped together
        when the value, library and part identifiers match
        
        """
        groups = []

        # Make sure to start off will all components ungrouped to begin with        
        for c in self.components:
            c.grouped = False

        # Group components based on the value, library and part identifiers
        for c in self.components:
            if c.grouped == False:
                c.grouped = True
                newgroup = []
                newgroup.append(c)
                
                # Check every other ungrouped component against this component
                # and add to the group as necessary                
                for ci in self.components:                
                    if ci.grouped == False and ci == c:
                        newgroup.append(ci)
                        ci.grouped = True
                
                # Add the new component group to the groups list                
                groups.append(newgroup)
        
        # Each group is a list of components, we need to sort each list first
        # to get them in order as this makes for easier to read BOM's
        for g in groups:
            g = sorted(g, key=lambda g: g.getRef())
            
        # Finally, sort the groups to order the references alphabetically
        groups = sorted(groups, key=lambda group: group[0].getRef())
                
        return groups
    
    def formatXML(self):
        """Return the whole netlist formatted in XML"""
        return self.tree.formatXML()

    def formatHTML(self):
        """Return the whole netlist formatted in HTML"""
        return self.tree.formatHTML()
                                     
    def load(self, fname):
        """Load a kicad generic netlist
        
        Keywords:
        fname -- The name of the generic netlist file to open 
        
        """
        try:
            self._reader = sax.make_parser()
            self._reader.setContentHandler(_gNetReader(self))
            self._reader.parse(fname)
        except IOError as e:
            print >> sys.stderr, __file__, ":", e
            sys.exit(-1)    


class part():
    """Class for a library part"""
    def __init__(self, part):
        # The part is a reference to a libpart generic netlist element
        self.element = part
    
    def __str__(self):
        # simply print the generic netlist element associated with this part
        return str(self.element)
        
    def getDatasheet(self):
        return self.element.get("docs")

    def getLib(self):
        return self.element.get("libpart", "lib")
    
    def getPart(self):
        return self.element.get("libpart", "part")

    def getDescription(self):
        return self.element.get("description")
                
    def getField(self, name):
        return self.element.get("field", "name", name)


class _gNetReader(sax.handler.ContentHandler):
    """SAX kicad generic netlist content handler - passes most of the work back
    to the gNetlist class which builds a complete tree in RAM for the design
    
    """
    def __init__(self, aParent):
        self.parent = aParent
    
    def startElement(self, name, attrs):
        """Start of a new XML element event"""
        element = self.parent.addElement(name)
        
        for name in attrs.getNames():
            element.addAttribute(name, attrs.getValue(name))
        
    def endElement(self, name):
        self.parent.endElement()
    
    def characters(self, content):
        # Ignore erroneous white space - ignoreableWhitespace does not get rid
        # of the need for this!
        if not content.isspace():
            self.parent.addChars(content)
            
    def endDocument(self):
        """End of the XML document event"""
        self.parent.endDocument()

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef NETLIST_EXPORT_GENERIC_H
#define NETLIST_EXPORT_GENERIC_H

#include <netlist_exporter.h>

#include <xnode.h>      // also nests: <wx/xml/xml.h>

#define GENERIC_INTERMEDIATE_NETLIST_EXT wxT( "xml" )

/**
 * Class NETLIST_EXPORTER_GENERIC
 * generates a generic XML based netlist file. This allows using XSLT or other methods to
 * transform the XML to other netlist formats outside of the C++ codebase.
 */
class NETLIST_EXPORTER_GENERIC : public NETLIST_EXPORTER
{
protected:
   /**
     * Function node
     * is a convenience function that creates a new XNODE with an optional textual child.
     * It also provides some insulation from a possible change in XML library.
     *
     * @param aName is the name to associate with a new node of type wxXML_ELEMENT_NODE.
     * @param aTextualContent is optional, and if given is the text to include in a child
     *   of the returned node, and has type wxXML_TEXT_NODE.
     */
    XNODE* node( const wxString& aName, const wxString& aTextualContent = wxEmptyString );
    /**
     * Function writeGENERICListOfNets
     * writes out nets (ranked by Netcode), and elements that are
     * connected as part of that net.
     */
    bool writeListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );
    /**
     * Function makeGenericRoot
     * builds the entire document tree for the generic export.  This is factored
     * out here so we can write the tree in either S-expression file format
     * or in XML if we put the tree built here into a wxXmlDocument.
     * @return XNODE* - the root nodes
     */
    XNODE* makeRoot();

    /**
     * Function makeComponents
     * @return XNODE* - returns a sub-tree holding all the schematic components.
     */
    XNODE* makeComponents();

    /**
     * Function makeDesignHeader
     * fills out a project "design" header into an XML node.
     * @return XNODE* - the design header
     */
    XNODE* makeDesignHeader();

    /**
     * Function makeLibParts
     * fills out an XML node with the unique library parts and returns it.
     * @return XNODE* - the library parts nodes
     */
    XNODE* makeLibParts();

    /**
     * Function makeListOfNets
     * fills out an XML node with a list of nets and returns it.
     * @return XNODE* - the list of nets nodes
     */
    XNODE* makeListOfNets();

    /**
     * Function makeLibraries
     * fills out an XML node with a list of used libraries and returns it.
     * Must have called makeGenericLibParts() before this function.
     * @return XNODE* - the library nodes
     */
    XNODE* makeLibraries();

public:
    NETLIST_EXPORTER_GENERIC( NETLIST_OBJECT_LIST* aMasterList, PART_LIBS* aLibs ) :
        NETLIST_EXPORTER( aMasterList, aLibs )
    {
    }

    /**
     * Function Write
     * writes to specified output file
     */
    bool Write( const wxString& aOutFileName, unsigned aNetlistOptions );
};

#endif

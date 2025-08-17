/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef NETLIST_EXPORT_XML_H
#define NETLIST_EXPORT_XML_H

#include <netlist_exporter_base.h>

#include <project.h>

#include <sch_edit_frame.h>

class CONNECTION_GRAPH;
class XNODE;

#define GENERIC_INTERMEDIATE_NETLIST_EXT wxT( "xml" )

/**
 * A set of bits which control the totality of the tree built by makeRoot()
 */
enum GNL_T
{
    GNL_LIBRARIES   = 1 << 0,
    GNL_SYMBOLS     = 1 << 1,
    GNL_PARTS       = 1 << 2,
    GNL_HEADER      = 1 << 3,
    GNL_NETS        = 1 << 4,
    GNL_OPT_KICAD   = 1 << 5,
    GNL_OPT_BOM     = 1 << 6,
};


/**
 * Generate a generic XML based netlist file.
 *
 * This allows using XSLT or other methods to transform the XML to other netlist formats
 * outside of the C++ codebase.
 */
class NETLIST_EXPORTER_XML : public NETLIST_EXPORTER_BASE
{
public:
    NETLIST_EXPORTER_XML( SCHEMATIC* aSchematic ) :
            NETLIST_EXPORTER_BASE( aSchematic ),
            m_resolveTextVars( true )
    {}

    /**
     * Write generic netlist to \a aOutFileName.
     *
     * @param aOutFileName is the file name to write.
     * @param aNetlistOptions are the options used to control the netlist output.
     *
     * @return true if the netlist was written successfully.
     */
    bool WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                       REPORTER& aReporter ) override;

#define GNL_ALL     ( GNL_LIBRARIES | GNL_SYMBOLS | GNL_PARTS | GNL_HEADER | GNL_NETS )

protected:
   /**
     * A convenience function that creates a new XNODE with an optional textual child.
     * It also provides some insulation from a possible change in XML library.
     *
     * @param aName is the name to associate with a new node of type wxXML_ELEMENT_NODE.
     * @param aTextualContent is optional, and if given is the text to include in a child
     *   of the returned node, and has type wxXML_TEXT_NODE.
     */
    XNODE* node( const wxString& aName, const wxString& aTextualContent = wxEmptyString );

    /**
     * Build the entire document tree for the generic export.  This is factored
     * out here so we can write the tree in either S-expression file format
     * or in XML if we put the tree built here into a wxXmlDocument.
     * @param aCtl a bitset or-ed together from GNL_ENUM values
     * @return the root nodes
     */
    XNODE* makeRoot( unsigned aCtl = GNL_ALL );

    /**
     * @return a sub-tree holding all the schematic symbols.
     */
    XNODE* makeSymbols( unsigned aCtl );

    /**
     * @return a sub-tree holding all the schematic groups.
     */
    XNODE* makeGroups();

    /**
     * Fill out a project "design" header into an XML node.
     * @return the design header
     */
    XNODE* makeDesignHeader();

    /**
     * Fill out an XML node with the unique library parts and returns it.
     * @return the library parts nodes
     */
    XNODE* makeLibParts();

    /**
     * Fill out an XML node with a list of nets and returns it.
     * @return the list of nets nodes
     */
    XNODE* makeListOfNets( unsigned aCtl );

    /**
     * Fill out an XML node with a list of used libraries and returns it.
     * Must have called makeGenericLibParts() before this function.
     * @return the library nodes
     */
    XNODE* makeLibraries();

    void addSymbolFields( XNODE* aNode, SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aSheet,
                          const SCH_SHEET_LIST& aSheetList);

    /**
     * Finds all component class names attached to any sub-unit of a given symbol
     */
    std::vector<wxString>
    getComponentClassNamesForAllSymbolUnits( SCH_SYMBOL*           aSymbol,
                                             const SCH_SHEET_PATH& aSymbolSheet,
                                             const SCH_SHEET_LIST& aSheetList );

    bool                m_resolveTextVars;   // Export textVar references resolved

private:
    std::set<wxString>  m_libraries;         // Set of library nicknames.
};

#endif

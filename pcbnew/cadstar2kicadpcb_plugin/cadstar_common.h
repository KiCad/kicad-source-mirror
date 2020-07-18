/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <@Qbort>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file cadstar_common.h
 * @brief Helper functions and common defines
 */

#ifndef CADSTAR_COMMON_H_
#define CADSTAR_COMMON_H_

#include <class_board.h>
#include <dsnlexer.h>
#include <macros.h>
#include <wx/wx.h>
#include <wx/xml/xml.h>
#include <xnode.h>

#define THROW_MISSING_NODE_IO_ERROR( nodename, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Missing node '%s' in '%s'" ), nodename, location ) )

#define THROW_UNKNOWN_NODE_IO_ERROR( nodename, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Unknown node '%s' in '%s'" ), nodename, location ) )

#define THROW_MISSING_PARAMETER_IO_ERROR( param, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Missing Parameter '%s' in '%s'" ), param, location ) )

#define THROW_UNKNOWN_PARAMETER_IO_ERROR( param, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Unknown Parameter '%s' in '%s'" ), param, location ) )

#define THROW_PARSING_IO_ERROR( param, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Unable to parse '%s' in '%s'" ), param, location ) )


namespace CADSTAR_COMMON
{
enum class FILE_TYPE
{
    PCB_ARCHIVE,
    SCHEMATIC_ARCHIVE //for future schematic importer
                      //cadstar libraries?
                      //etc.
};


/**
 * @brief Represents a floating value in E notation
 */
struct EVALUE
{
    long Base     = 0;
    long Exponent = 0;

    void   Parse( XNODE* aNode );
    double GetDouble();
};

extern void InsertAttributeAtEnd( XNODE* aNode, wxString aValue );

/**
 * @brief Reads a CADSTAR Archive file (S-parameter format)
 * @param aFileName 
 * @param aType 
 * @return XNODE pointing to the top of the tree for further parsing. Each node has the first 
           element as the node's name and subsequent elements as node attributes ("attr0", 
           "attr1", "attr2", etc.). Caller is responsible for deleting to avoid memory leaks.
 * @throws IO_ERROR
 */
extern XNODE* LoadArchiveFile(
        const wxString& aFileName, FILE_TYPE aType = FILE_TYPE::PCB_ARCHIVE );


/**
 * @brief 
 * @param aNode 
 * @param aID 
 * @return returns the value of attribute "attrX" in aNode where 'X' is aID
 * @throws IO_ERROR if attribute does not exist
 */
extern wxString GetAttributeID( XNODE* aNode, unsigned int aID );


/**
 * @brief 
 * @param aNode 
 * @param aID 
 * @return returns the value of attribute "attrX" in aNode where 'X' is aID
 * @throws IO_ERROR if attribute does not exist
 */
extern long GetAttributeIDLong( XNODE* aNode, unsigned int aID );


/**
 * @brief 
 * @param aNode 
 * @throw IO_ERROR if a child node was found
 */
extern void CheckNoChildNodes( XNODE* aNode );


/**
 * @brief 
 * @param aNode 
 * @throw IO_ERROR if a node adjacent to aNode was found
 */
extern void CheckNoNextNodes( XNODE* aNode );


/**
 * @brief
 * @param aNode with a child node containing an EVALUE
 * @param aValueToParse 
 * @throw IO_ERROR if unable to parse or node is not an EVALUE
*/
extern void ParseChildEValue( XNODE* aNode, EVALUE& aValueToParse );


} // namespace CADSTAR_COMMON

#endif // CADSTAR_COMMON_H_

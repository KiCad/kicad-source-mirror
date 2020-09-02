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
 * @file cadstar_archive_parser.h
 * @brief Helper functions and common defines between schematic and PCB Archive files
 */

#ifndef CADSTAR_ARHIVE_PARSER_H_
#define CADSTAR_ARHIVE_PARSER_H_

#include <dsnlexer.h>
#include <macros.h>
#include <vector>
#include <wx/wx.h>
#include <wx/xml/xml.h>
#include <xnode.h>

// THROW_IO_ERROR definitions to ensure consident wording is used in the error messages

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

/**
 * @brief Helper functions and common structures for CADSTAR PCB and Schematic archive files.
*/
class CADSTAR_ARCHIVE_PARSER
{
public:
    static const long UNDEFINED_VALUE = -1;

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

    /**
     * @brief Represents a point in x,y coordinates
     */
    struct POINT : wxPoint
    {
        POINT() : wxPoint( UNDEFINED_VALUE, UNDEFINED_VALUE )
        {
        }

        void Parse( XNODE* aNode );
    };

    
    struct LONGPOINT
    {
        long x = UNDEFINED_VALUE;
        long y = UNDEFINED_VALUE;

        void Parse( XNODE* aNode );
    };


    enum class VERTEX_TYPE
    {
        POINT,
        CLOCKWISE_ARC,
        CLOCKWISE_SEMICIRCLE,
        ANTICLOCKWISE_ARC,
        ANTICLOCKWISE_SEMICIRCLE
    };


    /**
     * @brief Represents a vertex in a shape. E.g. A circle is made by two semicircles with the same
     * center point.
     */
    struct VERTEX
    {
        VERTEX_TYPE Type;
        POINT       Center;
        POINT       End;

        static bool IsVertex( XNODE* aNode );
        void        Parse( XNODE* aNode );
    };


    /**
     * @brief Represents a cutout in a closed shape (e.g. OUTLINE)
     */
    struct CUTOUT
    {
        std::vector<VERTEX> Vertices;

        void Parse( XNODE* aNode );
    };


    enum class SHAPE_TYPE
    {
        OPENSHAPE, ///< Unfilled open shape. Cannot have cutouts.
        OUTLINE,   ///< Unfilled closed shape.
        SOLID,     ///< Filled closed shape (solid fill).
        HATCHED    ///< Filled closed shape (hatch fill).
    };


    struct SHAPE
    {
        SHAPE_TYPE          Type;
        std::vector<VERTEX> Vertices;
        std::vector<CUTOUT> Cutouts;     ///< Not Applicable to OPENSHAPE Type
        wxString            HatchCodeID; ///< Only Applicable for HATCHED Type

        static bool IsShape( XNODE* aNode );
        void        Parse( XNODE* aNode );
    };


    static void InsertAttributeAtEnd( XNODE* aNode, wxString aValue );

    /**
     * @brief Reads a CADSTAR Archive file (S-parameter format)
     * @param aFileName 
     * @param aFileTypeIdentifier Identifier of the first node in the file to check against.
              E.g. "CADSTARPCB"
     * @return XNODE pointing to the top of the tree for further parsing. Each node has the first 
     *         element as the node's name and subsequent elements as node attributes ("attr0", 
     *         "attr1", "attr2", etc.). Caller is responsible for deleting to avoid memory leaks.
     * @throws IO_ERROR
     */
    static XNODE* LoadArchiveFile( const wxString& aFileName, const wxString& aFileTypeIdentifier );

    /**
     * @brief 
     * @param aAttribute 
     * @return 
     */
    static bool IsValidAttribute( wxXmlAttribute* aAttribute );


    /**
     * @brief 
     * @param aNode 
     * @param aID 
     * @return returns the value of attribute "attrX" in aNode where 'X' is aID
     * @throws IO_ERROR if attribute does not exist
     */
    static wxString GetXmlAttributeIDString( XNODE* aNode, unsigned int aID );


    /**
     * @brief 
     * @param aNode 
     * @param aID 
     * @return returns the value of attribute "attrX" in aNode where 'X' is aID
     * @throws IO_ERROR if attribute does not exist
     */
    static long GetXmlAttributeIDLong( XNODE* aNode, unsigned int aID );


    /**
     * @brief 
     * @param aNode 
     * @throw IO_ERROR if a child node was found
     */
    static void CheckNoChildNodes( XNODE* aNode );


    /**
     * @brief 
     * @param aNode 
     * @throw IO_ERROR if a node adjacent to aNode was found
     */
    static void CheckNoNextNodes( XNODE* aNode );


    /**
     * @brief
     * @param aNode with a child node containing an EVALUE
     * @param aValueToParse 
     * @throw IO_ERROR if unable to parse or node is not an EVALUE
     */
    static void ParseChildEValue( XNODE* aNode, EVALUE& aValueToParse );


    /**
     * @brief if no childs are present, it just returns an empty
     *        vector (without throwing an exception)
     * @param aNode containing a series of POINT objects
     * @param aTestAllChildNodes
     * @param aExpectedNumPoints if UNDEFINED_VALUE (i.e. -1), this is check is disabled
     * @return std::vector containing all POINT objects
     * @throw IO_ERROR if one of the following:
     *         - Unable to parse a POINT object
     *         - aTestAllChildNodes is true and one of the child nodes is not a valid POINT object
     *         - aExpectedNumPoints is non-negative and the number of POINT objects found is different
     */
    static std::vector<POINT> ParseAllChildPoints( XNODE* aNode, bool aTestAllChildNodes = false,
            int aExpectedNumPoints = UNDEFINED_VALUE );


    /**
     * @brief if no childs are present, it just returns an empty
     *        vector (without throwing an exception)
     * @param aNode containing a series of VERTEX objects
     * @param aTestAllChildNodes
     * @param aExpectedNumPoints if -1, this is check is disabled
     * @return std::vector containing all VERTEX objects
     * @throw IO_ERROR if one of the following:
     *         - Unable to parse a VERTEX object
     *         - aTestAllChildNodes is true and one of the child nodes is not a valid VERTEX object
     */
    static std::vector<VERTEX> ParseAllChildVertices(
            XNODE* aNode, bool aTestAllChildNodes = false );


    /**
     * @brief if no childs are present, it just returns an empty
     *        vector (without throwing an exception)
     * @param aNode containing a series of CUTOUT objects
     * @param aTestAllChildNodes
     * @param aExpectedNumPoints if -1, this is check is disabled
     * @return std::vector containing all CUTOUT objects
     * @throw IO_ERROR if one of the following:
     *         - Unable to parse a CUTOUT object
     *         - aTestAllChildNodes is true and one of the child nodes is not a valid CUTOUT object
     */
    static std::vector<CUTOUT> ParseAllChildCutouts(
            XNODE* aNode, bool aTestAllChildNodes = false );


}; // class CADSTAR_ARHIVE_PARSER

#endif // CADSTAR_ARHIVE_PARSER_H_

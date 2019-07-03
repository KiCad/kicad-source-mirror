/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 2012-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcb_parser.h
 * @brief Pcbnew s-expression file format parser definition.
 */

#ifndef _PCBNEW_PARSER_H_
#define _PCBNEW_PARSER_H_

#include <pcb_lexer.h>
#include <hashtables.h>
#include <layers_id_colors_and_visibility.h>    // PCB_LAYER_ID
#include <common.h>                             // KiROUND
#include <convert_to_biu.h>                     // IU_PER_MM

#include <unordered_map>


class BOARD;
class BOARD_ITEM;
class D_PAD;
class BOARD_DESIGN_SETTINGS;
class DIMENSION;
class DRAWSEGMENT;
class EDA_TEXT;
class EDGE_MODULE;
class TEXTE_MODULE;
class TEXTE_PCB;
class TRACK;
class MODULE;
class PCB_TARGET;
class VIA;
class ZONE_CONTAINER;
class MODULE_3D_SETTINGS;
struct LAYER;


/**
 * Class PCB_PARSER
 * reads a Pcbnew s-expression formatted #LINE_READER object and returns the appropriate
 * #BOARD_ITEM object.
 */
class PCB_PARSER : public PCB_LEXER
{
    typedef std::unordered_map< std::string, PCB_LAYER_ID >   LAYER_ID_MAP;
    typedef std::unordered_map< std::string, LSET >       LSET_MAP;

    BOARD*              m_board;
    LAYER_ID_MAP        m_layerIndices;     ///< map layer name to it's index
    LSET_MAP            m_layerMasks;       ///< map layer names to their masks
    std::set<wxString>  m_undefinedLayers;  ///< set of layers not defined in layers section
    std::vector<int>    m_netCodes;         ///< net codes mapping for boards being loaded
    bool                m_tooRecent;        ///< true if version parses as later than supported
    int                 m_requiredVersion;  ///< set to the KiCad format version this board requires

    bool                m_showLegacyZoneWarning;

    ///> Converts net code using the mapping table if available,
    ///> otherwise returns unchanged net code if < 0 or if is is out of range
    inline int getNetCode( int aNetCode )
    {
        if( ( aNetCode >= 0 ) && ( aNetCode < (int) m_netCodes.size() ) )
            return m_netCodes[aNetCode];

        return aNetCode;
    }

    /**
     * function pushValueIntoMap
     * Add aValue value in netcode mapping (m_netCodes) at index aIndex
     * ensure there is room in m_netCodes for that, and add room if needed.
     * @param aIndex = the index ( expected >=0 )of the location to use in m_netCodes
     * @param aValue = the netcode value to map
     */
    void pushValueIntoMap( int aIndex, int aValue );

    /**
     * Function init
     * clears and re-establishes m_layerMap with the default layer names.
     * m_layerMap will have some of its entries overwritten whenever a (new) board
     * is encountered.
     */
    void init();

    /**
     * Creates a mapping from the (short-lived) bug where layer names were translated
     * TODO: Remove this once we support custom layer names
     *
     * @param aMap string mapping from translated to English layer names
     */
    void createOldLayerMapping( std::unordered_map< std::string, std::string >& aMap );

    void parseHeader();
    void parseGeneralSection();
    void parsePAGE_INFO();
    void parseTITLE_BLOCK();

    void parseLayers();
    void parseLayer( LAYER* aLayer );

    void parseSetup();
    void parseDefaults( BOARD_DESIGN_SETTINGS& aSettings );
    void parseDefaultTextDims( BOARD_DESIGN_SETTINGS& aSettings, int aLayer );
    void parseNETINFO_ITEM();
    void parseNETCLASS();

    /** Read a DRAWSEGMENT description.
     * @param aAllowCirclesZeroWidth = true to allow items with 0 width
     * Only used in custom pad shapes for filled circles.
     */
    DRAWSEGMENT*    parseDRAWSEGMENT( bool aAllowCirclesZeroWidth = false );
    TEXTE_PCB*      parseTEXTE_PCB();
    DIMENSION*      parseDIMENSION();

    /**
     * Function parseMODULE_unchecked
     * Parse a module, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
     */
    MODULE*         parseMODULE_unchecked( wxArrayString* aInitialComments = 0 );
    TEXTE_MODULE*   parseTEXTE_MODULE();
    EDGE_MODULE*    parseEDGE_MODULE();
    D_PAD*          parseD_PAD( MODULE* aParent = NULL );
    // Parse only the (option ...) inside a pad description
    bool            parseD_PAD_option( D_PAD* aPad );
    TRACK*          parseTRACK();
    VIA*            parseVIA();
    ZONE_CONTAINER* parseZONE_CONTAINER();
    PCB_TARGET*     parsePCB_TARGET();
    BOARD*          parseBOARD();

    /**
     * Function parseBOARD_unchecked
     * Parse a module, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
     */
    BOARD*          parseBOARD_unchecked();


    /**
     * Function lookUpLayer
     * parses the current token for the layer definition of a #BOARD_ITEM object.
     *
     * @param aMap is the LAYER_{NUM|MSK}_MAP to use for the lookup.
     *
     * @throw IO_ERROR if the layer is not valid.
     * @throw PARSE_ERROR if the layer syntax is incorrect.
     * @return int - The result of the parsed #BOARD_ITEM layer or set designator.
     */
    template<class T, class M>
    T lookUpLayer( const M& aMap );

    /**
     * Function parseBoardItemLayer
     * parses the layer definition of a #BOARD_ITEM object.
     *
     * @throw IO_ERROR if the layer is not valid.
     * @throw PARSE_ERROR if the layer syntax is incorrect.
     * @return The index the parsed #BOARD_ITEM layer.
     */
    PCB_LAYER_ID parseBoardItemLayer();

    /**
     * Function parseBoardItemLayersAsMask
     * parses the layers definition of a #BOARD_ITEM object.
     *
     * @throw IO_ERROR if any of the layers is not valid.
     * @throw PARSE_ERROR if the layers syntax is incorrect.
     * @return The mask of layers the parsed #BOARD_ITEM is on.
     */
    LSET parseBoardItemLayersAsMask();

    /**
     * Function parseXY
     * parses a coordinate pair (xy X Y) in board units (mm).
     *
     * The parser checks if the previous token was T_LEFT and parses the remainder of
     * the token syntax.  This is used when parsing a list of coordinate points.  This
     * way the parser can be used in either case.
     *
     * @throw PARSE_ERROR if the coordinate pair syntax is incorrect.
     * @return A wxPoint object containing the coordinate pair.
     */
    wxPoint parseXY();

    void parseXY( int* aX, int* aY );

    /**
     * Function parseEDA_TEXT
     * parses the common settings for any object derived from #EDA_TEXT.
     *
     * @throw PARSE_ERROR if the text syntax is not valid.
     * @param aText A point to the #EDA_TEXT object to save the parsed settings into.
     */
    void parseEDA_TEXT( EDA_TEXT* aText );

    MODULE_3D_SETTINGS* parse3DModel();

    /**
     * Function parseDouble
     * parses the current token as an ASCII numeric string with possible leading
     * whitespace into a double precision floating point number.
     *
     * @throw IO_ERROR if an error occurs attempting to convert the current token.
     * @return The result of the parsed token.
     */
    double parseDouble();

    inline double parseDouble( const char* aExpected )
    {
        NeedNUMBER( aExpected );
        return parseDouble();
    }

    inline double parseDouble( PCB_KEYS_T::T aToken )
    {
        return parseDouble( GetTokenText( aToken ) );
    }

    inline int parseBoardUnits()
    {
        // There should be no major rounding issues here, since the values in
        // the file are in mm and get converted to nano-meters.
        // See test program tools/test-nm-biu-to-ascii-mm-round-tripping.cpp
        // to confirm or experiment.  Use a similar strategy in both places, here
        // and in the test program. Make that program with:
        // $ make test-nm-biu-to-ascii-mm-round-tripping
        auto retval = parseDouble() * IU_PER_MM;

        // N.B. we currently represent board units as integers.  Any values that are
        // larger or smaller than those board units represent undefined behavior for
        // the system.  We limit values to the largest that is visible on the screen
        // This is the diagonal distance of the full screen ~1.5m
        double int_limit = std::numeric_limits<int>::max() * 0.7071;    // 0.7071 = roughly 1/sqrt(2)
        return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
    }

    inline int parseBoardUnits( const char* aExpected )
    {
        auto retval = parseDouble( aExpected ) * IU_PER_MM;

        // N.B. we currently represent board units as integers.  Any values that are
        // larger or smaller than those board units represent undefined behavior for
        // the system.  We limit values to the largest that is visible on the screen
        double int_limit = std::numeric_limits<int>::max() * 0.7071;

        // Use here KiROUND, not KIROUND (see comments about them)
        // when having a function as argument, because it will be called twice
        // with KIROUND
        return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
    }

    inline int parseBoardUnits( PCB_KEYS_T::T aToken )
    {
        return parseBoardUnits( GetTokenText( aToken ) );
    }

    inline int parseInt()
    {
        return (int)strtol( CurText(), NULL, 10 );
    }

    inline int parseInt( const char* aExpected )
    {
        NeedNUMBER( aExpected );
        return parseInt();
    }

    inline long parseHex()
    {
        NextTok();
        return strtol( CurText(), NULL, 16 );
    }

    bool parseBool();

    /**
     * Parse a format version tag like (version 20160417) return the version.
     * Expects to start on 'version', and eats the closing paren.
     */
    int parseVersion();

public:

    PCB_PARSER( LINE_READER* aReader = NULL ) :
        PCB_LEXER( aReader ),
        m_board( 0 )
    {
        init();
    }

    // ~PCB_PARSER() {}

    /**
     * Function SetLineReader
     * sets @a aLineReader into the parser, and returns the previous one, if any.
     * @param aReader is what to read from for tokens, no ownership is received.
     * @return LINE_READER* - previous LINE_READER or NULL if none.
     */
    LINE_READER* SetLineReader( LINE_READER* aReader )
    {
        LINE_READER* ret = PopReader();
        PushReader( aReader );
        return ret;
    }

    void SetBoard( BOARD* aBoard )
    {
        init();
        m_board = aBoard;
    }

    BOARD_ITEM* Parse();
    /**
     * Function parseMODULE
     * @param aInitialComments may be a pointer to a heap allocated initial comment block
     *   or NULL.  If not NULL, then caller has given ownership of a wxArrayString to
     *   this function and care must be taken to delete it even on exception.
     */
    MODULE*         parseMODULE( wxArrayString* aInitialComments = 0 );

    /**
     * Return whether a version number, if any was parsed, was too recent
     */
    bool IsTooRecent()
    {
        return m_tooRecent;
    }

    /**
     * Return a string representing the version of kicad required to open this
     * file. Not particularly meaningful if IsTooRecent() returns false.
     */
    wxString GetRequiredVersion();

};


#endif    // _PCBNEW_PARSER_H_

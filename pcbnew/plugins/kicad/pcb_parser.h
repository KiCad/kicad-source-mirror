/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * Copyright (C) 2012-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <convert_to_biu.h>                      // IU_PER_MM
#include <hashtables.h>
#include <layers_id_colors_and_visibility.h>     // PCB_LAYER_ID
#include <math/util.h>                           // KiROUND, Clamp
#include <pcb_lexer.h>

#include <unordered_map>


class ARC;
class BOARD;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class PAD;
class BOARD_DESIGN_SETTINGS;
class DIMENSION_BASE;
class PCB_SHAPE;
class EDA_TEXT;
class FP_SHAPE;
class FP_TEXT;
class PCB_TEXT;
class TRACK;
class FOOTPRINT;
class PCB_GROUP;
class PCB_TARGET;
class VIA;
class ZONE;
class FP_3DMODEL;
struct LAYER;


/**
 * Read a Pcbnew s-expression formatted #LINE_READER object and returns the appropriate
 * #BOARD_ITEM object.
 */
class PCB_PARSER : public PCB_LEXER
{
public:
    PCB_PARSER( LINE_READER* aReader = NULL ) :
        PCB_LEXER( aReader ),
        m_board( nullptr ),
        m_resetKIIDs( false )
    {
        init();
    }

    // ~PCB_PARSER() {}

    /**
     * Set @a aLineReader into the parser, and returns the previous one, if any.
     *
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

        if( aBoard != nullptr )
            m_resetKIIDs = true;
    }

    BOARD_ITEM* Parse();

    /**
     * @param aInitialComments may be a pointer to a heap allocated initial comment block
     *                         or NULL.  If not NULL, then caller has given ownership of a
     *                         wxArrayString to this function and care must be taken to
     *                         delete it even on exception.
     */
    FOOTPRINT* parseFOOTPRINT( wxArrayString* aInitialComments = 0 );

    /**
     * Return whether a version number, if any was parsed, was too recent
     */
    bool IsTooRecent()
    {
        return m_tooRecent;
    }

    /**
     * Return a string representing the version of KiCad required to open this
     * file. Not particularly meaningful if IsTooRecent() returns false.
     */
    wxString GetRequiredVersion();

private:
    ///< Convert net code using the mapping table if available,
    ///< otherwise returns unchanged net code if < 0 or if is is out of range
    inline int getNetCode( int aNetCode )
    {
        if( ( aNetCode >= 0 ) && ( aNetCode < (int) m_netCodes.size() ) )
            return m_netCodes[aNetCode];

        return aNetCode;
    }

    /**
     * Add aValue value in netcode mapping (m_netCodes) at \a aIndex.
     *
     * Ensure there is room in m_netCodes for that, and add room if needed.
     *
     * @param aIndex is the index ( expected >=0 )of the location to use in m_netCodes.
     * @param aValue is the netcode value to map.
     */
    void pushValueIntoMap( int aIndex, int aValue );

    /**
     * Clear and re-establish m_layerMap with the default layer names.
     *
     * m_layerMap will have some of its entries overwritten whenever a (new) board
     * is encountered.
     */
    void init();

    /**
     * Create a mapping from the (short-lived) bug where layer names were translated.
     *
     * @todo Remove this once we support custom layer names.
     *
     * @param aMap string mapping from translated to English layer names.
     */
    void createOldLayerMapping( std::unordered_map< std::string, std::string >& aMap );

    /**
     * Skip the current token level, i.e search for the RIGHT parenthesis which closes the
     * current description.
     */
    void skipCurrent();

    void parseHeader();
    void parseGeneralSection();
    void parsePAGE_INFO();
    void parseTITLE_BLOCK();

    void parseLayers();
    void parseLayer( LAYER* aLayer );

    void parseBoardStackup();

    void parseSetup();
    void parseDefaults( BOARD_DESIGN_SETTINGS& aSettings );
    void parseDefaultTextDims( BOARD_DESIGN_SETTINGS& aSettings, int aLayer );
    void parseNETINFO_ITEM();
    void parseNETCLASS();

    PCB_SHAPE*      parsePCB_SHAPE();
    PCB_TEXT*       parsePCB_TEXT();
    DIMENSION_BASE* parseDIMENSION();

    // Parse a footprint, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
    FOOTPRINT*      parseFOOTPRINT_unchecked( wxArrayString* aInitialComments = 0 );

    FP_TEXT*        parseFP_TEXT();
    FP_SHAPE*       parseFP_SHAPE();
    PAD*            parsePAD( FOOTPRINT* aParent = NULL );

    // Parse only the (option ...) inside a pad description
    bool            parsePAD_option( PAD* aPad );

    ARC*            parseARC();
    TRACK*          parseTRACK();
    VIA*            parseVIA();
    ZONE*           parseZONE( BOARD_ITEM_CONTAINER* aParent );
    PCB_TARGET*     parsePCB_TARGET();
    BOARD*          parseBOARD();
    void            parseGROUP( BOARD_ITEM* aParent );

    // Parse a board, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
    BOARD*          parseBOARD_unchecked();

    /**
     * Parse the current token for the layer definition of a #BOARD_ITEM object.
     *
     * @param aMap is the LAYER_{NUM|MSK}_MAP to use for the lookup.
     * @return The result of the parsed #BOARD_ITEM layer or set designator.
     * @throw IO_ERROR if the layer is not valid.
     * @throw PARSE_ERROR if the layer syntax is incorrect.
     */
    template<class T, class M>
    T lookUpLayer( const M& aMap );

    /**
     * Parse the layer definition of a #BOARD_ITEM object.
     *
     * @return The index the parsed #BOARD_ITEM layer.
     * @throw IO_ERROR if the layer is not valid.
     * @throw PARSE_ERROR if the layer syntax is incorrect.
     */
    PCB_LAYER_ID parseBoardItemLayer();

    /**
     * Parse the layers definition of a #BOARD_ITEM object.
     *
     * @return The mask of layers the parsed #BOARD_ITEM is on.
     * @throw IO_ERROR if any of the layers is not valid.
     * @throw PARSE_ERROR if the layers syntax is incorrect.
     */
    LSET parseBoardItemLayersAsMask();

    /**
     * Parse a coordinate pair (xy X Y) in board units (mm).
     *
     * The parser checks if the previous token was T_LEFT and parses the remainder of
     * the token syntax.  This is used when parsing a list of coordinate points.  This
     * way the parser can be used in either case.
     *
     * @return A wxPoint object containing the coordinate pair.
     * @throw PARSE_ERROR if the coordinate pair syntax is incorrect.
     */
    wxPoint parseXY();

    void parseXY( int* aX, int* aY );

    std::pair<wxString, wxString> parseProperty();

    /**
     * Parse the common settings for any object derived from #EDA_TEXT.
     *
     * @param aText A point to the #EDA_TEXT object to save the parsed settings into.
     * @throw PARSE_ERROR if the text syntax is not valid.
     */
    void parseEDA_TEXT( EDA_TEXT* aText );

    FP_3DMODEL* parse3DModel();

    /**
     * Parse the current token as an ASCII numeric string with possible leading
     * whitespace into a double precision floating point number.
     *
     * @return The result of the parsed token.
     * @throw IO_ERROR if an error occurs attempting to convert the current token.
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
        double int_limit = std::numeric_limits<int>::max() * 0.7071;  // 0.7071 = roughly 1/sqrt(2)
        return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
    }

    inline int parseBoardUnits( const char* aExpected )
    {
        auto retval = parseDouble( aExpected ) * IU_PER_MM;

        // N.B. we currently represent board units as integers.  Any values that are
        // larger or smaller than those board units represent undefined behavior for
        // the system.  We limit values to the largest that is visible on the screen
        double int_limit = std::numeric_limits<int>::max() * 0.7071;

        // Use here #KiROUND, not EKIROUND (see comments about them) when having a function as
        // argument, because it will be called twice with #KIROUND.
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

    /*
     * @return if m_resetKIIDs, returns new KIID(), otherwise returns CurStr() as KIID.
     */
    KIID CurStrToKIID();

    /**
     * Called after parsing a footprint definition or board to build the group membership
     * lists.
     */
    void resolveGroups( BOARD_ITEM* aParent );

    typedef std::unordered_map< std::string, PCB_LAYER_ID > LAYER_ID_MAP;
    typedef std::unordered_map< std::string, LSET >         LSET_MAP;
    typedef std::unordered_map< wxString, KIID >            KIID_MAP;

    BOARD*              m_board;
    LAYER_ID_MAP        m_layerIndices;     ///< map layer name to it's index
    LSET_MAP            m_layerMasks;       ///< map layer names to their masks
    std::set<wxString>  m_undefinedLayers;  ///< set of layers not defined in layers section
    std::vector<int>    m_netCodes;         ///< net codes mapping for boards being loaded
    bool                m_tooRecent;        ///< true if version parses as later than supported
    int                 m_requiredVersion;  ///< set to the KiCad format version this board requires
    bool                m_resetKIIDs;       ///< reading into an existing board; reset UUIDs

    ///< if resetting UUIDs, record new ones to update groups with.
    KIID_MAP            m_resetKIIDMap;

    bool                m_showLegacyZoneWarning;

    // Group membership info refers to other Uuids in the file.
    // We don't want to rely on group declarations being last in the file, so
    // we store info about the group declarations here during parsing and then resolve
    // them into BOARD_ITEM* after we've parsed the rest of the file.
    typedef struct
    {
        BOARD_ITEM*       parent;
        wxString          name;
        KIID              uuid;
        std::vector<KIID> memberUuids;
    } GROUP_INFO;

    std::vector<GROUP_INFO> m_groupInfos;
};


#endif    // _PCBNEW_PARSER_H_

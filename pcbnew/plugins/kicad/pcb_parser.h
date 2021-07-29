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
#include <core/wx_stl_compat.h>
#include <hashtables.h>
#include <layer_ids.h>     // PCB_LAYER_ID
#include <pcb_lexer.h>
#include <kiid.h>

#include <unordered_map>


class PCB_ARC;
class BOARD;
class BOARD_ITEM;
class BOARD_ITEM_CONTAINER;
class PAD;
class BOARD_DESIGN_SETTINGS;
class PCB_DIMENSION_BASE;
class PCB_SHAPE;
class EDA_TEXT;
class FP_SHAPE;
class FP_TEXT;
class PCB_TEXT;
class PCB_TRACK;
class FOOTPRINT;
class PCB_GROUP;
class PCB_TARGET;
class PCB_VIA;
class ZONE;
class FP_3DMODEL;
class SHAPE_LINE_CHAIN;
struct LAYER;
class PROGRESS_REPORTER;


/**
 * Read a Pcbnew s-expression formatted #LINE_READER object and returns the appropriate
 * #BOARD_ITEM object.
 */
class PCB_PARSER : public PCB_LEXER
{
public:
    PCB_PARSER( LINE_READER* aReader = nullptr ) :
        PCB_LEXER( aReader ),
        m_board( nullptr ),
        m_resetKIIDs( false ),
        m_progressReporter( nullptr ),
        m_lineReader( nullptr ),
        m_lastProgressLine( 0 ),
        m_lineCount( 0 )
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

    void SetProgressReporter( PROGRESS_REPORTER* aProgressReporter, const LINE_READER* aLineReader,
                              unsigned aLineCount )
    {
        m_progressReporter = aProgressReporter;
        m_lineReader = aLineReader;
        m_lastProgressLine = 0;
        m_lineCount = aLineCount;
    }

    BOARD_ITEM* Parse();

    /**
     * @param aInitialComments may be a pointer to a heap allocated initial comment block
     *                         or NULL.  If not NULL, then caller has given ownership of a
     *                         wxArrayString to this function and care must be taken to
     *                         delete it even on exception.
     */
    FOOTPRINT* parseFOOTPRINT( wxArrayString* aInitialComments = nullptr );

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

    void checkpoint();

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

    PCB_SHAPE*          parsePCB_SHAPE();
    PCB_TEXT*           parsePCB_TEXT();
    PCB_DIMENSION_BASE* parseDIMENSION();

    // Parse a footprint, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
    FOOTPRINT*          parseFOOTPRINT_unchecked( wxArrayString* aInitialComments = nullptr );

    FP_TEXT*            parseFP_TEXT();
    FP_SHAPE*           parseFP_SHAPE();
    PAD*                parsePAD( FOOTPRINT* aParent = nullptr );

    // Parse only the (option ...) inside a pad description
    bool                parsePAD_option( PAD* aPad );

    PCB_ARC*            parseARC();
    PCB_TRACK*          parsePCB_TRACK();
    PCB_VIA*            parsePCB_VIA();
    ZONE*               parseZONE( BOARD_ITEM_CONTAINER* aParent );
    PCB_TARGET*         parsePCB_TARGET();
    BOARD*              parseBOARD();
    void                parseGROUP( BOARD_ITEM* aParent );

    // Parse a board, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
    BOARD*              parseBOARD_unchecked();

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
     * Parses possible outline points and stores them into \p aPoly.  This accepts points
     * for DRAWSEGMENT polygons, EDGEMODULE polygons and ZONE_CONTAINER polygons.  Points
     * and arcs are added to the most recent outline
     *
     * @param aPoly polygon container to add points and arcs
     */
    void parseOutlinePoints( SHAPE_LINE_CHAIN& aPoly );

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

    int parseBoardUnits();

    int parseBoardUnits( const char* aExpected );

    inline int parseBoardUnits( PCB_KEYS_T::T aToken )
    {
        return parseBoardUnits( GetTokenText( aToken ) );
    }

    inline int parseInt()
    {
        return (int)strtol( CurText(), nullptr, 10 );
    }

    inline int parseInt( const char* aExpected )
    {
        NeedNUMBER( aExpected );
        return parseInt();
    }

    inline long parseHex()
    {
        NextTok();
        return strtol( CurText(), nullptr, 16 );
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

    PROGRESS_REPORTER*  m_progressReporter;  ///< optional; may be nullptr
    const LINE_READER*  m_lineReader;        ///< for progress reporting
    unsigned            m_lastProgressLine;
    unsigned            m_lineCount;         ///< for progress reporting

    // Group membership info refers to other Uuids in the file.
    // We don't want to rely on group declarations being last in the file, so
    // we store info about the group declarations here during parsing and then resolve
    // them into BOARD_ITEM* after we've parsed the rest of the file.
    struct GROUP_INFO
    {
        BOARD_ITEM*       parent;
        wxString          name;
        bool              locked;
        KIID              uuid;
        std::vector<KIID> memberUuids;
    };

    std::vector<GROUP_INFO> m_groupInfos;
};


#endif    // _PCBNEW_PARSER_H_

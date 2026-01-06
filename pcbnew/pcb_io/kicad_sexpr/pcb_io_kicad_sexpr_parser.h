/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
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

/**
 * @file pcb_io_kicad_sexpr_parser.h
 * @brief Pcbnew s-expression file format parser definition.
 */

#ifndef _PCBNEW_PARSER_H_
#define _PCBNEW_PARSER_H_

#include <eda_units.h>
#include <core/wx_stl_compat.h>
#include <hashtables.h>
#include <lib_id.h>
#include <layer_ids.h>     // PCB_LAYER_ID
#include <lset.h>
#include <pcb_lexer.h>
#include <kiid.h>
#include <math/box2.h>
#include <string_any_map.h>
#include <padstack.h>

#include <chrono>
#include <unordered_map>


class PCB_ARC;
class BOARD;
class BOARD_ITEM;
class ZONE_SETTINGS;
class BOARD_CONNECTED_ITEM;
class BOARD_ITEM_CONTAINER;
class PAD;
class BOARD_DESIGN_SETTINGS;
class PCB_DIMENSION_BASE;
class PCB_SHAPE;
class PCB_REFERENCE_IMAGE;
class EDA_TEXT;
class PCB_TEXT;
class PCB_TEXTBOX;
class PCB_TRACK;
class PCB_TABLE;
class PCB_TABLECELL;
class FOOTPRINT;
class PCB_GROUP;
class PCB_POINT;
class PCB_TARGET;
class PCB_VIA;
class ZONE;
struct ZONE_LAYER_PROPERTIES;
class PCB_BARCODE;
class FP_3DMODEL;
class SHAPE_LINE_CHAIN;
struct LAYER;
class PROGRESS_REPORTER;
class TEARDROP_PARAMETERS;


/**
 * Read a Pcbnew s-expression formatted #LINE_READER object and returns the appropriate
 * #BOARD_ITEM object.
 */
class PCB_IO_KICAD_SEXPR_PARSER : public PCB_LEXER
{
public:

    typedef std::unordered_map< std::string, PCB_LAYER_ID > LAYER_ID_MAP;
    typedef std::unordered_map< std::string, LSET >         LSET_MAP;
    typedef std::unordered_map< wxString, KIID >            KIID_MAP;

    PCB_IO_KICAD_SEXPR_PARSER( LINE_READER* aReader, BOARD* aAppendToMe,
                               std::function<bool( wxString, int, wxString, wxString )> aQueryUserCallback,
                               PROGRESS_REPORTER* aProgressReporter = nullptr, unsigned aLineCount = 0 ) :
            PCB_LEXER( aReader ),
            m_board( aAppendToMe ),
            m_appendToExisting( aAppendToMe != nullptr ),
            m_progressReporter( aProgressReporter ),
            m_lastProgressTime( std::chrono::steady_clock::now() ),
            m_lineCount( aLineCount ),
            m_queryUserCallback( std::move( aQueryUserCallback ) )
    {
        init();
    }

    // ~PCB_IO_KICAD_SEXPR_PARSER() {}

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

    /**
     * Partially parse the input and check if it matches expected header
     * @return true if expected header matches
     */
    bool IsValidBoardHeader();

    /**
     * Return any non-fatal parse warnings that occurred during parsing.
     * These are errors that were handled gracefully but should be reported to the user.
     */
    const std::vector<wxString>& GetParseWarnings() const { return m_parseWarnings; }

private:

    // Group membership info refers to other Uuids in the file.
    // We don't want to rely on group declarations being last in the file, so
    // we store info about the group declarations here during parsing and then resolve
    // them into BOARD_ITEM* after we've parsed the rest of the file.
    struct GROUP_INFO
    {
        virtual ~GROUP_INFO() = default; // Make polymorphic

        BOARD_ITEM*       parent;
        wxString          name;
        bool              locked;
        KIID              uuid;
        LIB_ID            libId;
        std::vector<KIID> memberUuids;
    };

    struct GENERATOR_INFO : GROUP_INFO
    {
        PCB_LAYER_ID   layer;
        wxString       genType;
        STRING_ANY_MAP properties;
    };

    ///< Convert net code using the mapping table if available,
    ///< otherwise returns unchanged net code if < 0 or if it's out of range
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

    void parseTEARDROP_PARAMETERS( TEARDROP_PARAMETERS* tdParams );

    void parseTextBoxContent( PCB_TEXTBOX* aTextBox );

    PCB_SHAPE*           parsePCB_SHAPE( BOARD_ITEM* aParent );
    PCB_TEXT*            parsePCB_TEXT( BOARD_ITEM* aParent, PCB_TEXT* aBaseText = nullptr );
    void                 parsePCB_TEXT_effects( PCB_TEXT* aText, PCB_TEXT* aBaseText = nullptr );
    PCB_REFERENCE_IMAGE* parsePCB_REFERENCE_IMAGE( BOARD_ITEM* aParent );
    PCB_TEXTBOX*         parsePCB_TEXTBOX( BOARD_ITEM* aParent );
    PCB_BARCODE*         parsePCB_BARCODE( BOARD_ITEM* aParent );
    PCB_TABLECELL*       parsePCB_TABLECELL( BOARD_ITEM* aParent );
    PCB_TABLE*           parsePCB_TABLE( BOARD_ITEM* aParent );
    PCB_DIMENSION_BASE*  parseDIMENSION( BOARD_ITEM* aParent );

    // Parse a footprint, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
    FOOTPRINT*  parseFOOTPRINT_unchecked( wxArrayString* aInitialComments = nullptr );
    void        parseFootprintStackup( FOOTPRINT& aFootprint );

    PAD*        parsePAD( FOOTPRINT* aParent = nullptr );

    // Parse only the (option ...) inside a pad description
    bool        parsePAD_option( PAD* aPad );
    void        parsePostMachining( PADSTACK::POST_MACHINING_PROPS& aProps );

    void        parsePadstack( PAD* aPad );

    PCB_ARC*    parseARC();
    PCB_TRACK*  parsePCB_TRACK();
    PCB_VIA*    parsePCB_VIA();
    void        parseViastack( PCB_VIA* aVia );
    ZONE*       parseZONE( BOARD_ITEM_CONTAINER* aParent );
    PCB_TARGET* parsePCB_TARGET();
    PCB_POINT*  parsePCB_POINT();
    BOARD*      parseBOARD();
    void        parseGROUP_members( GROUP_INFO& aGroupInfo );
    void        parseGROUP( BOARD_ITEM* aParent );
    void        parseGENERATOR( BOARD_ITEM* aParent );

    // Parse a board, but do not replace PARSE_ERROR with FUTURE_FORMAT_ERROR automatically.
    BOARD*      parseBOARD_unchecked();

    /**
     * Parse the current token for the layer definition of a #BOARD_ITEM object.
     *
     * @param aMap is the LAYER_{NUM|MSK}_MAP to use for the lookup.
     * @return The result of the parsed #BOARD_ITEM layer or set designator.
     * @throw IO_ERROR if the layer is not valid.
     * @throw PARSE_ERROR if the layer syntax is incorrect.
     */
    PCB_LAYER_ID lookUpLayer( const LAYER_ID_MAP& aMap );
    LSET lookUpLayerSet( const LSET_MAP& aMap );

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
     * Parse the layers definition of a #BOARD_ITEM object
     * that has a single copper layer and optional soldermask layer.
     *
     * @return The mask of layers the parsed #BOARD_ITEM is on.
     * @throw IO_ERROR if any of the layers is not valid.
     * @throw PARSE_ERROR if the layers syntax is incorrect.
     */
    LSET parseLayersForCuItemWithSoldermask();

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
    VECTOR2I parseXY();

    void parseXY( int* aX, int* aY );

    void parseMargins( int& aLeft, int& aTop, int& aRight, int& aBottom );

    void parseZoneDefaults( ZONE_SETTINGS& aZoneSettings );

    void parseZoneLayerProperty( std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>& aProperties );

    std::pair<wxString, wxString> parseBoardProperty();

    void parseVariants();
    void parseFootprintVariant( FOOTPRINT* aFootprint );

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
     * @param aText A pointer to the #EDA_TEXT object to save the parsed settings into.
     * @throw PARSE_ERROR if the text syntax is not valid.
     */
    void parseEDA_TEXT( EDA_TEXT* aText );

    /**
     * Parse the render cache for any object derived from #EDA_TEXT.
     *
     * @param aText A pointer to the #EDA_TEXT object to save the parsed settings into.
     * @throw PARSE_ERROR if the text syntax is not valid.
     */
    void parseRenderCache( EDA_TEXT* text );

    FP_3DMODEL* parse3DModel();

    /**
     * Parse the current token as an ASCII numeric string with possible leading
     * whitespace into a double precision floating point number.
     *
     * @return The result of the parsed token.
     * @throw IO_ERROR if an error occurs attempting to convert the current token.
     */

    int parseBoardUnits();

    int parseBoardUnits( const char* aExpected, EDA_DATA_TYPE aDataType );

    inline int parseBoardUnits( const PCB_KEYS_T::T aToken, const EDA_DATA_TYPE aDataType = EDA_DATA_TYPE::DISTANCE )
    {
        return parseBoardUnits( GetTokenText( aToken ), aDataType );
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

    std::optional<bool> parseOptBool();

    /**
     * Parses a boolean flag inside a list that existed before boolean normalization.
     *
     * For example, this will handle both (legacy_teardrops) and (legacy_teardrops yes).
     * Call this after parsing the T_legacy_teardrops, and aDefaultValue will be returned for the
     * first case, or true will be returned for the second case.
     *
     * @param aDefaultValue will be returned if the end of the list is encountered as the next token
     * @return the parsed boolean
     */
    bool parseMaybeAbsentBool( bool aDefaultValue );

    std::pair<std::optional<bool>, std::optional<bool>> parseFrontBackOptBool( bool aAllowLegacyFormat = false );

    void parseNet( BOARD_CONNECTED_ITEM* aItem );

    /*
     * @return if m_appendToExisting, returns new KIID(), otherwise returns CurStr() as KIID.
     */
    KIID CurStrToKIID();

    /**
     * Called after parsing a footprint definition or board to build the group membership
     * lists.
     */
    void resolveGroups( BOARD_ITEM* aParent );

    ///< The type of progress bar timeout
    using TIMEOUT = std::chrono::milliseconds;

    ///< The clock used for the timestamp (guaranteed to be monotonic).
    using CLOCK = std::chrono::steady_clock;

    ///< The type of the time stamps.
    using TIME_PT = std::chrono::time_point<CLOCK>;

    BOARD*              m_board;
    LAYER_ID_MAP        m_layerIndices;     ///< map layer name to it's index
    LSET_MAP            m_layerMasks;       ///< map layer names to their masks
    std::set<wxString>  m_undefinedLayers;  ///< set of layers not defined in layers section
    std::vector<int>    m_netCodes;         ///< net codes mapping for boards being loaded
    bool                m_tooRecent;        ///< true if version parses as later than supported
    int                 m_requiredVersion;  ///< set to the KiCad format version this board requires
    wxString            m_generatorVersion; ///< Set to the generator version this board requires
    bool                m_appendToExisting; ///< reading into an existing board; reset UUIDs

    ///< if resetting UUIDs, record new ones to update groups with.
    KIID_MAP            m_resetKIIDMap;

    bool                m_showLegacySegmentZoneWarning;
    bool                m_showLegacy5ZoneWarning;

    PROGRESS_REPORTER*  m_progressReporter;  ///< optional; may be nullptr
    TIME_PT             m_lastProgressTime;  ///< for progress reporting
    unsigned            m_lineCount;         ///< for progress reporting

    std::vector<GROUP_INFO>     m_groupInfos;
    std::vector<GENERATOR_INFO> m_generatorInfos;

    std::function<bool( wxString aTitle, int aIcon, wxString aMsg, wxString aAction )> m_queryUserCallback;

    std::vector<wxString>       m_parseWarnings;    ///< Non-fatal warnings collected during parsing
};


#endif    // _PCBNEW_PARSER_H_

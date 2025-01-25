/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * @brief Schematic and symbol library s-expression file format parser definitions.
 */

#ifndef SCH_IO_KICAD_SEXPR_PARSER_H_
#define SCH_IO_KICAD_SEXPR_PARSER_H_

#include <symbol_library_common.h>
#include <progress_reporter.h>
#include <schematic_lexer.h>
#include <sch_file_versions.h>
#include <default_values.h>    // For some default values


class SCH_PIN;
class PAGE_INFO;
class SCH_BITMAP;
class SCH_BUS_WIRE_ENTRY;
class SCH_SYMBOL;
class SCH_FIELD;
class SCH_ITEM;
class SCH_SHAPE;
class SCH_RULE_AREA;
class SCH_JUNCTION;
class SCH_LINE;
class SCH_NO_CONNECT;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SHEET_PIN;
class SCH_TEXT;
class SCH_TEXTBOX;
class SCH_TABLE;
class SCH_TABLECELL;
class TITLE_BLOCK;


/**
 * Simple container to manage fill parameters.
 */
class FILL_PARAMS
{
public:
    FILL_T  m_FillType;
    COLOR4D m_Color;
};


/**
 * Object to parser s-expression symbol library and schematic file formats.
 */
class SCH_IO_KICAD_SEXPR_PARSER : public SCHEMATIC_LEXER
{
public:
    SCH_IO_KICAD_SEXPR_PARSER( LINE_READER* aLineReader = nullptr,
                      PROGRESS_REPORTER* aProgressReporter = nullptr, unsigned aLineCount = 0,
                      SCH_SHEET* aRootSheet = nullptr, bool aIsAppending = false );

    void ParseLib( LIB_SYMBOL_MAP& aSymbolLibMap );

    /**
     * Parse internal #LINE_READER object into symbols and return all found.
     */
    LIB_SYMBOL* ParseSymbol( LIB_SYMBOL_MAP& aSymbolLibMap,
                             int aFileVersion = SEXPR_SYMBOL_LIB_FILE_VERSION );

    SCH_ITEM* ParseSymbolDrawItem();

    /**
     * Parse the internal #LINE_READER object into \a aSheet.
     *
     * When \a aIsCopyableOnly is true, only schematic objects that are viewable on the canvas
     * for copy and paste purposes are parsed.  Other schematic content such as bus definitions
     * or instance data will throw an #IO_ERROR exception.
     *
     * When \a aIsCopyableOnly is false, full schematic file parsing is performed.
     *
     * @note This does not load any sub-sheets or decent complex sheet hierarchies.
     *
     * @param aSheet The #SCH_SHEET object to store the parsed schematic file.
     * @param aIsCopyableOnly Load only the schematic objects that can be copied into \a aSheet
     *                        if true.  Otherwise, load the full schematic file format.
     * @param aFileVersion The schematic file version to parser.  Defaults to the schematic
     *                     file being parsed when \a aIsCopyableOnly is false.
     */
    void ParseSchematic( SCH_SHEET* aSheet, bool aIsCopyablyOnly = false,
                         int aFileVersion = SEXPR_SCHEMATIC_FILE_VERSION );

    int GetParsedRequiredVersion() const { return m_requiredVersion; }

private:
    // Group membership info refers to other Uuids in the file.
    // We don't want to rely on group declarations being last in the file, so
    // we store info about the group declarations here during parsing and then resolve
    // them into BOARD_ITEM* after we've parsed the rest of the file.
    struct GROUP_INFO
    {
        virtual ~GROUP_INFO() = default; // Make polymorphic

        wxString          name;
        KIID              uuid;
        LIB_ID            libId;
        std::vector<KIID> memberUuids;
    };

    void checkpoint();

    KIID parseKIID();

    void parseHeader( TSCHEMATIC_T::T aHeaderType, int aFileVersion );

    inline long parseHex()
    {
        NextTok();
        return strtol( CurText(), nullptr, 16 );
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

    int parseInternalUnits();

    int parseInternalUnits( const char* aExpected );

    int parseInternalUnits( TSCHEMATIC_T::T aToken )
    {
        return parseInternalUnits( GetTokenText( aToken ) );
    }

    VECTOR2I parseXY( bool aInvertY = false )
    {
        VECTOR2I xy;

        xy.x = parseInternalUnits( "X coordinate" );
        xy.y = aInvertY ? -parseInternalUnits( "Y coordinate" )
                        :  parseInternalUnits( "Y coordinate" );

        return xy;
    }

    bool parseBool();

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

    LIB_SYMBOL* parseLibSymbol( LIB_SYMBOL_MAP& aSymbolLibMap );

    /**
     * Parse stroke definition \a aStroke.
     *
     * @param aStrokeDef A reference to the #STROKE_PARAMS structure to write to.
     */
    void parseStroke( STROKE_PARAMS& aStroke );

    void parseFill( FILL_PARAMS& aFill );

    void parseMargins( int& aLeft, int& aTop, int& aRight, int& aBottom )
    {
        aLeft = parseInternalUnits( "left margin" );
        aTop = parseInternalUnits( "top margin" );
        aRight = parseInternalUnits( "right margin" );
        aBottom = parseInternalUnits( "bottom margin" );
    }

    void parseEDA_TEXT( EDA_TEXT* aText, bool aConvertOverbarSyntax, bool aEnforceMinTextSize = true );
    void parseBodyStyles( std::unique_ptr<LIB_SYMBOL>& aSymbol );
    void parsePinNames( std::unique_ptr<LIB_SYMBOL>& aSymbol );
    void parsePinNumbers( std::unique_ptr<LIB_SYMBOL>& aSymbol );

    SCH_FIELD* parseProperty( std::unique_ptr<LIB_SYMBOL>& aSymbol );

    SCH_SHAPE* parseSymbolArc();
    SCH_SHAPE* parseSymbolBezier();
    SCH_SHAPE* parseSymbolCircle();
    SCH_PIN* parseSymbolPin();
    SCH_SHAPE* parseSymbolPolyLine();
    SCH_SHAPE* parseSymbolRectangle();
    SCH_ITEM* parseSymbolText();
    SCH_TEXTBOX* parseSymbolTextBox();

    void parsePAGE_INFO( PAGE_INFO& aPageInfo );
    void parseTITLE_BLOCK( TITLE_BLOCK& aTitleBlock );
    void parseSchSymbolInstances( SCH_SCREEN* aScreen );
    void parseSchSheetInstances( SCH_SHEET* aRootSheet, SCH_SCREEN* aScreen );

    void parseGroup();
    void parseGroupMembers( GROUP_INFO& aGroupInfo );

    SCH_SHEET_PIN* parseSchSheetPin( SCH_SHEET* aSheet );
    SCH_FIELD* parseSchField( SCH_ITEM* aParent );
    SCH_SYMBOL* parseSchematicSymbol();
    SCH_BITMAP* parseImage();
    SCH_SHEET* parseSheet();
    SCH_JUNCTION* parseJunction();
    SCH_NO_CONNECT* parseNoConnect();
    SCH_BUS_WIRE_ENTRY* parseBusEntry();
    SCH_LINE* parseLine();
    SCH_SHAPE* parseSchPolyLine();
    SCH_SHAPE* parseSchArc();
    SCH_SHAPE* parseSchCircle();
    SCH_SHAPE* parseSchRectangle();
    SCH_SHAPE* parseSchBezier();
    SCH_RULE_AREA* parseSchRuleArea();
    SCH_TEXT* parseSchText();
    SCH_TEXTBOX* parseSchTextBox();
    void parseSchTextBoxContent( SCH_TEXTBOX* aTextBox );
    SCH_TABLECELL* parseSchTableCell();
    SCH_TABLE* parseSchTable();
    void parseBusAlias( SCH_SCREEN* aScreen );

    void resolveGroups( SCH_SCREEN* aParent );

private:
    int      m_requiredVersion;   ///< Set to the symbol library file version required.
    wxString m_generatorVersion;
    int      m_unit;              ///< The current unit being parsed.
    int      m_bodyStyle;         ///< The current body style being parsed.
    wxString m_symbolName;        ///< The current symbol name.
    bool     m_appending;         ///< Appending load status.

    std::set<KIID>     m_uuids;

    PROGRESS_REPORTER* m_progressReporter;  // optional; may be nullptr
    const LINE_READER* m_lineReader;        // for progress reporting
    unsigned           m_lastProgressLine;
    unsigned           m_lineCount;         // for progress reporting

    KIID               m_rootUuid;          // The UUID of the root schematic.

    /// The rootsheet for full project loads or null for importing a schematic.
    SCH_SHEET*         m_rootSheet;

    /// Max deviation allowed when approximating bezier curves
    int                m_maxError;

    std::vector<GROUP_INFO> m_groupInfos;
};

#endif    // SCH_IO_KICAD_SEXPR_PARSER_H_

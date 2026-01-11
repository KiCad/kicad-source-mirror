/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * @brief Schematic and symbol library s-expression file format parser implementations.
 */

// For some reason wxWidgets is built with wxUSE_BASE64 unset so expose the wxWidgets
// base64 code.
#include <charconv>

#include <fmt/format.h>
#define wxUSE_BASE64 1
#include <wx/base64.h>
#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/tokenzr.h>

#include <base_units.h>
#include <bitmap_base.h>
#include <embedded_files_parser.h>
#include <lib_id.h>
#include <sch_pin.h>
#include <math/util.h>                           // KiROUND
#include <font/font.h>
#include <font/fontconfig.h>
#include <pgm_base.h>
#include <string_utils.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>          // SYM_ORIENT_XXX
#include <sch_field.h>
#include <sch_group.h>
#include <sch_line.h>
#include <sch_rule_area.h>
#include <sch_textbox.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_label.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet_pin.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>
#include <stroke_params_parser.h>
#include <template_fieldnames.h>
#include <trigo.h>
#include <progress_reporter.h>
#include <sim/sim_model.h>


using namespace TSCHEMATIC_T;


SCH_IO_KICAD_SEXPR_PARSER::SCH_IO_KICAD_SEXPR_PARSER( LINE_READER* aLineReader,
                                                      PROGRESS_REPORTER* aProgressReporter,
                                                      unsigned aLineCount, SCH_SHEET* aRootSheet,
                                                      bool aIsAppending ) :
        SCHEMATIC_LEXER( aLineReader ),
        m_requiredVersion( 0 ),
        m_unit( 1 ),
        m_bodyStyle( 1 ),
        m_appending( aIsAppending ),
        m_progressReporter( aProgressReporter ),
        m_lineReader( aLineReader ),
        m_lastProgressLine( 0 ),
        m_lineCount( aLineCount ),
        m_rootSheet( aRootSheet ),
        m_maxError( ARC_LOW_DEF_MM * schIUScale.IU_PER_MM )
{
}


void SCH_IO_KICAD_SEXPR_PARSER::checkpoint()
{
    const unsigned PROGRESS_DELTA = 500;

    if( m_progressReporter )
    {
        unsigned curLine = m_lineReader->LineNumber();

        if( curLine > m_lastProgressLine + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) curLine )
                                                            / std::max( 1U, m_lineCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( _( "Open canceled by user." ) );

            m_lastProgressLine = curLine;
        }
    }
}


KIID SCH_IO_KICAD_SEXPR_PARSER::parseKIID()
{
    KIID id( FromUTF8() );

    while( m_uuids.count( id ) )
        id.Increment();

    m_uuids.insert( id );

    return id;
}


bool SCH_IO_KICAD_SEXPR_PARSER::parseBool()
{
    T token = NextTok();

    if( token == T_yes )
        return true;
    else if( token == T_no )
        return false;
    else
        Expecting( "yes or no" );

    return false;
}


/*
 * e.g. "hide", "hide)", "(hide yes)"
 */
bool SCH_IO_KICAD_SEXPR_PARSER::parseMaybeAbsentBool( bool aDefaultValue )
{
    bool ret = aDefaultValue;

    if( PrevTok() == T_LEFT )
    {
        T token = NextTok();

        // "hide)"
        if( static_cast<int>( token ) == DSN_RIGHT )
            return aDefaultValue;

        if( token == T_yes )
            ret = true;
        else if( token == T_no )
            ret = false;
        else
            Expecting( "yes or no" );

        NeedRIGHT();
    }
    else
    {
        // "hide"
        return aDefaultValue;
    }

    return ret;
}


void SCH_IO_KICAD_SEXPR_PARSER::ParseLib( LIB_SYMBOL_MAP& aSymbolLibMap )
{
    T token;

    NeedLEFT();
    NextTok();
    parseHeader( T_kicad_symbol_lib, SEXPR_SYMBOL_LIB_FILE_VERSION );

    // Prior to this, bar was a valid string char for unquoted strings.
    SetKnowsBar( m_requiredVersion >= 20240529 );

    bool versionChecked = false;

    auto checkVersion =
            [&]()
            {
                if( !versionChecked && m_requiredVersion > SEXPR_SYMBOL_LIB_FILE_VERSION )
                {
                    throw FUTURE_FORMAT_ERROR( fmt::format( "{}", m_requiredVersion ),
                                               m_generatorVersion );
                }

                versionChecked = true;
            };

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_generator:
            // (generator "genname"); we don't care about it at the moment.
            NeedSYMBOL();
            NeedRIGHT();
            break;

        case T_host:
        {
            // (host eeschema ["5.99"]); old version of generator token
            NeedSYMBOL();

            // Really old versions also included a host version
            if( m_requiredVersion < 20200827 )
                NeedSYMBOL();

            NeedRIGHT();
            break;
        }

        case T_generator_version:
        {
            NextTok();
            m_generatorVersion = FromUTF8();
            NeedRIGHT();

            // If the format includes a generator version, by this point we have enough info to
            // do the version check here
            checkVersion();
            break;
        }

        case T_symbol:
        {
            // By the time we get to the first symbol, we can check the version
            checkVersion();

            m_unit = 1;
            m_bodyStyle = 1;

            try
            {
                LIB_SYMBOL* symbol = parseLibSymbol( aSymbolLibMap );
                aSymbolLibMap[symbol->GetName()] = symbol;
            }
            catch( const IO_ERROR& e )
            {
                // Record the error and skip to the end of this symbol block
                wxString warning = wxString::Format(
                        _( "Error parsing symbol at line %d: %s\nSkipping symbol and continuing." ),
                        CurLineNumber(), e.What() );
                m_parseWarnings.push_back( warning );

                // Skip to the end of this symbol's S-expression block
                // We're already past T_symbol, so we're inside the symbol definition
                skipToBlockEnd( 1 );
            }

            break;
        }

        default:
            Expecting( "symbol, generator, or generator_version" );
        }
    }
}


LIB_SYMBOL* SCH_IO_KICAD_SEXPR_PARSER::ParseSymbol( LIB_SYMBOL_MAP& aSymbolLibMap,
                                                    int aFileVersion )
{
    LIB_SYMBOL* newSymbol = nullptr;

    NextTok();

    // If there actually isn't anything here, don't throw just return a nullptr
    if( CurTok() == T_LEFT )
    {
        NextTok();

        if( CurTok() == T_symbol )
        {
            m_requiredVersion = aFileVersion;
            newSymbol         = parseLibSymbol( aSymbolLibMap );

            const std::vector<wxString>* embeddedFonts =
                    newSymbol->GetEmbeddedFiles()->UpdateFontFiles();

            newSymbol->RunOnChildren(
                    [&]( SCH_ITEM* aChild )
                    {
                        if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( aChild ) )
                            textItem->ResolveFont( embeddedFonts );
                    },
                    RECURSE_MODE::NO_RECURSE );
        }
        else
        {
            wxString msg = wxString::Format( _( "Cannot parse %s as a symbol" ),
                                             GetTokenString( CurTok() ) );
            THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
        }
    }

    return newSymbol;
}


LIB_SYMBOL* SCH_IO_KICAD_SEXPR_PARSER::parseLibSymbol( LIB_SYMBOL_MAP& aSymbolLibMap )
{
    wxCHECK_MSG( CurTok() == T_symbol, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a symbol." ) );

    T token;
    long tmp;
    wxString name;
    wxString error;
    SCH_ITEM* item;
    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( wxEmptyString );

    symbol->SetUnitCount( 1, true );

    token = NextTok();

    if( !IsSymbol( token ) )
        THROW_PARSE_ERROR( _( "Invalid symbol name" ), CurSource(), CurLine(), CurLineNumber(), CurOffset() );

    name = FromUTF8();

    // Some symbol LIB_IDs have the '/' character escaped which can break derived symbol links.
    // The '/' character is no longer an illegal LIB_ID character so it doesn't need to be
    // escaped.
    name.Replace( wxS( "{slash}" ), wxT( "/" ) );

    LIB_ID id;
    int    bad_pos = id.Parse( name );

    if( bad_pos >= 0 )
    {
        if( static_cast<int>( name.size() ) > bad_pos )
        {
            wxString msg = wxString::Format( _( "Symbol %s contains invalid character '%c'" ),
                                             name,
                                             name[bad_pos] );

            THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
        }


        THROW_PARSE_ERROR( _( "Invalid library identifier" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }

    m_symbolName = id.GetLibItemName().wx_str();
    symbol->SetName( m_symbolName );
    symbol->SetLibId( id );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_power:
            symbol->SetGlobalPower();
            token = NextTok();

            if( token == T_RIGHT )
                break;

            if( token == T_local )
                symbol->SetLocalPower();
            else if( token != T_global )
                Expecting( "global or local" );

            NeedRIGHT();
            break;

        case T_body_styles:
            parseBodyStyles( symbol );
            break;

        case T_pin_names:
            parsePinNames( symbol );
            break;

        case T_pin_numbers:
            parsePinNumbers( symbol );
            break;

        case T_exclude_from_sim:
            symbol->SetExcludedFromSim( parseBool() );
            NeedRIGHT();
            break;

        case T_in_bom:
            symbol->SetExcludedFromBOM( !parseBool() );
            NeedRIGHT();
            break;

        case T_on_board:
            symbol->SetExcludedFromBoard( !parseBool() );
            NeedRIGHT();
            break;

        case T_in_pos_files:
            symbol->SetExcludedFromPosFiles( !parseBool() );
            NeedRIGHT();
            break;

        case T_duplicate_pin_numbers_are_jumpers:
            symbol->SetDuplicatePinNumbersAreJumpers( parseBool() );
            NeedRIGHT();
            break;

        case T_jumper_pin_groups:
        {
            std::vector<std::set<wxString>>& groups = symbol->JumperPinGroups();
            std::set<wxString>* currentGroup = nullptr;

            for( token = NextTok(); currentGroup || token != T_RIGHT; token = NextTok() )
            {
                switch( static_cast<int>( token ) )
                {
                case T_LEFT:
                    currentGroup = &groups.emplace_back();
                    break;

                case DSN_STRING:
                    currentGroup->insert( FromUTF8() );
                    break;

                case T_RIGHT:
                    currentGroup = nullptr;
                    break;

                default:
                    Expecting( "list of pin names" );
                }
            }

            break;
        }

        case T_property:
            parseProperty( symbol );
            break;

        case T_extends:
        {
            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid parent symbol name" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );
            }

            name = FromUTF8();

            // Some symbol LIB_IDs have the '/' character escaped which can break derived
            // symbol links.  The '/' character is no longer an illegal LIB_ID character so
            // it doesn't need to be escaped.
            name.Replace( wxS( "{slash}" ), wxT( "/" ) );

            symbol->SetParentName( name );
            NeedRIGHT();
            break;
        }

        case T_symbol:
        {
            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid symbol unit name" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );
            }

            name = FromUTF8();

            // Some symbol LIB_IDs have the '/' character escaped which can break derived
            // symbol links.  The '/' character is no longer an illegal LIB_ID character so
            // it doesn't need to be escaped.
            name.Replace( wxS( "{slash}" ), wxT( "/" ) );

            if( !name.StartsWith( m_symbolName ) )
            {
                error.Printf( _( "Invalid symbol unit name prefix %s" ), name.c_str() );
                THROW_PARSE_ERROR( error, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            name = name.Right( name.Length() - m_symbolName.Length() - 1 );

            wxStringTokenizer tokenizer( name, "_" );

            if( tokenizer.CountTokens() != 2 )
            {
                error.Printf( _( "Invalid symbol unit name suffix %s" ), name.c_str() );
                THROW_PARSE_ERROR( error, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            if( !tokenizer.GetNextToken().ToLong( &tmp ) )
            {
                error.Printf( _( "Invalid symbol unit number %s" ), name.c_str() );
                THROW_PARSE_ERROR( error, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            m_unit = static_cast<int>( tmp );

            if( !tokenizer.GetNextToken().ToLong( &tmp ) )
            {
                error.Printf( _( "Invalid symbol body style number %s" ), name.c_str() );
                THROW_PARSE_ERROR( error, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            m_bodyStyle = static_cast<int>( tmp );

            if( m_bodyStyle > 1 )
                symbol->SetBodyStyleCount( m_bodyStyle, false, false );

            if( m_unit > symbol->GetUnitCount() )
                symbol->SetUnitCount( m_unit, false );

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_unit_name:
                    token = NextTok();

                    if( IsSymbol( token ) )
                        symbol->GetUnitDisplayNames()[m_unit] = FromUTF8();

                    NeedRIGHT();
                    break;

                case T_arc:
                case T_bezier:
                case T_circle:
                case T_pin:
                case T_polyline:
                case T_rectangle:
                case T_text:
                case T_text_box:
                    item = ParseSymbolDrawItem();

                    wxCHECK_MSG( item, nullptr, "Invalid draw item pointer." );

                    item->SetParent( symbol.get() );
                    symbol->AddDrawItem( item, false );
                    break;

                default:
                    Expecting( "arc, bezier, circle, pin, polyline, rectangle, or text" );
                };
            }

            m_unit = 1;
            m_bodyStyle = 1;
            break;
        }

        case T_arc:
        case T_bezier:
        case T_circle:
        case T_pin:
        case T_polyline:
        case T_rectangle:
        case T_text:
        case T_text_box:
            item = ParseSymbolDrawItem();

            wxCHECK_MSG( item, nullptr, "Invalid draw item pointer." );

            item->SetParent( symbol.get() );
            symbol->AddDrawItem( item, false );
            break;

        case T_embedded_fonts:
        {
            symbol->SetAreFontsEmbedded( parseBool() );
            NeedRIGHT();
            break;
        }

        case T_embedded_files:
        {
            EMBEDDED_FILES_PARSER embeddedFilesParser( reader );
            embeddedFilesParser.SyncLineReaderWith( *this );

            try
            {
                embeddedFilesParser.ParseEmbedded( symbol->GetEmbeddedFiles() );
            }
            catch( const IO_ERROR& e )
            {
                m_parseWarnings.push_back( e.What() );
            }

            SyncLineReaderWith( embeddedFilesParser );
            break;
        }

        default:
            Expecting( "pin_names, pin_numbers, arc, bezier, circle, pin, polyline, "
                       "rectangle, or text" );
        }
    }

    symbol->GetDrawItems().sort();
    m_symbolName.clear();

    const std::vector<wxString>* embeddedFonts = symbol->GetEmbeddedFiles()->UpdateFontFiles();

    symbol->RunOnChildren(
            [&]( SCH_ITEM* aChild )
            {
                if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( aChild ) )
                    textItem->ResolveFont( embeddedFonts );
            },
            RECURSE_MODE::NO_RECURSE );

    // Before V10 we didn't store the number of body styles in a symbol, we just looked at all its
    // drawings each time we wanted to know.
    if( m_requiredVersion < 20250827 )
        symbol->SetHasDeMorganBodyStyles( symbol->HasLegacyAlternateBodyStyle() );

    return symbol.release();
}


SCH_ITEM* SCH_IO_KICAD_SEXPR_PARSER::ParseSymbolDrawItem()
{
    switch( CurTok() )
    {
    case T_arc:       return parseSymbolArc();       break;
    case T_bezier:    return parseSymbolBezier();    break;
    case T_circle:    return parseSymbolCircle();    break;
    case T_pin:       return parseSymbolPin();       break;
    case T_polyline:  return parseSymbolPolyLine();  break;
    case T_rectangle: return parseSymbolRectangle(); break;
    case T_text:      return parseSymbolText();      break;
    case T_text_box:  return parseSymbolTextBox();   break;
    default:          Expecting( "arc, bezier, circle, pin, polyline, rectangle, or text" );
    }

    return nullptr;
}


int SCH_IO_KICAD_SEXPR_PARSER::parseInternalUnits()
{
    auto retval = parseDouble() * schIUScale.IU_PER_MM;

    // Schematic internal units are represented as integers.  Any values that are
    // larger or smaller than the schematic units represent undefined behavior for
    // the system.  Limit values to the largest that can be displayed on the screen.
    constexpr double int_limit = std::numeric_limits<int>::max() * 0.7071; // 0.7071 = roughly 1/sqrt(2)

    return KiROUND( std::clamp( retval, -int_limit, int_limit ) );
}


int SCH_IO_KICAD_SEXPR_PARSER::parseInternalUnits( const char* aExpected )
{
    auto retval = parseDouble( aExpected ) * schIUScale.IU_PER_MM;

    constexpr double int_limit = std::numeric_limits<int>::max() * 0.7071;

    return KiROUND( std::clamp( retval, -int_limit, int_limit ) );
}


void SCH_IO_KICAD_SEXPR_PARSER::parseStroke( STROKE_PARAMS& aStroke )
{
    STROKE_PARAMS_PARSER strokeParser( reader, schIUScale.IU_PER_MM );
    strokeParser.SyncLineReaderWith( *this );

    strokeParser.ParseStroke( aStroke );
    SyncLineReaderWith( strokeParser );
}


void SCH_IO_KICAD_SEXPR_PARSER::parseFill( FILL_PARAMS& aFill )
{
    wxCHECK_RET( CurTok() == T_fill, "Cannot parse " + GetTokenString( CurTok() ) + " as a fill." );

    aFill.m_FillType = FILL_T::NO_FILL;
    aFill.m_Color = COLOR4D::UNSPECIFIED;

    T token;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_type:
        {
            token = NextTok();

            switch( token )
            {
            case T_none:          aFill.m_FillType = FILL_T::NO_FILL;                  break;
            case T_outline:       aFill.m_FillType = FILL_T::FILLED_SHAPE;             break;
            case T_background:    aFill.m_FillType = FILL_T::FILLED_WITH_BG_BODYCOLOR; break;
            case T_color:         aFill.m_FillType = FILL_T::FILLED_WITH_COLOR;        break;
            case T_hatch:         aFill.m_FillType = FILL_T::HATCH;                    break;
            case T_reverse_hatch: aFill.m_FillType = FILL_T::REVERSE_HATCH;            break;
            case T_cross_hatch:   aFill.m_FillType = FILL_T::CROSS_HATCH;              break;
            default:              Expecting( "none, outline, hatch, reverse_hatch, "
                                             "cross_hatch, color or background" );
            }

            NeedRIGHT();
            break;
        }

        case T_color:
        {
            COLOR4D color;

            color.r = parseInt( "red" ) / 255.0;
            color.g = parseInt( "green" ) / 255.0;
            color.b = parseInt( "blue" ) / 255.0;
            color.a = std::clamp( parseDouble( "alpha" ), 0.0, 1.0 );
            aFill.m_Color = color;
            NeedRIGHT();
            break;
        }

        default:
            Expecting( "type or color" );
        }
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parseEDA_TEXT( EDA_TEXT* aText, bool aConvertOverbarSyntax,
                                               bool aEnforceMinTextSize )
{
    wxCHECK_RET( aText && ( CurTok() == T_effects || CurTok() == T_href ),
                 "Cannot parse " + GetTokenString( CurTok() ) + " as an EDA_TEXT." );

    // In version 20210606 the notation for overbars was changed from `~...~` to `~{...}`.
    // We need to convert the old syntax to the new one.
    if( aConvertOverbarSyntax && m_requiredVersion < 20210606 )
        aText->SetText( ConvertToNewOverbarNotation( aText->GetText() ) );

    T        token;
    bool     bold = false;
    bool     italic = false;
    COLOR4D  color = COLOR4D::UNSPECIFIED;

    // Various text objects (text boxes, schematic text, etc.) all have their own defaults,
    // but the file format default is {center,center} so we have to set that before parsing.
    aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_font:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_face:
                    NeedSYMBOL();
                    aText->SetUnresolvedFontName( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_size:
                {
                    VECTOR2I sz;
                    sz.y = parseInternalUnits( "text height" );
                    sz.x = parseInternalUnits( "text width" );
                    aText->SetTextSize( sz, aEnforceMinTextSize );
                    NeedRIGHT();
                    break;
                }

                case T_thickness:
                    aText->SetTextThickness( parseInternalUnits( "text thickness" ) );
                    NeedRIGHT();
                    break;

                case T_bold:
                    bold = parseMaybeAbsentBool( true );
                    aText->SetBoldFlag( bold );
                    break;

                case T_italic:
                    italic = parseMaybeAbsentBool( true );
                    aText->SetItalicFlag( italic );
                    break;

                case T_color:
                    color.r = parseInt( "red" ) / 255.0;
                    color.g = parseInt( "green" ) / 255.0;
                    color.b = parseInt( "blue" ) / 255.0;
                    color.a = std::clamp( parseDouble( "alpha" ), 0.0, 1.0 );
                    aText->SetTextColor( color );
                    NeedRIGHT();
                    break;

                case T_line_spacing:
                    aText->SetLineSpacing( parseDouble( "line spacing" ) );
                    NeedRIGHT();
                    break;

                default:
                    Expecting( "face, size, thickness, line_spacing, bold, or italic" );
                }
            }

            break;

        case T_justify:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                switch( token )
                {
                case T_left:   aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );  break;
                case T_right:  aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT ); break;
                case T_top:    aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );    break;
                case T_bottom: aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM ); break;
                // Do not set mirror property for schematic text elements
                case T_mirror: break;
                default:       Expecting( "left, right, top, bottom, or mirror" );
                }
            }

            break;

        case T_href:
        {
            NeedSYMBOL();
            wxString hyperlink = FromUTF8();

            if( !EDA_TEXT::ValidateHyperlink( hyperlink ) )
            {
                THROW_PARSE_ERROR( wxString::Format( _( "Invalid hyperlink url '%s'" ), hyperlink ),
                                   CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }
            else
            {
                aText->SetHyperlink( hyperlink );
            }

            NeedRIGHT();
        }
        break;

        case T_hide:
        {
            bool hide = parseMaybeAbsentBool( true );
            aText->SetVisible( !hide );
            break;
        }

        default:
            Expecting( "font, justify, hide or href" );
        }
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parseHeader( TSCHEMATIC_T::T aHeaderType, int aFileVersion )
{
    wxCHECK_RET( CurTok() == aHeaderType,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a header." ) );

    NeedLEFT();

    T tok = NextTok();

    if( tok == T_version )
    {
        m_requiredVersion = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );
        NeedRIGHT();
    }
    else
    {
        m_requiredVersion = aFileVersion;
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parseBodyStyles( std::unique_ptr<LIB_SYMBOL>& aSymbol )
{
    wxCHECK_RET( CurTok() == T_body_styles,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as a body_styles token." );

    std::vector<wxString> names;

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token == T_demorgan )
        {
            aSymbol->SetHasDeMorganBodyStyles( true );
            continue;
        }
        else if( !IsSymbol( token ) )
        {
            THROW_PARSE_ERROR( _( "Invalid property value" ), CurSource(), CurLine(), CurLineNumber(),
                               CurOffset() );
        }

        names.push_back( FromUTF8() );
    }

    if( !names.empty() )
        aSymbol->SetBodyStyleNames( names );
}


void SCH_IO_KICAD_SEXPR_PARSER::parsePinNames( std::unique_ptr<LIB_SYMBOL>& aSymbol )
{
    wxCHECK_RET( CurTok() == T_pin_names,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as a pin_name token." );

    /**
     * (pin_names
     *    (offset 123)  ; optional
     *    (hide yes/no) ; optional
     *    hide          ; optional, pre-20241004
     * )
     */

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        // Pre-20241004 format - bare 'hide' keyword
        if( token == T_hide )
        {
            aSymbol->SetShowPinNames( false );
            continue;
        }

        if( token != T_LEFT )
        {
            Expecting( T_LEFT );
        }

        token = NextTok();

        switch( token )
        {
        case T_offset:
            aSymbol->SetPinNameOffset( parseInternalUnits( "pin name offset" ) );
            NeedRIGHT();
            break;

        case T_hide:
            aSymbol->SetShowPinNames( !parseBool() );
            NeedRIGHT();
            break;

        default:
            Expecting( "offset or hide" );
        }
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parsePinNumbers( std::unique_ptr<LIB_SYMBOL>& aSymbol )
{
    wxCHECK_RET( CurTok() == T_pin_numbers,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as a pin_number token." );

    /**
     * (pin_numbers
     *    (hide yes/no) ; optional
     *    hide          ; optional (pre-20241004)
     * )
     */

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        // Pre-20241004 format - bare 'hide' keyword
        if( token == T_hide )
        {
            aSymbol->SetShowPinNumbers( false );
            continue;
        }

        if( token != T_LEFT )
        {
            Expecting( T_LEFT );
        }

        token = NextTok();

        switch( token )
        {
        case T_hide:
            aSymbol->SetShowPinNumbers( !parseBool() );
            NeedRIGHT();
            break;

        default:
            Expecting( "hide" );
            break;
        }
    }
}


SCH_FIELD* SCH_IO_KICAD_SEXPR_PARSER::parseProperty( std::unique_ptr<LIB_SYMBOL>& aSymbol )
{
    wxCHECK_MSG( CurTok() == T_property, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a property." ) );
    wxCHECK( aSymbol, nullptr );

    FIELD_T  fieldId = FIELD_T::USER;
    wxString name;
    wxString value;
    bool     isPrivate = false;
    bool     isVisible = true;

    T token = NextTok();

    if( token == T_private )
    {
        isPrivate = true;
        token = NextTok();
    }

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid property name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    name = FromUTF8();

    if( name.IsEmpty() )
    {
        THROW_PARSE_ERROR( _( "Empty property name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    // Correctly set the ID based on canonical (untranslated) field name
    for( FIELD_T id : MANDATORY_FIELDS )
    {
        if( name.CmpNoCase( GetCanonicalFieldName( id ) ) == 0 )
        {
            fieldId = id;
            break;
        }
    }

    auto field = std::make_unique<SCH_FIELD>( aSymbol.get(), fieldId, name );
    field->SetPrivate( isPrivate );
    field->SetVisible( isVisible );

    token = NextTok();

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid property value" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    // Empty property values are valid.

    if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
        value = wxEmptyString;
    else
        value = FromUTF8();

    field->SetText( value );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_id:  // legacy token; ignore
            parseInt( "field ID" );
            NeedRIGHT();
            break;

        case T_at:
            field->SetPosition( parseXY( true ) );
            field->SetTextAngle( EDA_ANGLE( parseDouble( "text angle" ), DEGREES_T ) );
            NeedRIGHT();
            break;

        case T_hide:
            field->SetVisible( !parseBool() );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( field.get() ),
                           field->GetId() == FIELD_T::VALUE );
            break;

        case T_show_name:
        {
            bool show = parseMaybeAbsentBool( true );
            field->SetNameShown( show );
            break;
        }

        case T_do_not_autoplace:
        {
            bool doNotAutoplace = parseMaybeAbsentBool( true );
            field->SetCanAutoplace( !doNotAutoplace );
            break;
        }

        default:
            Expecting( "id, at, hide, show_name, do_not_autoplace, or effects" );
        }
    }

    SCH_FIELD* existingField;

    if( field->IsMandatory() )
    {
        existingField = aSymbol->GetField( field->GetId() );

        *existingField = *field;
        return existingField;
    }
    else if( name == "ki_keywords" )
    {
        // Not a SCH_FIELD object yet.
        aSymbol->SetKeyWords( value );
        return nullptr;
    }
    // In v7 and earlier the description field didn't exist and was a key/value
    else if( name == "ki_description" )
    {
        aSymbol->SetDescription( value );
        return nullptr;
    }
    else if( name == "ki_fp_filters" )
    {
        // Not a SCH_FIELD object yet.
        wxArrayString filters;
        wxStringTokenizer tokenizer( value, " \t\r\n", wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            wxString curr_token = UnescapeString( tokenizer.GetNextToken() );
            filters.Add( curr_token );
        }

        aSymbol->SetFPFilters( filters );
        return nullptr;
    }
    else if( name == "ki_locked" )
    {
        // This is a temporary SCH_FIELD object until interchangeable units are determined on
        // the fly.
        aSymbol->LockUnits( true );
        return nullptr;
    }
    else
    {
        // At this point, a user field is read.
        existingField = aSymbol->GetField( field->GetCanonicalName() );

#if 1   // Enable it to modify the name of the field to add if already existing
        // Disable it to skip the field having the same name as previous field
        if( existingField )
        {
            // We cannot handle 2 fields with the same name, so because the field name
            // is already in use, try to build a new name (oldname_x)
            wxString base_name = field->GetCanonicalName();

            // Arbitrary limit 10 attempts to find a new name
            for( int ii = 1; ii < 10 && existingField; ii++ )
            {
                wxString newname = base_name;
                newname << '_' << ii;

                existingField = aSymbol->GetField( newname );

                if( !existingField )    // the modified name is not found, use it
                    field->SetName( newname );
            }
        }
#endif
        if( !existingField )
        {
            aSymbol->AddDrawItem( field.get(), false );
            return field.release();
        }
        else
        {
            // We cannot handle 2 fields with the same name, so skip this one
            return nullptr;
        }
    }
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolArc()
{
    wxCHECK_MSG( CurTok() == T_arc, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as an arc." ) );

    T             token;
    VECTOR2I      startPoint( 1, 0 ); // Initialize to a non-degenerate arc just for safety
    VECTOR2I      midPoint( 1, 1 );
    VECTOR2I      endPoint( 0, 1 );
    bool          hasMidPoint = false;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;

    // Parameters for legacy format
    VECTOR2I      center( 0, 0 );
    EDA_ANGLE     startAngle = ANGLE_0;
    EDA_ANGLE     endAngle = ANGLE_90;
    bool          hasAngles = false;

    std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE );

    arc->SetUnit( m_unit );
    arc->SetBodyStyle( m_bodyStyle );

    token = NextTok();

    if( token == T_private )
    {
        arc->SetPrivate( true );
        token = NextTok();
    }

    for( ;  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:
            startPoint = parseXY( true );
            NeedRIGHT();
            break;

        case T_mid:
            midPoint = parseXY( true );
            NeedRIGHT();
            hasMidPoint = true;
            break;

        case T_end:
            endPoint = parseXY( true );
            NeedRIGHT();
            break;

        case T_radius:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_at:
                    center = parseXY( true );
                    NeedRIGHT();
                    break;

                case T_length:
                    parseInternalUnits( "radius length" );
                    NeedRIGHT();
                    break;

                case T_angles:
                {
                    startAngle = EDA_ANGLE( parseDouble( "start radius angle" ), DEGREES_T );
                    endAngle = EDA_ANGLE( parseDouble( "end radius angle" ), DEGREES_T );
                    startAngle.Normalize();
                    endAngle.Normalize();
                    NeedRIGHT();
                    hasAngles = true;
                    break;
                }

                default:
                    Expecting( "at, length, or angles" );
                }
            }

            break;

        case T_stroke:
            parseStroke( stroke );
            arc->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            arc->SetFillMode( fill.m_FillType );
            arc->SetFillColor( fill.m_Color );
            break;

        default:
            Expecting( "start, mid, end, radius, stroke, or fill" );
        }
    }

    if( hasMidPoint )
    {
        arc->SetArcGeometry( startPoint, midPoint, endPoint );

        if( m_requiredVersion <= 20230121 ) // Versions before 7.0
        {
            // Should be not required. Unfortunately it is needed because some bugs created
            // incorrect data after conversion of old libraries to the new arc format using
            // startPoint, midPoint, endPoint
            // Until now, in Eeschema the arc angle should be <= 180 deg.
            // If > 180 (bug...) we need to swap arc ends.
            // However arc angle == 180 deg can also create issues in some cases (plotters, hittest)
            // so also avoid arc == 180 deg
            EDA_ANGLE arc_start, arc_end, arc_angle;
            arc->CalcArcAngles( arc_start, arc_end );
            arc_angle = arc_end - arc_start;

            if( arc_angle > ANGLE_180 )
            {
                // Change arc to its complement (360deg - arc_angle)
                arc->SetStart( endPoint );
                arc->SetEnd( startPoint );
                VECTOR2I new_center =
                        CalcArcCenter( arc->GetStart(), arc->GetEnd(), ANGLE_360 - arc_angle );
                arc->SetCenter( new_center );
            }
            else if( arc_angle == ANGLE_180 )
            {
                VECTOR2I new_center = CalcArcCenter( arc->GetStart(), arc->GetEnd(),
                                                     EDA_ANGLE( 179.5, DEGREES_T ) );
                arc->SetCenter( new_center );
            }
        }
    }
    else if( hasAngles )
    {
        arc->SetCenter( center );
        /*
         * Older versions stored start-end with an implied winding, but the winding was different
         * between LibEdit and PCBNew.  Since we now use a common class (EDA_SHAPE) for both we
         * need to flip one of them.  LibEdit drew the short straw.
         */
        arc->SetStart( endPoint );
        arc->SetEnd( startPoint );

        // Like previously, 180 degrees arcs that create issues are just modified
        // to be < 180 degrees to do not break some other functions ( Draw, Plot, HitTest)
        EDA_ANGLE arc_start, arc_end, arc_angle;
        arc->CalcArcAngles( arc_start, arc_end );
        arc_angle = arc_end - arc_start;

        // The arc angle should be <= 180 deg in old libraries.
        // If > 180 we need to swap arc ends (the first choice was not good)
        if( arc_angle > ANGLE_180 )
        {
            arc->SetStart( startPoint );
            arc->SetEnd( endPoint );
        }
        else if( arc_angle == ANGLE_180 )
        {
            // Disabled (since the Y axis was reversed in library editor
            // and arcs having a 180 deg arc do not create issues
            // However keep it for info, just in case
            #if 0
            arc->SetStart( startPoint );    // not working with Y axis reversed
            arc->SetEnd( endPoint );        // not working with Y axis reversed
            // Useless now arcs >= 180 deg are well handled
            VECTOR2I new_center = CalcArcCenter( arc->GetStart(), arc->GetEnd(),
                                  EDA_ANGLE( 179.5, DEGREES_T ) );
            arc->SetCenter( new_center );
           #endif
        }
    }
    else
    {
        wxFAIL_MSG( "Setting arc without either midpoint or angles not implemented." );
    }

    return arc.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolBezier()
{
    wxCHECK_MSG( CurTok() == T_bezier, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bezier." ) );

    T             token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;

    std::unique_ptr<SCH_SHAPE> bezier = std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER, LAYER_DEVICE );

    bezier->SetUnit( m_unit );
    bezier->SetBodyStyle( m_bodyStyle );

    token = NextTok();

    if( token == T_private )
    {
        bezier->SetPrivate( true );
        token = NextTok();
    }

    for( ; token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_pts:
        {
            int ii = 0;

            for( token = NextTok();  token != T_RIGHT;  token = NextTok(), ++ii )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_xy )
                    Expecting( "xy" );

                switch( ii )
                {
                case 0:  bezier->SetStart( parseXY( true ) );    break;
                case 1:  bezier->SetBezierC1( parseXY( true ) ); break;
                case 2:  bezier->SetBezierC2( parseXY( true ) ); break;
                case 3:  bezier->SetEnd( parseXY( true ) );      break;
                default: Unexpected( "control point" );          break;
                }

                NeedRIGHT();
            }
        }
            break;

        case T_stroke:
            parseStroke( stroke );
            bezier->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            bezier->SetFillMode( fill.m_FillType );
            bezier->SetFillColor( fill.m_Color );
            break;

        default:
            Expecting( "pts, stroke, or fill" );
        }
    }

    bezier->RebuildBezierToSegmentsPointsList( m_maxError );

    return bezier.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolCircle()
{
    wxCHECK_MSG( CurTok() == T_circle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a circle." ) );

    T             token;
    VECTOR2I      center( 0, 0 );
    int           radius = 1;     // defaulting to 0 could result in troublesome math....
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;

    std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE );

    circle->SetUnit( m_unit );
    circle->SetBodyStyle( m_bodyStyle );

    token = NextTok();

    if( token == T_private )
    {
        circle->SetPrivate( true );
        token = NextTok();
    }

    for( ; token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_center:
            center = parseXY( true );
            NeedRIGHT();
            break;

        case T_radius:
            radius = parseInternalUnits( "radius length" );
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            circle->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            circle->SetFillMode( fill.m_FillType );
            circle->SetFillColor( fill.m_Color );
            break;

        default:
            Expecting( "center, radius, stroke, or fill" );
        }
    }

    circle->SetCenter( center );
    circle->SetEnd( VECTOR2I( center.x + radius, center.y ) );

    return circle.release();
}


SCH_PIN* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolPin()
{
    auto parseType =
            [&]( T token ) -> ELECTRICAL_PINTYPE
            {
                switch( token )
                {
                case T_input:          return ELECTRICAL_PINTYPE::PT_INPUT;
                case T_output:         return ELECTRICAL_PINTYPE::PT_OUTPUT;
                case T_bidirectional:  return ELECTRICAL_PINTYPE::PT_BIDI;
                case T_tri_state:      return ELECTRICAL_PINTYPE::PT_TRISTATE;
                case T_passive:        return ELECTRICAL_PINTYPE::PT_PASSIVE;
                case T_unspecified:    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
                case T_power_in:       return ELECTRICAL_PINTYPE::PT_POWER_IN;
                case T_power_out:      return ELECTRICAL_PINTYPE::PT_POWER_OUT;
                case T_open_collector: return ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR;
                case T_open_emitter:   return ELECTRICAL_PINTYPE::PT_OPENEMITTER;
                case T_unconnected:
                case T_no_connect:     return ELECTRICAL_PINTYPE::PT_NC;
                case T_free:           return ELECTRICAL_PINTYPE::PT_NIC;

                default:
                    Expecting( "input, output, bidirectional, tri_state, passive, unspecified, "
                               "power_in, power_out, open_collector, open_emitter, free or "
                               "no_connect" );
                    return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
                }
            };

    auto parseShape =
            [&]( T token ) -> GRAPHIC_PINSHAPE
            {
                switch( token )
                {
                case T_line:            return GRAPHIC_PINSHAPE::LINE;
                case T_inverted:        return GRAPHIC_PINSHAPE::INVERTED;
                case T_clock:           return GRAPHIC_PINSHAPE::CLOCK;
                case T_inverted_clock:  return GRAPHIC_PINSHAPE::INVERTED_CLOCK;
                case T_input_low:       return GRAPHIC_PINSHAPE::INPUT_LOW;
                case T_clock_low:       return GRAPHIC_PINSHAPE::CLOCK_LOW;
                case T_output_low:      return GRAPHIC_PINSHAPE::OUTPUT_LOW;
                case T_edge_clock_high: return GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK;
                case T_non_logic:       return GRAPHIC_PINSHAPE::NONLOGIC;

                default:
                    Expecting( "line, inverted, clock, inverted_clock, input_low, clock_low, "
                               "output_low, edge_clock_high, non_logic" );
                    return GRAPHIC_PINSHAPE::LINE;
                }
            };

    wxCHECK_MSG( CurTok() == T_pin, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a pin token." ) );

    T                        token;
    std::unique_ptr<SCH_PIN> pin = std::make_unique<SCH_PIN>( nullptr );

    pin->SetUnit( m_unit );
    pin->SetBodyStyle( m_bodyStyle );

    // Pin electrical type.
    token = NextTok();
    pin->SetType( parseType( token ) );

    // Pin shape.
    token = NextTok();
    pin->SetShape( parseShape( token ) );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        // Pre-2024104 format (bare 'hide' keyword)
        if( token == T_hide )
        {
            pin->SetVisible( false );
            continue;
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            pin->SetPosition( parseXY( true ) );

            switch( parseInt( "pin orientation" ) )
            {
            case 0:   pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT ); break;
            case 90:  pin->SetOrientation( PIN_ORIENTATION::PIN_UP );    break;
            case 180: pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT );  break;
            case 270: pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );  break;
            default:  Expecting( "0, 90, 180, or 270" );
            }

            NeedRIGHT();
            break;

        case T_length:
            pin->SetLength( parseInternalUnits( "pin length" ) );
            NeedRIGHT();
            break;

        case T_hide:
            pin->SetVisible( !parseBool() );
            NeedRIGHT();
            break;

        case T_name:
            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid pin name" ), CurSource(), CurLine(), CurLineNumber(),
                                   CurOffset() );
            }

            if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                pin->SetName( wxEmptyString );
            else if( m_requiredVersion < 20210606 )
                pin->SetName( ConvertToNewOverbarNotation( FromUTF8() ) );
            else
                pin->SetName( FromUTF8() );

            token = NextTok();

            if( token != T_RIGHT )
            {
                token = NextTok();

                if( token == T_effects )
                {
                    // The EDA_TEXT font effects formatting is used so use and EDA_TEXT object
                    // so duplicate parsing is not required.
                    EDA_TEXT text( schIUScale );

                    parseEDA_TEXT( &text, true, false );
                    pin->SetNameTextSize( text.GetTextHeight() );
                    NeedRIGHT();
                }
                else
                {
                    Expecting( "effects" );
                }
            }

            break;

        case T_number:
            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid pin number" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );
            }

            if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                pin->SetNumber( wxEmptyString );
            else if( m_requiredVersion < 20210606 )
                pin->SetNumber( ConvertToNewOverbarNotation( FromUTF8() ) );
            else
                pin->SetNumber( FromUTF8() );

            token = NextTok();

            if( token != T_RIGHT )
            {
                token = NextTok();

                if( token == T_effects )
                {
                    // The EDA_TEXT font effects formatting is used so use and EDA_TEXT object
                    // so duplicate parsing is not required.
                    EDA_TEXT text( schIUScale );

                    parseEDA_TEXT( &text, false, false );
                    pin->SetNumberTextSize( text.GetTextHeight() );
                    NeedRIGHT();
                }
                else
                {
                    Expecting( "effects" );
                }
            }

            break;

        case T_alternate:
        {
            SCH_PIN::ALT alt;

            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid alternate pin name" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );
            }

            alt.m_Name = FromUTF8();

            token = NextTok();
            alt.m_Type = parseType( token );

            token = NextTok();
            alt.m_Shape = parseShape( token );

            pin->GetAlternates()[ alt.m_Name ] = alt;

            NeedRIGHT();
            break;
        }

        default:
            Expecting( "at, name, number, hide, length, or alternate" );
        }
    }

    return pin.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolPolyLine()
{
    wxCHECK_MSG( CurTok() == T_polyline, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a poly." ) );

    T token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS fill;
    std::unique_ptr<SCH_SHAPE> poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE );

    poly->SetUnit( m_unit );
    poly->SetBodyStyle( m_bodyStyle );

    token = NextTok();

    if( token == T_private )
    {
        poly->SetPrivate( true );
        token = NextTok();
    }

    for( ;  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_pts:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_xy )
                    Expecting( "xy" );

                poly->AddPoint( parseXY( true ) );

                NeedRIGHT();
            }

            break;

        case T_stroke:
            parseStroke( stroke );
            poly->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            poly->SetFillMode( fill.m_FillType );
            poly->SetFillColor( fill.m_Color );
            break;

        default:
            Expecting( "pts, stroke, or fill" );
        }
    }

    return poly.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolRectangle()
{
    wxCHECK_MSG( CurTok() == T_rectangle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a rectangle." ) );

    T token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS fill;
    auto rectangle = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );

    rectangle->SetUnit( m_unit );
    rectangle->SetBodyStyle( m_bodyStyle );

    token = NextTok();

    if( token == T_private )
    {
        rectangle->SetPrivate( true );
        token = NextTok();
    }

    for( ; token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:
            rectangle->SetPosition( parseXY( true ) );
            NeedRIGHT();
            break;

        case T_end:
            rectangle->SetEnd( parseXY( true ) );
            NeedRIGHT();
            break;

        case T_radius:
            rectangle->SetCornerRadius( parseDouble( "corner radius" ) * schIUScale.IU_PER_MM );
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            rectangle->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            rectangle->SetFillMode( fill.m_FillType );
            rectangle->SetFillColor( fill.m_Color );
            break;

        default:
            Expecting( "start, end, stroke, or fill" );
        }
    }

    return rectangle.release();
}


SCH_ITEM* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolText()
{
    wxCHECK_MSG( CurTok() == T_text, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a text token." ) );

    T token;
    std::unique_ptr<SCH_TEXT> text = std::make_unique<SCH_TEXT>();

    text->SetLayer( LAYER_DEVICE );
    text->SetUnit( m_unit );
    text->SetBodyStyle( m_bodyStyle );
    token = NextTok();

    if( token == T_private )
    {
        text->SetPrivate( true );
        token = NextTok();
    }

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid text string" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    text->SetText( FromUTF8() );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            text->SetPosition( parseXY( true ) );
            // Yes, LIB_TEXT is really decidegrees even though all the others are degrees. :(
            text->SetTextAngle( EDA_ANGLE( parseDouble( "text angle" ), TENTHS_OF_A_DEGREE_T ) );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( text.get(), true );
            break;

        default:
            Expecting( "at or effects" );
        }
    }

    // Convert hidden symbol text (which is no longer supported) into a hidden field
    if( !text->IsVisible() )
        return new SCH_FIELD( text.get(), FIELD_T::USER );

    return text.release();
}


SCH_TEXTBOX* SCH_IO_KICAD_SEXPR_PARSER::parseSymbolTextBox()
{
    wxCHECK_MSG( CurTok() == T_text_box, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a text box." ) );

    T             token;
    VECTOR2I      pos;
    VECTOR2I      end;
    VECTOR2I      size;
    int           left;
    int           top;
    int           right;
    int           bottom;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    bool          foundEnd = false;
    bool          foundSize = false;
    bool          foundMargins = false;

    std::unique_ptr<SCH_TEXTBOX> textBox = std::make_unique<SCH_TEXTBOX>( LAYER_DEVICE );

    textBox->SetUnit( m_unit );
    textBox->SetBodyStyle( m_bodyStyle );
    token = NextTok();

    if( token == T_private )
    {
        textBox->SetPrivate( true );
        token = NextTok();
    }

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid text string" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    textBox->SetText( FromUTF8() );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:       // Legacy token during 6.99 development; fails to handle angle
            pos = parseXY( true );
            NeedRIGHT();
            break;

        case T_end:         // Legacy token during 6.99 development; fails to handle angle
            end = parseXY( true );
            foundEnd = true;
            NeedRIGHT();
            break;

        case T_at:
            pos = parseXY( true );
            textBox->SetTextAngle( EDA_ANGLE( parseDouble( "textbox angle" ), DEGREES_T ) );
            NeedRIGHT();
            break;

        case T_size:
            size = parseXY( true );
            foundSize = true;
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            textBox->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            textBox->SetFillMode( fill.m_FillType );
            textBox->SetFillColor( fill.m_Color );
            break;

        case T_margins:
            parseMargins( left, top, right, bottom );
            textBox->SetMarginLeft( left );
            textBox->SetMarginTop( top );
            textBox->SetMarginRight( right );
            textBox->SetMarginBottom( bottom );
            foundMargins = true;
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( textBox.get() ), false );
            break;

        default:
            Expecting( "at, size, stroke, fill or effects" );
        }
    }

    textBox->SetPosition( pos );

    if( foundEnd )
        textBox->SetEnd( end );
    else if( foundSize )
        textBox->SetEnd( pos + size );
    else
        Expecting( "size" );

    if( !foundMargins )
    {
        int margin = textBox->GetLegacyTextMargin();
        textBox->SetMarginLeft( margin );
        textBox->SetMarginTop( margin );
        textBox->SetMarginRight( margin );
        textBox->SetMarginBottom( margin );
    }

    return textBox.release();
}


void SCH_IO_KICAD_SEXPR_PARSER::parsePAGE_INFO( PAGE_INFO& aPageInfo )
{
    wxCHECK_RET( ( CurTok() == T_page && m_requiredVersion <= 20200506 ) || CurTok() == T_paper,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a PAGE_INFO." ) );

    T token;

    NeedSYMBOL();

    wxString pageType = FromUTF8();

    if( !aPageInfo.SetType( pageType ) )
    {
        THROW_PARSE_ERROR( _( "Invalid page type" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    if( aPageInfo.GetType() == PAGE_SIZE_TYPE::User )
    {
        double width = parseDouble( "width" );

        // Perform some controls to avoid crashes if the size is edited by hands
        if( width < MIN_PAGE_SIZE_MM )
            width = MIN_PAGE_SIZE_MM;
        else if( width > MAX_PAGE_SIZE_EESCHEMA_MM )
            width = MAX_PAGE_SIZE_EESCHEMA_MM;

        double height = parseDouble( "height" );

        if( height < MIN_PAGE_SIZE_MM )
            height = MIN_PAGE_SIZE_MM;
        else if( height > MAX_PAGE_SIZE_EESCHEMA_MM )
            height = MAX_PAGE_SIZE_EESCHEMA_MM;

        aPageInfo.SetWidthMM( width );
        aPageInfo.SetHeightMM( height );
    }

    token = NextTok();

    if( token == T_portrait )
    {
        aPageInfo.SetPortrait( true );
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        Expecting( "portrait" );
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parseTITLE_BLOCK( TITLE_BLOCK& aTitleBlock )
{
    wxCHECK_RET( CurTok() == T_title_block,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as a TITLE_BLOCK." );

    T token;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_title:
            NextTok();
            aTitleBlock.SetTitle( FromUTF8() );
            break;

        case T_date:
            NextTok();
            aTitleBlock.SetDate( FromUTF8() );
            break;

        case T_rev:
            NextTok();
            aTitleBlock.SetRevision( FromUTF8() );
            break;

        case T_company:
            NextTok();
            aTitleBlock.SetCompany( FromUTF8() );
            break;

        case T_comment:
        {
            int commentNumber = parseInt( "comment" );

            switch( commentNumber )
            {
            case 1:
                NextTok();
                aTitleBlock.SetComment( 0, FromUTF8() );
                break;

            case 2:
                NextTok();
                aTitleBlock.SetComment( 1, FromUTF8() );
                break;

            case 3:
                NextTok();
                aTitleBlock.SetComment( 2, FromUTF8() );
                break;

            case 4:
                NextTok();
                aTitleBlock.SetComment( 3, FromUTF8() );
                break;

            case 5:
                NextTok();
                aTitleBlock.SetComment( 4, FromUTF8() );
                break;

            case 6:
                NextTok();
                aTitleBlock.SetComment( 5, FromUTF8() );
                break;

            case 7:
                NextTok();
                aTitleBlock.SetComment( 6, FromUTF8() );
                break;

            case 8:
                NextTok();
                aTitleBlock.SetComment( 7, FromUTF8() );
                break;

            case 9:
                NextTok();
                aTitleBlock.SetComment( 8, FromUTF8() );
                break;

            default:
                THROW_PARSE_ERROR( _( "Invalid title block comment number" ), CurSource(),
                                   CurLine(), CurLineNumber(), CurOffset() );
            }

            break;
        }

        default:
            Expecting( "title, date, rev, company, or comment" );
        }

        NeedRIGHT();
    }
}


SCH_FIELD* SCH_IO_KICAD_SEXPR_PARSER::parseSchField( SCH_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_property, nullptr,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as a property token." );

    bool is_private = false;

    T token = NextTok();

    if( token == T_private )
    {
        is_private = true;
        token = NextTok();
    }

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid property name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    wxString name = FromUTF8();

    if( name.IsEmpty() )
    {
        THROW_PARSE_ERROR( _( "Empty property name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    token = NextTok();

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid property value" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    // Empty property values are valid.
    wxString value;

    if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
        value = wxEmptyString;
    else
        value = FromUTF8();

    FIELD_T fieldId = FIELD_T::USER;

    // Correctly set the ID based on canonical (untranslated) field name
    if( aParent->Type() == SCH_SYMBOL_T )
    {
        for( FIELD_T id : MANDATORY_FIELDS )
        {
            if( name.CmpNoCase( GetCanonicalFieldName( id ) ) == 0 )
            {
                fieldId = id;
                break;
            }
        }
    }
    else if( aParent->Type() == SCH_SHEET_T )
    {
        fieldId = FIELD_T::SHEET_USER;  // This is the default id for user fields

        for( FIELD_T id : SHEET_MANDATORY_FIELDS )
        {
            if( name.CmpNoCase( GetCanonicalFieldName( id ) ) == 0 )
            {
                fieldId = id;
                break;
            }
        }

        // Legacy support for old field names
        if( name.CmpNoCase( wxT( "Sheet name" ) ) == 0 )
            fieldId = FIELD_T::SHEET_NAME;
        else if( name.CmpNoCase( wxT( "Sheet file" ) ) == 0 )
            fieldId = FIELD_T::SHEET_FILENAME;
    }
    else if( aParent->Type() == SCH_GLOBAL_LABEL_T )
    {
        for( FIELD_T id : GLOBALLABEL_MANDATORY_FIELDS )
        {
            if( name.CmpNoCase( GetCanonicalFieldName( id ) ) == 0 )
            {
                fieldId = id;
                break;
            }
        }

        // Legacy support for old field names
        if( name.CmpNoCase( wxT( "Intersheet References" ) ) == 0 )
            fieldId = FIELD_T::INTERSHEET_REFS;
    }

    std::unique_ptr<SCH_FIELD> field = std::make_unique<SCH_FIELD>( aParent, fieldId, name );
    field->SetText( value );

    if( fieldId == FIELD_T::USER )
        field->SetPrivate( is_private );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_id:  // legacy token; ignore
            parseInt( "field ID" );
            NeedRIGHT();
            break;

        case T_at:
            field->SetPosition( parseXY() );
            field->SetTextAngle( EDA_ANGLE( parseDouble( "text angle" ), DEGREES_T ) );
            NeedRIGHT();
            break;

        case T_hide:
            field->SetVisible( !parseBool() );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( field.get() ),
                           field->GetId() == FIELD_T::VALUE );
            break;

        case T_show_name:
        {
            bool show = parseMaybeAbsentBool( true );
            field->SetNameShown( show );
            break;
        }

        case T_do_not_autoplace:
        {
            bool doNotAutoplace = parseMaybeAbsentBool( true );
            field->SetCanAutoplace( !doNotAutoplace );
            break;
        }

        default:
            Expecting( "id, at, hide, show_name, do_not_autoplace or effects" );
        }
    }

    return field.release();
}


SCH_SHEET_PIN* SCH_IO_KICAD_SEXPR_PARSER::parseSchSheetPin( SCH_SHEET* aSheet )
{
    wxCHECK_MSG( aSheet != nullptr, nullptr, "" );
    wxCHECK_MSG( CurTok() == T_pin, nullptr,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as a sheet pin token." );

    T token = NextTok();

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid sheet pin name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    wxString name = FromUTF8();

    if( name.IsEmpty() )
    {
        THROW_PARSE_ERROR( _( "Empty sheet pin name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    auto sheetPin = std::make_unique<SCH_SHEET_PIN>( aSheet, VECTOR2I( 0, 0 ), name );

    token = NextTok();

    switch( token )
    {
    case T_input:          sheetPin->SetShape( LABEL_FLAG_SHAPE::L_INPUT );        break;
    case T_output:         sheetPin->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );       break;
    case T_bidirectional:  sheetPin->SetShape( LABEL_FLAG_SHAPE::L_BIDI );         break;
    case T_tri_state:      sheetPin->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE );     break;
    case T_passive:        sheetPin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );  break;
    default:
        Expecting( "input, output, bidirectional, tri_state, or passive" );
    }

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
        {
            sheetPin->SetPosition( parseXY() );

            double angle = parseDouble( "sheet pin angle (side)" );

            if( angle == 0.0 )
                sheetPin->SetSide( SHEET_SIDE::RIGHT );
            else if( angle == 90.0 )
                sheetPin->SetSide( SHEET_SIDE::TOP );
            else if( angle == 180.0 )
                sheetPin->SetSide( SHEET_SIDE::LEFT );
            else if( angle == 270.0 )
                sheetPin->SetSide( SHEET_SIDE::BOTTOM );
            else
                Expecting( "0, 90, 180, or 270" );

            NeedRIGHT();
            break;
        }

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( sheetPin.get() ), true );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( sheetPin->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at, uuid or effects" );
        }
    }

    return sheetPin.release();
}


void SCH_IO_KICAD_SEXPR_PARSER::parseSchSheetInstances( SCH_SHEET* aRootSheet, SCH_SCREEN* aScreen )
{
    wxCHECK_RET( CurTok() == T_sheet_instances,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as an instances token." );
    wxCHECK( aScreen, /* void */ );

    T token;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_path:
        {
            NeedSYMBOL();

            SCH_SHEET_INSTANCE instance;

            instance.m_Path = KIID_PATH( FromUTF8() );

            if( ( !m_appending && aRootSheet->GetScreen() == aScreen ) &&
                ( aScreen->GetFileFormatVersionAtLoad() < 20221002 ) )
                instance.m_Path.insert( instance.m_Path.begin(), m_rootUuid );

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                std::vector<wxString> whitespaces = { wxT( "\r" ), wxT( "\n" ), wxT( "\t" ),
                                                      wxT( " " ) };

                size_t numReplacements = 0;

                switch( token )
                {
                case T_page:
                    NeedSYMBOL();
                    instance.m_PageNumber = FromUTF8();

                    // Empty page numbers are not permitted
                    if( instance.m_PageNumber.IsEmpty() )
                    {
                        // Use hash character instead
                        instance.m_PageNumber = wxT( "#" );
                        numReplacements++;
                    }
                    else
                    {
                        // Whitespaces are not permitted
                        for( const wxString& ch : whitespaces )
                            numReplacements += instance.m_PageNumber.Replace( ch, wxEmptyString );

                    }

                    // Set the file as modified so the user can be warned.
                    if( numReplacements > 0 )
                        aScreen->SetContentModified();

                    NeedRIGHT();
                    break;

                default:
                    Expecting( "path or page" );
                }
            }

            if( ( aScreen->GetFileFormatVersionAtLoad() >= 20221110 )
              && ( instance.m_Path.empty() ) )
            {
                SCH_SHEET_PATH rootSheetPath;

                rootSheetPath.push_back( aRootSheet );
                rootSheetPath.SetPageNumber( instance.m_PageNumber );
            }
            else
            {
                aScreen->m_sheetInstances.emplace_back( instance );
            }

            break;
        }

        default:
            Expecting( "path" );
        }
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parseSchSymbolInstances( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( CurTok() == T_symbol_instances,
                 "Cannot parse " + GetTokenString( CurTok() ) + " as an instances token." );
    wxCHECK( aScreen, /* void */ );
    wxCHECK( m_rootUuid != NilUuid(), /* void */ );

    T token;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_path:
        {
            NeedSYMBOL();

            SCH_SYMBOL_INSTANCE instance;

            instance.m_Path = KIID_PATH( FromUTF8() );

            if( !m_appending )
                instance.m_Path.insert( instance.m_Path.begin(), m_rootUuid );

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_reference:
                    NeedSYMBOL();
                    instance.m_Reference = FromUTF8();
                    NeedRIGHT();
                    break;

                case T_unit:
                    instance.m_Unit = parseInt( "symbol unit" );
                    NeedRIGHT();
                    break;

                case T_value:
                    NeedSYMBOL();

                    if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                        instance.m_Value = wxEmptyString;
                    else
                        instance.m_Value = FromUTF8();

                    NeedRIGHT();
                    break;

                case T_footprint:
                    NeedSYMBOL();

                    if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                        instance.m_Footprint = wxEmptyString;
                    else
                        instance.m_Footprint = FromUTF8();

                    NeedRIGHT();
                    break;

                default:
                    Expecting( "path, unit, value or footprint" );
                }
            }

            aScreen->m_symbolInstances.emplace_back( instance );
            break;
        }

        default:
            Expecting( "path" );
        }
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::ParseSchematic( SCH_SHEET* aSheet, bool aIsCopyableOnly,
                                                int aFileVersion )
{
    wxCHECK( aSheet != nullptr, /* void */ );

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen != nullptr, /* void */ );

    if( SCHEMATIC* schematic = dynamic_cast<SCHEMATIC*>( screen->GetParent() ) )
        m_maxError = schematic->Settings().m_MaxError;

    if( aIsCopyableOnly )
        m_requiredVersion = aFileVersion;

    bool fileHasUuid = false;

    auto checkVersion =
            [&]()
            {
                if( m_requiredVersion > SEXPR_SCHEMATIC_FILE_VERSION )
                {
                    throw FUTURE_FORMAT_ERROR( fmt::format( "{}", m_requiredVersion ),
                                               m_generatorVersion );
                }
            };

    T token;

    if( !aIsCopyableOnly )
    {
        NeedLEFT();
        NextTok();

        if( CurTok() != T_kicad_sch )
            Expecting( "kicad_sch" );

        parseHeader( T_kicad_sch, SEXPR_SCHEMATIC_FILE_VERSION );

        // Prior to this, bar was a valid string char for unquoted strings.
        SetKnowsBar( m_requiredVersion >= 20240620 );

        // Prior to schematic file version 20210406, schematics did not have UUIDs so we need
        // to generate one for the root schematic for instance paths.
        if( m_requiredVersion < 20210406 )
            m_rootUuid = screen->GetUuid();

        // Prior to version 20231120, generator_version was not present, so check the date here
        if( m_requiredVersion < 20231120 )
            checkVersion();
    }

    screen->SetFileFormatVersionAtLoad( m_requiredVersion );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( aIsCopyableOnly && token == T_EOF )
            break;

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        checkpoint();

        if( !aIsCopyableOnly && token == T_page && m_requiredVersion <= 20200506 )
            token = T_paper;

        switch( token )
        {
        case T_group:
            parseGroup();
            break;

        case T_generator:
            // (generator "genname"); we don't care about it at the moment.
            NeedSYMBOL();
            NeedRIGHT();
            break;

        case T_host:
        {
            // (host eeschema ["5.99"]); old version of generator token
            NeedSYMBOL();

            // Really old versions also included a host version
            if( m_requiredVersion < 20200827 )
                NeedSYMBOL();

            NeedRIGHT();
            break;
        }

        case T_generator_version:
            NextTok();
            m_generatorVersion = FromUTF8();
            NeedRIGHT();
            checkVersion();
            break;

        case T_uuid:
            NeedSYMBOL();
            screen->m_uuid = parseKIID();

            // Set the root sheet UUID with the schematic file UUID.  Root sheets are virtual
            // and always get a new UUID so this prevents file churn now that the root UUID
            // is saved in the symbol instance path.
            if( aSheet == m_rootSheet )
            {
                const_cast<KIID&>( aSheet->m_Uuid ) = screen->GetUuid();
                m_rootUuid = screen->GetUuid();
                fileHasUuid = true;
            }

            NeedRIGHT();
            break;

        case T_paper:
        {
            if( aIsCopyableOnly )
                Unexpected( T_paper );

            PAGE_INFO pageInfo;
            parsePAGE_INFO( pageInfo );
            screen->SetPageSettings( pageInfo );
            break;
        }

        case T_page:
        {
            if( aIsCopyableOnly )
                Unexpected( T_page );

            // Only saved for top-level sniffing in Kicad Manager frame and other external
            // tool usage with flat hierarchies
            NeedSYMBOLorNUMBER();
            NeedSYMBOLorNUMBER();
            NeedRIGHT();
            break;
        }

        case T_title_block:
        {
            if( aIsCopyableOnly )
                Unexpected( T_title_block );

            TITLE_BLOCK tb;
            parseTITLE_BLOCK( tb );
            screen->SetTitleBlock( tb );
            break;
        }

        case T_lib_symbols:
        {
            // Dummy map.  No derived symbols are allowed in the library cache.
            LIB_SYMBOL_MAP symbolLibMap;
            LIB_SYMBOL* symbol;

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_symbol:
                    symbol = parseLibSymbol( symbolLibMap );
                    screen->AddLibSymbol( symbol );
                    break;

                default:
                    Expecting( "symbol" );
                }
            }

            break;
        }

        case T_symbol:
            screen->Append( static_cast<SCH_ITEM*>( parseSchematicSymbol() ) );
            break;

        case T_image:
            screen->Append( static_cast<SCH_ITEM*>( parseImage() ) );
            break;

        case T_sheet:
        {
            SCH_SHEET* sheet = parseSheet();

            // Set the parent to aSheet.  This effectively creates a method to find
            // the root sheet from any sheet so a pointer to the root sheet does not
            // need to be stored globally.  Note: this is not the same as a hierarchy.
            // Complex hierarchies can have multiple copies of a sheet.  This only
            // provides a simple tree to find the root sheet.
            sheet->SetParent( aSheet );
            screen->Append( sheet );
            break;
        }

        case T_junction:
            screen->Append( parseJunction() );
            break;

        case T_no_connect:
            screen->Append( parseNoConnect() );
            break;

        case T_bus_entry:
            screen->Append( parseBusEntry() );
            break;

        case T_polyline:
        {
            // polyline keyword is used in eeschema both for SCH_SHAPE and SCH_LINE items.
            // In symbols it describes a polygon, having n corners and can be filled
            // In schematic it describes a line (with no fill descr), but could be extended to a
            // polygon (for instance when importing files) because the schematic handles all
            // SCH_SHAPE.

            // parseSchPolyLine() returns always a SCH_SHAPE, io convert it to a simple SCH_LINE
            // For compatibility reasons, keep SCH_SHAPE for a polygon and convert to SCH_LINE
            // when the item has only 2 corners, similar to a SCH_LINE
            SCH_SHAPE* poly = parseSchPolyLine();

            if( poly->GetPointCount() > 2 )
            {
                screen->Append( poly );
            }
            else
            {
                // For SCH_SHAPE having only 2 points, this is a "old" SCH_LINE entity.
                // So convert the SCH_SHAPE to a simple SCH_LINE
                SCH_LINE* line = new SCH_LINE( VECTOR2I(), LAYER_NOTES );
                SHAPE_LINE_CHAIN& outline = poly->GetPolyShape().Outline(0);
                line->SetStartPoint( outline.CPoint(0) );
                line->SetEndPoint( outline.CPoint(1) );
                line->SetStroke( poly->GetStroke() );
                const_cast<KIID&>( line->m_Uuid ) = poly->m_Uuid;

                screen->Append( line );

                delete poly;
            }
        }
            break;

        case T_bus:
        case T_wire:
            screen->Append( parseLine() );
            break;

        case T_arc:
            screen->Append( parseSchArc() );
            break;

        case T_circle:
            screen->Append( parseSchCircle() );
            break;

        case T_rectangle:
            screen->Append( parseSchRectangle() );
            break;

        case T_bezier:
            screen->Append( parseSchBezier() );
            break;

        case T_rule_area:
            screen->Append( parseSchRuleArea() );
            break;

        case T_netclass_flag:       // present only during early development of 7.0
            KI_FALLTHROUGH;

        case T_text:
        case T_label:
        case T_global_label:
        case T_hierarchical_label:
        case T_directive_label:
            screen->Append( parseSchText() );
            break;

        case T_text_box:
            screen->Append( parseSchTextBox() );
            break;

        case T_table:
            screen->Append( parseSchTable() );
            break;

        case T_sheet_instances:
            parseSchSheetInstances( aSheet, screen );
            break;

        case T_symbol_instances:
            parseSchSymbolInstances( screen );
            break;

        case T_bus_alias:
            if( aIsCopyableOnly )
                Unexpected( T_bus_alias );

            parseBusAlias( screen );
            break;

        case T_embedded_fonts:
        {
            SCHEMATIC* schematic = screen->Schematic();

            if( !schematic )
                THROW_PARSE_ERROR( _( "No schematic object" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );

            schematic->GetEmbeddedFiles()->SetAreFontsEmbedded( parseBool() );
            NeedRIGHT();
            break;
        }

        case T_embedded_files:
        {
            SCHEMATIC* schematic = screen->Schematic();

            if( !schematic )
                THROW_PARSE_ERROR( _( "No schematic object" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );

            EMBEDDED_FILES_PARSER embeddedFilesParser( reader );
            embeddedFilesParser.SyncLineReaderWith( *this );

            try
            {
                embeddedFilesParser.ParseEmbedded( schematic->GetEmbeddedFiles() );
            }
            catch( const PARSE_ERROR& e )
            {
                m_parseWarnings.push_back( e.What() );
            }

            SyncLineReaderWith( embeddedFilesParser );
            break;
        }


        default:
            Expecting( "bitmap, bus, bus_alias, bus_entry, class_label, embedded_files, global_label, "
                       "hierarchical_label, junction, label, line, no_connect, page, paper, rule_area, "
                       "sheet, symbol, symbol_instances, text, title_block" );
        }
    }

    // Older s-expression schematics may not have a UUID so use the one automatically generated
    // as the virtual root sheet UUID.
    if( ( aSheet == m_rootSheet ) && !fileHasUuid )
    {
        const_cast<KIID&>( aSheet->m_Uuid ) = screen->GetUuid();
        m_rootUuid = screen->GetUuid();
    }

    screen->UpdateLocalLibSymbolLinks();
    screen->FixupEmbeddedData();

    resolveGroups( screen );

    SCHEMATIC* schematic = screen->Schematic();

    if( !schematic )
        THROW_PARSE_ERROR( _( "No schematic object" ), CurSource(), CurLine(),
                            CurLineNumber(), CurOffset() );

    // When loading the schematic, take a moment to cache the fonts so that the font
    // picker can show the embedded fonts immediately.
    std::vector<std::string> fontNames;
    Fontconfig()->ListFonts( fontNames, std::string( Pgm().GetLanguageTag().utf8_str() ),
                             schematic->GetEmbeddedFiles()->GetFontFiles(), true );

    if( m_requiredVersion < 20200828 )
        screen->SetLegacySymbolInstanceData();
}


SCH_SYMBOL* SCH_IO_KICAD_SEXPR_PARSER::parseSchematicSymbol()
{
    wxCHECK_MSG( CurTok() == T_symbol, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a symbol." ) );

    T token;
    wxString libName;
    SCH_FIELD* field;
    std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();
    TRANSFORM transform;
    std::set<int> fieldIDsRead;

    // We'll reset this if we find a fields_autoplaced token
    symbol->SetFieldsAutoplaced( AUTOPLACE_NONE );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_lib_name:
        {
            LIB_ID libId;

            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid symbol library name" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );
            }

            libName = FromUTF8();
            // Some symbol LIB_IDs or lib_name have the '/' character escaped which can break
            // symbol links.  The '/' character is no longer an illegal LIB_ID character so
            // it doesn't need to be escaped.
            libName.Replace( "{slash}", "/" );

            NeedRIGHT();
            break;
        }

        case T_lib_id:
        {
            token = NextTok();

            if( !IsSymbol( token ) && token != T_NUMBER )
                Expecting( "symbol|number" );

            LIB_ID libId;
            wxString name = FromUTF8();
            // Some symbol LIB_IDs have the '/' character escaped which can break
            // symbol links.  The '/' character is no longer an illegal LIB_ID character so
            // it doesn't need to be escaped.
            name.Replace( "{slash}", "/" );

            int bad_pos = libId.Parse( name );

            if( bad_pos >= 0 )
            {
                if( static_cast<int>( name.size() ) > bad_pos )
                {
                    wxString msg = wxString::Format( _( "Symbol %s contains invalid character '%c'" ),
                                                     name,
                                                     name[bad_pos] );

                    THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
                }

                THROW_PARSE_ERROR( _( "Invalid symbol library ID" ), CurSource(), CurLine(),
                                   CurLineNumber(), CurOffset() );
            }

            symbol->SetLibId( libId );
            NeedRIGHT();
            break;
        }

        case T_at:
            symbol->SetPosition( parseXY() );

            switch( static_cast<int>( parseDouble( "symbol orientation" ) ) )
            {
            case 0:    transform = TRANSFORM();                 break;
            case 90:   transform = TRANSFORM( 0, 1, -1, 0 );    break;
            case 180:  transform = TRANSFORM( -1, 0, 0, -1 );   break;
            case 270:  transform = TRANSFORM( 0, -1, 1, 0 );    break;
            default:   Expecting( "0, 90, 180, or 270" );
            }

            symbol->SetTransform( transform );
            NeedRIGHT();
            break;

        case T_mirror:
            token = NextTok();

            if( token == T_x )
                symbol->SetOrientation( SYM_MIRROR_X );
            else if( token == T_y )
                symbol->SetOrientation( SYM_MIRROR_Y );
            else
                Expecting( "x or y" );

            NeedRIGHT();
            break;

        case T_unit:
            symbol->SetUnit( parseInt( "symbol unit" ) );
            NeedRIGHT();
            break;

        case T_convert:     // Legacy token
        case T_body_style:
            symbol->SetBodyStyle( parseInt( "symbol body style" ) );
            NeedRIGHT();
            break;

        case T_exclude_from_sim:
            symbol->SetExcludedFromSim( parseBool() );
            NeedRIGHT();
            break;

        case T_in_bom:
            symbol->SetExcludedFromBOM( !parseBool() );
            NeedRIGHT();
            break;

        case T_on_board:
            symbol->SetExcludedFromBoard( !parseBool() );
            NeedRIGHT();
            break;

        case T_in_pos_files:
            symbol->SetExcludedFromPosFiles( !parseBool() );
            NeedRIGHT();
            break;

        case T_dnp:
            symbol->SetDNP( parseBool() );
            NeedRIGHT();
            break;

        case T_fields_autoplaced:
            if( parseMaybeAbsentBool( true ) )
                symbol->SetFieldsAutoplaced( AUTOPLACE_AUTO );

            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( symbol->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        case T_default_instance:
        {
            SCH_SYMBOL_INSTANCE defaultInstance;

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_reference:
                    NeedSYMBOL();
                    defaultInstance.m_Reference = FromUTF8();
                    NeedRIGHT();
                    break;

                case T_unit:
                    defaultInstance.m_Unit = parseInt( "symbol unit" );
                    NeedRIGHT();
                    break;

                case T_value:
                    NeedSYMBOL();

                    if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                        symbol->SetValueFieldText( wxEmptyString );
                    else
                        symbol->SetValueFieldText( FromUTF8() );

                    NeedRIGHT();
                    break;

                case T_footprint:
                    NeedSYMBOL();

                    if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                        symbol->SetFootprintFieldText( wxEmptyString );
                    else
                        symbol->SetFootprintFieldText( FromUTF8() );

                    NeedRIGHT();
                    break;

                default:
                    Expecting( "reference, unit, value or footprint" );
                }
            }

            break;
        }

        case T_instances:
        {
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_project )
                    Expecting( "project" );

                NeedSYMBOL();

                wxString projectName = FromUTF8();

                for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                {
                    if( token != T_LEFT )
                        Expecting( T_LEFT );

                    token = NextTok();

                    if( token != T_path )
                        Expecting( "path" );

                    SCH_SYMBOL_INSTANCE instance;

                    instance.m_ProjectName = projectName;

                    NeedSYMBOL();
                    instance.m_Path = KIID_PATH( FromUTF8() );

                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                    {
                        if( token != T_LEFT )
                            Expecting( T_LEFT );

                        token = NextTok();

                        switch( token )
                        {
                        case T_reference:
                            NeedSYMBOL();
                            instance.m_Reference = FromUTF8();
                            NeedRIGHT();
                            break;

                        case T_unit:
                            instance.m_Unit = parseInt( "symbol unit" );
                            NeedRIGHT();
                            break;

                        case T_value:
                            NeedSYMBOL();

                            if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                                symbol->SetValueFieldText( wxEmptyString );
                            else
                                symbol->SetValueFieldText( FromUTF8() );

                            NeedRIGHT();
                            break;

                        case T_footprint:
                            NeedSYMBOL();

                            if( m_requiredVersion < 20250318 && FromUTF8() == "~" )
                                symbol->SetFootprintFieldText( wxEmptyString );
                            else
                                symbol->SetFootprintFieldText( FromUTF8() );

                            NeedRIGHT();
                            break;


                        case T_variant:
                        {
                            SCH_SYMBOL_VARIANT variant;

                            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                            {
                                if( token != T_LEFT )
                                    Expecting( T_LEFT );

                                token = NextTok();

                                switch( token )
                                {
                                case T_name:
                                    NeedSYMBOL();
                                    variant.m_Name = FromUTF8();
                                    NeedRIGHT();
                                    break;

                                case T_dnp:
                                    variant.m_DNP = parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_exclude_from_sim:
                                    variant.m_ExcludedFromSim = parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_in_bom:
                                    variant.m_ExcludedFromBOM = parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_on_board:
                                    variant.m_ExcludedFromBoard = !parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_in_pos_files:
                                    variant.m_ExcludedFromPosFiles = !parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_field:
                                {
                                    wxString fieldName;
                                    wxString fieldValue;

                                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                                    {
                                        if( token != T_LEFT )
                                            Expecting( T_LEFT );

                                        token = NextTok();

                                        switch( token )
                                        {
                                        case T_name:
                                            NeedSYMBOL();
                                            fieldName = FromUTF8();
                                            NeedRIGHT();
                                            break;

                                        case T_value:
                                            NeedSYMBOL();
                                            fieldValue = FromUTF8();
                                            NeedRIGHT();
                                            break;

                                        default:
                                            Expecting( "name or value" );
                                        }
                                    }

                                    variant.m_Fields[fieldName] = fieldValue;
                                    break;
                                }

                                default:
                                    Expecting( "dnp, exclude_from_sim, field, in_bom, in_pos_files, name, or on_board" );
                                }

                                instance.m_Variants[variant.m_Name] = variant;
                            }

                            break;
                        }

                        default:
                            Expecting( "reference, unit, value, footprint, or variant" );
                        }
                    }

                    symbol->AddHierarchicalReference( instance );
                }
            }

            break;
        }

        case T_property:
            // The field parent symbol must be set and its orientation must be set before
            // the field positions are set.
            field = parseSchField( symbol.get() );

            // Exclude from simulation used to be managed by a Sim.Enable field set to "0" when
            // simulation was disabled.
            if( field->GetCanonicalName() == SIM_LEGACY_ENABLE_FIELD_V7 )
            {
                symbol->SetExcludedFromSim( field->GetText() == wxS( "0" ) );
                delete field;
                break;
            }

            // Even longer ago, we had a "Spice_Netlist_Enabled" field
            if( field->GetCanonicalName() == SIM_LEGACY_ENABLE_FIELD )
            {
                symbol->SetExcludedFromSim( field->GetText() == wxS( "N" ) );
                delete field;
                break;
            }

            SCH_FIELD* existing;

            if( field->IsMandatory() )
                existing = symbol->GetField( field->GetId() );
            else
                existing = symbol->GetField( field->GetName() );

            if( existing )
                *existing = *field;
            else
                symbol->AddField( *field );

            if( field->GetId() == FIELD_T::REFERENCE )
                symbol->UpdatePrefix();

            delete field;
            break;

        case T_pin:
        {
            // Read an alternate pin designation
            wxString number;
            KIID     uuid;
            wxString alt;

            NeedSYMBOL();
            number = FromUTF8();

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_alternate:
                    NeedSYMBOL();
                    alt = FromUTF8();
                    NeedRIGHT();
                    break;

                case T_uuid:
                    NeedSYMBOL();

                    // First version to write out pin uuids accidentally wrote out the symbol's
                    // uuid for each pin, so ignore uuids coming from that version.
                    if( m_requiredVersion >= 20210126 )
                        uuid = parseKIID();

                    NeedRIGHT();
                    break;

                default:
                    Expecting( "alternate or uuid" );
                }
            }

            symbol->GetRawPins().emplace_back( std::make_unique<SCH_PIN>( symbol.get(), number,
                                                                          alt, uuid ) );
            break;
        }

        default:
            Expecting( "lib_id, lib_name, at, mirror, uuid, exclude_from_sim, on_board, in_bom, dnp, "
                       "default_instance, property, pin, or instances" );
        }
    }

    if( !libName.IsEmpty() && ( symbol->GetLibId().Format().wx_str() != libName ) )
        symbol->SetSchSymbolLibraryName( libName );

    // Ensure edit/status flags are cleared after these initializations:
    symbol->ClearFlags();

    return symbol.release();
}


SCH_BITMAP* SCH_IO_KICAD_SEXPR_PARSER::parseImage()
{
    wxCHECK_MSG( CurTok() == T_image, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as an image." ) );

    T                           token;
    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>();
    REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            bitmap->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_scale:
        {
            const double scale = parseDouble( "image scale factor" );
            refImage.SetImageScale( std::isnormal( scale ) ? scale : 1.0 );

            NeedRIGHT();
            break;
        }
        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( bitmap->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        case T_data:
        {
            token = NextTok();

            wxString data;

            // Reserve 128K because most image files are going to be larger than the default
            // 1K that wxString reserves.
            data.reserve( 1 << 17 );

            while( token != T_RIGHT )
            {
                if( !IsSymbol( token ) )
                    Expecting( "base64 image data" );

                data += FromUTF8();
                token = NextTok();
            }

            wxMemoryBuffer       buffer = wxBase64Decode( data );

            if( !refImage.ReadImageFile( buffer ) )
                THROW_IO_ERROR( _( "Failed to read image data." ) );

            break;
        }

        default:
            Expecting( "at, scale, uuid or data" );
        }
    }

    // The image will be scaled by PPI in ReadImageFile.

    // 20230121 or older file format versions assumed 300 image PPI at load/save.
    // Let's keep compatibility by changing image scale.
    if( m_requiredVersion <= 20230121 )
    {
        refImage.SetImageScale( refImage.GetImageScale() * refImage.GetImage().GetPPI() / 300.0 );
    }

    return bitmap.release();
}


SCH_SHEET* SCH_IO_KICAD_SEXPR_PARSER::parseSheet()
{
    wxCHECK_MSG( CurTok() == T_sheet, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a sheet." ) );

    T token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS fill;
    SCH_FIELD* field;
    std::vector<SCH_FIELD> fields;
    std::unique_ptr<SCH_SHEET> sheet = std::make_unique<SCH_SHEET>();
    std::set<int> fieldIDsRead;

    // We'll reset this if we find a fields_autoplaced token
    sheet->SetFieldsAutoplaced( AUTOPLACE_NONE );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            sheet->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_size:
        {
            VECTOR2I size;
            size.x = parseInternalUnits( "sheet width" );
            size.y = parseInternalUnits( "sheet height" );
            sheet->SetSize( size );
            NeedRIGHT();
            break;
        }

        case T_exclude_from_sim:
            sheet->SetExcludedFromSim( parseBool() );
            NeedRIGHT();
            break;

        case T_in_bom:
            sheet->SetExcludedFromBOM( !parseBool() );
            NeedRIGHT();
            break;

        case T_on_board:
            sheet->SetExcludedFromBoard( !parseBool() );
            NeedRIGHT();
            break;

        case T_dnp:
            sheet->SetDNP( parseBool() );
            NeedRIGHT();
            break;

        case T_fields_autoplaced:
            if( parseMaybeAbsentBool( true ) )
                sheet->SetFieldsAutoplaced( AUTOPLACE_AUTO );

            break;

        case T_stroke:
            parseStroke( stroke );
            sheet->SetBorderWidth( stroke.GetWidth() );
            sheet->SetBorderColor( stroke.GetColor() );
            break;

        case T_fill:
            parseFill( fill );
            sheet->SetBackgroundColor( fill.m_Color );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( sheet->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        case T_property:
            field = parseSchField( sheet.get() );

            if( m_requiredVersion <= 20200310 )
            {
                // Earlier versions had the wrong ids (and names) saved for sheet fields.
                // Fortunately they only saved the sheetname and sheetfilepath (and always
                // in that order), so we can hack in a recovery.
                if( fields.empty() )
                    field->setId( FIELD_T::SHEET_NAME );
                else
                    field->setId( FIELD_T::SHEET_FILENAME );
            }

            fields.emplace_back( *field );

            delete field;
            break;

        case T_pin:
            sheet->AddPin( parseSchSheetPin( sheet.get() ) );
            break;

        case T_instances:
        {
            std::vector<SCH_SHEET_INSTANCE> instances;

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_project )
                    Expecting( "project" );

                NeedSYMBOL();

                wxString projectName = FromUTF8();

                for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                {
                    if( token != T_LEFT )
                        Expecting( T_LEFT );

                    token = NextTok();

                    if( token != T_path )
                        Expecting( "path" );

                    SCH_SHEET_INSTANCE instance;

                    instance.m_ProjectName = projectName;

                    NeedSYMBOL();
                    instance.m_Path = KIID_PATH( FromUTF8() );

                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                    {
                        if( token != T_LEFT )
                            Expecting( T_LEFT );

                        token = NextTok();

                        switch( token )
                        {
                        case T_page:
                        {
                            NeedSYMBOL();
                            instance.m_PageNumber = FromUTF8();

                            // Empty page numbers are not permitted
                            if( instance.m_PageNumber.IsEmpty() )
                            {
                                // Use hash character instead
                                instance.m_PageNumber = wxT( "#" );
                            }
                            else
                            {
                                // Whitespaces are not permitted
                                static std::vector<wxString> whitespaces =
                                        { wxT( "\r" ),
                                          wxT( "\n" ),
                                          wxT( "\t" ),
                                          wxT( " " ) };

                                for( wxString ch : whitespaces )
                                    instance.m_PageNumber.Replace( ch, wxEmptyString );
                            }

                            NeedRIGHT();
                            break;
                        }

                        case T_variant:
                        {
                            SCH_SHEET_VARIANT variant;

                            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                            {
                                if( token != T_LEFT )
                                    Expecting( T_LEFT );

                                token = NextTok();

                                switch( token )
                                {
                                case T_name:
                                    NeedSYMBOL();
                                    variant.m_Name = FromUTF8();
                                    NeedRIGHT();
                                    break;

                                case T_dnp:
                                    variant.m_DNP = parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_exclude_from_sim:
                                    variant.m_ExcludedFromSim = parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_in_bom:
                                    variant.m_ExcludedFromBOM = parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_on_board:
                                    variant.m_ExcludedFromBoard = !parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_in_pos_files:
                                    variant.m_ExcludedFromPosFiles = !parseBool();
                                    NeedRIGHT();
                                    break;

                                case T_field:
                                {
                                    wxString fieldName;
                                    wxString fieldValue;

                                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                                    {
                                        if( token != T_LEFT )
                                            Expecting( T_LEFT );

                                        token = NextTok();

                                        switch( token )
                                        {
                                        case T_name:
                                            NeedSYMBOL();
                                            fieldName = FromUTF8();
                                            NeedRIGHT();
                                            break;

                                        case T_value:
                                            NeedSYMBOL();
                                            fieldValue = FromUTF8();
                                            NeedRIGHT();
                                            break;

                                        default:
                                            Expecting( "name or value" );
                                        }
                                    }

                                    variant.m_Fields[fieldName] = fieldValue;
                                    break;
                                }

                                default:
                                    Expecting( "dnp, exclude_from_sim, field, in_bom, in_pos_files, name, or on_board" );
                                }

                                instance.m_Variants[variant.m_Name] = variant;
                            }

                            break;
                        }

                        default:
                            Expecting( "page or variant" );
                        }
                    }

                    instances.emplace_back( instance );
                }
            }

            sheet->setInstances( instances );
            break;
        }

        default:
            Expecting( "at, size, stroke, background, instances, uuid, property, or pin" );
        }
    }

    sheet->SetFields( fields );

    return sheet.release();
}


SCH_JUNCTION* SCH_IO_KICAD_SEXPR_PARSER::parseJunction()
{
    wxCHECK_MSG( CurTok() == T_junction, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a junction." ) );

    T                             token;
    std::unique_ptr<SCH_JUNCTION> junction = std::make_unique<SCH_JUNCTION>();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            junction->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_diameter:
            junction->SetDiameter( parseInternalUnits( "junction diameter" ) );
            NeedRIGHT();
            break;

        case T_color:
        {
            COLOR4D color;

            color.r = parseInt( "red" ) / 255.0;
            color.g = parseInt( "green" ) / 255.0;
            color.b = parseInt( "blue" ) / 255.0;
            color.a = std::clamp( parseDouble( "alpha" ), 0.0, 1.0 );

            junction->SetColor( color );
            NeedRIGHT();
            break;
        }

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( junction->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at, diameter, color or uuid" );
        }
    }

    return junction.release();
}


SCH_NO_CONNECT* SCH_IO_KICAD_SEXPR_PARSER::parseNoConnect()
{
    wxCHECK_MSG( CurTok() == T_no_connect, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a no connect." ) );

    T                               token;
    std::unique_ptr<SCH_NO_CONNECT> no_connect = std::make_unique<SCH_NO_CONNECT>();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            no_connect->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( no_connect->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at or uuid" );
        }
    }

    return no_connect.release();
}


SCH_BUS_WIRE_ENTRY* SCH_IO_KICAD_SEXPR_PARSER::parseBusEntry()
{
    wxCHECK_MSG( CurTok() == T_bus_entry, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bus entry." ) );

    T token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    std::unique_ptr<SCH_BUS_WIRE_ENTRY> busEntry = std::make_unique<SCH_BUS_WIRE_ENTRY>();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            busEntry->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_size:
        {
            VECTOR2I size;

            size.x = parseInternalUnits( "bus entry height" );
            size.y = parseInternalUnits( "bus entry width" );
            busEntry->SetSize( size );
            NeedRIGHT();
            break;
        }

        case T_stroke:
            parseStroke( stroke );
            busEntry->SetStroke( stroke );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( busEntry->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at, size, uuid or stroke" );
        }
    }

    return busEntry.release();
}


void fixupSchFillMode( SCH_SHAPE* aShape )
{
    if( aShape->GetFillMode() == FILL_T::FILLED_SHAPE )
    {
        aShape->SetFillColor( aShape->GetStroke().GetColor() );
        aShape->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    }
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSchPolyLine()
{
    T             token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;

    std::unique_ptr<SCH_SHAPE> polyline = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_NOTES );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_pts:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_xy )
                    Expecting( "xy" );

                polyline->AddPoint( parseXY() );

                NeedRIGHT();
            }
            break;

        case T_stroke:
            parseStroke( stroke );

            // In 6.0, the default schematic line style was Dashed.
            if( m_requiredVersion <= 20211123 && stroke.GetLineStyle() == LINE_STYLE::DEFAULT )
                stroke.SetLineStyle( LINE_STYLE::DASH );

            polyline->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            polyline->SetFillMode( fill.m_FillType );
            polyline->SetFillColor( fill.m_Color );
            fixupSchFillMode( polyline.get() );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( polyline->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "pts, uuid, stroke, or fill" );
        }
    }

    return polyline.release();
}


SCH_LINE* SCH_IO_KICAD_SEXPR_PARSER::parseLine()
{
    // Note: T_polyline is deprecated in this code: it is now handled by
    // parseSchPolyLine() that can handle true polygons, and not only one segment.

    T             token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    int           layer;

    switch( CurTok() )
    {
    case T_polyline: layer = LAYER_NOTES; break;
    case T_wire:     layer = LAYER_WIRE;  break;
    case T_bus:      layer = LAYER_BUS;   break;
    default:
        wxCHECK_MSG( false, nullptr, "Cannot parse " + GetTokenString( CurTok() ) + " as a line." );
    }

    std::unique_ptr<SCH_LINE> line = std::make_unique<SCH_LINE>( VECTOR2I(), layer );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_pts:
            NeedLEFT();
            token = NextTok();

            if( token != T_xy )
                Expecting( "xy" );

            line->SetStartPoint( parseXY() );
            NeedRIGHT();
            NeedLEFT();
            token = NextTok();

            if( token != T_xy )
                Expecting( "xy" );

            line->SetEndPoint( parseXY() );
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            line->SetStroke( stroke );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( line->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at, uuid or stroke" );
        }
    }

    return line.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSchArc()
{
    wxCHECK_MSG( CurTok() == T_arc, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as an arc." ) );

    T             token;
    VECTOR2I      startPoint;
    VECTOR2I      midPoint;
    VECTOR2I      endPoint;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    std::unique_ptr<SCH_SHAPE> arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:
            startPoint = parseXY();
            NeedRIGHT();
            break;

        case T_mid:
            midPoint = parseXY();
            NeedRIGHT();
            break;

        case T_end:
            endPoint = parseXY();
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            arc->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            arc->SetFillMode( fill.m_FillType );
            arc->SetFillColor( fill.m_Color );
            fixupSchFillMode( arc.get() );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( arc->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "start, mid, end, stroke, fill or uuid" );
        }
    }

    arc->SetArcGeometry( startPoint, midPoint, endPoint );

    return arc.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSchCircle()
{
    wxCHECK_MSG( CurTok() == T_circle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a circle." ) );

    T             token;
    VECTOR2I      center;
    int           radius = 0;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    std::unique_ptr<SCH_SHAPE> circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_center:
            center = parseXY();
            NeedRIGHT();
            break;

        case T_radius:
            radius = parseInternalUnits( "radius length" );
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            circle->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            circle->SetFillMode( fill.m_FillType );
            circle->SetFillColor( fill.m_Color );
            fixupSchFillMode( circle.get() );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( circle->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "center, radius, stroke, fill or uuid" );
        }
    }

    circle->SetCenter( center );
    circle->SetEnd( VECTOR2I( center.x + radius, center.y ) );

    return circle.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSchRectangle()
{
    wxCHECK_MSG( CurTok() == T_rectangle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a rectangle." ) );

    T             token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    std::unique_ptr<SCH_SHAPE> rectangle = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:
            rectangle->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_end:
            rectangle->SetEnd( parseXY() );
            NeedRIGHT();
            break;

        case T_radius:
            rectangle->SetCornerRadius( parseDouble( "corner radius" ) * schIUScale.IU_PER_MM );
            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            rectangle->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            rectangle->SetFillMode( fill.m_FillType );
            rectangle->SetFillColor( fill.m_Color );
            fixupSchFillMode( rectangle.get() );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( rectangle->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "start, end, stroke, fill or uuid" );
        }
    }

    return rectangle.release();
}


SCH_RULE_AREA* SCH_IO_KICAD_SEXPR_PARSER::parseSchRuleArea()
{
    wxCHECK_MSG( CurTok() == T_rule_area, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a rule area." ) );

    T             token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    std::unique_ptr<SCH_RULE_AREA> ruleArea = std::make_unique<SCH_RULE_AREA>();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_polyline:
        {
            std::unique_ptr<SCH_SHAPE> poly( parseSchPolyLine() );
            SHAPE_POLY_SET& sch_rule_poly = poly->GetPolyShape();

            // The polygon must be closed, it is a schematic closed polyline:
            sch_rule_poly.Outline(0).SetClosed( true );

            ruleArea->SetPolyShape( sch_rule_poly );

            ruleArea->SetStroke( poly->GetStroke() );
            ruleArea->SetFillMode( poly->GetFillMode() );
            ruleArea->SetFillColor( poly->GetFillColor() );

            // the uuid is saved to the shape but stored and saved out of the rulearea
            const_cast<KIID&>( ruleArea->m_Uuid ) = poly->m_Uuid;
            break;
        }

        case T_exclude_from_sim:
            ruleArea->SetExcludedFromSim( parseBool() );
            NeedRIGHT();
            break;

        case T_in_bom:
            ruleArea->SetExcludedFromBOM( !parseBool() );
            NeedRIGHT();
            break;

        case T_on_board:
            ruleArea->SetExcludedFromBoard( !parseBool() );
            NeedRIGHT();
            break;

        case T_dnp:
            ruleArea->SetDNP( parseBool() );
            NeedRIGHT();
            break;

        default:
            Expecting( "exclude_from_sim, on_board, in_bom, dnp, or polyline" );
        }
    }

    return ruleArea.release();
}


SCH_SHAPE* SCH_IO_KICAD_SEXPR_PARSER::parseSchBezier()
{
    wxCHECK_MSG( CurTok() == T_bezier, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bezier." ) );

    T             token;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    std::unique_ptr<SCH_SHAPE> bezier = std::make_unique<SCH_SHAPE>( SHAPE_T::BEZIER );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_pts:
        {
            int ii = 0;

            for( token = NextTok();  token != T_RIGHT;  token = NextTok(), ++ii )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_xy )
                    Expecting( "xy" );

                switch( ii )
                {
                case 0: bezier->SetStart( parseXY() );    break;
                case 1: bezier->SetBezierC1( parseXY() ); break;
                case 2: bezier->SetBezierC2( parseXY() ); break;
                case 3: bezier->SetEnd( parseXY() );      break;
                default: Unexpected( "control point" );   break;
                }

                NeedRIGHT();
            }
        }
            break;

        case T_stroke:
            parseStroke( stroke );
            bezier->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            bezier->SetFillMode( fill.m_FillType );
            bezier->SetFillColor( fill.m_Color );
            fixupSchFillMode( bezier.get() );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( bezier->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "pts, stroke, fill or uuid" );
        }
    }

    bezier->RebuildBezierToSegmentsPointsList( m_maxError );

    return bezier.release();
}


SCH_TEXT* SCH_IO_KICAD_SEXPR_PARSER::parseSchText()
{
    T                         token;
    std::unique_ptr<SCH_TEXT> text;

    switch( CurTok() )
    {
    case T_text:                text = std::make_unique<SCH_TEXT>();            break;
    case T_label:               text = std::make_unique<SCH_LABEL>();           break;
    case T_global_label:        text = std::make_unique<SCH_GLOBALLABEL>();     break;
    case T_hierarchical_label:  text = std::make_unique<SCH_HIERLABEL>();       break;
    case T_netclass_flag:       text = std::make_unique<SCH_DIRECTIVE_LABEL>(); break;
    case T_directive_label:     text = std::make_unique<SCH_DIRECTIVE_LABEL>(); break;
    default:
        wxCHECK_MSG( false, nullptr, "Cannot parse " + GetTokenString( CurTok() ) + " as text." );
    }

    // We'll reset this if we find a fields_autoplaced token
    text->SetFieldsAutoplaced( AUTOPLACE_NONE );

    NeedSYMBOL();

    text->SetText( FromUTF8() );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_exclude_from_sim:
            text->SetExcludedFromSim( parseBool() );
            NeedRIGHT();
            break;

        case T_at:
            text->SetPosition( parseXY() );
            text->SetTextAngle( EDA_ANGLE( parseDouble( "text angle" ), DEGREES_T ).KeepUpright() );

            if( SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( text.get() ) )
            {
                switch( static_cast<int>( label->GetTextAngle().AsDegrees() ) )
                {
                case 0:   label->SetSpinStyle( SPIN_STYLE::RIGHT );         break;
                case 90:  label->SetSpinStyle( SPIN_STYLE::UP );            break;
                case 180: label->SetSpinStyle( SPIN_STYLE::LEFT );          break;
                case 270: label->SetSpinStyle( SPIN_STYLE::BOTTOM );        break;
                default:  wxFAIL; label->SetSpinStyle( SPIN_STYLE::RIGHT ); break;
                }
            }

            NeedRIGHT();
            break;

        case T_shape:
        {
            if( text->Type() == SCH_TEXT_T || text->Type() == SCH_LABEL_T )
                Unexpected( T_shape );

            SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( text.get() );

            token = NextTok();

            switch( token )
            {
            case T_input:          label->SetShape( LABEL_FLAG_SHAPE::L_INPUT );        break;
            case T_output:         label->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT );       break;
            case T_bidirectional:  label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );         break;
            case T_tri_state:      label->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE );     break;
            case T_passive:        label->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED );  break;
            case T_dot:            label->SetShape( LABEL_FLAG_SHAPE::F_DOT );          break;
            case T_round:          label->SetShape( LABEL_FLAG_SHAPE::F_ROUND );        break;
            case T_diamond:        label->SetShape( LABEL_FLAG_SHAPE::F_DIAMOND );      break;
            case T_rectangle:      label->SetShape( LABEL_FLAG_SHAPE::F_RECTANGLE );    break;
            default:
                Expecting( "input, output, bidirectional, tri_state, passive, dot, round, diamond"
                           "or rectangle" );
            }

            NeedRIGHT();
            break;
        }

        case T_length:
        {
            if( text->Type() != SCH_DIRECTIVE_LABEL_T )
                Unexpected( T_length );

            SCH_DIRECTIVE_LABEL* label = static_cast<SCH_DIRECTIVE_LABEL*>( text.get() );

            label->SetPinLength( parseInternalUnits( "pin length" ) );
            NeedRIGHT();
        }
            break;

        case T_fields_autoplaced:
            if( parseMaybeAbsentBool( true ) )
                text->SetFieldsAutoplaced( AUTOPLACE_AUTO );

            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( text.get() ), true );

            // Hidden schematic text is no longer supported
            text->SetVisible( true );
            break;

        case T_iref:    // legacy format; current is a T_property (aka SCH_FIELD)
            if( text->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( text.get() );
                SCH_FIELD*       field = label->GetField( FIELD_T::INTERSHEET_REFS );

                field->SetTextPos( parseXY() );
                NeedRIGHT();

                field->SetVisible( true );
            }
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( text->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        case T_property:
        {
            if( text->Type() == SCH_TEXT_T )
                Unexpected( T_property );

            SCH_FIELD* field = parseSchField( text.get() );

            // Intersheetrefs is a mandatory field and so will already exist
            if( text->Type() == SCH_GLOBAL_LABEL_T && field->IsMandatory() )
            {
                SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( text.get() );
                *label->GetField( field->GetId() ) = *field;
            }
            else
            {
                static_cast<SCH_LABEL_BASE*>( text.get() )->GetFields().emplace_back( *field );
            }

            delete field;
            break;
        }

        default:
            Expecting( "at, shape, iref, uuid or effects" );
        }
    }

    SCH_LABEL_BASE* label = dynamic_cast<SCH_LABEL_BASE*>( text.get() );

    if( label && label->GetFields().empty() )
        label->SetFieldsAutoplaced( AUTOPLACE_AUTO );

    return text.release();
}


SCH_TEXTBOX* SCH_IO_KICAD_SEXPR_PARSER::parseSchTextBox()
{
    wxCHECK_MSG( CurTok() == T_text_box, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a text box." ) );

    std::unique_ptr<SCH_TEXTBOX> textBox = std::make_unique<SCH_TEXTBOX>();

    parseSchTextBoxContent( textBox.get() );

    return textBox.release();
}


SCH_TABLECELL* SCH_IO_KICAD_SEXPR_PARSER::parseSchTableCell()
{
    wxCHECK_MSG( CurTok() == T_table_cell, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a table cell." ) );

    std::unique_ptr<SCH_TABLECELL> cell = std::make_unique<SCH_TABLECELL>();

    parseSchTextBoxContent( cell.get() );

    return cell.release();
}


void SCH_IO_KICAD_SEXPR_PARSER::parseSchTextBoxContent( SCH_TEXTBOX* aTextBox )
{
    T             token;
    VECTOR2I      pos;
    VECTOR2I      end;
    VECTOR2I      size;
    int           left;
    int           top;
    int           right;
    int           bottom;
    STROKE_PARAMS stroke( schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS ), LINE_STYLE::DEFAULT );
    FILL_PARAMS   fill;
    bool          foundEnd = false;
    bool          foundSize = false;
    bool          foundMargins = false;

    NeedSYMBOL();

    aTextBox->SetText( FromUTF8() );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_exclude_from_sim:
            aTextBox->SetExcludedFromSim( parseBool() );
            NeedRIGHT();
            break;

        case T_start:       // Legacy token during 6.99 development; fails to handle angle
            pos = parseXY();
            NeedRIGHT();
            break;

        case T_end:         // Legacy token during 6.99 development; fails to handle angle
            end = parseXY();
            foundEnd = true;
            NeedRIGHT();
            break;

        case T_at:
            pos = parseXY();
            aTextBox->SetTextAngle( EDA_ANGLE( parseDouble( "textbox angle" ), DEGREES_T ) );
            NeedRIGHT();
            break;

        case T_size:
            size = parseXY();
            foundSize = true;
            NeedRIGHT();
            break;

        case T_span:
            if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( aTextBox ) )
            {
                cell->SetColSpan( parseInt( "column span" ) );
                cell->SetRowSpan( parseInt( "row span" ) );
            }
            else
            {
                Expecting( "at, size, stroke, fill, effects or uuid" );
            }

            NeedRIGHT();
            break;

        case T_stroke:
            parseStroke( stroke );
            aTextBox->SetStroke( stroke );
            break;

        case T_fill:
            parseFill( fill );
            aTextBox->SetFillMode( fill.m_FillType );
            aTextBox->SetFillColor( fill.m_Color );
            fixupSchFillMode( aTextBox );
            break;

        case T_margins:
            parseMargins( left, top, right, bottom );
            aTextBox->SetMarginLeft( left );
            aTextBox->SetMarginTop( top );
            aTextBox->SetMarginRight( right );
            aTextBox->SetMarginBottom( bottom );
            foundMargins = true;
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( aTextBox ), false );
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( aTextBox->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            if( dynamic_cast<SCH_TABLECELL*>( aTextBox ) != nullptr )
                Expecting( "at, size, stroke, fill, effects, span or uuid" );
            else
                Expecting( "at, size, stroke, fill, effects or uuid" );
        }
    }

    aTextBox->SetPosition( pos );

    if( foundEnd )
        aTextBox->SetEnd( end );
    else if( foundSize )
        aTextBox->SetEnd( pos + size );
    else
        Expecting( "size" );

    if( !foundMargins )
    {
        int margin = aTextBox->GetLegacyTextMargin();
        aTextBox->SetMarginLeft( margin );
        aTextBox->SetMarginTop( margin );
        aTextBox->SetMarginRight( margin );
        aTextBox->SetMarginBottom( margin );
    }
}


SCH_TABLE* SCH_IO_KICAD_SEXPR_PARSER::parseSchTable()
{
    wxCHECK_MSG( CurTok() == T_table, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a table." ) );

    T             token;
    int           defaultLineWidth = schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );
    STROKE_PARAMS borderStroke( defaultLineWidth, LINE_STYLE::DEFAULT );
    STROKE_PARAMS separatorsStroke( defaultLineWidth, LINE_STYLE::DEFAULT );
    std::unique_ptr<SCH_TABLE> table = std::make_unique<SCH_TABLE>( defaultLineWidth );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_column_count:
            table->SetColCount( parseInt( "column count" ) );
            NeedRIGHT();
            break;

        case T_column_widths:
        {
            int col = 0;

            while( ( token = NextTok() ) != T_RIGHT )
                table->SetColWidth( col++, parseInternalUnits() );

            break;
        }

        case T_row_heights:
        {
            int row = 0;

            while( ( token = NextTok() ) != T_RIGHT )
                table->SetRowHeight( row++, parseInternalUnits() );

            break;
        }

        case T_cells:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_table_cell )
                    Expecting( "table_cell" );

                table->AddCell( parseSchTableCell() );
            }

            break;

        case T_border:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_external:
                    table->SetStrokeExternal( parseBool() );
                    NeedRIGHT();
                    break;

                case T_header:
                    table->SetStrokeHeaderSeparator( parseBool() );
                    NeedRIGHT();
                    break;

                case T_stroke:
                    parseStroke( borderStroke );
                    table->SetBorderStroke( borderStroke );
                    break;

                default:
                    Expecting( "external, header or stroke" );
                    break;
                }
            }

            break;

        case T_separators:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_rows:
                    table->SetStrokeRows( parseBool() );
                    NeedRIGHT();
                    break;

                case T_cols:
                    table->SetStrokeColumns( parseBool() );
                    NeedRIGHT();
                    break;

                case T_stroke:
                    parseStroke( separatorsStroke );
                    table->SetSeparatorsStroke( separatorsStroke );
                    break;

                default:
                    Expecting( "rows, cols, or stroke" );
                    break;
                }
            }

            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( table->m_Uuid ) = parseKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "columns, col_widths, row_heights, border, separators, uuid, header or cells" );
        }
    }

    return table.release();
}


void SCH_IO_KICAD_SEXPR_PARSER::parseBusAlias( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( CurTok() == T_bus_alias,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bus alias." ) );
    wxCHECK( aScreen, /* void */ );

    T token;
    std::shared_ptr<BUS_ALIAS> busAlias = std::make_shared<BUS_ALIAS>();
    wxString alias;
    wxString member;

    NeedSYMBOL();

    alias = FromUTF8();

    if( m_requiredVersion < 20210621 )
        alias = ConvertToNewOverbarNotation( alias );

    busAlias->SetName( alias );

    NeedLEFT();
    token = NextTok();

    if( token != T_members )
        Expecting( "members" );

    token = NextTok();

    while( token != T_RIGHT )
    {
        if( !IsSymbol( token ) )
            Expecting( "quoted string" );

        member = FromUTF8();

        if( m_requiredVersion < 20210621 )
            member = ConvertToNewOverbarNotation( member );

        busAlias->Members().emplace_back( member );

        token = NextTok();
    }

    NeedRIGHT();

    aScreen->AddBusAlias( busAlias );
}


void SCH_IO_KICAD_SEXPR_PARSER::parseGroupMembers( GROUP_INFO& aGroupInfo )
{
    T token;

    while( ( token = NextTok() ) != T_RIGHT )
    {
        // This token is the Uuid of the item in the group.
        // Since groups are serialized at the end of the file/footprint, the Uuid should already
        // have been seen and exist in the board.
        KIID uuid( CurStr() );
        aGroupInfo.memberUuids.push_back( uuid );
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::parseGroup()
{
    wxCHECK_RET( CurTok() == T_group,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_GROUP." ) );

    T token;

    m_groupInfos.push_back( GROUP_INFO() );
    GROUP_INFO& groupInfo = m_groupInfos.back();

    while( ( token = NextTok() ) != T_LEFT )
    {
        if( token == T_STRING )
            groupInfo.name = FromUTF8();
        else
            Expecting( "group name or locked" );
    }

    for( ; token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_uuid:
            NextTok();
            groupInfo.uuid = parseKIID();
            NeedRIGHT();
            break;

        case T_lib_id:
        {
            token = NextTok();

            if( !IsSymbol( token ) && token != T_NUMBER )
                Expecting( "symbol|number" );

            LIB_ID   libId;
            wxString name = FromUTF8();
            // Some symbol LIB_IDs have the '/' character escaped which can break
            // symbol links.  The '/' character is no longer an illegal LIB_ID character so
            // it doesn't need to be escaped.
            name.Replace( "{slash}", "/" );

            int bad_pos = groupInfo.libId.Parse( name );

            if( bad_pos >= 0 )
            {
                if( static_cast<int>( name.size() ) > bad_pos )
                {
                    wxString msg = wxString::Format( _( "Group library link %s contains invalid character '%c'" ), name,
                                                     name[bad_pos] );

                    THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
                }

                THROW_PARSE_ERROR( _( "Invalid library ID" ), CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            NeedRIGHT();
            break;
        }

        case T_members:
        {
            parseGroupMembers( groupInfo );
            break;
        }

        default:
            Expecting( "uuid, lib_id, members" );
        }
    }
}


void SCH_IO_KICAD_SEXPR_PARSER::resolveGroups( SCH_SCREEN* aParent )
{
    if( !aParent )
        return;

    auto getItem =
            [&]( const KIID& aId )
            {
                SCH_ITEM* aItem = nullptr;

                for( SCH_ITEM* item : aParent->Items() )
                {
                    if( item->m_Uuid == aId )
                    {
                        aItem = item;
                        break;
                    }
                }

                return aItem;
            };

    // Now that we've parsed the other Uuids in the file we can resolve the uuids referred
    // to in the group declarations we saw.
    //
    // First add all group objects so subsequent GetItem() calls for nested groups work.
    for( const GROUP_INFO& groupInfo : m_groupInfos )
    {
        SCH_GROUP* group = nullptr;

        group = new SCH_GROUP( aParent );
        group->SetName( groupInfo.name );

        const_cast<KIID&>( group->m_Uuid ) = groupInfo.uuid;

        if( groupInfo.libId.IsValid() )
            group->SetDesignBlockLibId( groupInfo.libId );

        aParent->Append( group );
    }

    for( const GROUP_INFO& groupInfo : m_groupInfos )
    {
        SCH_GROUP* group = static_cast<SCH_GROUP*>( getItem( groupInfo.uuid ) );

        if( group && group->Type() == SCH_GROUP_T )
        {
            for( const KIID& aUuid : groupInfo.memberUuids )
            {
                if( SCH_ITEM* gItem = getItem( aUuid ) )
                    group->AddItem( gItem );
            }
        }
    }

    aParent->GroupsSanityCheck( true );
}


void SCH_IO_KICAD_SEXPR_PARSER::skipToBlockEnd( int aDepth )
{
    // Skip tokens until we exit the current S-expression block.
    // This is used for error recovery when parsing fails mid-symbol.
    while( aDepth > 0 )
    {
        T token = NextTok();

        if( token == T_EOF )
            break;
        else if( token == T_LEFT )
            aDepth++;
        else if( token == T_RIGHT )
            aDepth--;
    }
}

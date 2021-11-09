/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_sexpr_parser.cpp
 * @brief Schematic and symbol library s-expression file format parser implementations.
 */

// For some reason wxWidgets is built with wxUSE_BASE64 unset so expose the wxWidgets
// base64 code.
#define wxUSE_BASE64 1
#include <wx/base64.h>
#include <wx/mstream.h>
#include <wx/tokenzr.h>

#include <lib_id.h>
#include <lib_shape.h>
#include <lib_pin.h>
#include <lib_text.h>
#include <math/util.h>                           // KiROUND, Clamp
#include <string_utils.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_symbol.h>
#include <sch_edit_frame.h>          // SYM_ORIENT_XXX
#include <sch_field.h>
#include <sch_line.h>
#include <sch_junction.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_sheet_pin.h>
#include <sch_plugins/kicad/sch_sexpr_parser.h>
#include <template_fieldnames.h>
#include <trigo.h>
#include <progress_reporter.h>


using namespace TSCHEMATIC_T;


SCH_SEXPR_PARSER::SCH_SEXPR_PARSER( LINE_READER* aLineReader, PROGRESS_REPORTER* aProgressReporter,
                                    unsigned aLineCount ) :
    SCHEMATIC_LEXER( aLineReader ),
    m_requiredVersion( 0 ),
    m_fieldId( 0 ),
    m_unit( 1 ),
    m_convert( 1 ),
    m_progressReporter( aProgressReporter ),
    m_lineReader( aLineReader ),
    m_lastProgressLine( 0 ),
    m_lineCount( aLineCount )
{
}


void SCH_SEXPR_PARSER::checkpoint()
{
    const unsigned PROGRESS_DELTA = 250;

    if( m_progressReporter )
    {
        unsigned curLine = m_lineReader->LineNumber();

        if( curLine > m_lastProgressLine + PROGRESS_DELTA )
        {
            m_progressReporter->SetCurrentProgress( ( (double) curLine )
                                                            / std::max( 1U, m_lineCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( ( "Open cancelled by user." ) );

            m_lastProgressLine = curLine;
        }
    }
}


bool SCH_SEXPR_PARSER::parseBool()
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


void SCH_SEXPR_PARSER::ParseLib( LIB_SYMBOL_MAP& aSymbolLibMap )
{
    T token;

    NeedLEFT();
    NextTok();
    parseHeader( T_kicad_symbol_lib, SEXPR_SYMBOL_LIB_FILE_VERSION );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token == T_symbol )
        {
            m_unit = 1;
            m_convert = 1;
            LIB_SYMBOL* symbol = ParseSymbol( aSymbolLibMap, m_requiredVersion );
            aSymbolLibMap[symbol->GetName()] = symbol;
        }
        else
        {
            Expecting( "symbol" );
        }
    }
}


LIB_SYMBOL* SCH_SEXPR_PARSER::ParseSymbol( LIB_SYMBOL_MAP& aSymbolLibMap, int aFileVersion )
{
    wxCHECK_MSG( CurTok() == T_symbol, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a symbol." ) );

    T token;
    long tmp;
    wxString name;
    wxString error;
    LIB_ITEM* item;
    LIB_FIELD* field;
    std::unique_ptr<LIB_SYMBOL> symbol = std::make_unique<LIB_SYMBOL>( wxEmptyString );
    std::set<int> fieldIDsRead;

    m_requiredVersion = aFileVersion;
    symbol->SetUnitCount( 1 );

    m_fieldId = MANDATORY_FIELDS;

    token = NextTok();

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid symbol name" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    name = FromUTF8();

    LIB_ID id;

    if( id.Parse( name ) >= 0 )
    {
        THROW_PARSE_ERROR( _( "Invalid library identifier" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }

    m_symbolName = id.GetLibItemName().wx_str();
    symbol->SetName( m_symbolName );
    symbol->SetLibId( id );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_power:
            symbol->SetPower();
            NeedRIGHT();
            break;

        case T_pin_names:
            parsePinNames( symbol );
            break;

        case T_pin_numbers:
            token = NextTok();

            if( token != T_hide )
                Expecting( "hide" );

            symbol->SetShowPinNumbers( false );
            NeedRIGHT();
            break;

        case T_in_bom:
            symbol->SetIncludeInBom( parseBool() );
            NeedRIGHT();
            break;

        case T_on_board:
            symbol->SetIncludeOnBoard( parseBool() );
            NeedRIGHT();
            break;

        case T_property:
            field = parseProperty( symbol );

            if( field )
            {
                // It would appear that at some point we allowed duplicate ids to slip through
                // when writing files.  The easiest (and most complete) solution is to disallow
                // multiple instances of the same id (for all files since the source of the error
                // *might* in fact be hand-edited files).
                //
                // While no longer used, -1 is still a valid id for user field.  It gets converted
                // to the next unused number on save.
                if( fieldIDsRead.count( field->GetId() ) )
                    field->SetId( -1 );
                else if( field )
                    fieldIDsRead.insert( field->GetId() );
            }

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
            auto it = aSymbolLibMap.find( name );

            if( it == aSymbolLibMap.end() )
            {
                error.Printf( _( "No parent for extended symbol %s" ), name.c_str() );
                THROW_PARSE_ERROR( error, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            symbol->SetParent( it->second );
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
                error.Printf( _( "Invalid symbol convert number %s" ), name.c_str() );
                THROW_PARSE_ERROR( error, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            m_convert = static_cast<int>( tmp );

            if( m_convert > 1 )
                symbol->SetConversion( true, false );

            if( m_unit > symbol->GetUnitCount() )
                symbol->SetUnitCount( m_unit, false );

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_arc:
                case T_bezier:
                case T_circle:
                case T_pin:
                case T_polyline:
                case T_rectangle:
                case T_text:
                    item = ParseDrawItem();

                    wxCHECK_MSG( item, nullptr, "Invalid draw item pointer." );

                    item->SetParent( symbol.get() );
                    symbol->AddDrawItem( item, false );
                    break;

                default:
                    Expecting( "arc, bezier, circle, pin, polyline, rectangle, or text" );
                };
            }

            m_unit = 1;
            m_convert = 1;
            break;
        }

        case T_arc:
        case T_bezier:
        case T_circle:
        case T_pin:
        case T_polyline:
        case T_rectangle:
        case T_text:
            item = ParseDrawItem();

            wxCHECK_MSG( item, nullptr, "Invalid draw item pointer." );

            item->SetParent( symbol.get() );
            symbol->AddDrawItem( item, false );
            break;

        default:
            Expecting( "pin_names, pin_numbers, arc, bezier, circle, pin, polyline, "
                       "rectangle, or text" );
        }
    }

    symbol->GetDrawItems().sort();
    m_symbolName.clear();

    return symbol.release();
}


LIB_ITEM* SCH_SEXPR_PARSER::ParseDrawItem()
{
    switch( CurTok() )
    {
    case T_arc:
        return static_cast<LIB_ITEM*>( parseArc() );
        break;

    case T_bezier:
        return static_cast<LIB_ITEM*>( parseBezier() );
        break;

    case T_circle:
        return static_cast<LIB_ITEM*>( parseCircle() );
        break;

    case T_pin:
        return static_cast<LIB_ITEM*>( parsePin() );
        break;

    case T_polyline:
        return static_cast<LIB_ITEM*>( parsePolyLine() );
        break;

    case T_rectangle:
        return static_cast<LIB_ITEM*>( parseRectangle() );
        break;

    case T_text:
        return static_cast<LIB_TEXT*>( parseText() );
        break;

    default:
        Expecting( "arc, bezier, circle, pin, polyline, rectangle, or text" );
    }

    return nullptr;
}


double SCH_SEXPR_PARSER::parseDouble()
{
    char* tmp;

    // In case the file got saved with the wrong locale.
    if( strchr( CurText(), ',' ) != nullptr )
    {
        THROW_PARSE_ERROR( _( "Floating point number with incorrect locale" ), CurSource(),
                           CurLine(), CurLineNumber(), CurOffset() );
    }

    errno = 0;

    double fval = strtod( CurText(), &tmp );

    if( errno )
    {
        THROW_PARSE_ERROR( _( "Invalid floating point number" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }

    if( CurText() == tmp )
    {
        THROW_PARSE_ERROR( _( "Missing floating point number" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }

    return fval;
}


int SCH_SEXPR_PARSER::parseInternalUnits()
{
    auto retval = parseDouble() * IU_PER_MM;

    // Schematic internal units are represented as integers.  Any values that are
    // larger or smaller than the schematic units represent undefined behavior for
    // the system.  Limit values to the largest that can be displayed on the screen.
    double int_limit = std::numeric_limits<int>::max() * 0.7071; // 0.7071 = roughly 1/sqrt(2)

    return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
}


int SCH_SEXPR_PARSER::parseInternalUnits( const char* aExpected )
{
    auto retval = parseDouble( aExpected ) * IU_PER_MM;

    double int_limit = std::numeric_limits<int>::max() * 0.7071;

    return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
}


void SCH_SEXPR_PARSER::parseStroke( STROKE_PARAMS& aStroke )
{
    wxCHECK_RET( CurTok() == T_stroke,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a stroke." ) );

    aStroke.SetWidth( Mils2iu( DEFAULT_LINE_WIDTH_MILS ) );
    aStroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    aStroke.SetColor( COLOR4D::UNSPECIFIED );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_width:
            aStroke.SetWidth( parseInternalUnits( "stroke width" ) );
            NeedRIGHT();
            break;

        case T_type:
        {
            token = NextTok();

            switch( token )
            {
            case T_dash:      aStroke.SetPlotStyle( PLOT_DASH_TYPE::DASH );      break;
            case T_dot:       aStroke.SetPlotStyle( PLOT_DASH_TYPE::DOT );       break;
            case T_dash_dot:  aStroke.SetPlotStyle( PLOT_DASH_TYPE::DASHDOT );   break;
            case T_solid:     aStroke.SetPlotStyle( PLOT_DASH_TYPE::SOLID );     break;
            case T_default:   aStroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );   break;
            default:
                Expecting( "solid, dash, dash_dot, dot or default" );
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
            color.a = Clamp( parseDouble( "alpha" ), 0.0, 1.0 );

            aStroke.SetColor( color );
            NeedRIGHT();
            break;
        }

        default:
            Expecting( "width, type, or color" );
        }

    }
}


void SCH_SEXPR_PARSER::parseFill( FILL_PARAMS& aFill )
{
    wxCHECK_RET( CurTok() == T_fill,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as fill." ) );

    aFill.m_FillType = FILL_T::NO_FILL;
    aFill.m_Color = COLOR4D::UNSPECIFIED;

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            case T_none:       aFill.m_FillType = FILL_T::NO_FILL;                  break;
            case T_outline:    aFill.m_FillType = FILL_T::FILLED_SHAPE;             break;
            case T_background: aFill.m_FillType = FILL_T::FILLED_WITH_BG_BODYCOLOR; break;
            default:           Expecting( "none, outline, or background" );
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
            color.a = Clamp( parseDouble( "alpha" ), 0.0, 1.0 );
            aFill.m_Color = color;
            NeedRIGHT();
            break;
        }

        default:
            Expecting( "type or color" );
        }
    }
}


void SCH_SEXPR_PARSER::parseEDA_TEXT( EDA_TEXT* aText, bool aConvertOverbarSyntax )
{
    wxCHECK_RET( aText && CurTok() == T_effects,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDA_TEXT." ) );

    // In version 20210606 the notation for overbars was changed from `~...~` to `~{...}`.
    // We need to convert the old syntax to the new one.
    if( aConvertOverbarSyntax && m_requiredVersion < 20210606 )
        aText->SetText( ConvertToNewOverbarNotation( aText->GetText() ) );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_font:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_size:
                {
                    wxSize sz;
                    sz.SetHeight( parseInternalUnits( "text height" ) );
                    sz.SetWidth( parseInternalUnits( "text width" ) );
                    aText->SetTextSize( sz );
                    NeedRIGHT();
                    break;
                }

                case T_thickness:
                    aText->SetTextThickness( parseInternalUnits( "text thickness" ) );
                    NeedRIGHT();
                    break;

                case T_bold:
                    aText->SetBold( true );
                    break;

                case T_italic:
                    aText->SetItalic( true );
                    break;

                default:
                    Expecting( "size, bold, or italic" );
                }
            }

            break;

        case T_justify:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                switch( token )
                {
                case T_left:   aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );  break;
                case T_right:  aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT ); break;
                case T_top:    aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );    break;
                case T_bottom: aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM ); break;
                case T_mirror: aText->SetMirrored( true );                       break;
                default:       Expecting( "left, right, top, bottom, or mirror" );
                }
            }

            break;

        case T_hide:
            aText->SetVisible( false );
            break;

        default:
            Expecting( "font, justify, or hide" );
        }
    }
}


void SCH_SEXPR_PARSER::parseHeader( TSCHEMATIC_T::T aHeaderType, int aFileVersion )
{
    wxCHECK_RET( CurTok() == aHeaderType,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a header." ) );

    NeedLEFT();

    T tok = NextTok();

    if( tok == T_version )
    {
        m_requiredVersion = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );

        if( m_requiredVersion > aFileVersion )
            throw FUTURE_FORMAT_ERROR( FromUTF8() );

        NeedRIGHT();

        // Skip the host name and host build version information.
        NeedLEFT();
        NeedSYMBOL();
        NeedSYMBOL();

        if( m_requiredVersion < 20200827 )
            NeedSYMBOL();

        NeedRIGHT();
    }
    else
    {
        m_requiredVersion = aFileVersion;

        // Skip the host name and host build version information.
        NeedSYMBOL();
        NeedSYMBOL();
        NeedRIGHT();
    }
}


void SCH_SEXPR_PARSER::parsePinNames( std::unique_ptr<LIB_SYMBOL>& aSymbol )
{
    wxCHECK_RET( CurTok() == T_pin_names,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as a pin_name token." ) );

    wxString error;

    T token = NextTok();

    if( token == T_LEFT )
    {
        token = NextTok();

        if( token != T_offset )
            Expecting( "offset" );

        aSymbol->SetPinNameOffset( parseInternalUnits( "pin name offset" ) );
        NeedRIGHT();
        token = NextTok();  // Either ) or hide
    }

    if( token == T_hide )
    {
        aSymbol->SetShowPinNames( false );
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        THROW_PARSE_ERROR( _( "Invalid pin names definition" ), CurSource(), CurLine(),
                           CurLineNumber(), CurOffset() );
    }
}


LIB_FIELD* SCH_SEXPR_PARSER::parseProperty( std::unique_ptr<LIB_SYMBOL>& aSymbol )
{
    wxCHECK_MSG( CurTok() == T_property, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a property." ) );
    wxCHECK( aSymbol, nullptr );

    wxString error;
    wxString name;
    wxString value;
    std::unique_ptr<LIB_FIELD> field = std::make_unique<LIB_FIELD>( aSymbol.get(),
                                                                    MANDATORY_FIELDS );

    T token = NextTok();

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

    field->SetName( name );
    token = NextTok();

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid property value" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    // Empty property values are valid.
    value = FromUTF8();

    field->SetText( value );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_id:
            field->SetId( parseInt( "field ID" ) );
            NeedRIGHT();
            break;

        case T_at:
            field->SetPosition( parseXY() );
            field->SetTextAngle( static_cast<int>( parseDouble( "text angle" ) * 10.0 ) );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( field.get() ), field->GetId() == VALUE_FIELD );
            break;

        default:
            Expecting( "id, at or effects" );
        }
    }

    LIB_FIELD* existingField;

    if( field->GetId() < MANDATORY_FIELDS )
    {
        existingField = aSymbol->GetFieldById( field->GetId() );

        *existingField = *field;
        return existingField;
    }
    else if( name == "ki_keywords" )
    {
        // Not a LIB_FIELD object yet.
        aSymbol->SetKeyWords( value );
        return nullptr;
    }
    else if( name == "ki_description" )
    {
        // Not a LIB_FIELD object yet.
        aSymbol->SetDescription( value );
        return nullptr;
    }
    else if( name == "ki_fp_filters" )
    {
        // Not a LIB_FIELD object yet.
        wxArrayString filters;
        wxStringTokenizer tokenizer( value );

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
        // This is a temporary LIB_FIELD object until interchangeable units are determined on
        // the fly.
        aSymbol->LockUnits( true );
        return nullptr;
    }
    else
    {
        // At this point, a user field is read.
        existingField = aSymbol->FindField( field->GetCanonicalName() );

#if 1   // Enable it to modify the name of the field to add if already existing
        // Disable it to skip the field having the same name as previous field
        if( existingField )
        {
            // We cannot handle 2 fields with the same name, so because the field name
            // is already in use, try to build a new name (oldname_x)
            wxString base_name = field->GetCanonicalName();

            // Arbitrary limit 10 attempts to find a new name
            for( int ii = 1; ii < 10 && existingField ; ii++ )
            {
                wxString newname = base_name;
                newname << '_' << ii;

                existingField = aSymbol->FindField( newname );

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


LIB_SHAPE* SCH_SEXPR_PARSER::parseArc()
{
    wxCHECK_MSG( CurTok() == T_arc, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as an arc." ) );

    T             token;
    wxPoint       startPoint( 1, 0 );   // Initialize to a non-degenerate arc just for safety
    wxPoint       midPoint( 1, 1 );
    wxPoint       endPoint( 0, 1 );
    bool          hasMidPoint = false;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    FILL_PARAMS   fill;

    // Parameters for legacy format
    wxPoint       center( 0, 0 );
    int           startAngle = 0;
    int           endAngle = 900;
    bool          hasAngles = false;

    std::unique_ptr<LIB_SHAPE> arc = std::make_unique<LIB_SHAPE>( nullptr, SHAPE_T::ARC );

    arc->SetUnit( m_unit );
    arc->SetConvert( m_convert );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            hasMidPoint = true;
            break;

        case T_end:
            endPoint = parseXY();
            NeedRIGHT();
            break;

        case T_radius:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_at:
                    center = parseXY();
                    NeedRIGHT();
                    break;

                case T_length:
                    parseInternalUnits( "radius length" );
                    NeedRIGHT();
                    break;

                case T_angles:
                {
                    startAngle = KiROUND( parseDouble( "start radius angle" ) * 10.0 );
                    endAngle = KiROUND( parseDouble( "end radius angle" ) * 10.0 );
                    NORMALIZE_ANGLE_POS( startAngle );
                    NORMALIZE_ANGLE_POS( endAngle );
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
            arc->SetWidth( stroke.GetWidth() );
            break;

        case T_fill:
            parseFill( fill );
            arc->SetFillMode( fill.m_FillType );
            break;

        default:
            Expecting( "start, mid, end, radius, stroke, or fill" );
        }
    }

    arc->SetStart( startPoint );
    arc->SetEnd( endPoint );

    if( hasMidPoint )
    {
        arc->SetCenter( (wxPoint) CalcArcCenter( arc->GetStart(), midPoint, arc->GetEnd() ) );
    }
    else if( hasAngles )
    {
        /**
         * This accounts for an oddity in the old library format, where the symbol is overdefined.
         * The previous draw (based on wxwidgets) used start point and end point and always drew
         * counter-clockwise.  The new GAL draw takes center, radius and start/end angles.  All of
         * these points were stored in the file, so we need to mimic the swapping of start/end
         * points rather than using the stored angles in order to properly map edge cases.
         */
        if( !TRANSFORM().MapAngles( &startAngle, &endAngle ) )
        {
            wxPoint temp = arc->GetStart();
            arc->SetStart( arc->GetEnd() );
            arc->SetEnd( temp );
        }
        arc->SetCenter( center );
    }
    else
    {
        wxFAIL_MSG( "Setting arc without either midpoint or angles not implemented." );
    }

    return arc.release();
}


LIB_SHAPE* SCH_SEXPR_PARSER::parseBezier()
{
    wxCHECK_MSG( CurTok() == T_bezier, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bezier." ) );

    T             token;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    FILL_PARAMS   fill;

    std::unique_ptr<LIB_SHAPE> bezier = std::make_unique<LIB_SHAPE>( nullptr, SHAPE_T::BEZIER );

    bezier->SetUnit( m_unit );
    bezier->SetConvert( m_convert );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            bezier->SetWidth( stroke.GetWidth() );
            break;

        case T_fill:
            parseFill( fill );
            bezier->SetFillMode( fill.m_FillType );
            break;

        default:
            Expecting( "pts, stroke, or fill" );
        }
    }

    bezier->RebuildBezierToSegmentsPointsList( bezier->GetWidth() );

    return bezier.release();
}


LIB_SHAPE* SCH_SEXPR_PARSER::parseCircle()
{
    wxCHECK_MSG( CurTok() == T_circle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a circle." ) );

    T             token;
    wxPoint       center( 0, 0 );
    int           radius = 1;     // defaulting to 0 could result in troublesome math....
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    FILL_PARAMS   fill;

    std::unique_ptr<LIB_SHAPE> circle = std::make_unique<LIB_SHAPE>( nullptr, SHAPE_T::CIRCLE );

    circle->SetUnit( m_unit );
    circle->SetConvert( m_convert );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            circle->SetWidth( stroke.GetWidth() );
            break;

        case T_fill:
            parseFill( fill );
            circle->SetFillMode( fill.m_FillType );
            break;

        default:
            Expecting( "center, radius, stroke, or fill" );
        }
    }

    circle->SetCenter( center );
    circle->SetEnd( wxPoint( center.x + radius, center.y ) );

    return circle.release();
}


LIB_PIN* SCH_SEXPR_PARSER::parsePin()
{
    auto parseType = [&]( T token ) -> ELECTRICAL_PINTYPE
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
                             Expecting( "input, output, bidirectional, tri_state, passive, "
                                        "unspecified, power_in, power_out, open_collector, "
                                        "open_emitter, free or no_connect" );
                             return ELECTRICAL_PINTYPE::PT_UNSPECIFIED;
                         }
                     };

    auto parseShape = [&]( T token ) -> GRAPHIC_PINSHAPE
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
                              Expecting( "line, inverted, clock, inverted_clock, input_low, "
                                         "clock_low, output_low, edge_clock_high, non_logic" );
                              return GRAPHIC_PINSHAPE::LINE;
                          }
                      };

    wxCHECK_MSG( CurTok() == T_pin, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a pin token." ) );

    T token;
    wxString tmp;
    wxString error;
    std::unique_ptr<LIB_PIN> pin = std::make_unique<LIB_PIN>( nullptr );

    pin->SetUnit( m_unit );
    pin->SetConvert( m_convert );

    // Pin electrical type.
    token = NextTok();
    pin->SetType( parseType( token ) );

    // Pin shape.
    token = NextTok();
    pin->SetShape( parseShape( token ) );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
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
            pin->SetPosition( parseXY() );

            switch( parseInt( "pin orientation" ) )
            {
            case 0:   pin->SetOrientation( PIN_RIGHT ); break;
            case 90:  pin->SetOrientation( PIN_UP );    break;
            case 180: pin->SetOrientation( PIN_LEFT );  break;
            case 270: pin->SetOrientation( PIN_DOWN );  break;
            default:  Expecting( "0, 90, 180, or 270" );
            }

            NeedRIGHT();
            break;

        case T_length:
            pin->SetLength( parseInternalUnits( "pin length" ) );
            NeedRIGHT();
            break;

        case T_name:
            token = NextTok();

            if( !IsSymbol( token ) )
            {
                THROW_PARSE_ERROR( _( "Invalid pin name" ), CurSource(), CurLine(), CurLineNumber(),
                                   CurOffset() );
            }

            if( m_requiredVersion < 20210606 )
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
                    EDA_TEXT text;

                    parseEDA_TEXT( &text, true );
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

            pin->SetNumber( FromUTF8() );
            token = NextTok();

            if( token != T_RIGHT )
            {
                token = NextTok();

                if( token == T_effects )
                {
                    // The EDA_TEXT font effects formatting is used so use and EDA_TEXT object
                    // so duplicate parsing is not required.
                    EDA_TEXT text;

                    parseEDA_TEXT( &text, false );
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
            LIB_PIN::ALT alt;

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
        }
            break;

        default:
            Expecting( "at, name, number, length, or alternate" );
        }
    }

    return pin.release();
}


LIB_SHAPE* SCH_SEXPR_PARSER::parsePolyLine()
{
    wxCHECK_MSG( CurTok() == T_polyline, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a poly." ) );

    T token;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    FILL_PARAMS fill;
    std::unique_ptr<LIB_SHAPE> poly = std::make_unique<LIB_SHAPE>( nullptr, SHAPE_T::POLY );

    poly->SetUnit( m_unit );
    poly->SetConvert( m_convert );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_pts:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_xy )
                    Expecting( "xy" );

                poly->AddPoint( parseXY() );

                NeedRIGHT();
            }

            break;

        case T_stroke:
            parseStroke( stroke );
            poly->SetWidth( stroke.GetWidth() );
            break;

        case T_fill:
            parseFill( fill );
            poly->SetFillMode( fill.m_FillType );
            break;

        default:
            Expecting( "pts, stroke, or fill" );
        }
    }

    return poly.release();
}


LIB_SHAPE* SCH_SEXPR_PARSER::parseRectangle()
{
    wxCHECK_MSG( CurTok() == T_rectangle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a rectangle." ) );

    T token;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    FILL_PARAMS fill;
    std::unique_ptr<LIB_SHAPE> rectangle = std::make_unique<LIB_SHAPE>( nullptr, SHAPE_T::RECT );

    rectangle->SetUnit( m_unit );
    rectangle->SetConvert( m_convert );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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

        case T_stroke:
            parseStroke( stroke );
            rectangle->SetWidth( stroke.GetWidth() );
            break;

        case T_fill:
            parseFill( fill );
            rectangle->SetFillMode( fill.m_FillType );
            break;

        default:
            Expecting( "start, end, stroke, or fill" );
        }
    }

    return rectangle.release();
}


LIB_TEXT* SCH_SEXPR_PARSER::parseText()
{
    wxCHECK_MSG( CurTok() == T_text, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a text token." ) );

    T token;
    wxString tmp;
    std::unique_ptr<LIB_TEXT> text = std::make_unique<LIB_TEXT>( nullptr );

    text->SetUnit( m_unit );
    text->SetConvert( m_convert );
    token = NextTok();

    if( !IsSymbol( token ) )
    {
        THROW_PARSE_ERROR( _( "Invalid text string" ), CurSource(), CurLine(), CurLineNumber(),
                           CurOffset() );
    }

    text->SetText( FromUTF8() );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            text->SetPosition( parseXY() );
            text->SetTextAngle( parseDouble( "text angle" ) );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( text.get() ), true );
            break;

        default:
            Expecting( "at or effects" );
        }
    }

    return text.release();
}


void SCH_SEXPR_PARSER::parsePAGE_INFO( PAGE_INFO& aPageInfo )
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

    if( pageType == PAGE_INFO::Custom )
    {
        int width = Mm2mils( parseDouble( "width" ) ); // width stored in mm so we convert to mils

        // Perform some controls to avoid crashes if the size is edited by hands
        if( width < MIN_PAGE_SIZE_MILS )
            width = MIN_PAGE_SIZE_MILS;
        else if( width > MAX_PAGE_SIZE_MILS )
            width = MAX_PAGE_SIZE_MILS;

        int height = Mm2mils( parseDouble( "height" ) ); // height stored in mm so we convert to mils

        if( height < MIN_PAGE_SIZE_MILS )
            height = MIN_PAGE_SIZE_MILS;
        else if( height > MAX_PAGE_SIZE_MILS )
            height = MAX_PAGE_SIZE_MILS;

        aPageInfo.SetWidthMils( width );
        aPageInfo.SetHeightMils( height );
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


void SCH_SEXPR_PARSER::parseTITLE_BLOCK( TITLE_BLOCK& aTitleBlock )
{
    wxCHECK_RET( CurTok() == T_title_block,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as TITLE_BLOCK." ) );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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


SCH_FIELD* SCH_SEXPR_PARSER::parseSchField( SCH_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_property, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as a property token." ) );

    T token = NextTok();

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
    wxString value = FromUTF8();

    std::unique_ptr<SCH_FIELD> field = std::make_unique<SCH_FIELD>( wxDefaultPosition, -1,
                                                                    aParent, name );

    field->SetText( value );
    field->SetVisible( true );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_id:
            field->SetId( parseInt( "field ID" ) );
            NeedRIGHT();
            break;

        case T_at:
            field->SetPosition( parseXY() );
            field->SetTextAngle( static_cast<int>( parseDouble( "text angle" ) * 10.0 ) );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( field.get() ), field->GetId() == VALUE_FIELD );
            break;

        default:
            Expecting( "at or effects" );
        }
    }

    return field.release();
}


SCH_SHEET_PIN* SCH_SEXPR_PARSER::parseSchSheetPin( SCH_SHEET* aSheet )
{
    wxCHECK_MSG( aSheet != nullptr, nullptr, "" );
    wxCHECK_MSG( CurTok() == T_pin, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as a sheet pin token." ) );

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

    auto sheetPin = std::make_unique<SCH_SHEET_PIN>( aSheet, wxPoint( 0, 0 ), name );

    token = NextTok();

    switch( token )
    {
    case T_input:          sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );        break;
    case T_output:         sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );       break;
    case T_bidirectional:  sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );         break;
    case T_tri_state:      sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_TRISTATE );     break;
    case T_passive:        sheetPin->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );  break;
    default:
        Expecting( "input, output, bidirectional, tri_state, or passive" );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
                sheetPin->SetEdge( SHEET_SIDE::RIGHT );
            else if( angle == 90.0 )
                sheetPin->SetEdge( SHEET_SIDE::TOP );
            else if( angle == 180.0 )
                sheetPin->SetEdge( SHEET_SIDE::LEFT );
            else if( angle == 270.0 )
                sheetPin->SetEdge( SHEET_SIDE::BOTTOM );
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
            const_cast<KIID&>( sheetPin->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "at, uuid or effects" );
        }
    }

    return sheetPin.release();
}


void SCH_SEXPR_PARSER::parseSchSheetInstances( SCH_SHEET* aRootSheet, SCH_SCREEN* aScreen )
{
    wxCHECK_RET( CurTok() == T_sheet_instances,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as a instances token." ) );
    wxCHECK( aScreen, /* void */ );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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

                    // Whitespaces are not permitted
                    for( wxString ch : whitespaces )
                        numReplacements += instance.m_PageNumber.Replace( ch, wxEmptyString );


                    // Empty page numbers are not permitted
                    if( instance.m_PageNumber.IsEmpty() )
                    {
                        // Use hash character instead
                        instance.m_PageNumber = wxT( "#" );
                        numReplacements++;
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

            aScreen->m_sheetInstances.emplace_back( instance );
            break;
        }

        default:
            Expecting( "path" );
        }
    }
}


void SCH_SEXPR_PARSER::parseSchSymbolInstances( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( CurTok() == T_symbol_instances,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as a instances token." ) );
    wxCHECK( aScreen, /* void */ );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_path:
        {
            NeedSYMBOL();

            SYMBOL_INSTANCE_REFERENCE instance;

            instance.m_Path = KIID_PATH( FromUTF8() );

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
                    instance.m_Value = FromUTF8();
                    NeedRIGHT();
                    break;

                case T_footprint:
                    NeedSYMBOL();
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


void SCH_SEXPR_PARSER::ParseSchematic( SCH_SHEET* aSheet, bool aIsCopyableOnly, int aFileVersion )
{
    wxCHECK( aSheet != nullptr, /* void */ );

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen != nullptr, /* void */ );

    if( aIsCopyableOnly )
        m_requiredVersion = aFileVersion;

    T token;

    if( !aIsCopyableOnly )
    {
        NeedLEFT();
        NextTok();

        if( CurTok() != T_kicad_sch )
            Expecting( "kicad_sch" );

        parseHeader( T_kicad_sch, SEXPR_SCHEMATIC_FILE_VERSION );
    }

    screen->SetFileFormatVersionAtLoad( m_requiredVersion );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
        case T_uuid:
            NeedSYMBOL();
            screen->m_uuid = KIID( FromUTF8() );
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

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_symbol:
                    screen->AddLibSymbol( ParseSymbol( symbolLibMap, m_requiredVersion ) );
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
            screen->Append( static_cast<SCH_ITEM*>( sheet ) );
            break;
        }

        case T_junction:
            screen->Append( static_cast<SCH_ITEM*>( parseJunction() ) );
            break;

        case T_no_connect:
            screen->Append( static_cast<SCH_ITEM*>( parseNoConnect() ) );
            break;

        case T_bus_entry:
            screen->Append( static_cast<SCH_ITEM*>( parseBusEntry() ) );
            break;

        case T_polyline:
        case T_bus:
        case T_wire:
            screen->Append( static_cast<SCH_ITEM*>( parseLine() ) );
            break;

        case T_text:
        case T_label:
        case T_global_label:
        case T_hierarchical_label:
            screen->Append( static_cast<SCH_ITEM*>( parseSchText() ) );
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

        default:
            Expecting( "symbol, paper, page, title_block, bitmap, sheet, junction, no_connect, "
                       "bus_entry, line, bus, text, label, global_label, hierarchical_label, "
                       "symbol_instances, or bus_alias" );
        }
    }

    screen->UpdateLocalLibSymbolLinks();
}


SCH_SYMBOL* SCH_SEXPR_PARSER::parseSchematicSymbol()
{
    wxCHECK_MSG( CurTok() == T_symbol, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a symbol." ) );

    T token;
    wxString tmp;
    wxString libName;
    SCH_FIELD* field;
    std::unique_ptr<SCH_SYMBOL> symbol = std::make_unique<SCH_SYMBOL>();
    TRANSFORM transform;
    std::set<int> fieldIDsRead;

    // We'll reset this if we find a fields_autoplaced token
    symbol->ClearFieldsAutoplaced();

    m_fieldId = MANDATORY_FIELDS;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            NeedRIGHT();
            break;
        }

        case T_lib_id:
        {
            token = NextTok();

            if( !IsSymbol( token ) && token != T_NUMBER )
                Expecting( "symbol|number" );

            LIB_ID libId;

            if( libId.Parse( FromUTF8() ) >= 0 )
            {
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
            case 90:   transform = TRANSFORM( 0, -1, -1, 0 );   break;
            case 180:  transform = TRANSFORM( -1, 0, 0, 1 );    break;
            case 270:  transform = TRANSFORM( 0, 1, 1, 0 );     break;
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

        case T_convert:
            symbol->SetConvert( parseInt( "symbol convert" ) );
            NeedRIGHT();
            break;

        case T_in_bom:
            symbol->SetIncludeInBom( parseBool() );
            NeedRIGHT();
            break;

        case T_on_board:
            symbol->SetIncludeOnBoard( parseBool() );
            NeedRIGHT();
            break;

        case T_fields_autoplaced:
            symbol->SetFieldsAutoplaced();
            NeedRIGHT();
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( symbol->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        case T_property:
            // The field parent symbol must be set and its orientation must be set before
            // the field positions are set.
            field = parseSchField( symbol.get() );

            // It would appear that at some point we allowed duplicate ids to slip through
            // when writing files.  The easiest (and most complete) solution is to disallow
            // multiple instances of the same id (for all files since the source of the error
            // *might* in fact be hand-edited files).
            //
            // While no longer used, -1 is still a valid id for user field.  It gets converted
            // to the next unused number on save.
            if( fieldIDsRead.count( field->GetId() ) )
                field->SetId( -1 );
            else
                fieldIDsRead.insert( field->GetId() );

            // Set the default symbol reference prefix.
            if( field->GetId() == REFERENCE_FIELD )
            {
                wxString refDesignator = field->GetText();

                refDesignator.Replace( "~", " " );

                wxString prefix = refDesignator;

                while( prefix.Length() )
                {
                    if( ( prefix.Last() < '0' || prefix.Last() > '9') && prefix.Last() != '?' )
                        break;

                    prefix.RemoveLast();
                }

                // Avoid a prefix containing trailing/leading spaces
                prefix.Trim( true );
                prefix.Trim( false );

                if( prefix.IsEmpty() )
                    symbol->SetPrefix( wxString( "U" ) );
                else
                    symbol->SetPrefix( prefix );
            }

            if( symbol->GetFieldById( field->GetId() ) )
                *symbol->GetFieldById( field->GetId() ) = *field;
            else
                symbol->AddField( *field );

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

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
                        uuid = KIID( FromUTF8() );

                    NeedRIGHT();
                    break;

                default:
                    Expecting( "alternate or uuid" );
                }
            }

            symbol->GetRawPins().emplace_back( std::make_unique<SCH_PIN>( symbol.get(),
                                                                          number, alt ) );

            const_cast<KIID&>( symbol->GetRawPins().back()->m_Uuid ) = uuid;
        }
            break;

        default:
            Expecting( "lib_id, lib_name, at, mirror, uuid, property, pin, or instances" );
        }
    }

    if( !libName.IsEmpty() && ( symbol->GetLibId().Format().wx_str() != libName ) )
        symbol->SetSchSymbolLibraryName( libName );

    // Ensure edit/status flags are cleared after these initializations:
    symbol->ClearFlags();

    return symbol.release();
}


SCH_BITMAP* SCH_SEXPR_PARSER::parseImage()
{
    wxCHECK_MSG( CurTok() == T_image, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as an image." ) );

    T token;
    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            bitmap->GetImage()->SetScale( parseDouble( "image scale factor" ) );

            if( !std::isnormal( bitmap->GetImage()->GetScale() ) )
                bitmap->GetImage()->SetScale( 1.0 );

            NeedRIGHT();
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( bitmap->m_Uuid ) = KIID( FromUTF8() );
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

            wxMemoryBuffer buffer = wxBase64Decode( data );
            wxMemoryOutputStream stream( buffer.GetData(), buffer.GetBufSize() );
            wxImage* image = new wxImage();
            wxMemoryInputStream istream( stream );
            image->LoadFile( istream, wxBITMAP_TYPE_PNG );
            bitmap->GetImage()->SetImage( image );
            bitmap->GetImage()->SetBitmap( new wxBitmap( *image ) );
            break;
        }

        default:
            Expecting( "at, scale, uuid or data" );
        }
    }

    return bitmap.release();
}


SCH_SHEET* SCH_SEXPR_PARSER::parseSheet()
{
    wxCHECK_MSG( CurTok() == T_sheet, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a sheet." ) );

    T token;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    FILL_PARAMS fill;
    SCH_FIELD* field;
    std::vector<SCH_FIELD> fields;
    std::unique_ptr<SCH_SHEET> sheet = std::make_unique<SCH_SHEET>();
    std::set<int> fieldIDsRead;

    // We'll reset this if we find a fields_autoplaced token
    sheet->ClearFieldsAutoplaced();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            wxSize size;
            size.SetWidth( parseInternalUnits( "sheet width" ) );
            size.SetHeight( parseInternalUnits( "sheet height" ) );
            sheet->SetSize( size );
            NeedRIGHT();
            break;
        }

        case T_fields_autoplaced:
            sheet->SetFieldsAutoplaced();
            NeedRIGHT();
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
            const_cast<KIID&>( sheet->m_Uuid ) = KIID( FromUTF8() );
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
                    field->SetId( SHEETNAME );
                else
                    field->SetId( SHEETFILENAME );
            }

            // It would appear the problem persists past 20200310, but this time with the
            // earlier ids being re-used for later (user) fields.  The easiest (and most
            // complete) solution is to disallow multiple instances of the same id (for all
            // files since the source of the error *might* in fact be hand-edited files).
            //
            // While no longer used, -1 is still a valid id for user field.  It gets converted
            // to the next unused number on save.
            if( fieldIDsRead.count( field->GetId() ) )
                field->SetId( -1 );
            else
                fieldIDsRead.insert( field->GetId() );

            fields.emplace_back( *field );
            delete field;
            break;

        case T_pin:
            sheet->AddPin( parseSchSheetPin( sheet.get() ) );
            break;

        default:
            Expecting( "at, size, stroke, background, uuid, property, or pin" );
        }
    }

    sheet->SetFields( fields );

    return sheet.release();
}


SCH_JUNCTION* SCH_SEXPR_PARSER::parseJunction()
{
    wxCHECK_MSG( CurTok() == T_junction, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a junction." ) );

    T token;
    std::unique_ptr<SCH_JUNCTION> junction = std::make_unique<SCH_JUNCTION>();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            color.a = Clamp( parseDouble( "alpha" ), 0.0, 1.0 );

            junction->SetColor( color );
            NeedRIGHT();
            break;
        }

        default:
            Expecting( "at" );
        }
    }

    return junction.release();
}


SCH_NO_CONNECT* SCH_SEXPR_PARSER::parseNoConnect()
{
    wxCHECK_MSG( CurTok() == T_no_connect, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a no connect." ) );

    T token;
    std::unique_ptr<SCH_NO_CONNECT> no_connect = std::make_unique<SCH_NO_CONNECT>();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            const_cast<KIID&>( no_connect->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "at or uuid" );
        }
    }

    return no_connect.release();
}


SCH_BUS_WIRE_ENTRY* SCH_SEXPR_PARSER::parseBusEntry()
{
    wxCHECK_MSG( CurTok() == T_bus_entry, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bus entry." ) );

    T token;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    std::unique_ptr<SCH_BUS_WIRE_ENTRY> busEntry = std::make_unique<SCH_BUS_WIRE_ENTRY>();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            wxSize size;

            size.SetWidth( parseInternalUnits( "bus entry height" ) );
            size.SetHeight( parseInternalUnits( "bus entry width" ) );
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
            const_cast<KIID&>( busEntry->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "at, size, uuid or stroke" );
        }
    }

    return busEntry.release();
}


SCH_LINE* SCH_SEXPR_PARSER::parseLine()
{
    T token;
    STROKE_PARAMS stroke( Mils2iu( DEFAULT_LINE_WIDTH_MILS ), PLOT_DASH_TYPE::DEFAULT );
    std::unique_ptr<SCH_LINE> line = std::make_unique<SCH_LINE>();

    switch( CurTok() )
    {
    case T_polyline:   line->SetLayer( LAYER_NOTES );   break;
    case T_wire:       line->SetLayer( LAYER_WIRE );    break;
    case T_bus:        line->SetLayer( LAYER_BUS );     break;
    default:
        wxCHECK_MSG( false, nullptr,
                     wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a line." ) );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            const_cast<KIID&>( line->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        default:
            Expecting( "at, uuid or stroke" );
        }
    }

    return line.release();
}


SCH_TEXT* SCH_SEXPR_PARSER::parseSchText()
{
    T token;
    std::unique_ptr<SCH_TEXT> text;

    switch( CurTok() )
    {
    case T_text:                text = std::make_unique<SCH_TEXT>();          break;
    case T_label:               text = std::make_unique<SCH_LABEL>();         break;
    case T_global_label:        text = std::make_unique<SCH_GLOBALLABEL>();   break;
    case T_hierarchical_label:  text = std::make_unique<SCH_HIERLABEL>();     break;
    default:
        wxCHECK_MSG( false, nullptr, "Cannot parse " + GetTokenString( CurTok() ) + " as text." );
    }

    // We'll reset this if we find a fields_autoplaced token
    text->ClearFieldsAutoplaced();

    NeedSYMBOL();

    text->SetText( FromUTF8() );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            text->SetPosition( parseXY() );

            switch( static_cast<int>( parseDouble( "text angle" ) ) )
            {
            case 0:    text->SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT );    break;
            case 90:   text->SetLabelSpinStyle( LABEL_SPIN_STYLE::UP );       break;
            case 180:  text->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );     break;
            case 270:  text->SetLabelSpinStyle( LABEL_SPIN_STYLE::BOTTOM );   break;
            default:
                wxFAIL;
                text->SetLabelSpinStyle( LABEL_SPIN_STYLE::RIGHT );
                break;
            }

            NeedRIGHT();
            break;

        case T_shape:
            if( text->Type() == SCH_TEXT_T || text->Type() == SCH_LABEL_T )
                Unexpected( T_shape );

            token = NextTok();

            switch( token )
            {
            case T_input:          text->SetShape( PINSHEETLABEL_SHAPE::PS_INPUT );        break;
            case T_output:         text->SetShape( PINSHEETLABEL_SHAPE::PS_OUTPUT );       break;
            case T_bidirectional:  text->SetShape( PINSHEETLABEL_SHAPE::PS_BIDI );         break;
            case T_tri_state:      text->SetShape( PINSHEETLABEL_SHAPE::PS_TRISTATE );     break;
            case T_passive:        text->SetShape( PINSHEETLABEL_SHAPE::PS_UNSPECIFIED );  break;
            default:
                Expecting( "input, output, bidirectional, tri_state, or passive" );
            }

            NeedRIGHT();
            break;

        case T_fields_autoplaced:
            text->SetFieldsAutoplaced();
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( text.get() ), true );

            // Spin style is defined differently for graphical text (#SCH_TEXT) objects.
            if( text->Type() == SCH_TEXT_T )
            {
                if( text->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT
                  && text->GetTextAngle() == TEXT_ANGLE_VERT )
                {
                    // The vertically aligned text angle is always 90 (labels use 270 for the
                    // down direction) combined with the text justification flags.
                    text->SetLabelSpinStyle( LABEL_SPIN_STYLE::BOTTOM );
                }
                else if( text->GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT
                       && text->GetTextAngle() == TEXT_ANGLE_HORIZ )
                {
                    // The horizontally aligned text angle is always 0 (labels use 180 for the
                    // left direction) combined with the text justification flags.
                    text->SetLabelSpinStyle( LABEL_SPIN_STYLE::LEFT );
                }
            }

            break;

        case T_iref:    // legacy format; current is a T_property (aka SCH_FIELD)
            if( text->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( text.get() );
                SCH_FIELD*       field = label->GetIntersheetRefs();

                field->SetTextPos( parseXY() );
                NeedRIGHT();

                field->SetVisible( true );
            }
            break;

        case T_uuid:
            NeedSYMBOL();
            const_cast<KIID&>( text->m_Uuid ) = KIID( FromUTF8() );
            NeedRIGHT();
            break;

        case T_property:
            if( text->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_GLOBALLABEL* label = static_cast<SCH_GLOBALLABEL*>( text.get() );
                SCH_FIELD*       field = parseSchField( label );

                field->SetLayer( LAYER_GLOBLABEL );
                label->SetIntersheetRefs( *field );

                delete field;
            }
            break;

        default:
            Expecting( "at, shape, iref, uuid or effects" );
        }
    }

    return text.release();
}


void SCH_SEXPR_PARSER::parseBusAlias( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( CurTok() == T_bus_alias,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bus alias." ) );
    wxCHECK( aScreen, /* void */ );

    T token;
    std::shared_ptr<BUS_ALIAS> busAlias = std::make_shared<BUS_ALIAS>( aScreen );
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

        busAlias->AddMember( member );

        token = NextTok();
    }

    NeedRIGHT();

    aScreen->AddBusAlias( busAlias );
}

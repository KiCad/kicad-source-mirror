/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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
 * @file sch_sexpr_parser.cpp
 * @brief Schematic and symbol library s-expression file format parser implementations.
 */

#include <wx/tokenzr.h>

#include <class_libentry.h>
#include <lib_arc.h>
#include <lib_bezier.h>
#include <lib_circle.h>
#include <lib_pin.h>
#include <lib_polyline.h>
#include <lib_rectangle.h>
#include <lib_text.h>
#include <sch_file_versions.h>
#include <sch_sexpr_parser.h>
#include <template_fieldnames.h>


using namespace TSYMBOL_LIB_T;


SCH_SEXPR_PARSER::SCH_SEXPR_PARSER( LINE_READER* aLineReader ) :
    SYMBOL_LIB_LEXER( aLineReader ),
    m_requiredVersion( 0 ),
    m_unit( 1 ),
    m_convert( 1 )
{
}


bool SCH_SEXPR_PARSER::IsTooRecent() const
{
    return m_requiredVersion && m_requiredVersion > SEXPR_SYMBOL_LIB_FILE_VERSION;
}


void SCH_SEXPR_PARSER::ParseLib( LIB_PART_MAP& aSymbolLibMap )
{
    T token;

    NeedLEFT();
    NextTok();
    parseHeader();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token == T_symbol )
        {
            m_unit = 1;
            m_convert = 1;
            LIB_PART* symbol = ParseSymbol( aSymbolLibMap );
            aSymbolLibMap[symbol->GetName()] = symbol;
        }
        else
        {
            Expecting( "symbol" );
        }
    }
}


LIB_PART* SCH_SEXPR_PARSER::ParseSymbol( LIB_PART_MAP& aSymbolLibMap )
{
    wxCHECK_MSG( CurTok() == T_symbol, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a symbol." ) );

    T token;
    long tmp;
    wxString error;
    wxString name;
    LIB_ITEM* item;
    std::unique_ptr<LIB_PART> symbol( new LIB_PART( wxEmptyString ) );

    symbol->SetUnitCount( 1 );

    m_fieldId = MANDATORY_FIELDS;

    token = NextTok();

    if( !IsSymbol( token ) )
    {
        error.Printf( _( "Invalid symbol name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    name = FromUTF8();

    if( name.IsEmpty() )
    {
        error.Printf( _( "Empty symbol name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    m_symbolName = name;
    symbol->SetName( name );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
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

        case T_property:
            parseProperty( symbol );
            break;

        case T_extends:
        {
            token = NextTok();

            if( !IsSymbol( token ) )
            {
                error.Printf(
                    _( "Invalid symbol extends name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            name = FromUTF8();
            auto it = aSymbolLibMap.find( name );

            if( it == aSymbolLibMap.end() )
            {
                error.Printf(
                    _( "No parent for extended symbol %s in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    name.c_str(), CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
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
                error.Printf(
                    _( "Invalid symbol unit name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            name = FromUTF8();

            if( !name.StartsWith( m_symbolName ) )
            {
                error.Printf(
                    _( "Invalid symbol unit name prefix %s in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    name.c_str(), CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            name = name.Right( name.Length() - m_symbolName.Length() - 1 );

            wxStringTokenizer tokenizer( name, "_" );

            if( tokenizer.CountTokens() != 2 )
            {
                error.Printf(
                    _( "Invalid symbol unit name suffix %s in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    name.c_str(), CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            if( !tokenizer.GetNextToken().ToLong( &tmp ) )
            {
                error.Printf(
                    _( "Invalid symbol unit number %s in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    name.c_str(), CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            m_unit = static_cast<int>( tmp );

            if( !tokenizer.GetNextToken().ToLong( &tmp ) )
            {
                error.Printf(
                    _( "Invalid symbol convert number %s in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                    name.c_str(), CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
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
                    symbol->AddDrawItem( item );
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
            symbol->AddDrawItem( item );
            break;

        default:
            Expecting( "pin_names, pin_numbers, arc, bezier, circle, pin, polyline, "
                       "rectangle, or text" );
        }
    }

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

    errno = 0;

    double fval = strtod( CurText(), &tmp );

    if( errno )
    {
        wxString error;
        error.Printf( _( "Invalid floating point number in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    if( CurText() == tmp )
    {
        wxString error;
        error.Printf( _( "Missing floating point number in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    return fval;
}


FILL_T SCH_SEXPR_PARSER::parseFillMode()
{
    wxCHECK_MSG( CurTok() == T_fill, NO_FILL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as fill." ) );

    NeedLEFT();
    T token = NextTok();

    if( token != T_type )
        Expecting( "type" );

    token = NextTok();

    FILL_T fillType = NO_FILL;

    switch( token )
    {
    case T_none:
        fillType = NO_FILL;
        break;

    case T_outline:
        fillType = FILLED_SHAPE;
        break;

    case T_background:
        fillType = FILLED_WITH_BG_BODYCOLOR;
        break;

    default:
        Expecting( "none, outline, or background" );
    }

    NeedRIGHT();  // Closes type token.
    NeedRIGHT();  // Closes fill token.

    return fillType;
}


void SCH_SEXPR_PARSER::parseEDA_TEXT( EDA_TEXT* aText )
{
    wxCHECK_RET( aText && CurTok() == T_effects,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDA_TEXT." ) );

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
                    aText->SetThickness( parseInternalUnits( "text thickness" ) );
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
                case T_left:
                    aText->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                    break;

                case T_right:
                    aText->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
                    break;

                case T_top:
                    aText->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
                    break;

                case T_bottom:
                    aText->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
                    break;

                case T_mirror:
                    aText->SetMirrored( true );
                    break;

                default:
                    Expecting( "left, right, top, bottom, or mirror" );
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


void SCH_SEXPR_PARSER::parseHeader()
{
    wxCHECK_RET( CurTok() == T_kicad_symbol_lib,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a header." ) );

    NeedLEFT();

    T tok = NextTok();

    if( tok == T_version )
    {
        m_requiredVersion = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );
        NeedRIGHT();

        // Skip the host name and host build version information.
        NeedLEFT();
        NeedSYMBOL();
        NeedSYMBOL();
        NeedSYMBOL();
        NeedRIGHT();
    }
    else
    {
        m_requiredVersion = SEXPR_SYMBOL_LIB_FILE_VERSION;

        // Skip the host name and host build version information.
        NeedSYMBOL();
        NeedSYMBOL();
        NeedRIGHT();
    }
}


void SCH_SEXPR_PARSER::parsePinNames( std::unique_ptr<LIB_PART>& aSymbol )
{
    wxCHECK_RET( CurTok() == T_pin_names,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a pin_name token." ) );

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
        error.Printf(
            _( "Invalid symbol names definition in\nfile: \"%s\"\nline: %d\noffset: %d" ),
            CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }
}


void SCH_SEXPR_PARSER::parseProperty( std::unique_ptr<LIB_PART>& aSymbol )
{
    wxCHECK_RET( CurTok() == T_property,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a property token." ) );

    wxString error;
    wxString name;
    wxString value;
    std::unique_ptr<LIB_FIELD> tmp( new LIB_FIELD( MANDATORY_FIELDS ) );

    T token = NextTok();

    if( !IsSymbol( token ) )
    {
        error.Printf( _( "Invalid property name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    name = FromUTF8();

    if( name.IsEmpty() )
    {
        error.Printf( _( "Empty property name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    token = NextTok();

    if( !IsSymbol( token ) )
    {
        error.Printf( _( "Invalid property value in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    // Empty property values are valid.
    value = FromUTF8();

    LIB_FIELD* field;

    if( name == "ki_reference" )
    {
        field = &aSymbol->GetReferenceField();
        field->SetText( value );
    }
    else if( name == "ki_value" )
    {
        field = &aSymbol->GetValueField();
        field->SetText( value );
    }
    else if( name == "ki_footprint" )
    {
        field = &aSymbol->GetFootprintField();
        field->SetText( value );
    }
    else if( name == "ki_datasheet" )
    {
        field = aSymbol->GetField( DATASHEET );
        aSymbol->SetDocFileName( value );
    }
    else if( name == "ki_keywords" )
    {
        // Not a LIB_FIELD object yet.
        aSymbol->SetKeyWords( value );
        field = tmp.get();
    }
    else if( name == "ki_description" )
    {
        // Not a LIB_FIELD object yet.
        aSymbol->SetDescription( value );
        field = tmp.get();
    }
    else if( name == "ki_fp_filters" )
    {
        // Not a LIB_FIELD object yet.
        wxArrayString filters;
        wxStringTokenizer tokenizer( value );

        while( tokenizer.HasMoreTokens() )
            filters.Add( tokenizer.GetNextToken() );

        aSymbol->SetFootprintFilters( filters );
        field = tmp.get();
    }
    else if( name == "ki_locked" )
    {
        aSymbol->LockUnits( true );
        field = tmp.get();
    }
    else
    {
        field = aSymbol->FindField( name );

        if( !field )
        {
            field = new LIB_FIELD( m_fieldId, name );
            aSymbol->AddDrawItem( field );
            m_fieldId += 1;
        }

        field->SetText( value );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            field->SetPosition( parseXY() );
            field->SetTextAngle( static_cast<int>( parseDouble( "text angle" ) * 10.0 ) );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( field ) );
            break;

        default:
            Expecting( "at or effects" );
        }
    }
}


LIB_ARC* SCH_SEXPR_PARSER::parseArc()
{
    wxCHECK_MSG( CurTok() == T_arc, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as an arc token." ) );

    T token;
    wxPoint start;
    wxPoint mid;
    wxPoint end;
    wxPoint pos;
    bool hasMidPoint = false;
    std::unique_ptr<LIB_ARC> arc( new LIB_ARC( nullptr ) );

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
            start = parseXY();
            NeedRIGHT();
            break;

        case T_mid:
            mid = parseXY();
            NeedRIGHT();
            hasMidPoint = true;
            break;

        case T_end:
            end = parseXY();
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
                    pos = parseXY();
                    NeedRIGHT();
                    break;

                case T_length:
                    arc->SetRadius( parseInternalUnits( "radius length" ) );
                    NeedRIGHT();
                    break;

                case T_angles:
                {
                    int angle1 = KiROUND( parseDouble( "start radius angle" ) * 10.0 );
                    int angle2 = KiROUND( parseDouble( "end radius angle" ) * 10.0 );

                    NORMALIZE_ANGLE_POS( angle1 );
                    NORMALIZE_ANGLE_POS( angle2 );
                    arc->SetFirstRadiusAngle( angle1 );
                    arc->SetSecondRadiusAngle( angle2 );
                    NeedRIGHT();
                    break;
                }

                default:
                    Expecting( "at, length, or angle" );
                }
            }

            break;

        case T_stroke:
            NeedLEFT();
            token = NextTok();

            if( token != T_width )
                Expecting( "width" );

            arc->SetWidth( parseInternalUnits( "stroke width" ) );
            NeedRIGHT();   // Closes width token;
            NeedRIGHT();   // Closes stroke token;
            break;

        case T_fill:
            arc->SetFillMode( parseFillMode() );
            break;

        default:
            Expecting( "start, end, radius, stroke, or fill" );
        }
    }

    arc->SetPosition( pos );
    arc->SetStart( start );
    arc->SetEnd( end );

    if( hasMidPoint )
    {
        VECTOR2I center = GetArcCenter( arc->GetStart(), mid, arc->GetEnd() );

        arc->SetPosition( wxPoint( center.x, center.y ) );

        // @todo Calculate the radius.

        arc->CalcRadiusAngles();
    }

    return arc.release();
}


LIB_BEZIER* SCH_SEXPR_PARSER::parseBezier()
{
    wxCHECK_MSG( CurTok() == T_bezier, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a bezier." ) );

    T token;
    std::unique_ptr<LIB_BEZIER> bezier( new LIB_BEZIER( nullptr ) );

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
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_xy )
                    Expecting( "xy" );

                bezier->AddPoint( parseXY() );

                NeedRIGHT();
            }

            break;

        case T_stroke:
            NeedLEFT();
            token = NextTok();

            if( token != T_width )
                Expecting( "width" );

            bezier->SetWidth( parseInternalUnits( "stroke width" ) );
            NeedRIGHT();   // Closes width token;
            NeedRIGHT();   // Closes stroke token;
            break;

        case T_fill:
            bezier->SetFillMode( parseFillMode() );
            break;

        default:
            Expecting( "pts, stroke, or fill" );
        }
    }

    return bezier.release();
}


LIB_CIRCLE* SCH_SEXPR_PARSER::parseCircle()
{
    wxCHECK_MSG( CurTok() == T_circle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a circle token." ) );

    T token;
    std::unique_ptr<LIB_CIRCLE> circle( new LIB_CIRCLE( nullptr ) );

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
            circle->SetPosition( parseXY() );
            NeedRIGHT();
            break;

        case T_radius:
            circle->SetRadius( parseInternalUnits( "radius length" ) );
            NeedRIGHT();
            break;

        case T_stroke:
            NeedLEFT();
            token = NextTok();

            if( token != T_width )
                Expecting( "width" );

            circle->SetWidth( parseInternalUnits( "stroke width" ) );
            NeedRIGHT();   // Closes width token;
            NeedRIGHT();   // Closes stroke token;
            break;

        case T_fill:
            circle->SetFillMode( parseFillMode() );
            break;

        default:
            Expecting( "start, end, radius, stroke, or fill" );
        }
    }

    return circle.release();
}


LIB_PIN* SCH_SEXPR_PARSER::parsePin()
{
    wxCHECK_MSG( CurTok() == T_pin, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a pin token." ) );

    T token;
    wxString tmp;
    wxString error;
    std::unique_ptr<LIB_PIN> pin( new LIB_PIN( nullptr ) );

    pin->SetUnit( m_unit );
    pin->SetConvert( m_convert );

    // Pin electrical type.
    token = NextTok();

    switch( token )
    {
    case T_input:
        pin->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
        break;

    case T_output:
        pin->SetType( ELECTRICAL_PINTYPE::PT_OUTPUT );
        break;

    case T_bidirectional:
        pin->SetType( ELECTRICAL_PINTYPE::PT_BIDI );
        break;

    case T_tri_state:
        pin->SetType( ELECTRICAL_PINTYPE::PT_TRISTATE );
        break;

    case T_passive:
        pin->SetType( ELECTRICAL_PINTYPE::PT_PASSIVE );
        break;

    case T_unspecified:
        pin->SetType( ELECTRICAL_PINTYPE::PT_UNSPECIFIED );
        break;

    case T_power_in:
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
        break;

    case T_power_out:
        pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_OUT );
        break;

    case T_open_collector:
        pin->SetType( ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR );
        break;

    case T_open_emitter:
        pin->SetType( ELECTRICAL_PINTYPE::PT_OPENEMITTER );
        break;

    case T_unconnected:
        pin->SetType( ELECTRICAL_PINTYPE::PT_NC );
        break;

    default:
        Expecting( "input, output, bidirectional, tri_state, passive, unspecified, "
                   "power_in, power_out, open_collector, open_emitter, or unconnected" );
    }

    // Pin shape.
    token = NextTok();

    switch( token )
    {
    case T_line:
        pin->SetShape( GRAPHIC_PINSHAPE::LINE );
        break;

    case T_inverted:
        pin->SetShape( GRAPHIC_PINSHAPE::INVERTED );
        break;

    case T_clock:
        pin->SetShape( GRAPHIC_PINSHAPE::CLOCK );
        break;

    case T_inverted_clock:
        pin->SetShape( GRAPHIC_PINSHAPE::INVERTED_CLOCK );
        break;

    case T_input_low:
        pin->SetShape( GRAPHIC_PINSHAPE::INPUT_LOW );
        break;

    case T_clock_low:
        pin->SetShape( GRAPHIC_PINSHAPE::CLOCK_LOW );
        break;

    case T_output_low:
        pin->SetShape( GRAPHIC_PINSHAPE::OUTPUT_LOW );
        break;

    case T_edge_clock_high:
        pin->SetShape( GRAPHIC_PINSHAPE::FALLING_EDGE_CLOCK );
        break;

    case T_non_logic:
        pin->SetShape( GRAPHIC_PINSHAPE::NONLOGIC );
        break;

    default:
        Expecting( "line, inverted, clock, inverted_clock, input_low, clock_low, "
                   "output_low, edge_clock_high, non_logic" );
    }

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
            case 0:
                pin->SetOrientation( PIN_RIGHT );
                break;

            case 90:
                pin->SetOrientation( PIN_UP );
                break;

            case 180:
                pin->SetOrientation( PIN_LEFT );
                break;

            case 270:
                pin->SetOrientation( PIN_DOWN );
                break;

            default:
                Expecting( "0, 90, 180, or 270" );
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
                error.Printf( _( "Invalid pin name in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                              CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

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

                    parseEDA_TEXT( &text );
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
                error.Printf( _( "Invalid pin number in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                              CurSource().c_str(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
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

                    parseEDA_TEXT( &text );
                    pin->SetNumberTextSize( text.GetTextHeight(), false );
                    NeedRIGHT();
                }
                else
                {
                    Expecting( "effects" );
                }
            }

            break;

        default:
            Expecting( "at, name, number, or length" );
        }
    }

    return pin.release();
}


LIB_POLYLINE* SCH_SEXPR_PARSER::parsePolyLine()
{
    wxCHECK_MSG( CurTok() == T_polyline, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a polyline." ) );

    T token;
    std::unique_ptr<LIB_POLYLINE> polyLine( new LIB_POLYLINE( nullptr ) );

    polyLine->SetUnit( m_unit );
    polyLine->SetConvert( m_convert );

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

                polyLine->AddPoint( parseXY() );

                NeedRIGHT();
            }

            break;

        case T_stroke:
            NeedLEFT();
            token = NextTok();

            if( token != T_width )
                Expecting( "width" );

            polyLine->SetWidth( parseInternalUnits( "stroke width" ) );
            NeedRIGHT();   // Closes width token;
            NeedRIGHT();   // Closes stroke token;
            break;

        case T_fill:
            polyLine->SetFillMode( parseFillMode() );
            break;

        default:
            Expecting( "pts, stroke, or fill" );
        }
    }

    return polyLine.release();
}


LIB_RECTANGLE* SCH_SEXPR_PARSER::parseRectangle()
{
    wxCHECK_MSG( CurTok() == T_rectangle, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a rectangle token." ) );

    T token;
    std::unique_ptr<LIB_RECTANGLE> rectangle( new LIB_RECTANGLE( nullptr ) );

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
            NeedLEFT();
            token = NextTok();

            if( token != T_width )
                Expecting( "width" );

            rectangle->SetWidth( parseInternalUnits( "stroke width" ) );
            NeedRIGHT();   // Closes width token;
            NeedRIGHT();   // Closes stroke token;
            break;

        case T_fill:
            rectangle->SetFillMode( parseFillMode() );
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
    wxString error;
    std::unique_ptr<LIB_TEXT> text( new LIB_TEXT( nullptr ) );

    text->SetUnit( m_unit );
    text->SetConvert( m_convert );
    token = NextTok();

    if( !IsSymbol( token ) )
    {
        error.Printf( _( "Invalid text string in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      CurSource().c_str(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
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
            parseEDA_TEXT( static_cast<EDA_TEXT*>( text.get() ) );
            break;

        default:
            Expecting( "at or effects" );
        }
    }

    return text.release();
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
 * @author Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file pcb_parser.cpp
 * @brief Pcbnew s-expression file format parser implementation.
 */

#include <errno.h>
#include <common.h>
#include <confirm.h>
#include <macros.h>
#include <convert_from_iu.h>
#include <trigo.h>
#include <3d_struct.h>
#include <class_title_block.h>

#include <class_board.h>
#include <class_dimension.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <class_mire.h>
#include <class_module.h>
#include <class_netclass.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <kicad_plugin.h>
#include <pcb_plot_params_parser.h>
#include <pcb_plot_params.h>
#include <zones.h>
#include <pcb_parser.h>

#include <boost/make_shared.hpp>

using namespace PCB_KEYS_T;


void PCB_PARSER::init()
{
    m_layerIndices.clear();
    m_layerMasks.clear();

    // Add untranslated default (i.e. english) layernames.
    // Some may be overridden later if parsing a board rather than a footprint.
    // The english name will survive if parsing only a footprint.
    for( LAYER_NUM layer = 0;  layer < LAYER_ID_COUNT;  ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( LAYER_ID( layer ) ) ) );

        m_layerIndices[ untranslated ] = LAYER_ID( layer );
        m_layerMasks[ untranslated ]   = LSET( LAYER_ID( layer ) );
    }

    m_layerMasks[ "*.Cu" ]      = LSET::AllCuMask();
    m_layerMasks[ "F&B.Cu" ]    = LSET( 2, F_Cu, B_Cu );
    m_layerMasks[ "*.Adhes" ]   = LSET( 2, B_Adhes, F_Adhes );
    m_layerMasks[ "*.Paste" ]   = LSET( 2, B_Paste, F_Paste );
    m_layerMasks[ "*.Mask" ]    = LSET( 2, B_Mask,  F_Mask );
    m_layerMasks[ "*.SilkS" ]   = LSET( 2, B_SilkS, F_SilkS );
    m_layerMasks[ "*.Fab" ]     = LSET( 2, B_Fab,   F_Fab );
    m_layerMasks[ "*.CrtYd" ]   = LSET( 2, B_CrtYd, F_CrtYd );

    // This is for the first pretty & *.kicad_pcb formats, which had
    // Inner1_Cu - Inner14_Cu with the numbering sequence
    // reversed from the subsequent format's In1_Cu - In30_Cu numbering scheme.
    // The newer format brought in an additional 16 Cu layers and flipped the cu stack but
    // kept the gap between one of the outside layers and the last cu internal.

    for( int i=1; i<=14; ++i )
    {
        std::string key = StrPrintf( "Inner%d.Cu", i );

        m_layerMasks[ key ] = LSET( LAYER_ID( In15_Cu - i ) );
    }

#if defined(DEBUG) && 0
    printf( "m_layerMasks:\n" );
    for( LSET_MAP::const_iterator it = m_layerMasks.begin();  it != m_layerMasks.end();  ++it )
    {
        printf( " [%s] == 0x%s\n",  it->first.c_str(), it->second.FmtHex().c_str() );
    }

    printf( "m_layerIndices:\n" );
    for( LAYER_ID_MAP::const_iterator it = m_layerIndices.begin();  it != m_layerIndices.end();  ++it )
    {
        printf( " [%s] == %d\n",  it->first.c_str(), it->second );
    }
#endif

}


void PCB_PARSER::pushValueIntoMap( int aIndex, int aValue )
{
    // Add aValue in netcode mapping (m_netCodes) at index aNetCode
    // ensure there is room in m_netCodes for that, and add room if needed.

    if( (int)m_netCodes.size() <= aIndex )
        m_netCodes.resize( aIndex+1 );

    m_netCodes[aIndex] = aValue;
}

double PCB_PARSER::parseDouble() throw( IO_ERROR )
{
    char* tmp;

    errno = 0;

    double fval = strtod( CurText(), &tmp );

    if( errno )
    {
        wxString error;
        error.Printf( _( "invalid floating point number in\nfile: <%s>\nline: %d\noffset: %d" ),
                      GetChars( CurSource() ), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    if( CurText() == tmp )
    {
        wxString error;
        error.Printf( _( "missing floating point number in\nfile: <%s>\nline: %d\noffset: %d" ),
                      GetChars( CurSource() ), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    return fval;
}


bool PCB_PARSER::parseBool() throw( PARSE_ERROR )
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


wxPoint PCB_PARSER::parseXY() throw( PARSE_ERROR )
{
    if( CurTok() != T_LEFT )
        NeedLEFT();

    wxPoint pt;
    T token = NextTok();

    if( token != T_xy )
        Expecting( T_xy );

    pt.x = parseBoardUnits( "X coordinate" );
    pt.y = parseBoardUnits( "Y coordinate" );

    NeedRIGHT();

    return pt;
}


void PCB_PARSER::parseXY( int* aX, int* aY ) throw( PARSE_ERROR )
{
    wxPoint pt = parseXY();

    if( aX )
        *aX = pt.x;

    if( aY )
        *aY = pt.y;
}


void PCB_PARSER::parseEDA_TEXT( EDA_TEXT* aText ) throw( PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_effects,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDA_TEXT." ) );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_font:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    continue;

                switch( token )
                {
                case T_size:
                {
                    wxSize sz;
                    sz.SetHeight( parseBoardUnits( "text height" ) );
                    sz.SetWidth( parseBoardUnits( "text width" ) );
                    aText->SetSize( sz );
                    NeedRIGHT();
                    break;
                }

                case T_thickness:
                    aText->SetThickness( parseBoardUnits( "text thickness" ) );
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
                if( token == T_LEFT )
                    continue;

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


S3D_MASTER* PCB_PARSER::parse3DModel() throw( PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_model, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as S3D_MASTER." ) );

    T token;

    std::auto_ptr< S3D_MASTER > n3D( new S3D_MASTER( NULL ) );

    NeedSYMBOLorNUMBER();
    n3D->SetShape3DName( FromUTF8() );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_MatPosition.x = parseDouble( "x value" );
            n3D->m_MatPosition.y = parseDouble( "y value" );
            n3D->m_MatPosition.z = parseDouble( "z value" );
            NeedRIGHT();
            break;

        case T_scale:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_MatScale.x = parseDouble( "x value" );
            n3D->m_MatScale.y = parseDouble( "y value" );
            n3D->m_MatScale.z = parseDouble( "z value" );
            NeedRIGHT();
            break;

        case T_rotate:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_MatRotation.x = parseDouble( "x value" );
            n3D->m_MatRotation.y = parseDouble( "y value" );
            n3D->m_MatRotation.z = parseDouble( "z value" );
            NeedRIGHT();
            break;

        default:
            Expecting( "at, scale, or rotate" );
        }

        NeedRIGHT();
    }

    return n3D.release();
}


BOARD_ITEM* PCB_PARSER::Parse() throw( IO_ERROR, PARSE_ERROR )
{
    T               token;
    BOARD_ITEM*     item;
    LOCALE_IO       toggle;

    // MODULEs can be prefixed with an initial block of single line comments and these
    // are kept for Format() so they round trip in s-expression form.  BOARDs might
    // eventually do the same, but currently do not.
    std::auto_ptr<wxArrayString> initial_comments( ReadCommentLines() );

    token = CurTok();

    if( token != T_LEFT )
        Expecting( T_LEFT );

    switch( NextTok() )
    {
    case T_kicad_pcb:
        if( m_board == NULL )
            m_board = new BOARD();

        item = (BOARD_ITEM*) parseBOARD();
        break;

    case T_module:
        item = (BOARD_ITEM*) parseMODULE( initial_comments.release() );
        break;

    default:
        wxString err;
        err.Printf( _( "unknown token \"%s\"" ), GetChars( FromUTF8() ) );
        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    return item;
}


BOARD* PCB_PARSER::parseBOARD() throw( IO_ERROR, PARSE_ERROR )
{
    T token;

    parseHeader();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_general:
            parseGeneralSection();
            break;

        case T_page:
            parsePAGE_INFO();
            break;

        case T_title_block:
            parseTITLE_BLOCK();
            break;

        case T_layers:
            parseLayers();
            break;

        case T_setup:
            parseSetup();
            break;

        case T_net:
            parseNETINFO_ITEM();
            break;

        case T_net_class:
            parseNETCLASS();
            break;

        case T_gr_arc:
        case T_gr_circle:
        case T_gr_curve:
        case T_gr_line:
        case T_gr_poly:
            m_board->Add( parseDRAWSEGMENT(), ADD_APPEND );
            break;

        case T_gr_text:
            m_board->Add( parseTEXTE_PCB(), ADD_APPEND );
            break;

        case T_dimension:
            m_board->Add( parseDIMENSION(), ADD_APPEND );
            break;

        case T_module:
            m_board->Add( parseMODULE(), ADD_APPEND );
            break;

        case T_segment:
            m_board->Add( parseTRACK(), ADD_APPEND );
            break;

        case T_via:
            m_board->Add( parseVIA(), ADD_APPEND );
            break;

        case T_zone:
            m_board->Add( parseZONE_CONTAINER(), ADD_APPEND );
            break;

        case T_target:
            m_board->Add( parsePCB_TARGET(), ADD_APPEND );
            break;

        default:
            wxString err;
            err.Printf( _( "unknown token \"%s\"" ), GetChars( FromUTF8() ) );
            THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
        }
    }

    return m_board;
}


void PCB_PARSER::parseHeader() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_kicad_pcb,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a header." ) );

    T token;

    NeedLEFT();
    token = NextTok();

    if( token != T_version )
        Expecting( GetTokenText( T_version ) );

    // Get the file version.
    m_board->SetFileFormatVersionAtLoad( parseInt( GetTokenText( T_version ) ) );

    // Skip the host name and host build version information.
    NeedRIGHT();
    NeedLEFT();
    NeedSYMBOL();
    NeedSYMBOL();
    NeedSYMBOL();
    NeedRIGHT();
}


void PCB_PARSER::parseGeneralSection() throw( IO_ERROR, PARSE_ERROR )
{
     wxCHECK_RET( CurTok() == T_general,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as a general section." ) );

    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_thickness:
            m_board->GetDesignSettings().SetBoardThickness( parseBoardUnits( T_thickness ) );
            NeedRIGHT();
            break;

        case T_nets:
            m_netCodes.resize( parseInt( "nets number" ) );
            NeedRIGHT();
            break;

        case T_no_connects:
            m_board->SetUnconnectedNetCount( parseInt( "no connect count" ) );
            NeedRIGHT();
            break;

        default:              // Skip everything but the board thickness.
            wxLogDebug( wxT( "Skipping general section token %s " ),
                        GetChars( GetTokenString( token ) ) );

            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( !IsSymbol( token ) && token != T_NUMBER )
                    Expecting( "symbol or number" );
            }
        }
    }
}


void PCB_PARSER::parsePAGE_INFO() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_page,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a PAGE_INFO." ) );

    T token;
    PAGE_INFO pageInfo;

    NeedSYMBOL();

    wxString pageType = FromUTF8();

    if( !pageInfo.SetType( pageType ) )
    {
        wxString err;
        err.Printf( _( "page type \"%s\" is not valid " ), GetChars( FromUTF8() ) );
        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    if( pageType == PAGE_INFO::Custom )
    {
        double width = parseDouble( "width" );      // width in mm

        // Perform some controls to avoid crashes if the size is edited by hands
        if( width < 100.0 )
            width = 100.0;
        else if( width > 1200.0 )
            width = 1200.0;

        double height = parseDouble( "height" );    // height in mm

        if( height < 100.0 )
            height = 100.0;
        else if( height > 1200.0 )
            height = 1200.0;

        pageInfo.SetWidthMils( Mm2mils( width ) );
        pageInfo.SetHeightMils( Mm2mils( height ) );
    }

    token = NextTok();

    if( token == T_portrait )
    {
        pageInfo.SetPortrait( true );
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        Expecting( "portrait|)" );
    }

    m_board->SetPageSettings( pageInfo );
}


void PCB_PARSER::parseTITLE_BLOCK() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_title_block,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as TITLE_BLOCK." ) );

    T token;
    TITLE_BLOCK titleBlock;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_title:
            NextTok();
            titleBlock.SetTitle( FromUTF8() );
            break;

        case T_date:
            NextTok();
            titleBlock.SetDate( FromUTF8() );
            break;

        case T_rev:
            NextTok();
            titleBlock.SetRevision( FromUTF8() );
            break;

        case T_company:
            NextTok();
            titleBlock.SetCompany( FromUTF8() );
            break;

        case T_comment:
            {
                int commentNumber = parseInt( "comment" );

                switch( commentNumber )
                {
                case 1:
                    NextTok();
                    titleBlock.SetComment1( FromUTF8() );
                    break;

                case 2:
                    NextTok();
                    titleBlock.SetComment2( FromUTF8() );
                    break;

                case 3:
                    NextTok();
                    titleBlock.SetComment3( FromUTF8() );
                    break;

                case 4:
                    NextTok();
                    titleBlock.SetComment4( FromUTF8() );
                    break;

                default:
                    wxString err;
                    err.Printf( wxT( "%d is not a valid title block comment number" ), commentNumber );
                    THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
                }
            }
            break;

        default:
            Expecting( "title, date, rev, company, or comment" );
        }

        NeedRIGHT();
    }

    m_board->SetTitleBlock( titleBlock );
}


void PCB_PARSER::parseLayer( LAYER* aLayer ) throw( IO_ERROR, PARSE_ERROR )
{
    T           token;

    std::string name;
    std::string type;
    bool        isVisible = true;

    aLayer->clear();

    if( CurTok() != T_LEFT )
        Expecting( T_LEFT );

    // this layer_num is not used, we DO depend on LAYER_T however.
    LAYER_NUM layer_num = parseInt( "layer index" );

    NeedSYMBOL();
    name = CurText();

    NeedSYMBOL();
    type = CurText();

    token = NextTok();

    if( token == T_hide )
    {
        isVisible = false;
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        Expecting( "hide or )" );
    }

    aLayer->m_name    = FROM_UTF8( name.c_str() );
    aLayer->m_type    = LAYER::ParseType( type.c_str() );
    aLayer->m_number  = layer_num;
    aLayer->m_visible = isVisible;
}



void PCB_PARSER::parseLayers() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_layers,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as layers." ) );

    T       token;
    LSET    visibleLayers;
    LSET    enabledLayers;
    int     copperLayerCount = 0;
    LAYER   layer;

    std::vector<LAYER>  cu;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        parseLayer( &layer );

        if( layer.m_type == LT_UNDEFINED )     // it's a non-copper layer
            break;

        cu.push_back( layer );      // it's copper
    }

    // All Cu layers are parsed, but not the non-cu layers here.

    // The original *.kicad_pcb file format and the inverted
    // Cu stack format both have all the Cu layers first, so use this
    // trick to handle either. The layer number in the (layers ..)
    // s-expression element are ignored.
    if( cu.size() )
    {
        // Rework the layer numbers, which changed when the Cu stack
        // was flipped.  So we instead use position in the list.
        cu[cu.size()-1].m_number = B_Cu;

        for( unsigned i=0; i < cu.size()-1; ++i )
        {
            cu[i].m_number = i;
        }

        for( std::vector<LAYER>::const_iterator it = cu.begin(); it<cu.end();  ++it )
        {
            enabledLayers.set( it->m_number );

            if( it->m_visible )
                visibleLayers.set( it->m_number );

            m_board->SetLayerDescr( LAYER_ID( it->m_number ), *it );

            UTF8 name = it->m_name;

            m_layerIndices[ name ] = LAYER_ID( it->m_number );
            m_layerMasks[   name ] = LSET( LAYER_ID( it->m_number ) );
        }

        copperLayerCount = cu.size();
    }

    // process non-copper layers
    while( token != T_RIGHT )
    {
        LAYER_ID_MAP::const_iterator it = m_layerIndices.find( UTF8( layer.m_name ) );

        if( it == m_layerIndices.end() )
        {
            wxString error = wxString::Format(
                _( "Layer '%s' in file '%s' at line %d, is not in fixed layer hash" ),
                GetChars( layer.m_name ),
                GetChars( CurSource() ),
                CurLineNumber(),
                CurOffset()
                );

            THROW_IO_ERROR( error );
        }

        layer.m_number = it->second;

        enabledLayers.set( layer.m_number );

        if( layer.m_visible )
            visibleLayers.set( layer.m_number );

        // DBG( printf( "aux m_visible:%s\n", layer.m_visible ? "true" : "false" );)

        m_board->SetLayerDescr( it->second, layer );

        token = NextTok();

        if( token != T_LEFT )
            break;

        parseLayer( &layer );
    }

    // We need at least 2 copper layers and there must be an even number of them.
    if( copperLayerCount < 2 || (copperLayerCount % 2) != 0 )
    {
        wxString err = wxString::Format(
            _( "%d is not a valid layer count" ), copperLayerCount );

        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    m_board->SetCopperLayerCount( copperLayerCount );
    m_board->SetEnabledLayers( enabledLayers );

    // call SetEnabledLayers before SetVisibleLayers()
    m_board->SetVisibleLayers( visibleLayers );
}


template<class T, class M>
T PCB_PARSER::lookUpLayer( const M& aMap ) throw( PARSE_ERROR, IO_ERROR )
{
    // avoid constructing another std::string, use lexer's directly
    typename M::const_iterator it = aMap.find( curText );

    if( it == aMap.end() )
    {
#if 0 && defined(DEBUG)
        // dump the whole darn table, there's something wrong with it.
        for( it = aMap.begin();  it != aMap.end();  ++it )
        {
            wxLogDebug( &aMap == (void*)&m_layerIndices ? wxT( "lm[%s] = %d" ) :
                        wxT( "lm[%s] = %08X" ), it->first.c_str(), it->second );
        }
#endif

        wxString error = wxString::Format( _(
                "Layer '%s' in file\n"
                "'%s'\n"
                "at line %d, position %d\n"
                "was not defined in the layers section"
                ),
            GetChars( FROM_UTF8( CurText() ) ),
            GetChars( CurSource() ),
            CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    return it->second;
}


LAYER_ID PCB_PARSER::parseBoardItemLayer() throw( PARSE_ERROR, IO_ERROR )
{
    wxCHECK_MSG( CurTok() == T_layer, UNDEFINED_LAYER,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as layer." ) );

    NextTok();

    LAYER_ID layerIndex = lookUpLayer<LAYER_ID>( m_layerIndices );

    // Handle closing ) in object parser.

    return layerIndex;
}


LSET PCB_PARSER::parseBoardItemLayersAsMask() throw( PARSE_ERROR, IO_ERROR )
{
    wxCHECK_MSG( CurTok() == T_layers, LSET(),
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as item layer mask." ) );

    LSET layerMask;

    for( T token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        LSET mask = lookUpLayer<LSET>( m_layerMasks );
        layerMask |= mask;
    }

    return layerMask;
}


void PCB_PARSER::parseSetup() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_setup,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as setup." ) );

    T token;
    NETCLASSPTR defaultNetClass = m_board->GetDesignSettings().GetDefault();
    // TODO Orson: is it really necessary to first operate on a copy and then apply it?
    // would not it be better to use reference here and apply all the changes instantly?
    BOARD_DESIGN_SETTINGS designSettings = m_board->GetDesignSettings();
    ZONE_SETTINGS zoneSettings = m_board->GetZoneSettings();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_last_trace_width:    // not used now
            /* lastTraceWidth =*/ parseBoardUnits( T_last_trace_width );
            NeedRIGHT();
            break;

        case T_user_trace_width:
            designSettings.m_TrackWidthList.push_back( parseBoardUnits( T_user_trace_width ) );
            NeedRIGHT();
            break;

        case T_trace_clearance:
            defaultNetClass->SetClearance( parseBoardUnits( T_trace_clearance ) );
            NeedRIGHT();
            break;

        case T_zone_clearance:
            zoneSettings.m_ZoneClearance = parseBoardUnits( T_zone_clearance );
            NeedRIGHT();
            break;

        case T_zone_45_only:
            zoneSettings.m_Zone_45_Only = parseBool();
            NeedRIGHT();
            break;

        case T_trace_min:
            designSettings.m_TrackMinWidth = parseBoardUnits( T_trace_min );
            NeedRIGHT();
            break;

        case T_segment_width:
            designSettings.m_DrawSegmentWidth = parseBoardUnits( T_segment_width );
            NeedRIGHT();
            break;

        case T_edge_width:
            designSettings.m_EdgeSegmentWidth = parseBoardUnits( T_edge_width );
            NeedRIGHT();
            break;

        case T_via_size:
            defaultNetClass->SetViaDiameter( parseBoardUnits( T_via_size ) );
            NeedRIGHT();
            break;

        case T_via_drill:
            defaultNetClass->SetViaDrill( parseBoardUnits( T_via_drill ) );
            NeedRIGHT();
            break;

        case T_via_min_size:
            designSettings.m_ViasMinSize = parseBoardUnits( T_via_min_size );
            NeedRIGHT();
            break;

        case T_via_min_drill:
            designSettings.m_ViasMinDrill = parseBoardUnits( T_via_min_drill );
            NeedRIGHT();
            break;

        case T_user_via:
            {
                int viaSize = parseBoardUnits( "user via size" );
                int viaDrill = parseBoardUnits( "user via drill" );
                designSettings.m_ViasDimensionsList.push_back( VIA_DIMENSION( viaSize, viaDrill ) );
                NeedRIGHT();
            }
            break;

        case T_uvia_size:
            defaultNetClass->SetuViaDiameter( parseBoardUnits( T_uvia_size ) );
            NeedRIGHT();
            break;

        case T_uvia_drill:
            defaultNetClass->SetuViaDrill( parseBoardUnits( T_uvia_drill ) );
            NeedRIGHT();
            break;

        case T_uvias_allowed:
            designSettings.m_MicroViasAllowed = parseBool();
            NeedRIGHT();
            break;

        case T_blind_buried_vias_allowed:
            designSettings.m_BlindBuriedViaAllowed = parseBool();
            NeedRIGHT();
            break;

        case T_uvia_min_size:
            designSettings.m_MicroViasMinSize = parseBoardUnits( T_uvia_min_size );
            NeedRIGHT();
            break;

        case T_uvia_min_drill:
            designSettings.m_MicroViasMinDrill = parseBoardUnits( T_uvia_min_drill );
            NeedRIGHT();
            break;

        case T_pcb_text_width:
            designSettings.m_PcbTextWidth = parseBoardUnits( T_pcb_text_width );
            NeedRIGHT();
            break;

        case T_pcb_text_size:
            designSettings.m_PcbTextSize.x = parseBoardUnits( "pcb text width" );
            designSettings.m_PcbTextSize.y = parseBoardUnits( "pcb text height" );
            NeedRIGHT();
            break;

        case T_mod_edge_width:
            designSettings.m_ModuleSegmentWidth = parseBoardUnits( T_mod_edge_width );
            NeedRIGHT();
            break;

        case T_mod_text_size:
            designSettings.m_ModuleTextSize.x = parseBoardUnits( "module text width" );
            designSettings.m_ModuleTextSize.y = parseBoardUnits( "module text height" );
            NeedRIGHT();
            break;

        case T_mod_text_width:
            designSettings.m_ModuleTextWidth = parseBoardUnits( T_mod_text_width );
            NeedRIGHT();
            break;

        case T_pad_size:
            {
                wxSize sz;
                sz.SetWidth( parseBoardUnits( "master pad width" ) );
                sz.SetHeight( parseBoardUnits( "master pad height" ) );
                designSettings.m_Pad_Master.SetSize( sz );
                NeedRIGHT();
            }
            break;

        case T_pad_drill:
            {
                int drillSize = parseBoardUnits( T_pad_drill );
                designSettings.m_Pad_Master.SetDrillSize( wxSize( drillSize, drillSize ) );
                NeedRIGHT();
            }
            break;

        case T_pad_to_mask_clearance:
             designSettings.m_SolderMaskMargin = parseBoardUnits( T_pad_to_mask_clearance );
            NeedRIGHT();
            break;

        case T_solder_mask_min_width:
            designSettings.m_SolderMaskMinWidth = parseBoardUnits( T_solder_mask_min_width );
            NeedRIGHT();
            break;

        case T_pad_to_paste_clearance:
            designSettings.m_SolderPasteMargin = parseBoardUnits( T_pad_to_paste_clearance );
            NeedRIGHT();
            break;

        case T_pad_to_paste_clearance_ratio:
            designSettings.m_SolderPasteMarginRatio = parseDouble( T_pad_to_paste_clearance_ratio );
            NeedRIGHT();
            break;

        case T_aux_axis_origin:
            {
                int x = parseBoardUnits( "auxiliary origin X" );
                int y = parseBoardUnits( "auxiliary origin Y" );
                // m_board->SetAuxOrigin( wxPoint( x, y ) );    gets overwritten via SetDesignSettings below
                designSettings.m_AuxOrigin = wxPoint( x, y );
                NeedRIGHT();
            }
            break;

        case T_grid_origin:
            {
                int x = parseBoardUnits( "grid origin X" );
                int y = parseBoardUnits( "grid origin Y" );
                // m_board->SetGridOrigin( wxPoint( x, y ) );   gets overwritten SetDesignSettings below
                designSettings.m_GridOrigin = wxPoint( x, y );
                NeedRIGHT();
            }
            break;

        case T_visible_elements:
            designSettings.SetVisibleElements( parseHex() );
            NeedRIGHT();
            break;

        case T_pcbplotparams:
            {
                PCB_PLOT_PARAMS plotParams;
                PCB_PLOT_PARAMS_PARSER parser( reader );
                // parser must share the same current line as our current PCB parser
                // synchronize it.
                parser.SyncLineReaderWith( *this );

                plotParams.Parse( &parser );
                SyncLineReaderWith( parser );

                m_board->SetPlotOptions( plotParams );
            }
            break;

        default:
            Unexpected( CurText() );
        }
    }

    m_board->SetDesignSettings( designSettings );
    m_board->SetZoneSettings( zoneSettings );

    // Until such time as the *.brd file does not have the
    // global parameters:
    // "last_trace_width", "trace_min_width", "via_size", "via_drill",
    // "via_min_size", and "via_clearance", put those same global
    // values into the default NETCLASS until later board load
    // code should override them.  *.kicad_pcb files which have been
    // saved with knowledge of NETCLASSes will override these
    // defaults, old boards will not.
    //
    // @todo: I expect that at some point we can remove said global
    //        parameters from the *.brd file since the ones in the
    //        default netclass serve the same purpose.  If needed
    //        at all, the global defaults should go into a preferences
    //        file instead so they are there to start new board
    //        projects.
    defaultNetClass->SetParams( m_board->GetDesignSettings() );
}


void PCB_PARSER::parseNETINFO_ITEM() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_net,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as net." ) );

    int netCode = parseInt( "net number" );

    NeedSYMBOLorNUMBER();
    wxString name = FromUTF8();

    NeedRIGHT();

    // net 0 should be already in list, so store this net
    // if it is not the net 0, or if the net 0 does not exists.
    // (TODO: a better test.)
    if( netCode > 0 || m_board->FindNet( 0 ) == NULL )
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_board, name, netCode );
        m_board->AppendNet( net );

        // Store the new code mapping
        pushValueIntoMap( netCode, net->GetNet() );
    }
}


void PCB_PARSER::parseNETCLASS() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_RET( CurTok() == T_net_class,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as net class." ) );

    T token;

    NETCLASSPTR nc = boost::make_shared<NETCLASS>( wxEmptyString );

    // Read netclass name (can be a name or just a number like track width)
    NeedSYMBOLorNUMBER();
    nc->SetName( FromUTF8() );
    NeedSYMBOL();
    nc->SetDescription( FromUTF8() );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_clearance:
            nc->SetClearance( parseBoardUnits( T_clearance ) );
            break;

        case T_trace_width:
            nc->SetTrackWidth( parseBoardUnits( T_trace_width ) );
            break;

        case T_via_dia:
            nc->SetViaDiameter( parseBoardUnits( T_via_dia ) );
            break;

        case T_via_drill:
            nc->SetViaDrill( parseBoardUnits( T_via_drill ) );
            break;

        case T_uvia_dia:
            nc->SetuViaDiameter( parseBoardUnits( T_uvia_dia ) );
            break;

        case T_uvia_drill:
            nc->SetuViaDrill( parseBoardUnits( T_uvia_drill ) );
            break;

        case T_add_net:
            NeedSYMBOLorNUMBER();
            nc->Add( FromUTF8() );
            break;

        default:
            Expecting( "clearance, trace_width, via_dia, via_drill, uvia_dia, uvia_drill, or add_net" );
        }

        NeedRIGHT();
    }

    if( !m_board->GetDesignSettings().m_NetClasses.Add( nc ) )
    {
        // Must have been a name conflict, this is a bad board file.
        // User may have done a hand edit to the file.

        // auto_ptr will delete nc on this code path

        wxString error;
        error.Printf( _( "duplicate NETCLASS name '%s' in file <%s> at line %d, offset %d" ),
                      nc->GetName().GetData(), CurSource().GetData(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }
}


DRAWSEGMENT* PCB_PARSER::parseDRAWSEGMENT() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_gr_arc || CurTok() == T_gr_circle || CurTok() == T_gr_curve ||
                 CurTok() == T_gr_line || CurTok() == T_gr_poly, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as DRAWSEGMENT." ) );

    T token;
    wxPoint pt;
    std::auto_ptr< DRAWSEGMENT > segment( new DRAWSEGMENT( NULL ) );

    switch( CurTok() )
    {
    case T_gr_arc:
        segment->SetShape( S_ARC );
        NeedLEFT();
        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetEnd( pt );
        NeedRIGHT();
        break;

    case T_gr_circle:
        segment->SetShape( S_CIRCLE );
        NeedLEFT();
        token = NextTok();

        if( token != T_center )
            Expecting( T_center );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart( pt );
        NeedRIGHT();
        NeedLEFT();

        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetEnd( pt );
        NeedRIGHT();
        break;

    case T_gr_curve:
        segment->SetShape( S_CURVE );
        NeedLEFT();
        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        segment->SetStart( parseXY() );
        segment->SetBezControl1( parseXY() );
        segment->SetBezControl2( parseXY() );
        segment->SetEnd( parseXY() );
        NeedRIGHT();
        break;

    case T_gr_line:
        // Default DRAWSEGMENT type is S_SEGMENT.
        NeedLEFT();
        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetEnd( pt );
        NeedRIGHT();
        break;

    case T_gr_poly:
    {
        segment->SetShape( S_POLYGON );
        NeedLEFT();
        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        std::vector< wxPoint > pts;

        while( (token = NextTok()) != T_RIGHT )
            pts.push_back( parseXY() );

        segment->SetPolyPoints( pts );
    }
        break;

    default:
        Expecting( "gr_arc, gr_circle, gr_curve, gr_line, or gr_poly" );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_angle:
            segment->SetAngle( parseDouble( "segment angle" ) * 10.0 );
            break;

        case T_layer:
            segment->SetLayer( parseBoardItemLayer() );
            break;

        case T_width:
            segment->SetWidth( parseBoardUnits( T_width ) );
            break;

        case T_tstamp:
            segment->SetTimeStamp( parseHex() );
            break;

        case T_status:
            segment->SetStatus( static_cast<STATUS_FLAGS>( parseHex() ) );
            break;

        default:
            Expecting( "layer, width, tstamp, or status" );
        }

        NeedRIGHT();
    }

    return segment.release();
}


TEXTE_PCB* PCB_PARSER::parseTEXTE_PCB() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_gr_text, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as TEXTE_PCB." ) );

    T token;

    std::auto_ptr<TEXTE_PCB> text( new TEXTE_PCB( m_board ) );
    NeedSYMBOLorNUMBER();

    text->SetText( FromUTF8() );
    NeedLEFT();
    token = NextTok();

    if( token != T_at )
        Expecting( T_at );

    wxPoint pt;

    pt.x = parseBoardUnits( "X coordinate" );
    pt.y = parseBoardUnits( "Y coordinate" );
    text->SetTextPosition( pt );

    // If there is no orientation defined, then it is the default value of 0 degrees.
    token = NextTok();

    if( token == T_NUMBER )
    {
        text->SetOrientation( parseDouble() * 10.0 );
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        Unexpected( CurText() );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_layer:
            text->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_tstamp:
            text->SetTimeStamp( parseHex() );
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( (EDA_TEXT*) text.get() );
            break;

        default:
            Expecting( "layer, tstamp or effects" );
        }
    }

    return text.release();
}


DIMENSION* PCB_PARSER::parseDIMENSION() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_dimension, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as DIMENSION." ) );

    T token;

    std::auto_ptr<DIMENSION> dimension( new DIMENSION( NULL ) );

    dimension->SetValue( parseBoardUnits( "dimension value" ) );
    NeedLEFT();
    token = NextTok();

    if( token != T_width )
        Expecting( T_width );

    dimension->SetWidth( parseBoardUnits( "dimension width value" ) );
    NeedRIGHT();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_layer:
            dimension->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_tstamp:
            dimension->SetTimeStamp( parseHex() );
            NeedRIGHT();
            break;

        case T_gr_text:
        {
            TEXTE_PCB* text = parseTEXTE_PCB();
            dimension->Text() = *text;
            dimension->SetPosition( text->GetTextPosition() );
            delete text;
            break;
        }

        case T_feature1:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_featureLineDO.x, &dimension->m_featureLineDO.y );
            parseXY( &dimension->m_featureLineDF.x, &dimension->m_featureLineDF.y );
            dimension->UpdateHeight();
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_feature2:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_featureLineGO.x, &dimension->m_featureLineGO.y );
            parseXY( &dimension->m_featureLineGF.x, &dimension->m_featureLineGF.y );
            dimension->UpdateHeight();
            NeedRIGHT();
            NeedRIGHT();
            break;


        case T_crossbar:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_crossBarO.x, &dimension->m_crossBarO.y );
            parseXY( &dimension->m_crossBarF.x, &dimension->m_crossBarF.y );
            dimension->UpdateHeight();
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_arrow1a:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_crossBarF.x, &dimension->m_crossBarF.y );
            parseXY( &dimension->m_arrowD1F.x, &dimension->m_arrowD1F.y );
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_arrow1b:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_crossBarF.x, &dimension->m_crossBarF.y );
            parseXY( &dimension->m_arrowD2F.x, &dimension->m_arrowD2F.y );
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_arrow2a:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_crossBarO.x, &dimension->m_crossBarO.y );
            parseXY( &dimension->m_arrowG1F.x, &dimension->m_arrowG1F.y );
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_arrow2b:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( &dimension->m_crossBarO.x, &dimension->m_crossBarO.y );
            parseXY( &dimension->m_arrowG2F.x, &dimension->m_arrowG2F.y );
            NeedRIGHT();
            NeedRIGHT();
            break;

        default:
            Expecting( "layer, tstamp, gr_text, feature1, feature2 crossbar, arrow1a, "
                       "arrow1b, arrow2a, or arrow2b" );
        }
    }

    return dimension.release();
}


MODULE* PCB_PARSER::parseMODULE( wxArrayString* aInitialComments ) throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_module, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as MODULE." ) );

    wxString name;
    wxPoint  pt;
    T        token;
    FPID     fpid;

    std::auto_ptr<MODULE> module( new MODULE( m_board ) );

    module->SetInitialComments( aInitialComments );

    NeedSYMBOLorNUMBER();
    name = FromUTF8();

    if( !name.IsEmpty() && fpid.Parse( FromUTF8() ) >= 0 )
    {
        wxString error;
        error.Printf( _( "invalid PFID in\nfile: <%s>\nline: %d\noffset: %d" ),
                      GetChars( CurSource() ), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_locked:
            module->SetLocked( true );
            break;

        case T_placed:
            module->SetIsPlaced( true );
            break;

        case T_layer:
            module->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_tedit:
            module->SetLastEditTime( parseHex() );
            NeedRIGHT();
            break;

        case T_tstamp:
            module->SetTimeStamp( parseHex() );
            NeedRIGHT();
            break;

        case T_at:
            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            module->SetPosition( pt );
            token = NextTok();

            if( token == T_NUMBER )
            {
                module->SetOrientation( parseDouble() * 10.0 );
                NeedRIGHT();
            }
            else if( token != T_RIGHT )
            {
                Expecting( T_RIGHT );
            }

            break;

        case T_descr:
            NeedSYMBOLorNUMBER();   // some symbols can be 0508, so a number is also a symbol here
            module->SetDescription( FromUTF8() );
            NeedRIGHT();
            break;

        case T_tags:
            NeedSYMBOLorNUMBER();   // some symbols can be 0508, so a number is also a symbol here
            module->SetKeywords( FromUTF8() );
            NeedRIGHT();
            break;

        case T_path:
            NeedSYMBOLorNUMBER();   // Paths can be numerical so a number is also a symbol here
            module->SetPath( FromUTF8() );
            NeedRIGHT();
            break;

        case T_autoplace_cost90:
            module->SetPlacementCost90( parseInt( "auto place cost at 90 degrees" ) );
            NeedRIGHT();
            break;

        case T_autoplace_cost180:
            module->SetPlacementCost180( parseInt( "auto place cost at 180 degrees" ) );
            NeedRIGHT();
            break;

        case T_solder_mask_margin:
            module->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();
            break;

        case T_solder_paste_margin:
            module->SetLocalSolderPasteMargin(
                parseBoardUnits( "local solder paste margin value" ) );
            NeedRIGHT();
            break;

        case T_solder_paste_ratio:
            module->SetLocalSolderPasteMarginRatio(
                parseDouble( "local solder paste margin ratio value" ) );
            NeedRIGHT();
            break;

        case T_clearance:
            module->SetLocalClearance( parseBoardUnits( "local clearance value" ) );
            NeedRIGHT();
            break;

        case T_zone_connect:
            module->SetZoneConnection( (ZoneConnection) parseInt( "zone connection value" ) );
            NeedRIGHT();
            break;

        case T_thermal_width:
            module->SetThermalWidth( parseBoardUnits( "thermal width value" ) );
            NeedRIGHT();
            break;

        case T_thermal_gap:
            module->SetThermalGap( parseBoardUnits( "thermal gap value" ) );
            NeedRIGHT();
            break;

        case T_attr:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                switch( token )
                {
                case T_smd:
                    module->SetAttributes( module->GetAttributes() | MOD_CMS );
                    break;

                case T_virtual:
                    module->SetAttributes( module->GetAttributes() | MOD_VIRTUAL );
                    break;

                default:
                    Expecting( "smd and/or virtual" );
                }
            }

            break;

        case T_fp_text:
            {
                TEXTE_MODULE* text = parseTEXTE_MODULE();
                text->SetParent( module.get() );
                double orientation = text->GetOrientation();
                orientation -= module->GetOrientation();
                text->SetOrientation( orientation );
                text->SetDrawCoord();

                switch( text->GetType() )
                {
                case TEXTE_MODULE::TEXT_is_REFERENCE:
                    module->Reference() = *text;
                    delete text;
                    break;

                case TEXTE_MODULE::TEXT_is_VALUE:
                    module->Value() = *text;
                    delete text;
                    break;

                default:
                    module->GraphicalItems().PushBack( text );
                }
            }
            break;

        case T_fp_arc:
        case T_fp_circle:
        case T_fp_curve:
        case T_fp_line:
        case T_fp_poly:
            {
                EDGE_MODULE* em = parseEDGE_MODULE();
                em->SetParent( module.get() );
                em->SetDrawCoord();
                module->GraphicalItems().PushBack( em );
            }
            break;

        case T_pad:
            {
                D_PAD*  pad = parseD_PAD( module.get() );
                wxPoint pt = pad->GetPos0();

                RotatePoint( &pt, module->GetOrientation() );
                pad->SetPosition( pt + module->GetPosition() );
                module->Add( pad );
            }
            break;

        case T_model:
            module->Add3DModel( parse3DModel() );
            break;

        default:
            Expecting( "locked, placed, tedit, tstamp, at, descr, tags, path, "
                       "autoplace_cost90, autoplace_cost180, solder_mask_margin, "
                       "solder_paste_margin, solder_paste_ratio, clearance, "
                       "zone_connect, thermal_width, thermal_gap, attr, fp_text, "
                       "fp_arc, fp_circle, fp_curve, fp_line, fp_poly, pad, or model" );
        }
    }

    module->SetFPID( fpid );
    module->CalculateBoundingBox();

    return module.release();
}


TEXTE_MODULE* PCB_PARSER::parseTEXTE_MODULE() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_fp_text, NULL,
                 wxString::Format( wxT( "Cannot parse %s as TEXTE_MODULE at line %d, offset %d." ),
                                   GetChars( GetTokenString( CurTok() ) ),
                                   CurLineNumber(), CurOffset() ) );

    T token = NextTok();

    std::auto_ptr<TEXTE_MODULE> text( new TEXTE_MODULE( NULL ) );

    switch( token )
    {
    case T_reference:
        text->SetType( TEXTE_MODULE::TEXT_is_REFERENCE );
        break;

    case T_value:
        text->SetType( TEXTE_MODULE::TEXT_is_VALUE );
        break;

    case T_user:
        break;          // Default type is user text.

    default:
        THROW_IO_ERROR( wxString::Format( _( "cannot handle module text type %s" ),
                                          GetChars( FromUTF8() ) ) );
    }

    NeedSYMBOLorNUMBER();

    text->SetText( FromUTF8() );
    NeedLEFT();
    token = NextTok();

    if( token != T_at )
        Expecting( T_at );

    wxPoint pt;

    pt.x = parseBoardUnits( "X coordinate" );
    pt.y = parseBoardUnits( "Y coordinate" );
    text->SetPos0( pt );
    token = NextTok();

    // If there is no orientation defined, then it is the default value of 0 degrees.
    if( token == T_NUMBER )
    {
        text->SetOrientation( parseDouble() * 10.0 );
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        Unexpected( CurText() );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_layer:
            text->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_hide:
            text->SetVisible( false );
            break;

        case T_effects:
            parseEDA_TEXT( (EDA_TEXT*) text.get() );
            break;

        default:
            Expecting( "hide or effects" );
        }
    }

    return text.release();
}


EDGE_MODULE* PCB_PARSER::parseEDGE_MODULE() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_fp_arc || CurTok() == T_fp_circle || CurTok() == T_fp_curve ||
                 CurTok() == T_fp_line || CurTok() == T_fp_poly, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDGE_MODULE." ) );

    wxPoint pt;
    T token;

    std::auto_ptr< EDGE_MODULE > segment( new EDGE_MODULE( NULL ) );

    switch( CurTok() )
    {
    case T_fp_arc:
        segment->SetShape( S_ARC );
        NeedLEFT();
        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetEnd0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_angle )
            Expecting( T_angle );

        segment->SetAngle( parseDouble( "segment angle" ) * 10.0 );
        NeedRIGHT();
        break;

    case T_fp_circle:
        segment->SetShape( S_CIRCLE );
        NeedLEFT();
        token = NextTok();

        if( token != T_center )
            Expecting( T_center );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetEnd0( pt );
        NeedRIGHT();
        break;

    case T_fp_curve:
        segment->SetShape( S_CURVE );
        NeedLEFT();
        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        segment->SetStart0( parseXY() );
        segment->SetBezControl1( parseXY() );
        segment->SetBezControl2( parseXY() );
        segment->SetEnd0( parseXY() );
        NeedRIGHT();
        break;

    case T_fp_line:
        // Default DRAWSEGMENT type is S_SEGMENT.
        NeedLEFT();
        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart0( pt );

        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetEnd0( pt );
        NeedRIGHT();
        break;

    case T_fp_poly:
    {
        segment->SetShape( S_POLYGON );
        NeedLEFT();
        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        std::vector< wxPoint > pts;

        while( (token = NextTok()) != T_RIGHT )
            pts.push_back( parseXY() );

        segment->SetPolyPoints( pts );
    }
        break;

    default:
        Expecting( "fp_arc, fp_circle, fp_curve, fp_line, or fp_poly" );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_layer:
            segment->SetLayer( parseBoardItemLayer() );
            break;

        case T_width:
            segment->SetWidth( parseBoardUnits( T_width ) );
            break;

        case T_tstamp:
            segment->SetTimeStamp( parseHex() );
            break;

        case T_status:
            segment->SetStatus( static_cast<STATUS_FLAGS>( parseHex() ) );
            break;

        default:
            Expecting( "layer or width" );
        }

        NeedRIGHT();
    }

    return segment.release();
}


D_PAD* PCB_PARSER::parseD_PAD( MODULE* aParent ) throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_pad, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as D_PAD." ) );

    wxSize  sz;
    wxPoint pt;

    std::auto_ptr< D_PAD > pad( new D_PAD( aParent ) );

    NeedSYMBOLorNUMBER();
    pad->SetPadName( FromUTF8() );

    T token = NextTok();

    switch( token )
    {
    case T_thru_hole:
        pad->SetAttribute( PAD_STANDARD );
        break;

    case T_smd:
        pad->SetAttribute( PAD_SMD );

        // Default D_PAD object is thru hole with drill.
        // SMD pads have no hole
        pad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case T_connect:
        pad->SetAttribute( PAD_CONN );

        // Default D_PAD object is thru hole with drill.
        // CONN pads have no hole
        pad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case T_np_thru_hole:
        pad->SetAttribute( PAD_HOLE_NOT_PLATED );
        break;

    default:
        Expecting( "thru_hole, smd, connect, or np_thru_hole" );
    }

    token = NextTok();

    switch( token )
    {
    case T_circle:
        pad->SetShape( PAD_CIRCLE );
        break;

    case T_rect:
        pad->SetShape( PAD_RECT );
        break;

    case T_oval:
        pad->SetShape( PAD_OVAL );
        break;

    case T_trapezoid:
        pad->SetShape( PAD_TRAPEZOID );
        break;

    default:
        Expecting( "circle, rectangle, oval, or trapezoid" );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_size:
            sz.SetWidth( parseBoardUnits( "width value" ) );
            sz.SetHeight( parseBoardUnits( "height value" ) );
            pad->SetSize( sz );
            NeedRIGHT();
            break;

        case T_at:
            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            pad->SetPos0( pt );
            token = NextTok();

            if( token == T_NUMBER )
            {
                pad->SetOrientation( parseDouble() * 10.0 );
                NeedRIGHT();
            }
            else if( token != T_RIGHT )
            {
                Expecting( ") or angle value" );
            }

            break;

        case T_rect_delta:
            {
                wxSize delta;
                delta.SetWidth( parseBoardUnits( "rectangle delta width" ) );
                delta.SetHeight( parseBoardUnits( "rectangle delta height" ) );
                pad->SetDelta( delta );
                NeedRIGHT();
            }
            break;

        case T_drill:
            {
                bool    haveWidth = false;
                wxSize  drillSize = pad->GetDrillSize();

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                {
                    if( token == T_LEFT )
                        token = NextTok();

                    switch( token )
                    {
                    case T_oval:
                        pad->SetDrillShape( PAD_DRILL_OBLONG );
                        break;

                    case T_NUMBER:
                        {
                            if( !haveWidth )
                            {
                                drillSize.SetWidth( parseBoardUnits() );

                                // If height is not defined the width and height are the same.
                                drillSize.SetHeight( drillSize.GetWidth() );
                                haveWidth = true;
                            }
                            else
                            {
                                drillSize.SetHeight( parseBoardUnits() );
                            }

                        }
                        break;

                    case T_offset:
                        pt.x = parseBoardUnits( "drill offset x" );
                        pt.y = parseBoardUnits( "drill offset y" );
                        pad->SetOffset( pt );
                        NeedRIGHT();
                        break;

                    default:
                        Expecting( "oval, size, or offset" );
                    }
                }

                // This fixes a bug caused by setting the default D_PAD drill size to a value
                // other than 0 used to fix a bunch of debug assertions even though it is defined
                // as a through hole pad.  Wouldn't a though hole pad with no drill be a surface
                // mount pad (or a conn pad which is a smd pad with no solder paste)?
                if( ( pad->GetAttribute() != PAD_SMD ) && ( pad->GetAttribute() != PAD_CONN ) )
                    pad->SetDrillSize( drillSize );
                else
                    pad->SetDrillSize( wxSize( 0, 0 ) );

            }
            break;

        case T_layers:
            {
                LSET layerMask = parseBoardItemLayersAsMask();
                pad->SetLayerSet( layerMask );
            }
            break;

        case T_net:
            pad->SetNetCode( getNetCode( parseInt( "net number" ) ) );
            NeedSYMBOLorNUMBER();
            assert( FromUTF8() == m_board->FindNet( pad->GetNetCode() )->GetNetname() );
            NeedRIGHT();
            break;

        case T_die_length:
            pad->SetPadToDieLength( parseBoardUnits( T_die_length ) );
            NeedRIGHT();
            break;

        case T_solder_mask_margin:
            pad->SetLocalSolderMaskMargin( parseBoardUnits( T_solder_mask_margin ) );
            NeedRIGHT();
            break;

        case T_solder_paste_margin:
            pad->SetLocalSolderPasteMargin( parseBoardUnits( T_solder_paste_margin ) );
            NeedRIGHT();
            break;

        case T_solder_paste_margin_ratio:
            pad->SetLocalSolderPasteMarginRatio(
                parseDouble( "pad local solder paste margin ratio value" ) );
            NeedRIGHT();
            break;

        case T_clearance:
            pad->SetLocalClearance( parseBoardUnits( "local clearance value" ) );
            NeedRIGHT();
            break;

        case T_zone_connect:
            pad->SetZoneConnection( (ZoneConnection) parseInt( "zone connection value" ) );
            NeedRIGHT();
            break;

        case T_thermal_width:
            pad->SetThermalWidth( parseBoardUnits( T_thermal_width ) );
            NeedRIGHT();
            break;

        case T_thermal_gap:
            pad->SetThermalGap( parseBoardUnits( T_thermal_gap ) );
            NeedRIGHT();
            break;

        default:
            Expecting( "at, drill, layers, net, die_length, solder_mask_margin, "
                       "solder_paste_margin, solder_paste_margin_ratio, clearance, "
                       "zone_connect, thermal_width, or thermal_gap" );
        }
    }

    return pad.release();
}


TRACK* PCB_PARSER::parseTRACK() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_segment, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as TRACK." ) );

    wxPoint pt;
    T token;

    std::auto_ptr< TRACK > track( new TRACK( m_board ) );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:
            pt.x = parseBoardUnits( "start x" );
            pt.y = parseBoardUnits( "start y" );
            track->SetStart( pt );
            break;

        case T_end:
            pt.x = parseBoardUnits( "end x" );
            pt.y = parseBoardUnits( "end y" );
            track->SetEnd( pt );
            break;

        case T_width:
            track->SetWidth( parseBoardUnits( "width" ) );
            break;

        case T_layer:
            track->SetLayer( parseBoardItemLayer() );
            break;

        case T_net:
            track->SetNetCode( getNetCode( parseInt( "net number" ) ) );
            break;

        case T_tstamp:
            track->SetTimeStamp( parseHex() );
            break;

        case T_status:
            track->SetStatus( static_cast<STATUS_FLAGS>( parseHex() ) );
            break;

        default:
            Expecting( "start, end, width, layer, net, tstamp, or status" );
        }

        NeedRIGHT();
    }

    return track.release();
}


VIA* PCB_PARSER::parseVIA() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_via, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as VIA." ) );

    wxPoint pt;
    T token;

    std::auto_ptr< VIA > via( new VIA( m_board ) );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_blind:
            via->SetViaType( VIA_BLIND_BURIED );
            break;

        case T_micro:
            via->SetViaType( VIA_MICROVIA );
            break;

        case T_at:
            pt.x = parseBoardUnits( "start x" );
            pt.y = parseBoardUnits( "start y" );
            via->SetStart( pt );
            via->SetEnd( pt );
            NeedRIGHT();
            break;

        case T_size:
            via->SetWidth( parseBoardUnits( "via width" ) );
            NeedRIGHT();
            break;

        case T_drill:
            via->SetDrill( parseBoardUnits( "drill diameter" ) );
            NeedRIGHT();
            break;

        case T_layers:
            {
                LAYER_ID layer1, layer2;
                NextTok();
                layer1 = lookUpLayer<LAYER_ID>( m_layerIndices );
                NextTok();
                layer2 = lookUpLayer<LAYER_ID>( m_layerIndices );
                via->SetLayerPair( layer1, layer2 );
                NeedRIGHT();
            }
            break;

        case T_net:
            via->SetNetCode( getNetCode( parseInt( "net number" ) ) );
            NeedRIGHT();
            break;

        case T_tstamp:
            via->SetTimeStamp( parseHex() );
            NeedRIGHT();
            break;

        case T_status:
            via->SetStatus( static_cast<STATUS_FLAGS>( parseHex() ) );
            NeedRIGHT();
            break;

        default:
            Expecting( "blind, micro, at, size, drill, layers, net, tstamp, or status" );
        }
    }

    return via.release();
}


ZONE_CONTAINER* PCB_PARSER::parseZONE_CONTAINER() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_zone, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as ZONE_CONTAINER." ) );

    CPolyLine::HATCH_STYLE hatchStyle = CPolyLine::NO_HATCH;

    int     hatchPitch = Mils2iu( CPolyLine::GetDefaultHatchPitchMils() );
    wxPoint pt;
    T       token;
    int     tmp;
    wxString    netnameFromfile;    // the zone net name find in file

    // bigger scope since each filled_polygon is concatenated in here
    CPOLYGONS_LIST pts;

    std::auto_ptr< ZONE_CONTAINER > zone( new ZONE_CONTAINER( m_board ) );

    zone->SetPriority( 0 );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_net:
            // Init the net code only, not the netname, to be sure
            // the zone net name is the name read in file.
            // (When mismatch, the user will be prompted in DRC, to fix the actual name)
            tmp = getNetCode( parseInt( "net number" ) );

            if( tmp < 0 )
                tmp = 0;

            zone->SetNetCode( tmp );

            NeedRIGHT();
            break;

        case T_net_name:
            NeedSYMBOLorNUMBER();
            netnameFromfile = FromUTF8();
            NeedRIGHT();
            break;

        case T_layer:
            zone->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_tstamp:
            zone->SetTimeStamp( parseHex() );
            NeedRIGHT();
            break;

        case T_hatch:
            token = NextTok();

            if( token != T_none && token != T_edge && token != T_full )
                Expecting( "none, edge, or full" );

            switch( token )
            {
            default:
            case T_none:   hatchStyle = CPolyLine::NO_HATCH;        break;
            case T_edge:   hatchStyle = CPolyLine::DIAGONAL_EDGE;   break;
            case T_full:   hatchStyle = CPolyLine::DIAGONAL_FULL;
            }

            hatchPitch = parseBoardUnits( "hatch pitch" );
            NeedRIGHT();
            break;

        case T_priority:
            zone->SetPriority( parseInt( "zone priority" ) );
            NeedRIGHT();
            break;

        case T_connect_pads:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_yes:
                    zone->SetPadConnection( PAD_IN_ZONE );
                    break;

                case T_no:
                    zone->SetPadConnection( PAD_NOT_IN_ZONE );
                    break;

                case T_thru_hole_only:
                    zone->SetPadConnection( THT_THERMAL );
                    break;

                case T_clearance:
                    zone->SetZoneClearance( parseBoardUnits( "zone clearance" ) );
                    NeedRIGHT();
                    break;

                default:
                    Expecting( "yes, no, or clearance" );
                }
            }

            break;

        case T_min_thickness:
            zone->SetMinThickness( parseBoardUnits( T_min_thickness ) );
            NeedRIGHT();
            break;

        case T_fill:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_yes:
                    zone->SetIsFilled( true );
                    break;

                case T_mode:
                    token = NextTok();

                    if( token != T_segment && token != T_polygon )
                        Expecting( "segment or polygon" );

                    // @todo Create an enum for fill modes.
                    zone->SetFillMode( token == T_polygon ? 0 : 1 );
                    NeedRIGHT();
                    break;

                case T_arc_segments:
                    zone->SetArcSegmentCount( parseInt( "arc segment count" ) );
                    NeedRIGHT();
                    break;

                case T_thermal_gap:
                    zone->SetThermalReliefGap( parseBoardUnits( T_thermal_gap ) );
                    NeedRIGHT();
                    break;

                case T_thermal_bridge_width:
                    zone->SetThermalReliefCopperBridge( parseBoardUnits( T_thermal_bridge_width ) );
                    NeedRIGHT();
                    break;

                case T_smoothing:
                    switch( NextTok() )
                    {
                    case T_none:
                        zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_NONE );
                        break;

                    case T_chamfer:
                        zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_CHAMFER );
                        break;

                    case T_fillet:
                        zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_FILLET );
                        break;

                    default:
                        Expecting( "none, chamfer, or fillet" );
                    }
                    NeedRIGHT();
                    break;

                case T_radius:
                    zone->SetCornerRadius( parseBoardUnits( "corner radius" ) );
                    NeedRIGHT();
                    break;

                default:
                    Expecting( "mode, arc_segments, thermal_gap, thermal_bridge_width, "
                               "smoothing, or radius" );
                }
            }
            break;

        case T_keepout:
            zone->SetIsKeepout( true );

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_tracks:
                    token = NextTok();

                    if( token != T_allowed && token != T_not_allowed )
                        Expecting( "allowed or not_allowed" );
                    zone->SetDoNotAllowTracks( token == T_not_allowed );
                    break;

                case T_vias:
                    token = NextTok();

                    if( token != T_allowed && token != T_not_allowed )
                        Expecting( "allowed or not_allowed" );
                    zone->SetDoNotAllowVias( token == T_not_allowed );
                    break;

                case T_copperpour:
                    token = NextTok();

                    if( token != T_allowed && token != T_not_allowed )
                        Expecting( "allowed or not_allowed" );
                    zone->SetDoNotAllowCopperPour( token == T_not_allowed );
                    break;

                default:
                    Expecting( "tracks, vias or copperpour" );
                }

                NeedRIGHT();
            }

            break;

        case T_polygon:
            {
                std::vector< wxPoint > corners;

                NeedLEFT();
                token = NextTok();

                if( token != T_pts )
                    Expecting( T_pts );

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                {
                    corners.push_back( parseXY() );
                }

                NeedRIGHT();
                zone->AddPolygon( corners );
            }
            break;

        case T_filled_polygon:
            {
                // "(filled_polygon (pts"
                NeedLEFT();
                token = NextTok();

                if( token != T_pts )
                    Expecting( T_pts );

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                {
                    pts.Append( CPolyPt( parseXY() ) );
                }

                NeedRIGHT();
                pts.CloseLastContour();
            }
            break;

        case T_fill_segments:
            {
                std::vector< SEGMENT > segs;

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                {
                    if( token != T_LEFT )
                        Expecting( T_LEFT );

                    token = NextTok();

                    if( token != T_pts )
                        Expecting( T_pts );

                    SEGMENT segment( parseXY(), parseXY() );
                    NeedRIGHT();
                    segs.push_back( segment );
                }

                zone->AddFillSegments( segs );
            }
            break;

        default:
            Expecting( "net, layer, tstamp, hatch, priority, connect_pads, min_thickness, "
                       "fill, polygon, filled_polygon, or fill_segments" );
        }
    }

    if( zone->GetNumCorners() > 2 )
    {
        if( !zone->IsOnCopperLayer() )
        {
            zone->SetFillMode( 0 );
            zone->SetNetCode( NETINFO_LIST::UNCONNECTED );
        }

        // Set hatch here, after outlines corners are read
        zone->Outline()->SetHatch( hatchStyle, hatchPitch, true );
    }

    if( pts.GetCornersCount() )
        zone->AddFilledPolysList( pts );

    // Ensure keepout and non copper zones do not have a net
    // (which have no sense for these zones)
    // the netcode 0 is used for these zones
    bool zone_has_net = zone->IsOnCopperLayer() && !zone->GetIsKeepout();

    if( !zone_has_net )
        zone->SetNetCode( NETINFO_LIST::UNCONNECTED );

    // Ensure the zone net name is valid, and matches the net code, for copper zones
    if( zone_has_net && ( zone->GetNet()->GetNetname() != netnameFromfile ) )
    {
        // Can happens which old boards, with nonexistent nets ...
        // or after being edited by hand
        // We try to fix the mismatch.
        NETINFO_ITEM* net = m_board->FindNet( netnameFromfile );

        if( net )   // An existing net has the same net name. use it for the zone
            zone->SetNetCode( net->GetNet() );
        else    // Not existing net: add a new net to keep trace of the zone netname
        {
            int newnetcode = m_board->GetNetCount();
            net = new NETINFO_ITEM( m_board, netnameFromfile, newnetcode );
            m_board->AppendNet( net );

            // Store the new code mapping
            pushValueIntoMap( newnetcode, net->GetNet() );
            // and update the zone netcode
            zone->SetNetCode( net->GetNet() );

            // Prompt the user
            wxString msg;
            msg.Printf( _( "There is a zone that belongs to a not existing net\n"
                           "\"%s\"\n"
                           "you should verify and edit it (run DRC test)." ),
                           GetChars( netnameFromfile ) );
            DisplayError( NULL, msg );
        }
    }

    return zone.release();
}


PCB_TARGET* PCB_PARSER::parsePCB_TARGET() throw( IO_ERROR, PARSE_ERROR )
{
    wxCHECK_MSG( CurTok() == T_target, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TARGET." ) );

    wxPoint pt;
    T token;

    std::auto_ptr< PCB_TARGET > target( new PCB_TARGET( NULL ) );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_x:
            target->SetShape( 1 );
            break;

        case T_plus:
            target->SetShape( 0 );
            break;

        case T_at:
            pt.x = parseBoardUnits( "target x position" );
            pt.y = parseBoardUnits( "target y position" );
            target->SetPosition( pt );
            NeedRIGHT();
            break;

        case T_size:
            target->SetSize( parseBoardUnits( "target size" ) );
            NeedRIGHT();
            break;

        case T_width:
            target->SetWidth( parseBoardUnits( "target thickness" ) );
            NeedRIGHT();
            break;

        case T_layer:
            target->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_tstamp:
            target->SetTimeStamp( parseHex() );
            NeedRIGHT();
            break;

        default:
            Expecting( "x, plus, at, size, width, layer or tstamp" );
        }
    }

    return target.release();
}

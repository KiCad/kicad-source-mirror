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
 * @file pcb_parser.cpp
 * @brief Pcbnew s-expression file format parser implementation.
 */

#include <errno.h>
#include <common.h>
#include <confirm.h>
#include <macros.h>
#include <trigo.h>
#include <title_block.h>

#include <class_board.h>
#include <class_dimension.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <class_pcb_target.h>
#include <class_module.h>
#include <netclass.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <kicad_plugin.h>
#include <pcb_plot_params_parser.h>
#include <pcb_plot_params.h>
#include <zones.h>
#include <pcb_parser.h>
#include <convert_basic_shapes_to_polygon.h>    // for RECT_CHAMFER_POSITIONS definition

using namespace PCB_KEYS_T;


void PCB_PARSER::init()
{
    m_showLegacyZoneWarning = true;
    m_tooRecent = false;
    m_requiredVersion = 0;
    m_layerIndices.clear();
    m_layerMasks.clear();

    // Add untranslated default (i.e. english) layernames.
    // Some may be overridden later if parsing a board rather than a footprint.
    // The english name will survive if parsing only a footprint.
    for( LAYER_NUM layer = 0;  layer < PCB_LAYER_ID_COUNT;  ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );

        m_layerIndices[ untranslated ] = PCB_LAYER_ID( layer );
        m_layerMasks[ untranslated ]   = LSET( PCB_LAYER_ID( layer ) );
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

        m_layerMasks[ key ] = LSET( PCB_LAYER_ID( In15_Cu - i ) );
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

double PCB_PARSER::parseDouble()
{
    char* tmp;

    errno = 0;

    double fval = strtod( CurText(), &tmp );

    if( errno )
    {
        wxString error;
        error.Printf( _( "Invalid floating point number in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      GetChars( CurSource() ), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    if( CurText() == tmp )
    {
        wxString error;
        error.Printf( _( "Missing floating point number in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      GetChars( CurSource() ), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    return fval;
}


bool PCB_PARSER::parseBool()
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


int PCB_PARSER::parseVersion()
{
    if( NextTok() != T_version )
        Expecting( GetTokenText( T_version ) );

    int pcb_version = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );

    NeedRIGHT();

    return pcb_version;
}


wxString PCB_PARSER::GetRequiredVersion()
{
    int year, month, day;

    year = m_requiredVersion / 10000;
    month = ( m_requiredVersion / 100 ) - ( year * 100 );
    day = m_requiredVersion - ( year * 10000 ) - ( month * 100 );

    // wx throws an assertion, not a catchable exception, when the date is invalid.
    // User input shouldn't give wx asserts, so check manually and throw a proper
    // error instead
    if( day <= 0 || month <= 0 || month > 12 ||
            day > wxDateTime::GetNumberOfDays( (wxDateTime::Month)( month - 1 ), year ) )
    {
        wxString err;
        err.Printf( _( "Cannot interpret date code %d" ), m_requiredVersion );
        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    wxDateTime date( day, (wxDateTime::Month)( month - 1 ), year, 0, 0, 0, 0 );
    return date.FormatDate();
}


wxPoint PCB_PARSER::parseXY()
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


void PCB_PARSER::parseXY( int* aX, int* aY )
{
    wxPoint pt = parseXY();

    if( aX )
        *aX = pt.x;

    if( aY )
        *aY = pt.y;
}


void PCB_PARSER::parseEDA_TEXT( EDA_TEXT* aText )
{
    wxCHECK_RET( CurTok() == T_effects,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDA_TEXT." ) );

    T token;

    // Prior to v5.0 text size was omitted from file format if equal to 60mils
    // Now, it is always explicitly written to file
    bool foundTextSize = false;

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
                    continue;

                switch( token )
                {
                case T_size:
                    {
                        wxSize sz;
                        sz.SetHeight( parseBoardUnits( "text height" ) );
                        sz.SetWidth( parseBoardUnits( "text width" ) );
                        aText->SetTextSize( sz );
                        NeedRIGHT();

                        foundTextSize = true;
                    }
                    break;

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

    // Text size was not specified in file, force legacy default units
    // 60mils is 1.524mm
    if( !foundTextSize )
    {
    	const float defaultTextSize = 1.524f * IU_PER_MM;

    	aText->SetTextSize( wxSize( defaultTextSize, defaultTextSize ) );
    }
}


MODULE_3D_SETTINGS* PCB_PARSER::parse3DModel()
{
    wxCHECK_MSG( CurTok() == T_model, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as MODULE_3D_SETTINGS." ) );

    T token;

    MODULE_3D_SETTINGS* n3D = new MODULE_3D_SETTINGS;
    NeedSYMBOLorNUMBER();
    n3D->m_Filename = FromUTF8();

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

            /* Note:
             * Prior to KiCad v5, model offset was designated by "at",
             * and the units were in inches.
             * Now we use mm, but support reading of legacy files
             */

            n3D->m_Offset.x = parseDouble( "x value" ) * 25.4f;
            n3D->m_Offset.y = parseDouble( "y value" ) * 25.4f;
            n3D->m_Offset.z = parseDouble( "z value" ) * 25.4f;
            NeedRIGHT();
            break;

        case T_offset:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            /*
             * 3D model offset is in mm
             */
            n3D->m_Offset.x = parseDouble( "x value" );
            n3D->m_Offset.y = parseDouble( "y value" );
            n3D->m_Offset.z = parseDouble( "z value" );
            NeedRIGHT();
            break;

        case T_scale:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_Scale.x = parseDouble( "x value" );
            n3D->m_Scale.y = parseDouble( "y value" );
            n3D->m_Scale.z = parseDouble( "z value" );
            NeedRIGHT();
            break;

        case T_rotate:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_Rotation.x = parseDouble( "x value" );
            n3D->m_Rotation.y = parseDouble( "y value" );
            n3D->m_Rotation.z = parseDouble( "z value" );
            NeedRIGHT();
            break;

        default:
            Expecting( "at, offset, scale, or rotate" );
        }

        NeedRIGHT();
    }

    return n3D;
}


BOARD_ITEM* PCB_PARSER::Parse()
{
    T               token;
    BOARD_ITEM*     item;
    LOCALE_IO       toggle;

    // MODULEs can be prefixed with an initial block of single line comments and these
    // are kept for Format() so they round trip in s-expression form.  BOARDs might
    // eventually do the same, but currently do not.
    std::unique_ptr<wxArrayString> initial_comments( ReadCommentLines() );

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
        err.Printf( _( "Unknown token \"%s\"" ), GetChars( FromUTF8() ) );
        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    return item;
}


BOARD* PCB_PARSER::parseBOARD()
{
    try
    {
        return parseBOARD_unchecked();
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( m_tooRecent )
            throw FUTURE_FORMAT_ERROR( parse_error, GetRequiredVersion() );
        else
            throw;
    }
}


BOARD* PCB_PARSER::parseBOARD_unchecked()
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
            m_board->Add( parseTRACK(), ADD_INSERT );
            break;

        case T_via:
            m_board->Add( parseVIA(), ADD_INSERT );
            break;

        case T_zone:
            m_board->Add( parseZONE_CONTAINER(), ADD_APPEND );
            break;

        case T_target:
            m_board->Add( parsePCB_TARGET(), ADD_APPEND );
            break;

        default:
            wxString err;
            err.Printf( _( "Unknown token \"%s\"" ), GetChars( FromUTF8() ) );
            THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
        }
    }

    if( m_undefinedLayers.size() > 0 )
    {
        bool deleteItems;
        std::vector<BOARD_ITEM*> deleteList;
        wxString msg = wxString::Format( _( "Items found on undefined layers.  Do you wish to\n"
                                            "rescue them to the Cmts.User layer?" ) );
        wxString details = wxString::Format( _( "Undefined layers:" ) );

        for( const wxString& undefinedLayer : m_undefinedLayers )
            details += wxT( "\n   " ) + undefinedLayer;

        wxRichMessageDialog dlg( nullptr, msg, _( "Warning" ),
                                 wxYES_NO | wxCANCEL | wxCENTRE | wxICON_WARNING | wxSTAY_ON_TOP );
        dlg.ShowDetailedText( details );
        dlg.SetYesNoCancelLabels( _( "Rescue" ), _( "Delete" ), _( "Cancel" ) );

        switch( dlg.ShowModal() )
        {
        case wxID_YES:    deleteItems = false; break;
        case wxID_NO:     deleteItems = true;  break;
        case wxID_CANCEL:
        default:          THROW_IO_ERROR( wxT( "CANCEL" ) );
        }

        auto visitItem = [&]( BOARD_ITEM* item )
                            {
                                if( item->GetLayer() == Rescue )
                                {
                                    if( deleteItems )
                                        deleteList.push_back( item );
                                    else
                                        item->SetLayer( Cmts_User );
                                }
                            };

        for( auto segm : m_board->Tracks() )
        {
            if( segm->Type() == PCB_VIA_T )
            {
                VIA*         via = (VIA*) segm;
                PCB_LAYER_ID top_layer, bottom_layer;

                if( via->GetViaType() == VIA_THROUGH )
                    continue;

                via->LayerPair( &top_layer, &bottom_layer );

                if( top_layer == Rescue || bottom_layer == Rescue )
                {
                    if( deleteItems )
                        deleteList.push_back( via );
                    else
                    {
                        if( top_layer == Rescue )
                            top_layer = F_Cu;

                        if( bottom_layer == Rescue )
                            bottom_layer = B_Cu;

                        via->SetLayerPair( top_layer, bottom_layer );
                    }
                }
            }
            else
                visitItem( segm );
        }

        for( BOARD_ITEM* zone : m_board->Zones() )
            visitItem( zone );

        for( BOARD_ITEM* drawing : m_board->Drawings() )
            visitItem( drawing );

        for( BOARD_ITEM* item : deleteList )
            m_board->Delete( item );

        m_undefinedLayers.clear();
    }

    return m_board;
}


void PCB_PARSER::parseHeader()
{
    wxCHECK_RET( CurTok() == T_kicad_pcb,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a header." ) );

    NeedLEFT();

    T tok = NextTok();
    if( tok == T_version )
    {
        m_requiredVersion = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );
        m_tooRecent = ( m_requiredVersion > SEXPR_BOARD_FILE_VERSION );
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
        m_requiredVersion = SEXPR_BOARD_FILE_VERSION;
        m_tooRecent = ( m_requiredVersion > SEXPR_BOARD_FILE_VERSION );

        // Skip the host name and host build version information.
        NeedSYMBOL();
        NeedSYMBOL();
        NeedRIGHT();
    }

    m_board->SetFileFormatVersionAtLoad( m_requiredVersion );
}


void PCB_PARSER::parseGeneralSection()
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
            // ignore
            parseInt( "no connect count" );
            NeedRIGHT();
            break;

        default:              // Skip everything but the board thickness.
            //wxLogDebug( wxT( "Skipping general section token %s " ), GetChars( GetTokenString( token ) ) );

            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( !IsSymbol( token ) && token != T_NUMBER )
                    Expecting( "symbol or number" );
            }
        }
    }
}


void PCB_PARSER::parsePAGE_INFO()
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
        err.Printf( _( "Page type \"%s\" is not valid " ), GetChars( FromUTF8() ) );
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


void PCB_PARSER::parseTITLE_BLOCK()
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


void PCB_PARSER::parseLayer( LAYER* aLayer )
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

    NeedSYMBOLorNUMBER();
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


void PCB_PARSER::createOldLayerMapping( std::unordered_map< std::string, std::string >& aMap )
{
    // N.B. This mapping only includes Italian, Polish and French as they were the only languages that
    // mapped the layer names as of cc2022b1ac739aa673d2a0b7a2047638aa7a47b3 (kicad-i18n) when the
    // bug was fixed in KiCad source.

    // Italian
    aMap["Adesivo.Retro"] = "B.Adhes";
    aMap["Adesivo.Fronte"] = "F.Adhes";
    aMap["Pasta.Retro"] = "B.Paste";
    aMap["Pasta.Fronte"] = "F.Paste";
    aMap["Serigrafia.Retro"] = "B.SilkS";
    aMap["Serigrafia.Fronte"] = "F.SilkS";
    aMap["Maschera.Retro"] = "B.Mask";
    aMap["Maschera.Fronte"] = "F.Mask";
    aMap["Grafica"] = "Dwgs.User";
    aMap["Commenti"] = "Cmts.User";
    aMap["Eco1"] = "Eco1.User";
    aMap["Eco2"] = "Eco2.User";
    aMap["Contorno.scheda"] = "Edge.Cuts";

    // Polish
    aMap["Kleju_Dolna"] = "B.Adhes";
    aMap["Kleju_Gorna"] = "F.Adhes";
    aMap["Pasty_Dolna"] = "B.Paste";
    aMap["Pasty_Gorna"] = "F.Paste";
    aMap["Opisowa_Dolna"] = "B.SilkS";
    aMap["Opisowa_Gorna"] = "F.SilkS";
    aMap["Maski_Dolna"] = "B.Mask";
    aMap["Maski_Gorna"] = "F.Mask";
    aMap["Rysunkowa"] = "Dwgs.User";
    aMap["Komentarzy"] = "Cmts.User";
    aMap["ECO1"] = "Eco1.User";
    aMap["ECO2"] = "Eco2.User";
    aMap["Krawedziowa"] = "Edge.Cuts";

    // French
    aMap["Dessous.Adhes"] = "B.Adhes";
    aMap["Dessus.Adhes"] = "F.Adhes";
    aMap["Dessous.Pate"] = "B.Paste";
    aMap["Dessus.Pate"] = "F.Paste";
    aMap["Dessous.SilkS"] = "B.SilkS";
    aMap["Dessus.SilkS"] = "F.SilkS";
    aMap["Dessous.Masque"] = "B.Mask";
    aMap["Dessus.Masque"] = "F.Mask";
    aMap["Dessin.User"] = "Dwgs.User";
    aMap["Contours.Ci"] = "Edge.Cuts";
}


void PCB_PARSER::parseLayers()
{
    wxCHECK_RET( CurTok() == T_layers,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as layers." ) );

    T       token;
    LSET    visibleLayers;
    LSET    enabledLayers;
    int     copperLayerCount = 0;
    LAYER   layer;

    std::unordered_map< std::string, std::string > v3_layer_names;
    std::vector<LAYER>  cu;

    createOldLayerMapping( v3_layer_names );

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

            m_board->SetLayerDescr( PCB_LAYER_ID( it->m_number ), *it );

            UTF8 name = it->m_name;

            m_layerIndices[ name ] = PCB_LAYER_ID( it->m_number );
            m_layerMasks[   name ] = LSET( PCB_LAYER_ID( it->m_number ) );
        }

        copperLayerCount = cu.size();
    }

    // process non-copper layers
    while( token != T_RIGHT )
    {
        LAYER_ID_MAP::const_iterator it = m_layerIndices.find( UTF8( layer.m_name ) );

        if( it == m_layerIndices.end() )
        {
            auto new_layer_it = v3_layer_names.find( layer.m_name.ToStdString() );

            if( new_layer_it != v3_layer_names.end() )
                it = m_layerIndices.find( new_layer_it->second );

            if( it == m_layerIndices.end() )
            {
                wxString error = wxString::Format(
                    _( "Layer \"%s\" in file \"%s\" at line %d, is not in fixed layer hash" ),
                    GetChars( layer.m_name ),
                    GetChars( CurSource() ),
                    CurLineNumber(),
                    CurOffset()
                    );

                THROW_IO_ERROR( error );
            }

            // If we are here, then we have found a translated layer name.  Put it in the maps so that
            // items on this layer get the appropriate layer ID number
            m_layerIndices[ UTF8( layer.m_name ) ] = it->second;
            m_layerMasks[   UTF8( layer.m_name ) ] = it->second;
            layer.m_name = it->first;
        }

        layer.m_number = it->second;
        enabledLayers.set( layer.m_number );

        if( layer.m_visible )
            visibleLayers.set( layer.m_number );

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
T PCB_PARSER::lookUpLayer( const M& aMap )
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

        m_undefinedLayers.insert( curText );
        return Rescue;
    }

    return it->second;
}


PCB_LAYER_ID PCB_PARSER::parseBoardItemLayer()
{
    wxCHECK_MSG( CurTok() == T_layer, UNDEFINED_LAYER,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as layer." ) );

    NextTok();

    PCB_LAYER_ID layerIndex = lookUpLayer<PCB_LAYER_ID>( m_layerIndices );

    // Handle closing ) in object parser.

    return layerIndex;
}


LSET PCB_PARSER::parseBoardItemLayersAsMask()
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


void PCB_PARSER::parseSetup()
{
    wxCHECK_RET( CurTok() == T_setup,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as setup." ) );

    T token;
    NETCLASSPTR defaultNetClass = m_board->GetDesignSettings().GetDefault();
    // TODO Orson: is it really necessary to first operate on a copy and then apply it?
    // would not it be better to use reference here and apply all the changes instantly?
    BOARD_DESIGN_SETTINGS designSettings = m_board->GetDesignSettings();
    ZONE_SETTINGS zoneSettings = m_board->GetZoneSettings();

    // Missing soldermask min width value means that the user has set the value to 0 and
    // not the default value (0.25mm)
    designSettings.m_SolderMaskMinWidth = 0;

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
                designSettings.m_ViasDimensionsList.emplace_back( VIA_DIMENSION( viaSize, viaDrill ) );
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

        case T_user_diff_pair:
            {
                int width = parseBoardUnits( "user diff-pair width" );
                int gap = parseBoardUnits( "user diff-pair gap" );
                int viaGap = parseBoardUnits( "user diff-pair via gap" );
                designSettings.m_DiffPairDimensionsList.emplace_back( DIFF_PAIR_DIMENSION( width, gap, viaGap ) );
                NeedRIGHT();
            }
            break;

        case T_segment_width:   // note: legacy (pre-6.0) token
            designSettings.m_LineThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( T_segment_width );
            NeedRIGHT();
            break;

        case T_edge_width:      // note: legacy (pre-6.0) token
            designSettings.m_LineThickness[ LAYER_CLASS_EDGES ] = parseBoardUnits( T_edge_width );
            NeedRIGHT();
            break;

        case T_mod_edge_width:  // note: legacy (pre-6.0) token
            designSettings.m_LineThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( T_mod_edge_width );
            NeedRIGHT();
            break;

        case T_pcb_text_width:  // note: legacy (pre-6.0) token
            designSettings.m_TextThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( T_pcb_text_width );
            NeedRIGHT();
            break;

        case T_mod_text_width:  // note: legacy (pre-6.0) token
            designSettings.m_TextThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( T_mod_text_width );
            NeedRIGHT();
            break;

        case T_pcb_text_size:   // note: legacy (pre-6.0) token
            designSettings.m_TextSize[ LAYER_CLASS_COPPER ].x = parseBoardUnits( "pcb text width" );
            designSettings.m_TextSize[ LAYER_CLASS_COPPER ].y = parseBoardUnits( "pcb text height" );
            NeedRIGHT();
            break;

        case T_mod_text_size:   // note: legacy (pre-6.0) token
            designSettings.m_TextSize[ LAYER_CLASS_SILK ].x = parseBoardUnits( "module text width" );
            designSettings.m_TextSize[ LAYER_CLASS_SILK ].y = parseBoardUnits( "module text height" );
            NeedRIGHT();
            break;

        case T_defaults:
            parseDefaults( designSettings );
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
            designSettings.SetVisibleElements( parseHex() | MIN_VISIBILITY_MASK );
            NeedRIGHT();
            break;

        case T_max_error:
            designSettings.m_MaxError = parseBoardUnits( T_max_error );
            NeedRIGHT();
            break;

        case T_filled_areas_thickness:
            designSettings.m_ZoneUseNoOutlineInFill = not parseBool();
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
}


void PCB_PARSER::parseDefaults( BOARD_DESIGN_SETTINGS& designSettings )
{
    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_edge_clearance:
            designSettings.m_CopperEdgeClearance = parseBoardUnits( T_edge_clearance );
            NeedRIGHT();
            break;

        case T_copper_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_copper_text_dims:
            parseDefaultTextDims( designSettings, LAYER_CLASS_COPPER );
            break;

        case T_courtyard_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_COURTYARD ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_edge_cuts_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_EDGES ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_silk_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_silk_text_dims:
            parseDefaultTextDims( designSettings, LAYER_CLASS_SILK );
            break;

        case T_other_layers_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_OTHERS ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_other_layers_text_dims:
            parseDefaultTextDims( designSettings, LAYER_CLASS_OTHERS );
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void PCB_PARSER::parseDefaultTextDims( BOARD_DESIGN_SETTINGS& aSettings, int aLayer )
{
    T token;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_size:
            aSettings.m_TextSize[ aLayer ].x = parseBoardUnits( "default text size X" );
            aSettings.m_TextSize[ aLayer ].y = parseBoardUnits( "default text size Y" );
            NeedRIGHT();
            break;

        case T_thickness:
            aSettings.m_TextThickness[ aLayer ] = parseBoardUnits( "default text width" );
            NeedRIGHT();
            break;

        case T_italic:
            aSettings.m_TextItalic[ aLayer ] = true;
            break;

        case T_keep_upright:
            aSettings.m_TextUpright[ aLayer ] = true;
            break;

        default:
            Expecting( "size, thickness, italic or keep_upright" );
        }
    }
}


void PCB_PARSER::parseNETINFO_ITEM()
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
    if( netCode > NETINFO_LIST::UNCONNECTED || !m_board->FindNet( NETINFO_LIST::UNCONNECTED ) )
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_board, name, netCode );
        m_board->Add( net );

        // Store the new code mapping
        pushValueIntoMap( netCode, net->GetNet() );
    }
}


void PCB_PARSER::parseNETCLASS()
{
    wxCHECK_RET( CurTok() == T_net_class,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as net class." ) );

    T token;

    NETCLASSPTR nc = std::make_shared<NETCLASS>( wxEmptyString );

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

        case T_diff_pair_width:
            nc->SetDiffPairWidth( parseBoardUnits( T_diff_pair_width ) );
            break;

        case T_diff_pair_gap:
            nc->SetDiffPairGap( parseBoardUnits( T_diff_pair_gap ) );
            break;

        case T_add_net:
            NeedSYMBOLorNUMBER();
            nc->Add( FromUTF8() );
            break;

        default:
            Expecting( "clearance, trace_width, via_dia, via_drill, uvia_dia, uvia_drill, diff_pair_width, diff_pair_gap or add_net" );
        }

        NeedRIGHT();
    }

    if( !m_board->GetDesignSettings().m_NetClasses.Add( nc ) )
    {
        // Must have been a name conflict, this is a bad board file.
        // User may have done a hand edit to the file.

        // unique_ptr will delete nc on this code path

        wxString error;
        error.Printf( _( "Duplicate NETCLASS name \"%s\" in file \"%s\" at line %d, offset %d" ),
                      nc->GetName().GetData(), CurSource().GetData(), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }
}


DRAWSEGMENT* PCB_PARSER::parseDRAWSEGMENT( bool aAllowCirclesZeroWidth )
{
    wxCHECK_MSG( CurTok() == T_gr_arc || CurTok() == T_gr_circle || CurTok() == T_gr_curve ||
                 CurTok() == T_gr_line || CurTok() == T_gr_poly, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as DRAWSEGMENT." ) );

    T token;
    wxPoint pt;
    std::unique_ptr< DRAWSEGMENT > segment( new DRAWSEGMENT( NULL ) );

    switch( CurTok() )
    {
    case T_gr_arc:
        segment->SetShape( S_ARC );
        NeedLEFT();
        token = NextTok();

        // the start keyword actually gives the arc center
        // Allows also T_center for future change
        if( token != T_start && token != T_center )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetCenter( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )    // the end keyword actually gives the starting point of the arc
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetArcStart( pt );
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
        segment->SetCenter( pt );
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
        segment->SetWidth( 0 ); // this is the default value. will be (perhaps) modified later
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

    // Only filled polygons may have a zero-line width
    // This is not permitted in KiCad but some external tools generate invalid
    // files.
    // However in custom pad shapes, zero-line width is allowed for filled circles
    if( segment->GetShape() != S_POLYGON && segment->GetWidth() == 0 &&
        !( segment->GetShape() == S_CIRCLE && aAllowCirclesZeroWidth ) )
        segment->SetWidth( Millimeter2iu( DEFAULT_LINE_WIDTH ) );

    return segment.release();
}


TEXTE_PCB* PCB_PARSER::parseTEXTE_PCB()
{
    wxCHECK_MSG( CurTok() == T_gr_text, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as TEXTE_PCB." ) );

    T token;

    std::unique_ptr<TEXTE_PCB> text( new TEXTE_PCB( m_board ) );
    NeedSYMBOLorNUMBER();

    text->SetText( FromUTF8() );
    NeedLEFT();
    token = NextTok();

    if( token != T_at )
        Expecting( T_at );

    wxPoint pt;

    pt.x = parseBoardUnits( "X coordinate" );
    pt.y = parseBoardUnits( "Y coordinate" );
    text->SetTextPos( pt );

    // If there is no orientation defined, then it is the default value of 0 degrees.
    token = NextTok();

    if( token == T_NUMBER )
    {
        text->SetTextAngle( parseDouble() * 10.0 );
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


DIMENSION* PCB_PARSER::parseDIMENSION()
{
    wxCHECK_MSG( CurTok() == T_dimension, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as DIMENSION." ) );

    T token;

    std::unique_ptr<DIMENSION> dimension( new DIMENSION( NULL ) );

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

            // This copy  (using the copy constructor) rebuild the text timestamp,
            // that is not what we want.
            dimension->Text() = *text;
            // reinitialises the text time stamp to the right value (the dimension time stamp)
            dimension->Text().SetTimeStamp( dimension->GetTimeStamp() );
            dimension->SetPosition( text->GetTextPos() );

            EDA_UNITS_T units = INCHES;
            bool useMils = false;
            FetchUnitsFromString( text->GetText(), units, useMils );
            dimension->SetUnits( units, useMils );

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


MODULE* PCB_PARSER::parseMODULE( wxArrayString* aInitialComments )
{
    try
    {
        return parseMODULE_unchecked( aInitialComments );
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( m_tooRecent )
            throw FUTURE_FORMAT_ERROR( parse_error, GetRequiredVersion() );
        else
            throw;
    }
}


MODULE* PCB_PARSER::parseMODULE_unchecked( wxArrayString* aInitialComments )
{
    wxCHECK_MSG( CurTok() == T_module, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as MODULE." ) );

    wxString name;
    wxPoint  pt;
    T        token;
    LIB_ID   fpid;

    std::unique_ptr<MODULE> module( new MODULE( m_board ) );

    module->SetInitialComments( aInitialComments );

    token = NextTok();

    if( !IsSymbol( token ) && token != T_NUMBER )
        Expecting( "symbol|number" );

    name = FromUTF8();

    if( !name.IsEmpty() && fpid.Parse( name, LIB_ID::ID_PCB, true ) >= 0 )
    {
        wxString error;
        error.Printf( _( "Invalid footprint ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                      GetChars( CurSource() ), CurLineNumber(), CurOffset() );
        THROW_IO_ERROR( error );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_version:
        {
            // Theoretically a module nested in a PCB could declare its own version, though
            // as of writing this comment we don't do that. Just in case, take the greater
            // version.
            int this_version = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );
            NeedRIGHT();
            m_requiredVersion = std::max( m_requiredVersion, this_version );
            m_tooRecent = ( m_requiredVersion > SEXPR_BOARD_FILE_VERSION );
            break;
        }

        case T_locked:
            module->SetLocked( true );
            break;

        case T_placed:
            module->SetIsPlaced( true );
            break;

        case T_layer:
        {
            // Footprints can be only on the front side or the back side.
            // but because we can find some stupid layer in file, ensure a
            // acceptable layer is set for the footprint
            PCB_LAYER_ID layer = parseBoardItemLayer();
            module->SetLayer( layer == B_Cu ? B_Cu : F_Cu );
        }
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
                double orientation = text->GetTextAngle();
                orientation -= module->GetOrientation();
                text->SetTextAngle( orientation );
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
                    module->Add( text );
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
                module->Add( em );
            }
            break;

        case T_pad:
            {
                D_PAD*  pad = parseD_PAD( module.get() );
                pt = pad->GetPos0();

                RotatePoint( &pt, module->GetOrientation() );
                pad->SetPosition( pt + module->GetPosition() );
                module->Add( pad, ADD_APPEND );
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


TEXTE_MODULE* PCB_PARSER::parseTEXTE_MODULE()
{
    wxCHECK_MSG( CurTok() == T_fp_text, NULL,
                 wxString::Format( wxT( "Cannot parse %s as TEXTE_MODULE at line %d, offset %d." ),
                                   GetChars( GetTokenString( CurTok() ) ),
                                   CurLineNumber(), CurOffset() ) );

    T token = NextTok();

    std::unique_ptr<TEXTE_MODULE> text( new TEXTE_MODULE( NULL ) );

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
        THROW_IO_ERROR( wxString::Format( _( "Cannot handle footprint text type %s" ),
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

    NextTok();

    if( CurTok() == T_NUMBER )
    {
        text->SetTextAngle( parseDouble() * 10.0 );
        NextTok();
    }

    if( CurTok() == T_unlocked )
    {
        text->SetKeepUpright( false );
        NextTok();
    }

    if( CurTok() != T_RIGHT )
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


EDGE_MODULE* PCB_PARSER::parseEDGE_MODULE()
{
    wxCHECK_MSG( CurTok() == T_fp_arc || CurTok() == T_fp_circle || CurTok() == T_fp_curve ||
                 CurTok() == T_fp_line || CurTok() == T_fp_poly, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDGE_MODULE." ) );

    wxPoint pt;
    T token;

    std::unique_ptr< EDGE_MODULE > segment( new EDGE_MODULE( NULL ) );

    switch( CurTok() )
    {
    case T_fp_arc:
        segment->SetShape( S_ARC );
        NeedLEFT();
        token = NextTok();

        // the start keyword actually gives the arc center
        // Allows also T_center for future change
        if( token != T_start && token != T_center )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        segment->SetStart0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )    // end keyword actually gives the starting point of the arc
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
        segment->SetBezier0_C1( parseXY() );
        segment->SetBezier0_C2( parseXY() );
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

    // Only filled polygons may have a zero-line width
    // This is not permitted in KiCad but some external tools generate invalid
    // files.
    if( segment->GetShape() != S_POLYGON && segment->GetWidth() == 0 )
        segment->SetWidth( Millimeter2iu( DEFAULT_LINE_WIDTH ) );

    return segment.release();
}


D_PAD* PCB_PARSER::parseD_PAD( MODULE* aParent )
{
    wxCHECK_MSG( CurTok() == T_pad, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as D_PAD." ) );

    wxSize  sz;
    wxPoint pt;

    std::unique_ptr< D_PAD > pad( new D_PAD( aParent ) );

    NeedSYMBOLorNUMBER();
    pad->SetName( FromUTF8() );

    T token = NextTok();

    switch( token )
    {
    case T_thru_hole:
        pad->SetAttribute( PAD_ATTRIB_STANDARD );
        break;

    case T_smd:
        pad->SetAttribute( PAD_ATTRIB_SMD );

        // Default D_PAD object is thru hole with drill.
        // SMD pads have no hole
        pad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case T_connect:
        pad->SetAttribute( PAD_ATTRIB_CONN );

        // Default D_PAD object is thru hole with drill.
        // CONN pads have no hole
        pad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case T_np_thru_hole:
        pad->SetAttribute( PAD_ATTRIB_HOLE_NOT_PLATED );
        break;

    default:
        Expecting( "thru_hole, smd, connect, or np_thru_hole" );
    }

    token = NextTok();

    switch( token )
    {
    case T_circle:
        pad->SetShape( PAD_SHAPE_CIRCLE );
        break;

    case T_rect:
        pad->SetShape( PAD_SHAPE_RECT );
        break;

    case T_oval:
        pad->SetShape( PAD_SHAPE_OVAL );
        break;

    case T_trapezoid:
        pad->SetShape( PAD_SHAPE_TRAPEZOID );
        break;

    case T_roundrect:
        // Note: the shape can be PAD_SHAPE_ROUNDRECT or PAD_SHAPE_CHAMFERED_RECT
        // (if champfer parameters are found later in pad descr.)
        pad->SetShape( PAD_SHAPE_ROUNDRECT );
        break;

    case T_custom:
        pad->SetShape( PAD_SHAPE_CUSTOM );
        break;

    default:
        Expecting( "circle, rectangle, roundrect, oval, trapezoid or custom" );
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
                        pad->SetDrillShape( PAD_DRILL_SHAPE_OBLONG );
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
                if( ( pad->GetAttribute() != PAD_ATTRIB_SMD ) && ( pad->GetAttribute() != PAD_ATTRIB_CONN ) )
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
            if( ! pad->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
                THROW_IO_ERROR(
                    wxString::Format( _( "Invalid net ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                                      CurSource(), CurLineNumber(), CurOffset() )
                    );

            NeedSYMBOLorNUMBER();

            // Test validity of the netname in file for netcodes expected having a net name
            if( m_board && pad->GetNetCode() > 0 &&
                FromUTF8() != m_board->FindNet( pad->GetNetCode() )->GetNetname() )
                THROW_IO_ERROR(
                    wxString::Format( _( "Invalid net ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                                      CurSource(), CurLineNumber(), CurOffset() )
                    );

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

        case T_roundrect_rratio:
            pad->SetRoundRectRadiusRatio( parseDouble( "roundrect radius ratio" ) );
            NeedRIGHT();
            break;

        case T_chamfer_ratio:
            pad->SetChamferRectRatio( parseDouble( "chamfer ratio" ) );

            if( pad->GetChamferRectRatio() > 0 )
                pad->SetShape( PAD_SHAPE_CHAMFERED_RECT );

            NeedRIGHT();
            break;

        case T_chamfer:
        {
            int chamfers = 0;
            bool end_list = false;

            while( !end_list )
            {
                token = NextTok();
                switch( token )
                {
                case T_top_left:
                    chamfers |= RECT_CHAMFER_TOP_LEFT;
                    break;

                case T_top_right:
                    chamfers |= RECT_CHAMFER_TOP_RIGHT;
                    break;

                case T_bottom_left:
                    chamfers |= RECT_CHAMFER_BOTTOM_LEFT;
                    break;

                case T_bottom_right:
                    chamfers |= RECT_CHAMFER_BOTTOM_RIGHT;
                    break;

                case T_RIGHT:
                    pad->SetChamferPositions( chamfers );
                    end_list = true;
                    break;

                default:
                    Expecting( "chamfer_top_left chamfer_top_right chamfer_bottom_left or chamfer_bottom_right" );
                }
            }

            if( pad->GetChamferPositions() != RECT_NO_CHAMFER )
                pad->SetShape( PAD_SHAPE_CHAMFERED_RECT );
        }
            break;

        case T_options:
            parseD_PAD_option( pad.get() );
            break;

        case T_primitives:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                // Currently, I am using parseDRAWSEGMENT() to read basic shapes parameters,
                // because they are the same as a DRAWSEGMENT.
                // However it could be better to write a specific parser, to avoid possible issues
                // if the DRAWSEGMENT parser is modified.
                DRAWSEGMENT* dummysegm = NULL;

                switch( token )
                {
                case T_gr_arc:
                    dummysegm = parseDRAWSEGMENT();
                    pad->AddPrimitive( dummysegm->GetCenter(), dummysegm->GetArcStart(),
                                        dummysegm->GetAngle(), dummysegm->GetWidth() );
                    break;

                case T_gr_line:
                    dummysegm = parseDRAWSEGMENT();
                    pad->AddPrimitive( dummysegm->GetStart(), dummysegm->GetEnd(),
                                        dummysegm->GetWidth() );
                    break;

                case T_gr_circle:
                    dummysegm = parseDRAWSEGMENT( true );   // Circles with 0 thickness are allowed
                                                            // ( filled circles )
                    pad->AddPrimitive( dummysegm->GetCenter(), dummysegm->GetRadius(),
                                        dummysegm->GetWidth() );
                    break;

                case T_gr_poly:
                    dummysegm = parseDRAWSEGMENT();
                    pad->AddPrimitive( dummysegm->BuildPolyPointsList(), dummysegm->GetWidth() );
                    break;

                case T_gr_curve:
                    dummysegm = parseDRAWSEGMENT();
                    pad->AddPrimitive( dummysegm->GetStart(), dummysegm->GetEnd(),
                                       dummysegm->GetBezControl1(), dummysegm->GetBezControl2(),
                                       dummysegm->GetWidth() );
                    break;

                default:
                    Expecting( "gr_line, gr_arc, gr_circle, gr_curve or gr_poly" );
                    break;
                }

                delete dummysegm;
            }
            break;

        default:
            Expecting( "at, drill, layers, net, die_length, solder_mask_margin, roundrect_rratio,\n"
                       "solder_paste_margin, solder_paste_margin_ratio, clearance,\n"
                       "zone_connect, fp_poly, primitives, thermal_width, or thermal_gap" );
        }
    }

    // Be sure the custom shape polygon is built:
    if( pad->GetShape() == PAD_SHAPE_CUSTOM )
        pad->MergePrimitivesAsPolygon();

    return pad.release();
}


bool PCB_PARSER::parseD_PAD_option( D_PAD* aPad )
{
    // Parse only the (option ...) inside a pad description
    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_anchor:
            token = NextTok();
            // Custom shaped pads have a "anchor pad", which is the reference
            // for connection calculations.
            // Because this is an anchor, only the 2 very basic shapes are managed:
            // circle and rect. The default is circle
            switch( token )
            {
                case T_circle:  // default
                    break;

                case T_rect:
                    aPad->SetAnchorPadShape( PAD_SHAPE_RECT );
                    break;

                default:
                    // Currently, because pad options is a moving target
                    // just skip unknown keywords
                    break;
            }
            NeedRIGHT();
            break;

        case T_clearance:
            token = NextTok();
            // Custom shaped pads have a clearance area that is the pad shape
            // (like usual pads) or the convew hull of the pad shape.
            switch( token )
            {
            case T_outline:
                aPad->SetCustomShapeInZoneOpt( CUST_PAD_SHAPE_IN_ZONE_OUTLINE );
                break;

            case T_convexhull:
                aPad->SetCustomShapeInZoneOpt( CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL );
                break;

            default:
                // Currently, because pad options is a moving target
                // just skip unknown keywords
                break;
            }
            NeedRIGHT();
            break;

        default:
            // Currently, because pad options is a moving target
            // just skip unknown keywords
            while( (token = NextTok() ) != T_RIGHT )
            {}
            break;
        }
    }

    return true;
}


TRACK* PCB_PARSER::parseTRACK()
{
    wxCHECK_MSG( CurTok() == T_segment, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as TRACK." ) );

    wxPoint pt;
    T token;

    std::unique_ptr< TRACK > track( new TRACK( m_board ) );

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
            if( ! track->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
                THROW_IO_ERROR(
                    wxString::Format( _( "Invalid net ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                                      GetChars( CurSource() ), CurLineNumber(), CurOffset() )
                    );
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


VIA* PCB_PARSER::parseVIA()
{
    wxCHECK_MSG( CurTok() == T_via, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as VIA." ) );

    wxPoint pt;
    T token;

    std::unique_ptr< VIA > via( new VIA( m_board ) );

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
                PCB_LAYER_ID layer1, layer2;
                NextTok();
                layer1 = lookUpLayer<PCB_LAYER_ID>( m_layerIndices );
                NextTok();
                layer2 = lookUpLayer<PCB_LAYER_ID>( m_layerIndices );
                via->SetLayerPair( layer1, layer2 );
                NeedRIGHT();
            }
            break;

        case T_net:
            if(! via->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true))
                THROW_IO_ERROR(
                    wxString::Format( _( "Invalid net ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                                      GetChars( CurSource() ), CurLineNumber(), CurOffset() )
                    );
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


ZONE_CONTAINER* PCB_PARSER::parseZONE_CONTAINER()
{
    wxCHECK_MSG( CurTok() == T_zone, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as ZONE_CONTAINER." ) );

    ZONE_CONTAINER::HATCH_STYLE hatchStyle = ZONE_CONTAINER::NO_HATCH;

    int     hatchPitch = ZONE_CONTAINER::GetDefaultHatchPitch();
    wxPoint pt;
    T       token;
    int     tmp;
    wxString    netnameFromfile;    // the zone net name find in file

    // bigger scope since each filled_polygon is concatenated in here
    SHAPE_POLY_SET pts;

    std::unique_ptr< ZONE_CONTAINER > zone( new ZONE_CONTAINER( m_board ) );

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

            if( ! zone->SetNetCode( tmp, /* aNoAssert */ true ) )
                THROW_IO_ERROR(
                    wxString::Format( _( "Invalid net ID in\nfile: \"%s\"\nline: %d\noffset: %d" ),
                                      GetChars( CurSource() ), CurLineNumber(), CurOffset() )
                    );

            NeedRIGHT();
            break;

        case T_net_name:
            NeedSYMBOLorNUMBER();
            netnameFromfile = FromUTF8();
            NeedRIGHT();
            break;

        case T_layer:   // keyword for zones that are on only one layer
            zone->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_layers:  // keyword for zones that can live on a set of layer
                        // currently: keepout zones
            zone->SetLayerSet( parseBoardItemLayersAsMask() );
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
            case T_none:   hatchStyle = ZONE_CONTAINER::NO_HATCH;        break;
            case T_edge:   hatchStyle = ZONE_CONTAINER::DIAGONAL_EDGE;   break;
            case T_full:   hatchStyle = ZONE_CONTAINER::DIAGONAL_FULL;
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
                    zone->SetPadConnection( PAD_ZONE_CONN_FULL );
                    break;

                case T_no:
                    zone->SetPadConnection( PAD_ZONE_CONN_NONE );
                    break;

                case T_thru_hole_only:
                    zone->SetPadConnection( PAD_ZONE_CONN_THT_THERMAL );
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

        case T_filled_areas_thickness:
            zone->SetFilledPolysUseThickness( parseBool() );
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

                    if( token != T_segment && token != T_hatch && token != T_polygon )
                        Expecting( "segment, hatch or polygon" );

                    if( token == T_segment )    // deprecated
                    {
                        // SEGMENT fill mode no longer supported.  Make sure user is OK with converting them.
                        if( m_showLegacyZoneWarning )
                        {
                            KIDIALOG dlg( nullptr,
                                          _( "The legacy segment fill mode is no longer supported.\n"
                                             "Convert zones to polygon fills?"),
                                          _( "Legacy Zone Warning" ),
                                          wxYES_NO | wxICON_WARNING );

                            dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

                            if( dlg.ShowModal() == wxID_NO )
                                THROW_IO_ERROR( wxT( "CANCEL" ) );

                            m_showLegacyZoneWarning = false;
                        }

                        zone->SetFillMode( ZFM_POLYGONS );
                        m_board->SetModified();
                    }
                    else if( token == T_hatch )
                        zone->SetFillMode( ZFM_HATCH_PATTERN );
                    else
                        zone->SetFillMode( ZFM_POLYGONS );
                    NeedRIGHT();
                    break;

                case T_hatch_thickness:
                    zone->SetHatchFillTypeThickness( parseBoardUnits( T_hatch_thickness ) );
                    NeedRIGHT();
                    break;

                case T_hatch_gap:
                    zone->SetHatchFillTypeGap( parseBoardUnits( T_hatch_gap ) );
                    NeedRIGHT();
                    break;

                case T_hatch_orientation:
                    zone->SetHatchFillTypeOrientation( parseDouble( T_hatch_orientation ) );
                    NeedRIGHT();
                    break;

                case T_hatch_smoothing_level:
                    zone->SetHatchFillTypeSmoothingLevel( parseDouble( T_hatch_smoothing_level ) );
                    NeedRIGHT();
                    break;

                case T_hatch_smoothing_value:
                    zone->SetHatchFillTypeSmoothingValue( parseDouble( T_hatch_smoothing_value ) );
                    NeedRIGHT();
                    break;

                case T_arc_segments:
                    static_cast<void>( parseInt( "arc segment count" ) );
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
                        if( !zone->GetIsKeepout() ) // smoothing has meaning only for filled zones
                            zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_CHAMFER );
                        break;

                    case T_fillet:
                        if( !zone->GetIsKeepout() ) // smoothing has meaning only for filled zones
                            zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_FILLET );
                        break;

                    default:
                        Expecting( "none, chamfer, or fillet" );
                    }
                    NeedRIGHT();
                    break;

                case T_radius:
                    tmp = parseBoardUnits( "corner radius" );
                    if( !zone->GetIsKeepout() ) // smoothing has meaning only for filled zones
                       zone->SetCornerRadius( tmp );
                    NeedRIGHT();
                    break;

                default:
                    Expecting( "mode, arc_segments, thermal_gap, thermal_bridge_width, "
                               "hatch_thickness, hatch_gap, hatch_orientation, "
                               "hatch_smoothing_level, hatch_smoothing_value, smoothing, or radius" );
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

                for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                {
                    corners.push_back( parseXY() );
                }

                NeedRIGHT();

                // Remark: The first polygon is the main outline.
                // Others are holes inside the main outline.
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

                pts.NewOutline();

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                {
                    pts.Append( parseXY() );
                }

                NeedRIGHT();
            }
            break;

        case T_fill_segments:
            {
                ZONE_SEGMENT_FILL segs;

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                {
                    if( token != T_LEFT )
                        Expecting( T_LEFT );

                    token = NextTok();

                    if( token != T_pts )
                        Expecting( T_pts );

                    SEG segment( parseXY(), parseXY() );
                    NeedRIGHT();
                    segs.push_back( segment );
                }

                zone->SetFillSegments( segs );
            }
            break;

        default:
            Expecting( "net, layer/layers, tstamp, hatch, priority, connect_pads, min_thickness, "
                       "fill, polygon, filled_polygon, or fill_segments" );
        }
    }

    if( zone->GetNumCorners() > 2 )
    {
        if( !zone->IsOnCopperLayer() )
        {
            //zone->SetFillMode( ZFM_POLYGONS );
            zone->SetNetCode( NETINFO_LIST::UNCONNECTED );
        }

        // Set hatch here, after outlines corners are read
        zone->SetHatch( hatchStyle, hatchPitch, true );
    }

    if( !pts.IsEmpty() )
        zone->SetFilledPolysList( pts );

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
            m_board->Add( net );

            // Store the new code mapping
            pushValueIntoMap( newnetcode, net->GetNet() );
            // and update the zone netcode
            zone->SetNetCode( net->GetNet() );

            // FIXME: a call to any GUI item is not allowed in io plugins:
            // Change this code to generate a warning message outside this plugin
            // Prompt the user
            wxString msg;
            msg.Printf( _( "There is a zone that belongs to a not existing net\n"
                           "\"%s\"\n"
                           "you should verify and edit it (run DRC test)." ),
                           GetChars( netnameFromfile ) );
            DisplayError( NULL, msg );
        }
    }

    // Clear flags used in zone edition:
    zone->SetNeedRefill( false );

    return zone.release();
}


PCB_TARGET* PCB_PARSER::parsePCB_TARGET()
{
    wxCHECK_MSG( CurTok() == T_target, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TARGET." ) );

    wxPoint pt;
    T token;

    std::unique_ptr< PCB_TARGET > target( new PCB_TARGET( NULL ) );

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

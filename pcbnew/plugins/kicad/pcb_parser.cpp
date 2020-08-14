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
 * @file pcb_parser.cpp
 * @brief Pcbnew s-expression file format parser implementation.
 */

#include <cerrno>
#include <confirm.h>
#include <macros.h>
#include <title_block.h>
#include <trigo.h>

#include <board.h>
#include <board_design_settings.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <fp_shape.h>
#include <pcb_group.h>
#include <pcb_target.h>
#include <footprint.h>
#include <geometry/shape_line_chain.h>

#include <netclass.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <plugins/kicad/kicad_plugin.h>
#include <pcb_plot_params_parser.h>
#include <pcb_plot_params.h>
#include <locale_io.h>
#include <zones.h>
#include <plugins/kicad/pcb_parser.h>
#include <convert_basic_shapes_to_polygon.h>    // for RECT_CHAMFER_POSITIONS definition
#include <math/util.h>                           // KiROUND, Clamp
#include <kicad_string.h>
#include <wx/log.h>
#include <widgets/progress_reporter.h>

using namespace PCB_KEYS_T;


void PCB_PARSER::init()
{
    m_showLegacyZoneWarning = true;
    m_tooRecent = false;
    m_requiredVersion = 0;
    m_layerIndices.clear();
    m_layerMasks.clear();
    m_resetKIIDMap.clear();

    // Add untranslated default (i.e. English) layernames.
    // Some may be overridden later if parsing a board rather than a footprint.
    // The English name will survive if parsing only a footprint.
    for( LAYER_NUM layer = 0;  layer < PCB_LAYER_ID_COUNT;  ++layer )
    {
        std::string untranslated = TO_UTF8( wxString( LSET::Name( PCB_LAYER_ID( layer ) ) ) );

        m_layerIndices[ untranslated ] = PCB_LAYER_ID( layer );
        m_layerMasks[ untranslated ]   = LSET( PCB_LAYER_ID( layer ) );
    }

    m_layerMasks[ "*.Cu" ]      = LSET::AllCuMask();
    m_layerMasks[ "*In.Cu" ]    = LSET::InternalCuMask();
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
}


void PCB_PARSER::checkpoint()
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


void PCB_PARSER::skipCurrent()
{
    int curr_level = 0;
    T token;

    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            curr_level--;

        if( token == T_RIGHT )
        {
            curr_level++;

            if( curr_level > 0 )
                return;
        }
    }
}


void PCB_PARSER::pushValueIntoMap( int aIndex, int aValue )
{
    // Add aValue in netcode mapping (m_netCodes) at index aNetCode
    // ensure there is room in m_netCodes for that, and add room if needed.

    if( (int)m_netCodes.size() <= aIndex )
        m_netCodes.resize( static_cast<std::size_t>( aIndex ) + 1 );

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
        error.Printf( _( "Invalid floating point number in\nfile: '%s'\nline: %d\noffset: %d" ),
                      CurSource(), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    if( CurText() == tmp )
    {
        wxString error;
        error.Printf( _( "Missing floating point number in\nfile: '%s'\nline: %d\noffset: %d" ),
                      CurSource(), CurLineNumber(), CurOffset() );

        THROW_IO_ERROR( error );
    }

    return fval;
}


int PCB_PARSER::parseBoardUnits()
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
    double int_limit = std::numeric_limits<int>::max() * 0.7071; // 0.7071 = roughly 1/sqrt(2)
    return KiROUND( Clamp<double>( -int_limit, retval, int_limit ) );
}


int PCB_PARSER::parseBoardUnits( const char* aExpected )
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


void PCB_PARSER::parseOutlinePoints( SHAPE_LINE_CHAIN& aPoly )
{
    if( CurTok() != T_LEFT )
        NeedLEFT();

    T token = NextTok();

    switch( token )
    {
    case T_xy:
    {
        int x = parseBoardUnits( "X coordinate" );
        int y = parseBoardUnits( "Y coordinate" );

        NeedRIGHT();

        aPoly.Append( x, y );
        break;
    }
    case T_arc:
    {
        bool has_start = false;
        bool has_mid   = false;
        bool has_end   = false;

        VECTOR2I arc_start, arc_mid, arc_end;

        for( token = NextTok(); token != T_RIGHT; token = NextTok() )
        {
            if( token != T_LEFT )
                Expecting( T_LEFT );

            token = NextTok();

            switch( token )
            {
            case T_start:
                arc_start.x = parseBoardUnits( "start x" );
                arc_start.y = parseBoardUnits( "start y" );
                has_start = true;
                break;

            case T_mid:
                arc_mid.x = parseBoardUnits( "mid x" );
                arc_mid.y = parseBoardUnits( "mid y" );
                has_mid = true;
                break;

            case T_end:
                arc_end.x = parseBoardUnits( "end x" );
                arc_end.y = parseBoardUnits( "end y" );
                has_end = true;
                break;

            default:
                Expecting( "start, mid or end" );
            }

            NeedRIGHT();
        }

        if( !has_start )
            Expecting( "start" );

        if( !has_mid )
            Expecting( "mid" );

        if( !has_end )
            Expecting( "end" );

        SHAPE_ARC arc( arc_start, arc_mid, arc_end, 0 );

        aPoly.Append( arc );
        NeedRIGHT();

        break;
    }
    default:
        Expecting( "xy or arc" );
    }
}


void PCB_PARSER::parseXY( int* aX, int* aY )
{
    wxPoint pt = parseXY();

    if( aX )
        *aX = pt.x;

    if( aY )
        *aY = pt.y;
}


std::pair<wxString, wxString> PCB_PARSER::parseProperty()
{
    wxString pName;
    wxString pValue;

    NeedSYMBOL();
    pName = FromUTF8();
    NeedSYMBOL();
    pValue = FromUTF8();
    NeedRIGHT();

    return { pName, pValue };
}


void PCB_PARSER::parseEDA_TEXT( EDA_TEXT* aText )
{
    wxCHECK_RET( CurTok() == T_effects,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDA_TEXT." ) );

    // In version 20210606 the notation for overbars was changed from `~...~` to `~{...}`.
    // We need to convert the old syntax to the new one.
    if( m_requiredVersion < 20210606 )
        aText->SetText( ConvertToNewOverbarNotation( aText->GetText() ) );

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
                    aText->SetTextThickness( parseBoardUnits( "text thickness" ) );
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
        const double defaultTextSize = 1.524 * IU_PER_MM;

        aText->SetTextSize( wxSize( defaultTextSize, defaultTextSize ) );
    }
}


FP_3DMODEL* PCB_PARSER::parse3DModel()
{
    wxCHECK_MSG( CurTok() == T_model, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as FP_3DMODEL." ) );

    T token;

    FP_3DMODEL* n3D = new FP_3DMODEL;
    NeedSYMBOLorNUMBER();
    n3D->m_Filename = FromUTF8();

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
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

            NeedRIGHT();    // xyz
            NeedRIGHT();    // at
            break;

        case T_hide:
            n3D->m_Show = false;
            break;

        case T_opacity:
            n3D->m_Opacity = parseDouble( "opacity value" );
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

            NeedRIGHT();    // xyz
            NeedRIGHT();    // offset
            break;

        case T_scale:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_Scale.x = parseDouble( "x value" );
            n3D->m_Scale.y = parseDouble( "y value" );
            n3D->m_Scale.z = parseDouble( "z value" );

            NeedRIGHT();    // xyz
            NeedRIGHT();    // scale
            break;

        case T_rotate:
            NeedLEFT();
            token = NextTok();

            if( token != T_xyz )
                Expecting( T_xyz );

            n3D->m_Rotation.x = parseDouble( "x value" );
            n3D->m_Rotation.y = parseDouble( "y value" );
            n3D->m_Rotation.z = parseDouble( "z value" );

            NeedRIGHT();    // xyz
            NeedRIGHT();    // rotate
            break;

        default:
            Expecting( "at, hide, opacity, offset, scale, or rotate" );
        }

    }

    return n3D;
}


BOARD_ITEM* PCB_PARSER::Parse()
{
    T               token;
    BOARD_ITEM*     item;
    LOCALE_IO       toggle;

    m_groupInfos.clear();

    // FOOTPRINTS can be prefixed with an initial block of single line comments and these are
    // kept for Format() so they round trip in s-expression form.  BOARDs might  eventually do
    // the same, but currently do not.
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

    case T_module:      // legacy token
    case T_footprint:
        item = (BOARD_ITEM*) parseFOOTPRINT( initial_comments.release() );

        // Locking a footprint has no meaning outside of a board.
        item->SetLocked( false );
        break;

    default:
        wxString err;
        err.Printf( _( "Unknown token '%s'" ), FromUTF8() );
        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    resolveGroups( item );

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
    std::map<wxString, wxString> properties;

    parseHeader();

    std::vector<BOARD_ITEM*> bulkAddedItems;
    BOARD_ITEM* item = nullptr;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        checkpoint();

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token == T_page && m_requiredVersion <= 20200119 )
            token = T_paper;

        switch( token )
        {
        case T_host:            // legacy token
            NeedSYMBOL();
            m_board->SetGenerator( FromUTF8() );

            // Older formats included build data
            if( m_requiredVersion < BOARD_FILE_HOST_VERSION )
                NeedSYMBOL();

            NeedRIGHT();
            break;

        case T_generator:
            NeedSYMBOL();
            m_board->SetGenerator( FromUTF8() );
            NeedRIGHT();
            break;

        case T_general:
            parseGeneralSection();
            break;

        case T_paper:
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

        case T_property:
            properties.insert( parseProperty() );
            break;

        case T_net:
            parseNETINFO_ITEM();
            break;

        case T_net_class:
            parseNETCLASS();
            m_board->m_LegacyNetclassesLoaded = true;
            break;

        case T_gr_arc:
        case T_gr_curve:
        case T_gr_line:
        case T_gr_poly:
        case T_gr_circle:
        case T_gr_rect:
            item = parsePCB_SHAPE();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_gr_text:
            item = parsePCB_TEXT();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_dimension:
            item = parseDIMENSION();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_module:      // legacy token
        case T_footprint:
            item = parseFOOTPRINT();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_segment:
            item = parsePCB_TRACK();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_arc:
            item = parseARC();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_group:
            parseGROUP( m_board );
            break;

        case T_via:
            item = parsePCB_VIA();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_zone:
            item = parseZONE( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        case T_target:
            item = parsePCB_TARGET();
            m_board->Add( item, ADD_MODE::BULK_APPEND );
            bulkAddedItems.push_back( item );
            break;

        default:
            wxString err;
            err.Printf( _( "Unknown token '%s'" ), FromUTF8() );
            THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
        }
    }

    if( bulkAddedItems.size() > 0 )
        m_board->FinalizeBulkAdd( bulkAddedItems );

    m_board->SetProperties( properties );

    if( m_undefinedLayers.size() > 0 )
    {
        bool deleteItems;
        std::vector<BOARD_ITEM*> deleteList;
        wxString msg = wxString::Format( _( "Items found on undefined layers.  Do you wish to\n"
                                            "rescue them to the User.Comments layer?" ) );
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

        auto visitItem = [&]( BOARD_ITEM* curr_item )
                            {
                                if( curr_item->GetLayer() == Rescue )
                                {
                                    if( deleteItems )
                                        deleteList.push_back( curr_item );
                                    else
                                        curr_item->SetLayer( Cmts_User );
                                }
                            };

        for( PCB_TRACK* track : m_board->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                PCB_VIA*     via = static_cast<PCB_VIA*>( track );
                PCB_LAYER_ID top_layer, bottom_layer;

                if( via->GetViaType() == VIATYPE::THROUGH )
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
            {
                visitItem( track );
            }
        }

        for( BOARD_ITEM* zone : m_board->Zones() )
            visitItem( zone );

        for( BOARD_ITEM* drawing : m_board->Drawings() )
            visitItem( drawing );

        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            for( BOARD_ITEM* drawing : fp->GraphicalItems() )
                visitItem( drawing );

            for( BOARD_ITEM* zone : fp->Zones() )
                visitItem( zone );
        }

        for( BOARD_ITEM* curr_item : deleteList )
            m_board->Delete( curr_item );

        m_undefinedLayers.clear();
    }

    return m_board;
}


void PCB_PARSER::resolveGroups( BOARD_ITEM* aParent )
{
    auto getItem = [&]( const KIID& aId )
    {
        BOARD_ITEM* aItem = nullptr;

        if( dynamic_cast<BOARD*>( aParent ) )
        {
            aItem = static_cast<BOARD*>( aParent )->GetItem( aId );
        }
        else if( aParent->Type() == PCB_FOOTPRINT_T )
        {
            static_cast<FOOTPRINT*>( aParent )->RunOnChildren(
                    [&]( BOARD_ITEM* child )
                    {
                        if( child->m_Uuid == aId )
                            aItem = child;
                    } );
        }

        return aItem;
    };

    // Now that we've parsed the other Uuids in the file we can resolve the uuids referred
    // to in the group declarations we saw.
    //
    // First add all group objects so subsequent GetItem() calls for nested groups work.

    for( size_t idx = 0; idx < m_groupInfos.size(); idx++ )
    {
        GROUP_INFO& aGrp  = m_groupInfos[idx];
        PCB_GROUP*  group = new PCB_GROUP( aGrp.parent );

        group->SetName( aGrp.name );
        const_cast<KIID&>( group->m_Uuid ) = aGrp.uuid;

        if( aGrp.parent->Type() == PCB_FOOTPRINT_T )
            static_cast<FOOTPRINT*>( aGrp.parent )->Add( group );
        else
            static_cast<BOARD*>( aGrp.parent )->Add( group );
    }

    wxString error;

    for( size_t idx = 0; idx < m_groupInfos.size(); idx++ )
    {
        GROUP_INFO& aGrp  = m_groupInfos[idx];
        BOARD_ITEM* bItem = getItem( aGrp.uuid );

        if( bItem == nullptr || bItem->Type() != PCB_GROUP_T )
            continue;

        PCB_GROUP* group = static_cast<PCB_GROUP*>( bItem );

        for( const KIID& aUuid : aGrp.memberUuids )
        {
            BOARD_ITEM* item;

            if( m_resetKIIDs )
                item = getItem( m_resetKIIDMap[ aUuid.AsString() ] );
            else
                item = getItem( aUuid );

            if( item && item->Type() != NOT_USED )
            {
                switch( item->Type() )
                {
                // We used to allow fp items in non-footprint groups.  It was a mistake.
                case PCB_FP_TEXT_T:
                case PCB_FP_SHAPE_T:
                case PCB_FP_ZONE_T:
                    if( item->GetParent() == group->GetParent() )
                        group->AddItem( item );

                    break;

                // This is the deleted item singleton, which means we didn't find the uuid.
                case NOT_USED:
                    break;

                default:
                    group->AddItem( item );
                }
            }
        }
    }

    // Don't allow group cycles
    if( m_board )
        m_board->GroupsSanityCheck( true );
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
    }
    else
    {
        m_requiredVersion = 20201115;   // Last version before we started writing version #s
                                        // in footprint files as well as board files.
        m_tooRecent = ( m_requiredVersion > SEXPR_BOARD_FILE_VERSION );
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

        default:              // Skip everything but the board thickness.
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
    wxCHECK_RET( ( CurTok() == T_page && m_requiredVersion <= 20200119 ) || CurTok() == T_paper,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a PAGE_INFO." ) );

    T token;
    PAGE_INFO pageInfo;

    NeedSYMBOL();

    wxString pageType = FromUTF8();

    if( !pageInfo.SetType( pageType ) )
    {
        wxString err;
        err.Printf( _( "Page type '%s' is not valid." ), FromUTF8() );
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
                    titleBlock.SetComment( 0, FromUTF8() );
                    break;

                case 2:
                    NextTok();
                    titleBlock.SetComment( 1, FromUTF8() );
                    break;

                case 3:
                    NextTok();
                    titleBlock.SetComment( 2, FromUTF8() );
                    break;

                case 4:
                    NextTok();
                    titleBlock.SetComment( 3, FromUTF8() );
                    break;

                case 5:
                    NextTok();
                    titleBlock.SetComment( 4, FromUTF8() );
                    break;

                case 6:
                    NextTok();
                    titleBlock.SetComment( 5, FromUTF8() );
                    break;

                case 7:
                    NextTok();
                    titleBlock.SetComment( 6, FromUTF8() );
                    break;

                case 8:
                    NextTok();
                    titleBlock.SetComment( 7, FromUTF8() );
                    break;

                case 9:
                    NextTok();
                    titleBlock.SetComment( 8, FromUTF8() );
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
    std::string userName;
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

    // @todo Figure out why we are looking for a hide token in the layer definition.
    if( token == T_hide )
    {
        isVisible = false;
        NeedRIGHT();
    }
    else if( token == T_STRING )
    {
        userName = CurText();
        NeedRIGHT();
    }
    else if( token != T_RIGHT )
    {
        Expecting( "hide, user defined name, or )" );
    }

    aLayer->m_name    = FROM_UTF8( name.c_str() );
    aLayer->m_type    = LAYER::ParseType( type.c_str() );
    aLayer->m_number  = layer_num;
    aLayer->m_visible = isVisible;

    if( !userName.empty() )
        aLayer->m_userName = FROM_UTF8( userName.c_str() );

    // The canonical name will get reset back to the default for copper layer on the next
    // save.  The user defined name is now a separate optional layer token from the canonical
    // name.
    if( aLayer->m_name != LSET::Name( static_cast<PCB_LAYER_ID>( aLayer->m_number ) ) )
        aLayer->m_userName = aLayer->m_name;
}


void PCB_PARSER::parseBoardStackup()
{
    T token;
    wxString name;
    int dielectric_idx = 1;     // the index of dielectric layers
    BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( CurTok() != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_layer )
        {
            switch( token )
            {
            case T_copper_finish:
                NeedSYMBOL();
                stackup.m_FinishType = FromUTF8();
                NeedRIGHT();
                break;

            case T_edge_plating:
                token = NextTok();
                stackup.m_EdgePlating = token == T_yes;
                NeedRIGHT();
                break;

            case T_dielectric_constraints:
                token = NextTok();
                stackup.m_HasDielectricConstrains = token == T_yes;
                NeedRIGHT();
                break;

            case T_edge_connector:
                token = NextTok();
                stackup.m_EdgeConnectorConstraints = BS_EDGE_CONNECTOR_NONE;

                if( token == T_yes )
                    stackup.m_EdgeConnectorConstraints = BS_EDGE_CONNECTOR_IN_USE;
                else if( token == T_bevelled )
                    stackup.m_EdgeConnectorConstraints = BS_EDGE_CONNECTOR_BEVELLED;

                NeedRIGHT();
                break;

            case T_castellated_pads:
                token = NextTok();
                stackup.m_CastellatedPads = token == T_yes;
                NeedRIGHT();
                break;

            default:
                // Currently, skip this item if not defined, because the stackup def
                // is a moving target
                //Expecting( "copper_finish, edge_plating, dielectric_constrains, edge_connector, castellated_pads" );
                skipCurrent();
                break;
            }

            continue;
        }

        NeedSYMBOL();
        name = FromUTF8();

        // init the layer id. For dielectric, layer id = UNDEFINED_LAYER
        PCB_LAYER_ID layerId = m_board->GetLayerID( name );

        // Init the type
        BOARD_STACKUP_ITEM_TYPE type = BS_ITEM_TYPE_UNDEFINED;

        if( layerId == F_SilkS || layerId == B_SilkS )
            type = BS_ITEM_TYPE_SILKSCREEN;
        else if( layerId == F_Mask || layerId == B_Mask )
            type = BS_ITEM_TYPE_SOLDERMASK;
        else if( layerId == F_Paste || layerId == B_Paste )
            type = BS_ITEM_TYPE_SOLDERPASTE;
        else if( layerId == UNDEFINED_LAYER )
            type = BS_ITEM_TYPE_DIELECTRIC;
        else if( layerId >= F_Cu && layerId <= B_Cu )
            type = BS_ITEM_TYPE_COPPER;

        BOARD_STACKUP_ITEM* item = nullptr;

        if( type != BS_ITEM_TYPE_UNDEFINED )
        {
            item = new BOARD_STACKUP_ITEM( type );
            item->SetBrdLayerId( layerId );

            if( type == BS_ITEM_TYPE_DIELECTRIC )
                item->SetDielectricLayerId( dielectric_idx++ );

            stackup.Add( item );
        }
        else
            Expecting( "layer_name" );

        bool has_next_sublayer = true;
        int sublayer_idx = 0;       // the index of dielectric sub layers
                                    // sublayer 0 is always existing (main sublayer)

        while( has_next_sublayer )
        {
            has_next_sublayer = false;

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token == T_addsublayer )
                {
                    has_next_sublayer = true;
                    break;
                }

                if( token == T_LEFT )
                {
                    token = NextTok();

                    switch( token )
                    {
                    case T_type:
                        NeedSYMBOL();
                        item->SetTypeName( FromUTF8() );
                        NeedRIGHT();
                        break;

                    case T_thickness:
                        item->SetThickness( parseBoardUnits( T_thickness ), sublayer_idx );
                        token = NextTok();

                        if( token == T_LEFT )
                            break;

                        if( token == T_locked )
                        {
                            // Dielectric thickness can be locked (for impedance controlled layers)
                            if( type == BS_ITEM_TYPE_DIELECTRIC )
                                item->SetThicknessLocked( true, sublayer_idx );

                            NeedRIGHT();
                        }
                        break;

                    case T_material:
                        NeedSYMBOL();
                        item->SetMaterial( FromUTF8(), sublayer_idx );
                        NeedRIGHT();
                        break;

                    case T_epsilon_r:
                        NextTok();
                        item->SetEpsilonR( parseDouble(), sublayer_idx );
                        NeedRIGHT();
                        break;

                    case T_loss_tangent:
                        NextTok();
                        item->SetLossTangent( parseDouble(), sublayer_idx );
                        NeedRIGHT();
                        break;

                    case T_color:
                        NeedSYMBOL();
                        item->SetColor( FromUTF8() );
                        NeedRIGHT();
                        break;

                    default:
                        // Currently, skip this item if not defined, because the stackup def
                        // is a moving target
                        //Expecting( "type, thickness, material, epsilon_r, loss_tangent, color" );
                        skipCurrent();
                    }
                }
            }

            if( has_next_sublayer )     // Prepare reading the next sublayer description
            {
                sublayer_idx++;
                item->AddDielectricPrms( sublayer_idx );
            }
        }
    }

    if( token != T_RIGHT )
    {
        Expecting( ")" );
    }

    // Success:
    m_board->GetDesignSettings().m_HasStackup = true;
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
    bool    anyHidden = false;

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
            else
                anyHidden = true;

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
                wxString error;
                error.Printf( _( "Layer '%s' in file '%s' at line %d is not in fixed layer hash." ),
                              layer.m_name,
                              CurSource(),
                              CurLineNumber(),
                              CurOffset() );

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
        else
            anyHidden = true;

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

    // Only set this if any layers were explicitly marked as hidden.  Otherwise, we want to leave
    // this alone; default visibility will show everything
    if( anyHidden )
        m_board->m_LegacyVisibleLayers = visibleLayers;
}


template<class T, class M>
T PCB_PARSER::lookUpLayer( const M& aMap )
{
    // avoid constructing another std::string, use lexer's directly
    typename M::const_iterator it = aMap.find( curText );

    if( it == aMap.end() )
    {
        m_undefinedLayers.insert( curText );
        return Rescue;
    }

    // Some files may have saved items to the Rescue Layer due to an issue in v5
    if( it->second == Rescue )
        m_undefinedLayers.insert( curText );

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
    NETCLASS*              defaultNetClass = m_board->GetDesignSettings().GetDefault();
    BOARD_DESIGN_SETTINGS& designSettings  = m_board->GetDesignSettings();
    ZONE_SETTINGS&         zoneSettings    = designSettings.GetDefaultZoneSettings();

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
        case T_stackup:
            parseBoardStackup();
            break;

        case T_last_trace_width:    // not used now
            /* lastTraceWidth =*/ parseBoardUnits( T_last_trace_width );
            NeedRIGHT();
            break;

        case T_user_trace_width:
        {
            // Make room for the netclass value
            if( designSettings.m_TrackWidthList.empty() )
                designSettings.m_TrackWidthList.emplace_back( 0 );

            designSettings.m_TrackWidthList.push_back( parseBoardUnits( T_user_trace_width ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

        case T_trace_clearance:
            defaultNetClass->SetClearance( parseBoardUnits( T_trace_clearance ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_zone_clearance:
            zoneSettings.m_ZoneClearance = parseBoardUnits( T_zone_clearance );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_zone_45_only:
            zoneSettings.m_Zone_45_Only = parseBool();
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_clearance_min:
            designSettings.m_MinClearance = parseBoardUnits( T_clearance_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_trace_min:
            designSettings.m_TrackMinWidth = parseBoardUnits( T_trace_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_via_size:
            defaultNetClass->SetViaDiameter( parseBoardUnits( T_via_size ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_via_drill:
            defaultNetClass->SetViaDrill( parseBoardUnits( T_via_drill ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_via_min_annulus:
            designSettings.m_ViasMinAnnularWidth = parseBoardUnits( T_via_min_annulus );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_via_min_size:
            designSettings.m_ViasMinSize = parseBoardUnits( T_via_min_size );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_through_hole_min:
            designSettings.m_MinThroughDrill = parseBoardUnits( T_through_hole_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        // Legacy token for T_through_hole_min
        case T_via_min_drill:
            designSettings.m_MinThroughDrill = parseBoardUnits( T_via_min_drill );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_hole_to_hole_min:
            designSettings.m_HoleToHoleMin = parseBoardUnits( T_hole_to_hole_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_user_via:
            {
                int viaSize = parseBoardUnits( "user via size" );
                int viaDrill = parseBoardUnits( "user via drill" );

                // Make room for the netclass value
                if( designSettings.m_ViasDimensionsList.empty() )
                    designSettings.m_ViasDimensionsList.emplace_back( VIA_DIMENSION( 0, 0 ) );

                designSettings.m_ViasDimensionsList.emplace_back( VIA_DIMENSION( viaSize, viaDrill ) );
                m_board->m_LegacyDesignSettingsLoaded = true;
                NeedRIGHT();
            }
            break;

        case T_uvia_size:
            defaultNetClass->SetuViaDiameter( parseBoardUnits( T_uvia_size ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_uvia_drill:
            defaultNetClass->SetuViaDrill( parseBoardUnits( T_uvia_drill ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_uvias_allowed:
            designSettings.m_MicroViasAllowed = parseBool();
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_blind_buried_vias_allowed:
            designSettings.m_BlindBuriedViaAllowed = parseBool();
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_uvia_min_size:
            designSettings.m_MicroViasMinSize = parseBoardUnits( T_uvia_min_size );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_uvia_min_drill:
            designSettings.m_MicroViasMinDrill = parseBoardUnits( T_uvia_min_drill );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_user_diff_pair:
            {
                int width = parseBoardUnits( "user diff-pair width" );
                int gap = parseBoardUnits( "user diff-pair gap" );
                int viaGap = parseBoardUnits( "user diff-pair via gap" );
                designSettings.m_DiffPairDimensionsList.emplace_back( DIFF_PAIR_DIMENSION( width, gap, viaGap ) );
                m_board->m_LegacyDesignSettingsLoaded = true;
                NeedRIGHT();
            }
            break;

        case T_segment_width:   // note: legacy (pre-6.0) token
            designSettings.m_LineThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( T_segment_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_edge_width:      // note: legacy (pre-6.0) token
            designSettings.m_LineThickness[ LAYER_CLASS_EDGES ] = parseBoardUnits( T_edge_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_mod_edge_width:  // note: legacy (pre-6.0) token
            designSettings.m_LineThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( T_mod_edge_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_pcb_text_width:  // note: legacy (pre-6.0) token
            designSettings.m_TextThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( T_pcb_text_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_mod_text_width:  // note: legacy (pre-6.0) token
            designSettings.m_TextThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( T_mod_text_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_pcb_text_size:   // note: legacy (pre-6.0) token
            designSettings.m_TextSize[ LAYER_CLASS_COPPER ].x = parseBoardUnits( "pcb text width" );
            designSettings.m_TextSize[ LAYER_CLASS_COPPER ].y = parseBoardUnits( "pcb text height" );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_mod_text_size:   // note: legacy (pre-6.0) token
            designSettings.m_TextSize[ LAYER_CLASS_SILK ].x = parseBoardUnits( "footprint text width" );
            designSettings.m_TextSize[ LAYER_CLASS_SILK ].y = parseBoardUnits( "footprint text height" );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_defaults:
            parseDefaults( designSettings );
            m_board->m_LegacyDesignSettingsLoaded = true;
            break;

        case T_pad_size:
            {
                wxSize sz;
                sz.SetWidth( parseBoardUnits( "master pad width" ) );
                sz.SetHeight( parseBoardUnits( "master pad height" ) );
                designSettings.m_Pad_Master->SetSize( sz );
                m_board->m_LegacyDesignSettingsLoaded = true;
                NeedRIGHT();
            }
            break;

        case T_pad_drill:
            {
                int drillSize = parseBoardUnits( T_pad_drill );
                designSettings.m_Pad_Master->SetDrillSize( wxSize( drillSize, drillSize ) );
                m_board->m_LegacyDesignSettingsLoaded = true;
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
                designSettings.m_AuxOrigin = wxPoint( x, y );
                // Aux origin still stored in board for the moment
                //m_board->m_LegacyDesignSettingsLoaded = true;
                NeedRIGHT();
            }
            break;

        case T_grid_origin:
            {
                int x = parseBoardUnits( "grid origin X" );
                int y = parseBoardUnits( "grid origin Y" );
                designSettings.m_GridOrigin = wxPoint( x, y );
                // Grid origin still stored in board for the moment
                //m_board->m_LegacyDesignSettingsLoaded = true;
                NeedRIGHT();
            }
            break;

        // Stored in board prior to 6.0
        case T_visible_elements:
            {
                // Make sure to start with DefaultVisible so all new layers are set
                m_board->m_LegacyVisibleItems = GAL_SET::DefaultVisible();

                int visible = parseHex() | MIN_VISIBILITY_MASK;

                for( size_t i = 0; i < sizeof( int ) * CHAR_BIT; i++ )
                    m_board->m_LegacyVisibleItems.set( i, visible & ( 1u << i ) );

                NeedRIGHT();
            }
            break;

        case T_max_error:
            designSettings.m_MaxError = parseBoardUnits( T_max_error );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_filled_areas_thickness:  // Note: legacy (early 5.99) token
            designSettings.m_ZoneFillVersion = parseBool() ? 5 : 6;
            m_board->m_LegacyDesignSettingsLoaded = true;
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
            m_board->m_LegacyCopperEdgeClearanceLoaded = true;
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

        case T_fab_layers_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_FAB ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_fab_layers_text_dims:
            parseDefaultTextDims( designSettings, LAYER_CLASS_FAB );
            break;

        case T_other_layers_line_width:
            designSettings.m_LineThickness[ LAYER_CLASS_OTHERS ] = parseBoardUnits( token );
            NeedRIGHT();
            break;

        case T_other_layers_text_dims:
            parseDefaultTextDims( designSettings, LAYER_CLASS_OTHERS );
            break;

        case T_dimension_units:
            designSettings.m_DimensionUnitsMode =
                    static_cast<DIM_UNITS_MODE>( parseInt( "dimension units" ) );
            NeedRIGHT();
            break;

        case T_dimension_precision:
            designSettings.m_DimensionPrecision = parseInt( "dimension precision" );
            NeedRIGHT();
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

    // Convert overbar syntax from `~...~` to `~{...}`.  These were left out of the first merge
    // so the version is a bit later.
    if( m_requiredVersion < 20210606 )
        name = ConvertToNewOverbarNotation( name );

    NeedRIGHT();

    // net 0 should be already in list, so store this net
    // if it is not the net 0, or if the net 0 does not exists.
    // (TODO: a better test.)
    if( netCode > NETINFO_LIST::UNCONNECTED || !m_board->FindNet( NETINFO_LIST::UNCONNECTED ) )
    {
        NETINFO_ITEM* net = new NETINFO_ITEM( m_board, name, netCode );
        m_board->Add( net );

        // Store the new code mapping
        pushValueIntoMap( netCode, net->GetNetCode() );
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

            // Convert overbar syntax from `~...~` to `~{...}`.  These were left out of the
            // first merge so the version is a bit later.
            if( m_requiredVersion < 20210606 )
                nc->Add( ConvertToNewOverbarNotation( FromUTF8() ) );
            else
                nc->Add( FromUTF8() );

            break;

        default:
            Expecting( "clearance, trace_width, via_dia, via_drill, uvia_dia, uvia_drill, "
                       "diff_pair_width, diff_pair_gap or add_net" );
        }

        NeedRIGHT();
    }

    if( !m_board->GetDesignSettings().GetNetClasses().Add( nc ) )
    {
        // Must have been a name conflict, this is a bad board file.
        // User may have done a hand edit to the file.

        // unique_ptr will delete nc on this code path

        wxString error;
        error.Printf( _( "Duplicate NETCLASS name '%s' in file '%s' at line %d, offset %d." ),
                      nc->GetName().GetData(),
                      CurSource().GetData(),
                      CurLineNumber(),
                      CurOffset() );
        THROW_IO_ERROR( error );
    }
}


PCB_SHAPE* PCB_PARSER::parsePCB_SHAPE()
{
    wxCHECK_MSG( CurTok() == T_gr_arc || CurTok() == T_gr_circle || CurTok() == T_gr_curve ||
                 CurTok() == T_gr_rect || CurTok() == T_gr_line || CurTok() == T_gr_poly, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_SHAPE." ) );

    T token;
    wxPoint pt;
    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( nullptr );

    switch( CurTok() )
    {
    case T_gr_arc:
        shape->SetShape( PCB_SHAPE_TYPE::ARC );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        // the start keyword actually gives the arc center
        // Allows also T_center for future change
        if( token != T_start && token != T_center )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetCenter( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )    // the end keyword actually gives the starting point of the arc
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetArcStart( pt );
        NeedRIGHT();
        break;

    case T_gr_circle:
        shape->SetShape( PCB_SHAPE_TYPE::CIRCLE );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_center )
            Expecting( T_center );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetCenter( pt );
        NeedRIGHT();
        NeedLEFT();

        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd( pt );
        NeedRIGHT();
        break;

    case T_gr_curve:
        shape->SetShape( PCB_SHAPE_TYPE::CURVE );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        shape->SetStart( parseXY() );
        shape->SetBezControl1( parseXY() );
        shape->SetBezControl2( parseXY() );
        shape->SetEnd( parseXY() );
        NeedRIGHT();
        break;

    case T_gr_rect:
        shape->SetShape( PCB_SHAPE_TYPE::RECT );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetStart( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd( pt );
        NeedRIGHT();
        break;

    case T_gr_line:
        // Default PCB_SHAPE type is S_SEGMENT.
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetStart( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd( pt );
        NeedRIGHT();
        break;

    case T_gr_poly:
    {
        shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
        shape->SetWidth( 0 ); // this is the default value. will be (perhaps) modified later
        shape->SetPolyPoints( {} );

        SHAPE_LINE_CHAIN& outline = shape->GetPolyShape().Outline( 0 );

        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        while( (token = NextTok() ) != T_RIGHT )
        {
            parseOutlinePoints( outline );
        }
    }
        break;

    default:
        Expecting( "gr_arc, gr_circle, gr_curve, gr_line, gr_poly, or gp_rect" );
    }

    bool foundFill = false;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_angle:
            shape->SetAngle( parseDouble( "segment angle" ) * 10.0 );
            NeedRIGHT();
            break;

        case T_layer:
            shape->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_width:
            shape->SetWidth( parseBoardUnits( T_width ) );
            NeedRIGHT();
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( shape->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_fill:
            foundFill = true;

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                // T_yes was used to indicate filling when first introduced,
                // so treat it like a solid fill since that was the only fill available
                case T_yes:
                case T_solid:
                    shape->SetFilled( true );
                    break;

                case T_none:
                    shape->SetFilled( false );
                    break;

                default:
                    Expecting( "yes, none, solid" );
                }
            }
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            shape->SetStatus( static_cast<EDA_ITEM_FLAGS>( parseHex() ) );
            NeedRIGHT();
            break;

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            shape->SetLocked( true );
            NeedRIGHT();
            break;

        default:
            Expecting( "layer, width, fill, tstamp, locked or status" );
        }
    }

    if( !foundFill )
    {
        // Legacy versions didn't have a filled flag but allowed some shapes to indicate they
        // should be filled by specifying a 0 stroke-width.
        if( shape->GetWidth() == 0
            && ( shape->GetShape() == PCB_SHAPE_TYPE::RECT
                 || shape->GetShape() == PCB_SHAPE_TYPE::CIRCLE ) )
        {
            shape->SetFilled( true );
        }
        // Polygons on non-Edge_Cuts layers were always filled
        else if( shape->GetShape() == PCB_SHAPE_TYPE::POLYGON && shape->GetLayer() != Edge_Cuts )
        {
            shape->SetFilled( true );
        }
    }

    // Only filled shapes may have a zero line-width.  This is not permitted in KiCad but some
    // external tools can generate invalid files.
    if( shape->GetWidth() <= 0 && !shape->IsFilled() )
    {
        shape->SetWidth( Millimeter2iu( DEFAULT_LINE_WIDTH ) );
    }

    return shape.release();
}


PCB_TEXT* PCB_PARSER::parsePCB_TEXT()
{
    wxCHECK_MSG( CurTok() == T_gr_text, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TEXT." ) );

    T token;

    std::unique_ptr<PCB_TEXT> text = std::make_unique<PCB_TEXT>( m_board );
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

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
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
            NextTok();
            const_cast<KIID&>( text->m_Uuid ) = CurStrToKIID();
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


PCB_DIMENSION_BASE* PCB_PARSER::parseDIMENSION()
{
    wxCHECK_MSG( CurTok() == T_dimension, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as DIMENSION." ) );

    T token;
    bool locked = false;
    std::unique_ptr<PCB_DIMENSION_BASE> dimension;

    token = NextTok();

    if( token == T_locked )
    {
        locked = true;
        token = NextTok();
    }

    // skip value that used to be saved
    if( token != T_LEFT )
        NeedLEFT();

    token = NextTok();

    bool isLegacyDimension = false;

    // Old format
    if( token == T_width )
    {
        isLegacyDimension = true;
        dimension = std::make_unique<PCB_DIM_ALIGNED>( nullptr );
        dimension->SetLineThickness( parseBoardUnits( "dimension width value" ) );
        NeedRIGHT();
    }
    else
    {
        if( token != T_type )
            Expecting( T_type );

        switch( NextTok() )
        {
        case T_aligned:
            dimension = std::make_unique<PCB_DIM_ALIGNED>( nullptr );
            break;

        case T_orthogonal:
            dimension = std::make_unique<PCB_DIM_ORTHOGONAL>( nullptr );
            break;

        case T_leader:
            dimension = std::make_unique<PCB_DIM_LEADER>( nullptr );
            break;

        case T_center:
            dimension = std::make_unique<PCB_DIM_CENTER>( nullptr );
            break;

        default:
            wxFAIL_MSG( wxT( "Cannot parse unknown dimension type %s" ) +
                        GetTokenString( CurTok() ) );
        }

        NeedRIGHT();
    }

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
            NextTok();
            const_cast<KIID&>( dimension->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_gr_text:
        {
            PCB_TEXT* text = parsePCB_TEXT();
            dimension->Text() = *text;

            // The text is part of the dimension and shares its uuid
            const_cast<KIID&>( dimension->Text().m_Uuid ) = dimension->m_Uuid;

            // Fetch other dimension properties out of the text item
            dimension->Text().SetTextPos( text->GetTextPos() );

            if( isLegacyDimension )
            {
                EDA_UNITS units   = EDA_UNITS::INCHES;
                FetchUnitsFromString( text->GetText(), units );
                dimension->SetUnits( units );
            }

            delete text;
            break;
        }

        // New format: feature points
        case T_pts:
        {
            wxPoint point;

            parseXY( &point.x, &point.y );
            dimension->SetStart( point );
            parseXY( &point.x, &point.y );
            dimension->SetEnd( point );

            NeedRIGHT();
            break;
        }

        case T_height:
        {
            wxCHECK_MSG( dimension->Type() == PCB_DIM_ALIGNED_T ||
                         dimension->Type() == PCB_DIM_ORTHOGONAL_T, nullptr,
                         wxT( "Invalid height token" ) );
            PCB_DIM_ALIGNED* aligned = static_cast<PCB_DIM_ALIGNED*>( dimension.get() );
            aligned->SetHeight( parseBoardUnits( "dimension height value" ) );
            NeedRIGHT();
            break;
        }

        case T_orientation:
        {
            wxCHECK_MSG( dimension->Type() == PCB_DIM_ORTHOGONAL_T, nullptr,
                         wxT( "Invalid orientation token" ) );
            PCB_DIM_ORTHOGONAL* ortho = static_cast<PCB_DIM_ORTHOGONAL*>( dimension.get() );

            int orientation = parseInt( "orthogonal dimension orientation" );
            orientation     = std::max( 0, std::min( 1, orientation ) );
            ortho->SetOrientation( static_cast<PCB_DIM_ORTHOGONAL::DIR>( orientation ) );
            NeedRIGHT();
            break;
        }

        case T_format:
        {
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                switch( token )
                {
                case T_LEFT:
                    continue;

                case T_prefix:
                    NeedSYMBOLorNUMBER();
                    dimension->SetPrefix( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_suffix:
                    NeedSYMBOLorNUMBER();
                    dimension->SetSuffix( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_units:
                {
                    int mode = parseInt( "dimension units mode" );
                    mode = std::max( 0, std::min( 4, mode ) );
                    dimension->SetUnitsMode( static_cast<DIM_UNITS_MODE>( mode ) );
                    NeedRIGHT();
                    break;
                }

                case T_units_format:
                {
                    int format = parseInt( "dimension units format" );
                    format = std::max( 0, std::min( 3, format ) );
                    dimension->SetUnitsFormat( static_cast<DIM_UNITS_FORMAT>( format ) );
                    NeedRIGHT();
                    break;
                }

                case T_precision:
                    dimension->SetPrecision( parseInt( "dimension precision" ) );
                    NeedRIGHT();
                    break;

                case T_override_value:
                    NeedSYMBOLorNUMBER();
                    dimension->SetOverrideTextEnabled( true );
                    dimension->SetOverrideText( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_suppress_zeroes:
                    dimension->SetSuppressZeroes( true );
                    break;

                default:
                    Expecting( "prefix, suffix, units, units_format, precision, override_value, "
                               "suppress_zeroes" );
                }
            }
            break;
        }

        case T_style:
        {
            // new format: default to keep text aligned off unless token is present
            dimension->SetKeepTextAligned( false );

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                switch( token )
                {
                case T_LEFT:
                    continue;

                case T_thickness:
                    dimension->SetLineThickness( parseBoardUnits( "extension line thickness" ) );
                    NeedRIGHT();
                    break;

                case T_arrow_length:
                    dimension->SetArrowLength( parseBoardUnits( "arrow length" ) );
                    NeedRIGHT();
                    break;

                case T_text_position_mode:
                {
                    int mode = parseInt( "dimension text position mode" );
                    mode = std::max( 0, std::min( 3, mode ) );
                    dimension->SetTextPositionMode( static_cast<DIM_TEXT_POSITION>( mode ) );
                    NeedRIGHT();
                    break;
                }

                case T_extension_height:
                {
                    PCB_DIM_ALIGNED* aligned = dynamic_cast<PCB_DIM_ALIGNED*>( dimension.get() );
                    wxCHECK_MSG( aligned, nullptr, wxT( "Invalid extension_height token" ) );
                    aligned->SetExtensionHeight( parseBoardUnits( "extension height" ) );
                    NeedRIGHT();
                    break;
                }

                case T_extension_offset:
                    dimension->SetExtensionOffset( parseBoardUnits( "extension offset" ) );
                    NeedRIGHT();
                    break;

                case T_keep_text_aligned:
                    dimension->SetKeepTextAligned( true );
                    break;

                case T_text_frame:
                {
                    wxCHECK_MSG( dimension->Type() == PCB_DIM_LEADER_T, nullptr,
                                 wxT( "Invalid text_frame token" ) );
                    PCB_DIM_LEADER* leader = static_cast<PCB_DIM_LEADER*>( dimension.get() );

                    int textFrame = parseInt( "dimension text frame mode" );
                    textFrame = std::max( 0, std::min( 3, textFrame ) );
                    leader->SetTextFrame( static_cast<DIM_TEXT_FRAME>( textFrame ) );
                    NeedRIGHT();
                    break;
                }

                default:
                    Expecting( "thickness, arrow_length, text_position_mode, extension_height, "
                               "extension_offset" );
                }
            }

            break;
        }

        // Old format: feature1 stores a feature line.  We only care about the origin.
        case T_feature1:
        {
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            wxPoint point;

            parseXY( &point.x, &point.y );
            dimension->SetStart( point );

            parseXY( nullptr, nullptr ); // Ignore second point
            NeedRIGHT();
            NeedRIGHT();
            break;
        }

        // Old format: feature2 stores a feature line.  We only care about the end point.
        case T_feature2:
        {
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            wxPoint point;

            parseXY( &point.x, &point.y );
            dimension->SetEnd( point );

            parseXY( nullptr, nullptr ); // Ignore second point

            NeedRIGHT();
            NeedRIGHT();
            break;
        }

        case T_crossbar:
        {
            NeedLEFT();
            token = NextTok();

            if( token == T_pts )
            {
                // If we have a crossbar, we know we're an old aligned dimension
                PCB_DIM_ALIGNED* aligned = static_cast<PCB_DIM_ALIGNED*>( dimension.get() );

                // Old style: calculate height from crossbar
                wxPoint point1, point2;
                parseXY( &point1.x, &point1.y );
                parseXY( &point2.x, &point2.y );
                aligned->UpdateHeight( point2, point1 ); // Yes, backwards intentionally
                NeedRIGHT();
            }

            NeedRIGHT();
            break;
        }

        // Arrow: no longer saved; no-op
        case T_arrow1a:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( nullptr, nullptr );
            parseXY( nullptr, nullptr );
            NeedRIGHT();
            NeedRIGHT();
            break;

        // Arrow: no longer saved; no-op
        case T_arrow1b:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( nullptr, nullptr );
            parseXY( nullptr, nullptr );
            NeedRIGHT();
            NeedRIGHT();
            break;

        // Arrow: no longer saved; no-op
        case T_arrow2a:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( nullptr, nullptr );
            parseXY( nullptr, nullptr );
            NeedRIGHT();
            NeedRIGHT();
            break;

        // Arrow: no longer saved; no-op
        case T_arrow2b:
            NeedLEFT();
            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            parseXY( nullptr, nullptr );
            parseXY( nullptr, nullptr );
            NeedRIGHT();
            NeedRIGHT();
            break;

        default:
            Expecting( "layer, tstamp, gr_text, feature1, feature2, crossbar, arrow1a, "
                       "arrow1b, arrow2a, or arrow2b" );
        }
    }

    if( locked )
        dimension->SetLocked( true );

    dimension->Update();

    return dimension.release();
}


FOOTPRINT* PCB_PARSER::parseFOOTPRINT( wxArrayString* aInitialComments )
{
    try
    {
        return parseFOOTPRINT_unchecked( aInitialComments );
    }
    catch( const PARSE_ERROR& parse_error )
    {
        if( m_tooRecent )
            throw FUTURE_FORMAT_ERROR( parse_error, GetRequiredVersion() );
        else
            throw;
    }
}


FOOTPRINT* PCB_PARSER::parseFOOTPRINT_unchecked( wxArrayString* aInitialComments )
{
    wxCHECK_MSG( CurTok() == T_module || CurTok() == T_footprint, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as FOOTPRINT." ) );

    wxString name;
    wxPoint  pt;
    T        token;
    LIB_ID   fpid;
    int      attributes = 0;

    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );

    std::map<wxString, wxString> properties;

    footprint->SetInitialComments( aInitialComments );

    token = NextTok();

    if( !IsSymbol( token ) && token != T_NUMBER )
        Expecting( "symbol|number" );

    name = FromUTF8();

    if( !name.IsEmpty() && fpid.Parse( name, true ) >= 0 )
    {
        wxString error;
        error.Printf( _( "Invalid footprint ID in\nfile: '%s'\nline: %d\noffset: %d." ),
                      CurSource(),
                      CurLineNumber(),
                      CurOffset() );
        THROW_IO_ERROR( error );
    }

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_version:
        {
            // Theoretically a footprint nested in a PCB could declare its own version, though
            // as of writing this comment we don't do that. Just in case, take the greater
            // version.
            int this_version = parseInt( FromUTF8().mb_str( wxConvUTF8 ) );
            NeedRIGHT();
            m_requiredVersion = std::max( m_requiredVersion, this_version );
            m_tooRecent       = ( m_requiredVersion > SEXPR_BOARD_FILE_VERSION );
            break;
        }

        case T_generator:
            // We currently ignore the generator when parsing. It is included in the file for manual
            // indication of where the footprint came from.
            NeedSYMBOL();
            NeedRIGHT();
            break;

        case T_locked:
            footprint->SetLocked( true );
            break;

        case T_placed:
            footprint->SetIsPlaced( true );
            break;

        case T_layer:
        {
            // Footprints can be only on the front side or the back side.
            // but because we can find some stupid layer in file, ensure a
            // acceptable layer is set for the footprint
            PCB_LAYER_ID layer = parseBoardItemLayer();
            footprint->SetLayer( layer == B_Cu ? B_Cu : F_Cu );
        }
            NeedRIGHT();
            break;

        case T_tedit:
            footprint->SetLastEditTime( parseHex() );
            NeedRIGHT();
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( footprint->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_at:
            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            footprint->SetPosition( pt );
            token = NextTok();

            if( token == T_NUMBER )
            {
                footprint->SetOrientation( parseDouble() * 10.0 );
                NeedRIGHT();
            }
            else if( token != T_RIGHT )
            {
                Expecting( T_RIGHT );
            }

            break;

        case T_descr:
            NeedSYMBOLorNUMBER(); // some symbols can be 0508, so a number is also a symbol here
            footprint->SetDescription( FromUTF8() );
            NeedRIGHT();
            break;

        case T_tags:
            NeedSYMBOLorNUMBER(); // some symbols can be 0508, so a number is also a symbol here
            footprint->SetKeywords( FromUTF8() );
            NeedRIGHT();
            break;

        case T_property:
            properties.insert( parseProperty() );
            break;

        case T_path:
            NeedSYMBOLorNUMBER(); // Paths can be numerical so a number is also a symbol here
            footprint->SetPath( KIID_PATH( FromUTF8() ) );
            NeedRIGHT();
            break;

        case T_autoplace_cost90:
            footprint->SetPlacementCost90( parseInt( "auto place cost at 90 degrees" ) );
            NeedRIGHT();
            break;

        case T_autoplace_cost180:
            footprint->SetPlacementCost180( parseInt( "auto place cost at 180 degrees" ) );
            NeedRIGHT();
            break;

        case T_solder_mask_margin:
            footprint->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();
            break;

        case T_solder_paste_margin:
            footprint->SetLocalSolderPasteMargin(
                    parseBoardUnits( "local solder paste margin value" ) );
            NeedRIGHT();
            break;

        case T_solder_paste_ratio:
            footprint->SetLocalSolderPasteMarginRatio(
                    parseDouble( "local solder paste margin ratio value" ) );
            NeedRIGHT();
            break;

        case T_clearance:
            footprint->SetLocalClearance( parseBoardUnits( "local clearance value" ) );
            NeedRIGHT();
            break;

        case T_zone_connect:
            footprint->SetZoneConnection((ZONE_CONNECTION) parseInt( "zone connection value" ) );
            NeedRIGHT();
            break;

        case T_thermal_width:
            footprint->SetThermalWidth( parseBoardUnits( "thermal width value" ) );
            NeedRIGHT();
            break;

        case T_thermal_gap:
            footprint->SetThermalGap( parseBoardUnits( "thermal gap value" ) );
            NeedRIGHT();
            break;

        case T_attr:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                switch( token )
                {
                case T_virtual:     // legacy token prior to version 20200826
                    attributes |= FP_EXCLUDE_FROM_POS_FILES | FP_EXCLUDE_FROM_BOM;
                    break;

                case T_through_hole:
                    attributes |= FP_THROUGH_HOLE;
                    break;

                case T_smd:
                    attributes |= FP_SMD;
                    break;

                case T_board_only:
                    attributes |= FP_BOARD_ONLY;
                    break;

                case T_exclude_from_pos_files:
                    attributes |= FP_EXCLUDE_FROM_POS_FILES;
                    break;

                case T_exclude_from_bom:
                    attributes |= FP_EXCLUDE_FROM_BOM;
                    break;

                default:
                    Expecting( "through_hole, smd, virtual, board_only, exclude_from_pos_files "
                               "or exclude_from_bom" );
                }
            }
            break;

        case T_fp_text:
        {
            FP_TEXT* text = parseFP_TEXT();
            text->SetParent( footprint.get() );
            double orientation = text->GetTextAngle();
            orientation -= footprint->GetOrientation();
            text->SetTextAngle( orientation );
            text->SetDrawCoord();

            switch( text->GetType() )
            {
            case FP_TEXT::TEXT_is_REFERENCE:
                footprint->Reference() = *text;
                const_cast<KIID&>( footprint->Reference().m_Uuid ) = text->m_Uuid;
                delete text;
                break;

            case FP_TEXT::TEXT_is_VALUE:
                footprint->Value() = *text;
                const_cast<KIID&>( footprint->Value().m_Uuid ) = text->m_Uuid;
                delete text;
                break;

            default:
                footprint->Add( text, ADD_MODE::APPEND );
            }
        }
            break;

        case T_fp_arc:
        {
            FP_SHAPE* shape = parseFP_SHAPE();

            // Drop 0 and NaN angles as these can corrupt/crash the schematic
            if( std::isnormal( shape->GetAngle() ) )
            {
                shape->SetParent( footprint.get() );
                shape->SetDrawCoord();
                footprint->Add( shape, ADD_MODE::APPEND );
            }
            else
                delete shape;
        }
            break;

        case T_fp_circle:
        case T_fp_curve:
        case T_fp_rect:
        case T_fp_line:
        case T_fp_poly:
        {
            FP_SHAPE* shape = parseFP_SHAPE();
            shape->SetParent( footprint.get() );
            shape->SetDrawCoord();
            footprint->Add( shape, ADD_MODE::APPEND );
        }
            break;

        case T_pad:
        {
            PAD* pad = parsePAD( footprint.get() );
            pt       = pad->GetPos0();

            RotatePoint( &pt, footprint->GetOrientation() );
            pad->SetPosition( pt + footprint->GetPosition() );
            footprint->Add( pad, ADD_MODE::APPEND );
        }
            break;

        case T_model:
        {
            FP_3DMODEL* model = parse3DModel();
            footprint->Add3DModel( model );
            delete model;
        }
            break;

        case T_zone:
        {
            ZONE* zone = parseZONE( footprint.get() );
            footprint->Add( zone, ADD_MODE::APPEND );
        }
            break;

        case T_group:
            parseGROUP( footprint.get() );
            break;

        default:
            Expecting( "locked, placed, tedit, tstamp, at, descr, tags, path, "
                       "autoplace_cost90, autoplace_cost180, solder_mask_margin, "
                       "solder_paste_margin, solder_paste_ratio, clearance, "
                       "zone_connect, thermal_width, thermal_gap, attr, fp_text, "
                       "fp_arc, fp_circle, fp_curve, fp_line, fp_poly, fp_rect, pad, "
                       "zone, group, generator, version or model" );
        }
    }

    // In legacy files the lack of attributes indicated a through-hole component which was by
    // default excluded from pos files.  However there was a hack to look for SMD pads and
    // consider those "mislabeled through-hole components" and therefore include them in place
    // files.  We probably don't want to get into that game so we'll just include them by
    // default and let the user change it if required.
    if( m_requiredVersion < 20200826 && attributes == 0 )
        attributes |= FP_THROUGH_HOLE;

    // Legacy files controlled pad locking at the footprint level.
    if( m_requiredVersion < 20210108 )
    {
        for( PAD* pad : footprint->Pads() )
            pad->SetLocked( footprint->IsLocked() || footprint->LegacyPadsLocked() );
    }

    footprint->SetAttributes( attributes );

    footprint->SetFPID( fpid );
    footprint->SetProperties( properties );

    return footprint.release();
}


FP_TEXT* PCB_PARSER::parseFP_TEXT()
{
    wxCHECK_MSG( CurTok() == T_fp_text, NULL,
                 wxString::Format( wxT( "Cannot parse %s as FP_TEXT at line %d, offset %d." ),
                                   GetTokenString( CurTok() ),
                                   CurLineNumber(), CurOffset() ) );

    T token = NextTok();

    std::unique_ptr<FP_TEXT> text = std::make_unique<FP_TEXT>( nullptr );

    switch( token )
    {
    case T_reference:
        text->SetType( FP_TEXT::TEXT_is_REFERENCE );
        break;

    case T_value:
        text->SetType( FP_TEXT::TEXT_is_VALUE );
        break;

    case T_user:
        break;          // Default type is user text.

    default:
        THROW_IO_ERROR(
                wxString::Format( _( "Cannot handle footprint text type %s" ), FromUTF8() ) );
    }

    NeedSYMBOLorNUMBER();

    wxString value = FromUTF8();
    value.Replace( "%V", "${VALUE}" );
    value.Replace( "%R", "${REFERENCE}" );
    text->SetText( value );
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

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( text->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "layer, hide, effects or tstamp" );
        }
    }

    return text.release();
}


FP_SHAPE* PCB_PARSER::parseFP_SHAPE()
{
    wxCHECK_MSG( CurTok() == T_fp_arc || CurTok() == T_fp_circle || CurTok() == T_fp_curve ||
                 CurTok() == T_fp_rect || CurTok() == T_fp_line || CurTok() == T_fp_poly, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as FP_SHAPE." ) );

    wxPoint pt;
    T token;

    std::unique_ptr<FP_SHAPE> shape = std::make_unique<FP_SHAPE>( nullptr );

    switch( CurTok() )
    {
    case T_fp_arc:
        shape->SetShape( PCB_SHAPE_TYPE::ARC );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        // the start keyword actually gives the arc center
        // Allows also T_center for future change
        if( token != T_start && token != T_center )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetStart0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )    // end keyword actually gives the starting point of the arc
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_angle )
            Expecting( T_angle );

        // Setting angle will set m_ThirdPoint0, so must be done after setting
        // m_Start0 and m_End0
        shape->SetAngle( parseDouble( "segment angle" ) * 10.0 );
        NeedRIGHT();
        break;

    case T_fp_circle:
        shape->SetShape( PCB_SHAPE_TYPE::CIRCLE );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_center )
            Expecting( T_center );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetStart0( pt );
        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd0( pt );
        NeedRIGHT();
        break;

    case T_fp_curve:
        shape->SetShape( PCB_SHAPE_TYPE::CURVE );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        shape->SetStart0( parseXY() );
        shape->SetBezier0_C1( parseXY() );
        shape->SetBezier0_C2( parseXY() );
        shape->SetEnd0( parseXY() );
        NeedRIGHT();
        break;

    case T_fp_rect:
        shape->SetShape( PCB_SHAPE_TYPE::RECT );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetStart0( pt );

        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd0( pt );
        NeedRIGHT();
        break;

    case T_fp_line:
        // Default PCB_SHAPE type is S_SEGMENT.
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_start )
            Expecting( T_start );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetStart0( pt );

        NeedRIGHT();
        NeedLEFT();
        token = NextTok();

        if( token != T_end )
            Expecting( T_end );

        pt.x = parseBoardUnits( "X coordinate" );
        pt.y = parseBoardUnits( "Y coordinate" );
        shape->SetEnd0( pt );
        NeedRIGHT();
        break;

    case T_fp_poly:
    {
        shape->SetShape( PCB_SHAPE_TYPE::POLYGON );
        shape->SetPolyPoints( {} );
        SHAPE_LINE_CHAIN& outline = shape->GetPolyShape().Outline( 0 );

        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_pts )
            Expecting( T_pts );

        while( (token = NextTok() ) != T_RIGHT )
            parseOutlinePoints( outline );
    }
        break;

    default:
        Expecting( "fp_arc, fp_circle, fp_curve, fp_line, fp_poly, or fp_rect" );
    }

    bool foundFill = false;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_layer:
            shape->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_width:
            shape->SetWidth( parseBoardUnits( T_width ) );
            NeedRIGHT();
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( shape->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_fill:
            foundFill = true;

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                // T_yes was used to indicate filling when first introduced,
                // so treat it like a solid fill since that was the only fill available
                case T_yes:
                case T_solid:
                    shape->SetFilled( true );
                    break;

                case T_none:
                    shape->SetFilled( false );
                    break;

                default:
                    Expecting( "yes, none, solid" );
                }
            }

            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            shape->SetStatus( static_cast<EDA_ITEM_FLAGS>( parseHex() ) );
            NeedRIGHT();
            break;

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            shape->SetLocked( true );
            NeedRIGHT();
            break;

        default:
            Expecting( "layer, width, fill, tstamp, locked, or status" );
        }
    }

    if( !foundFill )
    {
        // Legacy versions didn't have a filled flag but allowed some shapes to indicate they
        // should be filled by specifying a 0 stroke-width.
        if( shape->GetWidth() == 0
            && ( shape->GetShape() == PCB_SHAPE_TYPE::RECT
                 || shape->GetShape() == PCB_SHAPE_TYPE::CIRCLE ) )
        {
            shape->SetFilled( true );
        }
        // Polygons on non-Edge_Cuts layers were always filled
        else if( shape->GetShape() == PCB_SHAPE_TYPE::POLYGON && shape->GetLayer() != Edge_Cuts )
        {
            shape->SetFilled( true );
        }
    }

    // Only filled shapes may have a zero line-width.  This is not permitted in KiCad but some
    // external tools can generate invalid files.
    if( shape->GetWidth() <= 0 && !shape->IsFilled() )
    {
        shape->SetWidth( Millimeter2iu( DEFAULT_LINE_WIDTH ) );
    }

    return shape.release();
}


PAD* PCB_PARSER::parsePAD( FOOTPRINT* aParent )
{
    wxCHECK_MSG( CurTok() == T_pad, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PAD." ) );

    wxSize  sz;
    wxPoint pt;

    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aParent );

    // File only contains a token if KeepTopBottom is true
    pad->SetKeepTopBottom( false );

    NeedSYMBOLorNUMBER();
    pad->SetName( FromUTF8() );

    T token = NextTok();

    switch( token )
    {
    case T_thru_hole:
        pad->SetAttribute( PAD_ATTRIB::PTH );
        break;

    case T_smd:
        pad->SetAttribute( PAD_ATTRIB::SMD );

        // Default PAD object is thru hole with drill.
        // SMD pads have no hole
        pad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case T_connect:
        pad->SetAttribute( PAD_ATTRIB::CONN );

        // Default PAD object is thru hole with drill.
        // CONN pads have no hole
        pad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case T_np_thru_hole:
        pad->SetAttribute( PAD_ATTRIB::NPTH );
        break;

    default:
        Expecting( "thru_hole, smd, connect, or np_thru_hole" );
    }

    token = NextTok();

    switch( token )
    {
    case T_circle:
        pad->SetShape( PAD_SHAPE::CIRCLE );
        break;

    case T_rect:
        pad->SetShape( PAD_SHAPE::RECT );
        break;

    case T_oval:
        pad->SetShape( PAD_SHAPE::OVAL );
        break;

    case T_trapezoid:
        pad->SetShape( PAD_SHAPE::TRAPEZOID );
        break;

    case T_roundrect:
        // Note: the shape can be PAD_SHAPE::ROUNDRECT or PAD_SHAPE::CHAMFERED_RECT
        // (if chamfer parameters are found later in pad descr.)
        pad->SetShape( PAD_SHAPE::ROUNDRECT );
        break;

    case T_custom:
        pad->SetShape( PAD_SHAPE::CUSTOM );
        break;

    default:
        Expecting( "circle, rectangle, roundrect, oval, trapezoid or custom" );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_locked )
        {
            pad->SetLocked( true );
            token = NextTok();
        }

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

                // This fixes a bug caused by setting the default PAD drill size to a value other
                // than 0 used to fix a bunch of debug assertions even though it is defined as a
                // through hole pad.  Wouldn't a though hole pad with no drill be a surface mount
                // pad (or a conn pad which is a smd pad with no solder paste)?
                if( ( pad->GetAttribute() != PAD_ATTRIB::SMD ) && ( pad->GetAttribute() != PAD_ATTRIB::CONN ) )
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
            {
                wxLogError( _( "Invalid net ID in\nfile: %s\nline: %d offset: %d" ),
                            CurSource(),
                            CurLineNumber(),
                            CurOffset() );
            }

            NeedSYMBOLorNUMBER();

            // Test validity of the netname in file for netcodes expected having a net name
            if( m_board && pad->GetNetCode() > 0 )
            {
                wxString netName( FromUTF8() );

                // Convert overbar syntax from `~...~` to `~{...}`.  These were left out of the
                // first merge so the version is a bit later.
                if( m_requiredVersion < 20210606 )
                    netName = ConvertToNewOverbarNotation( netName );

                if( netName != m_board->FindNet( pad->GetNetCode() )->GetNetname() )
                {
                    pad->SetNetCode( NETINFO_LIST::ORPHANED, /* aNoAssert */ true );
                    wxLogError( _( "Net name doesn't match ID in\nfile: %s\nline: %d offset: %d" ),
                                 CurSource(),
                                 CurLineNumber(),
                                 CurOffset() );
                }
            }

            NeedRIGHT();
            break;

        case T_pinfunction:
            NeedSYMBOLorNUMBER();
            pad->SetPinFunction( FromUTF8() );
            NeedRIGHT();
            break;

        case T_pintype:
            NeedSYMBOLorNUMBER();
            pad->SetPinType( FromUTF8() );
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
            pad->SetZoneConnection( (ZONE_CONNECTION) parseInt( "zone connection value" ) );
            NeedRIGHT();
            break;

        case T_thermal_width:
            pad->SetThermalSpokeWidth( parseBoardUnits( T_thermal_width ) );
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
                pad->SetShape( PAD_SHAPE::CHAMFERED_RECT );

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
                pad->SetShape( PAD_SHAPE::CHAMFERED_RECT );
        }
            break;

        case T_property:
        {
            while( token != T_RIGHT )
            {
                token = NextTok();

                switch( token )
                {
                case T_pad_prop_bga:
                    pad->SetProperty( PAD_PROP::BGA );
                    break;

                case T_pad_prop_fiducial_glob:
                    pad->SetProperty( PAD_PROP::FIDUCIAL_GLBL );
                    break;

                case T_pad_prop_fiducial_loc:
                    pad->SetProperty( PAD_PROP::FIDUCIAL_LOCAL );
                    break;

                case T_pad_prop_testpoint:
                    pad->SetProperty( PAD_PROP::TESTPOINT );
                    break;

                case T_pad_prop_castellated:
                    pad->SetProperty( PAD_PROP::CASTELLATED );
                    break;

                case T_pad_prop_heatsink:
                    pad->SetProperty( PAD_PROP::HEATSINK );
                    break;

                case T_none:
                    pad->SetProperty( PAD_PROP::NONE );
                    break;

                case T_RIGHT:
                    break;

                default:
#if 0   // Currently: skip unknown property
                    Expecting( "pad_prop_bga pad_prop_fiducial_glob pad_prop_fiducial_loc"
                               " pad_prop_heatsink or pad_prop_castellated" );
#endif
                    break;
                }
            }
        }
            break;

        case T_options:
            parsePAD_option( pad.get() );
            break;

        case T_primitives:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                // Currently, I am using parsePCB_SHAPE() to read basic shapes parameters,
                // because they are the same as a PCB_SHAPE.
                // However it could be better to write a specific parser, to avoid possible issues
                // if the PCB_SHAPE parser is modified.
                PCB_SHAPE* dummysegm = NULL;

                switch( token )
                {
                case T_gr_arc:
                    dummysegm = parsePCB_SHAPE();
                    pad->AddPrimitiveArc( dummysegm->GetCenter(), dummysegm->GetArcStart(),
                                          dummysegm->GetAngle(), dummysegm->GetWidth() );
                    break;

                case T_gr_line:
                    dummysegm = parsePCB_SHAPE();
                    pad->AddPrimitiveSegment( dummysegm->GetStart(), dummysegm->GetEnd(),
                                              dummysegm->GetWidth() );
                    break;

                case T_gr_circle:
                    dummysegm = parsePCB_SHAPE();
                    pad->AddPrimitiveCircle( dummysegm->GetCenter(), dummysegm->GetRadius(),
                                             dummysegm->GetWidth(), dummysegm->IsFilled() );
                    break;

                case T_gr_rect:
                    dummysegm = parsePCB_SHAPE();
                    pad->AddPrimitiveRect( dummysegm->GetStart(), dummysegm->GetEnd(),
                                           dummysegm->GetWidth(), dummysegm->IsFilled() );
                    break;


                case T_gr_poly:
                    dummysegm = parsePCB_SHAPE();
                    pad->AddPrimitivePoly( dummysegm->BuildPolyPointsList(), dummysegm->GetWidth(),
                                           dummysegm->IsFilled() );
                    break;

                case T_gr_curve:
                    dummysegm = parsePCB_SHAPE();
                    pad->AddPrimitiveCurve( dummysegm->GetStart(), dummysegm->GetEnd(),
                                            dummysegm->GetBezControl1(),
                                            dummysegm->GetBezControl2(), dummysegm->GetWidth() );
                    break;

                default:
                    Expecting( "gr_line, gr_arc, gr_circle, gr_curve, gr_rect or gr_poly" );
                    break;
                }

                delete dummysegm;
            }
            break;

        case T_remove_unused_layers:
            pad->SetRemoveUnconnected( true );
            NeedRIGHT();
            break;

        case T_keep_end_layers:
            pad->SetKeepTopBottom( true );
            NeedRIGHT();
            break;

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            pad->SetLocked( true );
            NeedRIGHT();
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( pad->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at, locked, drill, layers, net, die_length, roundrect_rratio, "
                       "solder_mask_margin, solder_paste_margin, solder_paste_margin_ratio, "
                       "clearance, tstamp, primitives, remove_unused_layers, keep_end_layers, "
                       "pinfunction, pintype, zone_connect, thermal_width, or thermal_gap" );
        }
    }

    return pad.release();
}


bool PCB_PARSER::parsePAD_option( PAD* aPad )
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
                    aPad->SetAnchorPadShape( PAD_SHAPE::RECT );
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
            // (like usual pads) or the convex hull of the pad shape.
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


// Example of group format:
//     (group <(name “groupName”)> (id 12345679)
//         (members id_1 id_2 … id_last )
//     )
void PCB_PARSER::parseGROUP( BOARD_ITEM* aParent )
{
    wxCHECK_RET( CurTok() == T_group,
            wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_GROUP." ) );

    wxPoint pt;
    T       token;

    m_groupInfos.push_back( GROUP_INFO() );
    GROUP_INFO& groupInfo = m_groupInfos.back();
    groupInfo.parent = aParent;

    token = NextTok();

    if( token != T_LEFT )
    {
        // Optional group name present.

        if( !IsSymbol( token ) )
            Expecting( DSN_SYMBOL );

        groupInfo.name = FromUTF8();
    }

    NeedLEFT();
    token = NextTok();

    if( token != T_id )
    {
        Expecting( T_id );
    }

    NextTok();
    groupInfo.uuid = CurStrToKIID();
    NeedRIGHT();

    NeedLEFT();
    token = NextTok();

    if( token != T_members )
    {
        Expecting( T_members );
    }

    while( ( token = NextTok() ) != T_RIGHT )
    {
        // This token is the Uuid of the item in the group.
        // Since groups are serialized at the end of the file/footprint, the Uuid should already
        // have been seen and exist in the board.
        KIID uuid( CurStr() );
        groupInfo.memberUuids.push_back( uuid );
    }

    NeedRIGHT();
}


PCB_ARC* PCB_PARSER::parseARC()
{
    wxCHECK_MSG( CurTok() == T_arc, NULL,
            wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as ARC." ) );

    wxPoint pt;
    T       token;

    std::unique_ptr<PCB_ARC> arc = std::make_unique<PCB_ARC>( m_board );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token == T_locked )
        {
            arc->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_start:
            pt.x = parseBoardUnits( "start x" );
            pt.y = parseBoardUnits( "start y" );
            arc->SetStart( pt );
            break;

        case T_mid:
            pt.x = parseBoardUnits( "mid x" );
            pt.y = parseBoardUnits( "mid y" );
            arc->SetMid( pt );
            break;

        case T_end:
            pt.x = parseBoardUnits( "end x" );
            pt.y = parseBoardUnits( "end y" );
            arc->SetEnd( pt );
            break;

        case T_width:
            arc->SetWidth( parseBoardUnits( "width" ) );
            break;

        case T_layer:
            arc->SetLayer( parseBoardItemLayer() );
            break;

        case T_net:
            if( !arc->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
                THROW_IO_ERROR( wxString::Format(
                        _( "Invalid net ID in\nfile: '%s'\nline: %d\noffset: %d." ),
                        CurSource(),
                        CurLineNumber(),
                        CurOffset() ) );
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( arc->m_Uuid ) = CurStrToKIID();
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            arc->SetStatus( static_cast<EDA_ITEM_FLAGS>( parseHex() ) );
            break;

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            arc->SetLocked( true );
            break;

        default:
            Expecting( "start, mid, end, width, layer, net, tstamp, or status" );
        }

        NeedRIGHT();
    }

    return arc.release();
}


PCB_TRACK* PCB_PARSER::parsePCB_TRACK()
{
    wxCHECK_MSG( CurTok() == T_segment, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TRACK." ) );

    wxPoint pt;
    T token;

    std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( m_board );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_locked )
        {
            track->SetLocked( true );
            token = NextTok();
        }

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
            if( !track->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
                THROW_IO_ERROR( wxString::Format(
                        _( "Invalid net ID in\nfile: '%s'\nline: %d\noffset: %d." ), CurSource(),
                        CurLineNumber(), CurOffset() ) );
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( track->m_Uuid ) = CurStrToKIID();
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            track->SetStatus( static_cast<EDA_ITEM_FLAGS>( parseHex() ) );
            break;

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            track->SetLocked( true );
            break;

        default:
            Expecting( "start, end, width, layer, net, tstamp, or locked" );
        }

        NeedRIGHT();
    }

    return track.release();
}


PCB_VIA* PCB_PARSER::parsePCB_VIA()
{
    wxCHECK_MSG( CurTok() == T_via, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_VIA." ) );

    wxPoint pt;
    T token;

    std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( m_board );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_locked )
        {
            via->SetLocked( true );
            token = NextTok();
        }

        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_blind:
            via->SetViaType( VIATYPE::BLIND_BURIED );
            break;

        case T_micro:
            via->SetViaType( VIATYPE::MICROVIA );
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
            if( !via->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
            {
                THROW_IO_ERROR( wxString::Format( _( "Invalid net ID in\n"
                                                     "file: '%s'\n"
                                                     "line: %d\n"
                                                     "offset: %d" ),
                                      CurSource(),
                                      CurLineNumber(),
                                      CurOffset() ) );
            }

            NeedRIGHT();
            break;

        case T_remove_unused_layers:
            via->SetRemoveUnconnected( true );
            NeedRIGHT();
            break;

        case T_keep_end_layers:
            via->SetKeepTopBottom( true );
            NeedRIGHT();
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( via->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            via->SetStatus( static_cast<EDA_ITEM_FLAGS>( parseHex() ) );
            NeedRIGHT();
            break;

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            via->SetLocked( true );
            NeedRIGHT();
            break;

        case T_free:
            via->SetIsFree();
            NeedRIGHT();
            break;

        default:
            Expecting( "blind, micro, at, size, drill, layers, net, free, tstamp, or status" );
        }
    }

    return via.release();
}


ZONE* PCB_PARSER::parseZONE( BOARD_ITEM_CONTAINER* aParent )
{
    wxCHECK_MSG( CurTok() == T_zone, NULL,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) +
                 wxT( " as ZONE." ) );

    ZONE_BORDER_DISPLAY_STYLE hatchStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;

    int      hatchPitch = ZONE::GetDefaultHatchPitch();
    wxPoint  pt;
    T        token;
    int      tmp;
    wxString netnameFromfile;    // the zone net name find in file

    // bigger scope since each filled_polygon is concatenated in here
    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> pts;
    bool         inFootprint = false;
    PCB_LAYER_ID filledLayer;
    bool         addedFilledPolygons = false;

    if( dynamic_cast<FOOTPRINT*>( aParent ) )      // The zone belongs a footprint
        inFootprint = true;

    std::unique_ptr<ZONE> zone;

    if( inFootprint )
        zone = std::make_unique<FP_ZONE>( aParent );
    else
        zone = std::make_unique<ZONE>( aParent );

    zone->SetPriority( 0 );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_locked )
        {
            zone->SetLocked( true );
            token = NextTok();
        }

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

            if( !zone->SetNetCode( tmp, /* aNoAssert */ true ) )
                THROW_IO_ERROR( wxString::Format(
                        _( "Invalid net ID in\n file: '%s;\nline: %d\noffset: %d." ),
                        CurSource(),
                        CurLineNumber(),
                        CurOffset() ) );

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

        case T_layers:  // keyword for zones that can live on a set of layers
            zone->SetLayerSet( parseBoardItemLayersAsMask() );
            break;

        case T_tstamp:
            NextTok();
            const_cast<KIID&>( zone->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_hatch:
            token = NextTok();

            if( token != T_none && token != T_edge && token != T_full )
                Expecting( "none, edge, or full" );

            switch( token )
            {
            default:
            case T_none: hatchStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;      break;
            case T_edge: hatchStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; break;
            case T_full: hatchStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL; break;
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
                    zone->SetPadConnection( ZONE_CONNECTION::FULL );
                    break;

                case T_no:
                    zone->SetPadConnection( ZONE_CONNECTION::NONE );
                    break;

                case T_thru_hole_only:
                    zone->SetPadConnection( ZONE_CONNECTION::THT_THERMAL );
                    break;

                case T_clearance:
                    zone->SetLocalClearance( parseBoardUnits( "zone clearance" ) );
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
            zone->SetFillVersion( parseBool() ? 5 : 6 );
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

                        zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
                        m_board->SetModified();
                    }
                    else if( token == T_hatch )
                    {
                        zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
                    }
                    else
                    {
                        zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
                    }

                    NeedRIGHT();
                    break;

                case T_hatch_thickness:
                    zone->SetHatchThickness( parseBoardUnits( T_hatch_thickness ) );
                    NeedRIGHT();
                    break;

                case T_hatch_gap:
                    zone->SetHatchGap( parseBoardUnits( T_hatch_gap ) );
                    NeedRIGHT();
                    break;

                case T_hatch_orientation:
                    zone->SetHatchOrientation( parseDouble( T_hatch_orientation ) );
                    NeedRIGHT();
                    break;

                case T_hatch_smoothing_level:
                    zone->SetHatchSmoothingLevel( parseDouble( T_hatch_smoothing_level ) );
                    NeedRIGHT();
                    break;

                case T_hatch_smoothing_value:
                    zone->SetHatchSmoothingValue( parseDouble( T_hatch_smoothing_value ) );
                    NeedRIGHT();
                    break;

                case T_hatch_border_algorithm:
                    token = NextTok();

                    if( token != T_hatch_thickness && token != T_min_thickness )
                        Expecting( "hatch_thickness or min_thickness" );

                    zone->SetHatchBorderAlgorithm( token == T_hatch_thickness ? 1 : 0 );
                    NeedRIGHT();
                    break;

                case T_hatch_min_hole_area:
                    zone->SetHatchHoleMinArea( parseDouble( T_hatch_min_hole_area ) );
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
                    zone->SetThermalReliefSpokeWidth( parseBoardUnits( T_thermal_bridge_width ) );
                    NeedRIGHT();
                    break;

                case T_smoothing:
                    switch( NextTok() )
                    {
                    case T_none:
                        zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_NONE );
                        break;

                    case T_chamfer:
                        if( !zone->GetIsRuleArea() ) // smoothing has meaning only for filled zones
                            zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_CHAMFER );
                        break;

                    case T_fillet:
                        if( !zone->GetIsRuleArea() ) // smoothing has meaning only for filled zones
                            zone->SetCornerSmoothingType( ZONE_SETTINGS::SMOOTHING_FILLET );
                        break;

                    default:
                        Expecting( "none, chamfer, or fillet" );
                    }
                    NeedRIGHT();
                    break;

                case T_radius:
                    tmp = parseBoardUnits( "corner radius" );
                    if( !zone->GetIsRuleArea() ) // smoothing has meaning only for filled zones
                       zone->SetCornerRadius( tmp );
                    NeedRIGHT();
                    break;


                case T_island_removal_mode:
                    tmp = parseInt( "island_removal_mode" );

                    if( tmp >= 0 && tmp <= 2 )
                        zone->SetIslandRemovalMode( static_cast<ISLAND_REMOVAL_MODE>( tmp ) );

                    NeedRIGHT();
                    break;

                case T_island_area_min:
                {
                    int area = parseBoardUnits( T_island_area_min );
                    zone->SetMinIslandArea( area * IU_PER_MM );
                    NeedRIGHT();
                    break;
                }

                default:
                    Expecting( "mode, arc_segments, thermal_gap, thermal_bridge_width, "
                               "hatch_thickness, hatch_gap, hatch_orientation, "
                               "hatch_smoothing_level, hatch_smoothing_value, "
                               "hatch_border_algorithm, hatch_min_hole_area, smoothing, radius, "
                               "island_removal_mode, or island_area_min" );
                }
            }
            break;

        case T_keepout:
            // "keepout" now means rule area, but the file token stays the same
            zone->SetIsRuleArea( true );

            // Initialize these two because their tokens won't appear in older files:
            zone->SetDoNotAllowPads( false );
            zone->SetDoNotAllowFootprints( false );

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

                case T_pads:
                    token = NextTok();

                    if( token != T_allowed && token != T_not_allowed )
                        Expecting( "allowed or not_allowed" );
                    zone->SetDoNotAllowPads( token == T_not_allowed );
                    break;

                case T_footprints:
                    token = NextTok();

                    if( token != T_allowed && token != T_not_allowed )
                        Expecting( "allowed or not_allowed" );
                    zone->SetDoNotAllowFootprints( token == T_not_allowed );
                    break;

                default:
                    Expecting( "tracks, vias or copperpour" );
                }

                NeedRIGHT();
            }

            break;

        case T_polygon:
            {
                SHAPE_LINE_CHAIN outline;

                NeedLEFT();
                token = NextTok();

                if( token != T_pts )
                    Expecting( T_pts );

                for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                    parseOutlinePoints( outline );

                NeedRIGHT();

                outline.SetClosed( true );

                // Remark: The first polygon is the main outline.
                // Others are holes inside the main outline.
                zone->AddPolygon( outline );
            }
            break;

        case T_filled_polygon:
            {
                // "(filled_polygon (pts"
                NeedLEFT();
                token = NextTok();

                if( token == T_layer )
                {
                    filledLayer = parseBoardItemLayer();
                    NeedRIGHT();
                    token = NextTok();

                    if( token != T_LEFT )
                        Expecting( T_LEFT );

                    token = NextTok();
                }
                else
                {
                    filledLayer = zone->GetLayer();
                }

                bool island = false;

                if( token == T_island )
                {
                    island = true;
                    NeedRIGHT();
                    NeedLEFT();
                    token = NextTok();
                }

                if( token != T_pts )
                    Expecting( T_pts );

                if( !pts.count( filledLayer ) )
                    pts[filledLayer] = SHAPE_POLY_SET();

                SHAPE_POLY_SET& poly = pts.at( filledLayer );

                int idx = poly.NewOutline();
                SHAPE_LINE_CHAIN& chain = poly.Outline( idx );

                if( island )
                    zone->SetIsIsland( filledLayer, idx );

                for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
                    parseOutlinePoints( chain );

                NeedRIGHT();

                addedFilledPolygons |= !poly.IsEmpty();
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

                    if( token == T_layer )
                    {
                        filledLayer = parseBoardItemLayer();
                        NeedRIGHT();
                        token = NextTok();

                        if( token != T_LEFT )
                            Expecting( T_LEFT );

                        token = NextTok();
                    }
                    else
                    {
                        filledLayer = zone->GetLayer();
                    }

                    if( token != T_pts )
                        Expecting( T_pts );

                    SEG segment( parseXY(), parseXY() );
                    NeedRIGHT();
                    segs.push_back( segment );
                }

                zone->SetFillSegments( filledLayer, segs );
            }
            break;

        case T_name:
            {
                NextTok();
                zone->SetZoneName( FromUTF8() );
                NeedRIGHT();
            }
            break;

        default:
            Expecting( "net, layer/layers, tstamp, hatch, priority, connect_pads, min_thickness, "
                       "fill, polygon, filled_polygon, fill_segments, or name" );
        }
    }

    if( zone->GetNumCorners() > 2 )
    {
        if( !zone->IsOnCopperLayer() )
        {
            //zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
            zone->SetNetCode( NETINFO_LIST::UNCONNECTED );
        }

        // Set hatch here, after outlines corners are read
        zone->SetBorderDisplayStyle( hatchStyle, hatchPitch, true );
    }

    if( addedFilledPolygons )
    {
        for( auto& pair : pts )
            zone->SetFilledPolysList( pair.first, pair.second );

        zone->CalculateFilledArea();
    }

    // Ensure keepout and non copper zones do not have a net
    // (which have no sense for these zones)
    // the netcode 0 is used for these zones
    bool zone_has_net = zone->IsOnCopperLayer() && !zone->GetIsRuleArea();

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
        {
            zone->SetNetCode( net->GetNetCode() );
        }
        else    // Not existing net: add a new net to keep trace of the zone netname
        {
            int newnetcode = m_board->GetNetCount();
            net = new NETINFO_ITEM( m_board, netnameFromfile, newnetcode );
            m_board->Add( net );

            // Store the new code mapping
            pushValueIntoMap( newnetcode, net->GetNetCode() );
            // and update the zone netcode
            zone->SetNetCode( net->GetNetCode() );
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

    std::unique_ptr<PCB_TARGET> target = std::make_unique<PCB_TARGET>( nullptr );

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
            NextTok();
            const_cast<KIID&>( target->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "x, plus, at, size, width, layer or tstamp" );
        }
    }

    return target.release();
}


KIID PCB_PARSER::CurStrToKIID()
{
    KIID aId;

    if( m_resetKIIDs )
    {
        aId = KIID();
        m_resetKIIDMap.insert( std::make_pair( CurStr(), aId ) );
    }
    else
    {
        aId = KIID( CurStr() );
    }

    return aId;
}

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
 * @file pcb_io_kicad_sexpr_parser.cpp
 * @brief Pcbnew s-expression file format parser implementation.
 */

#include "layer_ids.h"
#include <cerrno>
#include <charconv>
#include <confirm.h>
#include <macros.h>
#include <fmt/format.h>
#include <title_block.h>
#include <trigo.h>

#include <board.h>
#include <board_design_settings.h>
#include <embedded_files_parser.h>
#include <font/fontconfig.h>
#include <magic_enum.hpp>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <pcb_reference_image.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <pcb_target.h>
#include <pcb_track.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pad.h>
#include <generators_mgr.h>
#include <zone.h>
#include <footprint.h>
#include <geometry/shape_line_chain.h>
#include <font/font.h>
#include <core/ignore.h>
#include <netclass.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_plot_params_parser.h>
#include <pcb_plot_params.h>
#include <zones.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>
#include <convert_basic_shapes_to_polygon.h>    // for RECT_CHAMFER_POSITIONS definition
#include <math/util.h>                           // KiROUND, Clamp
#include <string_utils.h>
#include <stroke_params_parser.h>
#include <wx/log.h>
#include <progress_reporter.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <pgm_base.h>

// For some reason wxWidgets is built with wxUSE_BASE64 unset so expose the wxWidgets
// base64 code. Needed for PCB_REFERENCE_IMAGE
#define wxUSE_BASE64 1
#include <wx/base64.h>
#include <wx/log.h>
#include <wx/mstream.h>

// We currently represent board units as integers.  Any values that are
// larger or smaller than those board units represent undefined behavior for
// the system.  We limit values to the largest usable
// i.e. std::numeric_limits<int>::max().
// However to avoid issues in comparisons, use a slightly smaller value
// Note also the usable limits are much smaller to avoid overflows in intermediate
// calculations.
constexpr double INT_LIMIT = std::numeric_limits<int>::max() - 10;

using namespace PCB_KEYS_T;


void PCB_IO_KICAD_SEXPR_PARSER::init()
{
    m_showLegacySegmentZoneWarning = true;
    m_showLegacy5ZoneWarning = true;
    m_tooRecent = false;
    m_requiredVersion = 0;
    m_layerIndices.clear();
    m_layerMasks.clear();
    m_resetKIIDMap.clear();

    // Add untranslated default (i.e. English) layernames.
    // Some may be overridden later if parsing a board rather than a footprint.
    // The English name will survive if parsing only a footprint.
    for( int layer = 0;  layer < PCB_LAYER_ID_COUNT;  ++layer )
    {
        std::string untranslated = TO_UTF8( LSET::Name( PCB_LAYER_ID( layer ) ) );

        m_layerIndices[untranslated] = PCB_LAYER_ID( layer );
        m_layerMasks[untranslated] = LSET( { PCB_LAYER_ID( layer ) } );
    }

    m_layerMasks[ "*.Cu" ]      = LSET::AllCuMask();
    m_layerMasks[ "*In.Cu" ]    = LSET::InternalCuMask();
    m_layerMasks[ "F&B.Cu" ]    = LSET( { F_Cu, B_Cu } );
    m_layerMasks[ "*.Adhes" ]   = LSET( { B_Adhes, F_Adhes } );
    m_layerMasks[ "*.Paste" ]   = LSET( { B_Paste, F_Paste } );
    m_layerMasks[ "*.Mask" ]    = LSET( { B_Mask,  F_Mask } );
    m_layerMasks[ "*.SilkS" ]   = LSET( { B_SilkS, F_SilkS } );
    m_layerMasks[ "*.Fab" ]     = LSET( { B_Fab,   F_Fab } );
    m_layerMasks[ "*.CrtYd" ]   = LSET( { B_CrtYd, F_CrtYd } );

    // This is for the first pretty & *.kicad_pcb formats, which had
    // Inner1_Cu - Inner14_Cu with the numbering sequence
    // reversed from the subsequent format's In1_Cu - In30_Cu numbering scheme.
    // The newer format brought in an additional 16 Cu layers and flipped the cu stack but
    // kept the gap between one of the outside layers and the last cu internal.

    for( int i=1; i<=14; ++i )
    {
        std::string key = StrPrintf( "Inner%d.Cu", i );

        m_layerMasks[key] = LSET( { PCB_LAYER_ID( In15_Cu - 2 * i ) } );
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::checkpoint()
{
    if( m_progressReporter )
    {
        TIME_PT curTime = CLOCK::now();
        unsigned curLine = reader->LineNumber();
        auto delta = std::chrono::duration_cast<TIMEOUT>( curTime - m_lastProgressTime );

        if( delta > std::chrono::milliseconds( 250 ) )
        {
            m_progressReporter->SetCurrentProgress( ( (double) curLine )
                                                            / std::max( 1U, m_lineCount ) );

            if( !m_progressReporter->KeepRefreshing() )
                THROW_IO_ERROR( _( "Open canceled by user." ) );

            m_lastProgressTime = curTime;
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::skipCurrent()
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


void PCB_IO_KICAD_SEXPR_PARSER::pushValueIntoMap( int aIndex, int aValue )
{
    // Add aValue in netcode mapping (m_netCodes) at index aNetCode
    // ensure there is room in m_netCodes for that, and add room if needed.

    if( (int)m_netCodes.size() <= aIndex )
        m_netCodes.resize( static_cast<std::size_t>( aIndex ) + 1 );

    m_netCodes[aIndex] = aValue;
}


int PCB_IO_KICAD_SEXPR_PARSER::parseBoardUnits()
{
    // There should be no major rounding issues here, since the values in
    // the file are in mm and get converted to nano-meters.
    // See test program tools/test-nm-biu-to-ascii-mm-round-tripping.cpp
    // to confirm or experiment.  Use a similar strategy in both places, here
    // and in the test program. Make that program with:
    // $ make test-nm-biu-to-ascii-mm-round-tripping
    auto retval = parseDouble() * pcbIUScale.IU_PER_MM;

    // N.B. we currently represent board units as integers.  Any values that are
    // larger or smaller than those board units represent undefined behavior for
    // the system.  We limit values to the largest that is visible on the screen
    return KiROUND( std::clamp( retval, -INT_LIMIT, INT_LIMIT ) );
}


int PCB_IO_KICAD_SEXPR_PARSER::parseBoardUnits( const char* aExpected )
{
    auto retval = parseDouble( aExpected ) * pcbIUScale.IU_PER_MM;

    // N.B. we currently represent board units as integers.  Any values that are
    // larger or smaller than those board units represent undefined behavior for
    // the system.  We limit values to the largest that is visible on the screen
    return KiROUND( std::clamp( retval, -INT_LIMIT, INT_LIMIT ) );
}


bool PCB_IO_KICAD_SEXPR_PARSER::parseBool()
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


std::optional<bool> PCB_IO_KICAD_SEXPR_PARSER::parseOptBool()
{
    T token = NextTok();

    if( token == T_yes )
        return true;
    else if( token == T_no )
        return false;
    else if( token == T_none )
        return std::nullopt;
    else
        Expecting( "yes, no or none" );

    return false;
}


/*
 * e.g. "hide", "hide)", "(hide yes)"
 */
bool PCB_IO_KICAD_SEXPR_PARSER::parseMaybeAbsentBool( bool aDefaultValue )
{
    bool ret = aDefaultValue;

    if( PrevTok() == T_LEFT )
    {
        T token = NextTok();

        // "hide)"
        if( static_cast<int>( token ) == DSN_RIGHT )
            return aDefaultValue;

        if( token == T_yes || token == T_true )
            ret = true;
        else if( token == T_no || token == T_false )
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


wxString PCB_IO_KICAD_SEXPR_PARSER::GetRequiredVersion()
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


VECTOR2I PCB_IO_KICAD_SEXPR_PARSER::parseXY()
{
    if( CurTok() != T_LEFT )
        NeedLEFT();

    VECTOR2I pt;
    T token = NextTok();

    if( token != T_xy )
        Expecting( T_xy );

    pt.x = parseBoardUnits( "X coordinate" );
    pt.y = parseBoardUnits( "Y coordinate" );

    NeedRIGHT();

    return pt;
}


void PCB_IO_KICAD_SEXPR_PARSER::parseOutlinePoints( SHAPE_LINE_CHAIN& aPoly )
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

        if( token != T_RIGHT )
            Expecting( T_RIGHT );

        break;
    }
    default:
        Expecting( "xy or arc" );
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseXY( int* aX, int* aY )
{
    VECTOR2I pt = parseXY();

    if( aX )
        *aX = pt.x;

    if( aY )
        *aY = pt.y;
}


void PCB_IO_KICAD_SEXPR_PARSER::parseMargins( int& aLeft, int& aTop, int& aRight, int& aBottom )
{
    aLeft = parseBoardUnits( "left margin" );
    aTop = parseBoardUnits( "top margin" );
    aRight = parseBoardUnits( "right margin" );
    aBottom = parseBoardUnits( "bottom margin" );
}


std::pair<wxString, wxString> PCB_IO_KICAD_SEXPR_PARSER::parseBoardProperty()
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


void PCB_IO_KICAD_SEXPR_PARSER::parseTEARDROP_PARAMETERS( TEARDROP_PARAMETERS* tdParams )
{
    tdParams->m_Enabled = false;
    tdParams->m_AllowUseTwoTracks = false;
    tdParams->m_TdOnPadsInZones = true;

    for( T token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_enabled:
            tdParams->m_Enabled = parseMaybeAbsentBool( true );
            break;

        case T_allow_two_segments:
            tdParams->m_AllowUseTwoTracks = parseMaybeAbsentBool( true );
            break;

        case T_prefer_zone_connections:
            tdParams->m_TdOnPadsInZones = !parseMaybeAbsentBool( false );
            break;

        case T_best_length_ratio:
            tdParams->m_BestLengthRatio = parseDouble( "teardrop best length ratio" );
            NeedRIGHT();
            break;

        case T_max_length:
            tdParams->m_TdMaxLen = parseBoardUnits( "teardrop max length" );
            NeedRIGHT();
            break;

        case T_best_width_ratio:
            tdParams->m_BestWidthRatio = parseDouble( "teardrop best width ratio" );
            NeedRIGHT();
            break;

        case T_max_width:
            tdParams->m_TdMaxWidth = parseBoardUnits( "teardrop max width" );
            NeedRIGHT();
            break;

        // Legacy token
        case T_curve_points:
            tdParams->m_CurvedEdges = parseInt( "teardrop curve points count" ) > 0;
            NeedRIGHT();
            break;

        case T_curved_edges:
            tdParams->m_CurvedEdges = parseMaybeAbsentBool( true );
            break;

        case T_filter_ratio:
            tdParams->m_WidthtoSizeFilterRatio = parseDouble( "teardrop filter ratio" );
            NeedRIGHT();
            break;

        default:
            Expecting( "enabled, allow_two_segments, prefer_zone_connections, best_length_ratio, "
                       "max_length, best_width_ratio, max_width, curve_points or filter_ratio" );
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseEDA_TEXT( EDA_TEXT* aText )
{
    wxCHECK_RET( CurTok() == T_effects,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as EDA_TEXT." ) );

    // These are not written out if center/center and/or no mirror,
    // so we have to make sure we start that way.
    // (these parameters will be set in T_justify section, when existing)
    aText->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    aText->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    aText->SetMirrored( false );

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
                case T_face:
                    NeedSYMBOL();
                    aText->SetUnresolvedFontName( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_size:
                {
                    VECTOR2I sz;
                    sz.y = parseBoardUnits( "text height" );
                    sz.x = parseBoardUnits( "text width" );
                    aText->SetTextSize( sz );
                    NeedRIGHT();

                    foundTextSize = true;
                    break;
                }

                case T_line_spacing:
                    aText->SetLineSpacing( parseDouble( "line spacing" ) );
                    NeedRIGHT();
                    break;

                case T_thickness:
                    aText->SetTextThickness( parseBoardUnits( "text thickness" ) );
                    NeedRIGHT();
                    break;

                case T_bold:
                    aText->SetBoldFlag( parseMaybeAbsentBool( true ) );
                    break;

                case T_italic:
                    aText->SetItalicFlag( parseMaybeAbsentBool( true ) );
                    break;

                default:
                    Expecting( "face, size, line_spacing, thickness, bold, or italic" );
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
                case T_left:   aText->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );     break;
                case T_right:  aText->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );    break;
                case T_top:    aText->SetVertJustify( GR_TEXT_V_ALIGN_TOP );       break;
                case T_bottom: aText->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );    break;
                case T_mirror: aText->SetMirrored( true );                         break;
                default:       Expecting( "left, right, top, bottom, or mirror" );
                }

            }

            break;

        case T_hide:
        {
            // In older files, the hide token appears bare, and indicates hide==true.
            // In newer files, it will be an explicit bool in a list like (hide yes)
            bool hide = parseMaybeAbsentBool( true );
            aText->SetVisible( !hide );
            break;
        }

        default:
            Expecting( "font, justify, or hide" );
        }
    }

    // Text size was not specified in file, force legacy default units
    // 60mils is 1.524mm
    if( !foundTextSize )
    {
        const double defaultTextSize = 1.524 * pcbIUScale.IU_PER_MM;

        aText->SetTextSize( VECTOR2I( defaultTextSize, defaultTextSize ) );
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseRenderCache( EDA_TEXT* text )
{
    T token;

    NeedSYMBOLorNUMBER();
    wxString  cacheText = From_UTF8( CurText() );
    EDA_ANGLE cacheAngle( parseDouble( "render cache angle" ), DEGREES_T );

    text->SetupRenderCache( cacheText, text->GetFont(), cacheAngle, { 0, 0 } );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( token != T_polygon )
            Expecting( T_polygon );

        SHAPE_POLY_SET poly;

        for( token = NextTok(); token != T_RIGHT; token = NextTok() )
        {
            if( token != T_LEFT )
                Expecting( T_LEFT );

            token = NextTok();

            if( token != T_pts )
                Expecting( T_pts );

            SHAPE_LINE_CHAIN lineChain;

            while( (token = NextTok() ) != T_RIGHT )
                parseOutlinePoints( lineChain );

            lineChain.SetClosed( true );

            if( poly.OutlineCount() == 0 )
                poly.AddOutline( lineChain );
            else
                poly.AddHole( lineChain );
        }

        text->AddRenderCacheGlyph( poly );
    }
}


FP_3DMODEL* PCB_IO_KICAD_SEXPR_PARSER::parse3DModel()
{
    wxCHECK_MSG( CurTok() == T_model, nullptr,
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
        {
            // In older files, the hide token appears bare, and indicates hide==true.
            // In newer files, it will be an explicit bool in a list like (hide yes)
            bool hide = parseMaybeAbsentBool( true );
            n3D->m_Show = !hide;
            break;
        }

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


bool PCB_IO_KICAD_SEXPR_PARSER::IsValidBoardHeader()
{
    m_groupInfos.clear();

    // See Parse() - FOOTPRINTS can be prefixed with an initial block of single line comments,
    // eventually BOARD might be the same
    ReadCommentLines();

    if( CurTok() != T_LEFT  )
        return false;

    if( NextTok() != T_kicad_pcb)
        return false;

    return true;
}


BOARD_ITEM* PCB_IO_KICAD_SEXPR_PARSER::Parse()
{
    T               token;
    BOARD_ITEM*     item;

    m_groupInfos.clear();

    // FOOTPRINTS can be prefixed with an initial block of single line comments and these are
    // kept for Format() so they round trip in s-expression form.  BOARDs might  eventually do
    // the same, but currently do not.
    std::unique_ptr<wxArrayString> initial_comments( ReadCommentLines() );

    token = CurTok();

    if( token == -1 )   // EOF
        Unexpected( token );

    if( token != T_LEFT )
        Expecting( T_LEFT );

    switch( NextTok() )
    {
    case T_kicad_pcb:
        if( m_board == nullptr )
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

    const std::vector<wxString>* embeddedFonts = item->GetEmbeddedFiles()->UpdateFontFiles();

    item->RunOnChildren(
            [&]( BOARD_ITEM* aChild )
            {
                if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( aChild ) )
                    textItem->ResolveFont( embeddedFonts );
            },
            RECURSE_MODE::RECURSE );

    resolveGroups( item );

    return item;
}


BOARD* PCB_IO_KICAD_SEXPR_PARSER::parseBOARD()
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


BOARD* PCB_IO_KICAD_SEXPR_PARSER::parseBOARD_unchecked()
{
    T token;
    std::map<wxString, wxString> properties;

    parseHeader();

    auto checkVersion =
            [&]()
            {
                if( m_requiredVersion > SEXPR_BOARD_FILE_VERSION )
                {
                    throw FUTURE_FORMAT_ERROR( fmt::format( "{}", m_requiredVersion ),
                                               m_generatorVersion );
                }
            };

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

        case T_generator_version:
        {
            NeedSYMBOL();
            m_generatorVersion = FromUTF8();
            NeedRIGHT();

            // If the format includes a generator version, by this point we have enough info to
            // do the version check here
            checkVersion();

            break;
        }

        case T_general:
            // Do another version check here, for older files that do not include generator_version
            checkVersion();

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
            properties.insert( parseBoardProperty() );
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
            item = parsePCB_SHAPE( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_image:
            item = parsePCB_REFERENCE_IMAGE( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_gr_text:
            item = parsePCB_TEXT( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_gr_text_box:
            item = parsePCB_TEXTBOX( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_table:
            item = parsePCB_TABLE( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_dimension:
            item = parseDIMENSION( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_module:      // legacy token
        case T_footprint:
            item = parseFOOTPRINT();
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_segment:
            if( PCB_TRACK* track = parsePCB_TRACK() )
            {
                m_board->Add( track, ADD_MODE::BULK_APPEND, true );
                bulkAddedItems.push_back( track );
            }

            break;

        case T_arc:
            if( PCB_ARC* arc = parseARC() )
            {
                m_board->Add( arc, ADD_MODE::BULK_APPEND, true );
                bulkAddedItems.push_back( arc );
            }

            break;

        case T_group:
            parseGROUP( m_board );
            break;

        case T_generated:
            parseGENERATOR( m_board );
            break;

        case T_via:
            item = parsePCB_VIA();
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_zone:
            item = parseZONE( m_board );
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_target:
            item = parsePCB_TARGET();
            m_board->Add( item, ADD_MODE::BULK_APPEND, true );
            bulkAddedItems.push_back( item );
            break;

        case T_embedded_fonts:
        {
            m_board->GetEmbeddedFiles()->SetAreFontsEmbedded( parseBool() );
            NeedRIGHT();
            break;
        }

        case T_embedded_files:
        {
            EMBEDDED_FILES_PARSER embeddedFilesParser( reader );
            embeddedFilesParser.SyncLineReaderWith( *this );

            try
            {
                embeddedFilesParser.ParseEmbedded( m_board->GetEmbeddedFiles() );
            }
            catch( const PARSE_ERROR& e )
            {
                wxLogError( e.What() );
            }

            SyncLineReaderWith( embeddedFilesParser );
            break;
        }

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
        PCB_LAYER_ID destLayer = Cmts_User;
        wxString msg, undefinedLayerNames, destLayerName;

        for( const wxString& layerName : m_undefinedLayers )
        {
            if( !undefinedLayerNames.IsEmpty() )
                undefinedLayerNames += wxT( ", " );

            undefinedLayerNames += layerName;
        }

        destLayerName = m_board->GetLayerName( destLayer );

        if( Pgm().IsGUI() && m_queryUserCallback )
        {
            msg.Printf( _( "Items found on undefined layers (%s).\n"
                           "Do you wish to rescue them to the %s layer?\n"
                           "\n"
                           "Zones will need to be refilled." ),
                        undefinedLayerNames, destLayerName );

            if( !m_queryUserCallback( _( "Undefined Layers Warning" ), wxICON_WARNING, msg,
                                      _( "Rescue" ) ) )
            {
                THROW_IO_ERROR( wxT( "CANCEL" ) );
            }

            // Make sure the destination layer is enabled, even if not in the file
            m_board->SetEnabledLayers( LSET( m_board->GetEnabledLayers() ).set( destLayer ) );

            const auto visitItem = [&]( BOARD_ITEM& curr_item )
            {
                LSET layers = curr_item.GetLayerSet();

                if( layers.test( Rescue ) )
                {
                    layers.set( destLayer );
                    layers.reset( Rescue );
                }

                curr_item.SetLayerSet( layers );
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
                        if( top_layer == Rescue )
                            top_layer = F_Cu;

                        if( bottom_layer == Rescue )
                            bottom_layer = B_Cu;

                        via->SetLayerPair( top_layer, bottom_layer );
                    }
                }
                else
                {
                    visitItem( *track );
                }
            }

            for( BOARD_ITEM* zone : m_board->Zones() )
                visitItem( *zone );

            for( BOARD_ITEM* drawing : m_board->Drawings() )
                visitItem( *drawing );

            for( FOOTPRINT* fp : m_board->Footprints() )
            {
                for( BOARD_ITEM* drawing : fp->GraphicalItems() )
                    visitItem( *drawing );

                for( BOARD_ITEM* zone : fp->Zones() )
                    visitItem( *zone );

                for( PCB_FIELD* field : fp->GetFields() )
                    visitItem( *field );
            }

            m_undefinedLayers.clear();
        }
        else
        {
            THROW_IO_ERROR( wxT( "One or more undefined undefinedLayerNames was found; "
                                 "open the board in the PCB Editor to resolve." ) );
        }
    }

    // Clear unused zone data
    {
        LSET layers = m_board->GetEnabledLayers();

        for( BOARD_ITEM* zone : m_board->Zones() )
        {
            ZONE* z = static_cast<ZONE*>( zone );

            z->SetLayerSetAndRemoveUnusedFills( z->GetLayerSet() & layers );
        }
    }

    // Ensure all footprints have their embedded data from the board
    m_board->FixupEmbeddedData();

    return m_board;
}


void PCB_IO_KICAD_SEXPR_PARSER::resolveGroups( BOARD_ITEM* aParent )
{
    auto getItem =
            [&]( const KIID& aId )
            {
                BOARD_ITEM* aItem = nullptr;

                if( BOARD* board = dynamic_cast<BOARD*>( aParent ) )
                {
                    aItem = board->ResolveItem( aId, true );
                }
                else if( FOOTPRINT* footprint = dynamic_cast<FOOTPRINT*>( aParent ) )
                {
                    footprint->RunOnChildren(
                            [&]( BOARD_ITEM* child )
                            {
                                if( child->m_Uuid == aId )
                                    aItem = child;
                            },
                            RECURSE_MODE::NO_RECURSE );
                }

                return aItem;
            };

    // Now that we've parsed the other Uuids in the file we can resolve the uuids referred
    // to in the group declarations we saw.
    //
    // First add all group objects so subsequent GetItem() calls for nested groups work.

    std::vector<const GROUP_INFO*> groupTypeObjects;

    for( const GROUP_INFO& groupInfo : m_groupInfos )
        groupTypeObjects.emplace_back( &groupInfo );

    for( const GENERATOR_INFO& genInfo : m_generatorInfos )
        groupTypeObjects.emplace_back( &genInfo );

    for( const GROUP_INFO* groupInfo : groupTypeObjects )
    {
        PCB_GROUP* group = nullptr;

        if( const GENERATOR_INFO* genInfo = dynamic_cast<const GENERATOR_INFO*>( groupInfo ) )
        {
            GENERATORS_MGR& mgr = GENERATORS_MGR::Instance();

            PCB_GENERATOR* gen;
            group = gen = mgr.CreateFromType( genInfo->genType );

            if( !gen )
            {
                THROW_IO_ERROR( wxString::Format( _( "Cannot create generated object of type '%s'" ),
                                                  genInfo->genType ) );
            }

            gen->SetLayer( genInfo->layer );
            gen->SetProperties( genInfo->properties );
        }
        else
        {
            group = new PCB_GROUP( groupInfo->parent );
            group->SetName( groupInfo->name );
        }

        const_cast<KIID&>( group->m_Uuid ) = groupInfo->uuid;

        if( groupInfo->libId.IsValid() )
            group->SetDesignBlockLibId( groupInfo->libId );

        if( groupInfo->locked )
            group->SetLocked( true );

        if( groupInfo->parent->Type() == PCB_FOOTPRINT_T )
            static_cast<FOOTPRINT*>( groupInfo->parent )->Add( group, ADD_MODE::INSERT, true );
        else
            static_cast<BOARD*>( groupInfo->parent )->Add( group, ADD_MODE::INSERT, true );
    }

    for( const GROUP_INFO* groupInfo : groupTypeObjects )
    {
        if( PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( getItem( groupInfo->uuid ) ) )
        {
            for( const KIID& aUuid : groupInfo->memberUuids )
            {
                BOARD_ITEM* item = nullptr;

                if( m_appendToExisting )
                    item = getItem( m_resetKIIDMap[ aUuid.AsString() ] );
                else
                    item = getItem( aUuid );

                // We used to allow fp items in non-footprint groups.  It was a mistake.  Check
                // to make sure they the item and group are owned by the same parent (will both
                // be nullptr in the board case).
                if( item && item->GetParentFootprint() == group->GetParentFootprint() )
                    group->AddItem( item );
            }
        }
    }

    // Don't allow group cycles
    if( m_board )
        m_board->GroupsSanityCheck( true );
}


void PCB_IO_KICAD_SEXPR_PARSER::parseHeader()
{
    wxCHECK_RET( CurTok() == T_kicad_pcb,
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
        m_requiredVersion = 20201115;   // Last version before we started writing version #s
                                        // in footprint files as well as board files.
    }

    m_tooRecent = ( m_requiredVersion > SEXPR_BOARD_FILE_VERSION );

    // Prior to this, bar was a valid string char for unquoted strings.
    SetKnowsBar( m_requiredVersion >= 20240706 );

    m_board->SetFileFormatVersionAtLoad( m_requiredVersion );
}


void PCB_IO_KICAD_SEXPR_PARSER::parseGeneralSection()
{
    wxCHECK_RET( CurTok() == T_general,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a general section." ) );

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

        case T_legacy_teardrops:
            m_board->SetLegacyTeardrops( parseMaybeAbsentBool( true ) );
            break;

        default:              // Skip everything else.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( !IsSymbol( token ) && token != T_NUMBER )
                    Expecting( "symbol or number" );
            }
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parsePAGE_INFO()
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
        if( width < MIN_PAGE_SIZE_MM )
            width = MIN_PAGE_SIZE_MM;
        else if( width > MAX_PAGE_SIZE_PCBNEW_MM )
            width = MAX_PAGE_SIZE_PCBNEW_MM;

        double height = parseDouble( "height" );    // height in mm

        if( height < MIN_PAGE_SIZE_MM )
            height = MIN_PAGE_SIZE_MM;
        else if( height > MAX_PAGE_SIZE_PCBNEW_MM )
            height = MAX_PAGE_SIZE_PCBNEW_MM;

        pageInfo.SetWidthMM( width );
        pageInfo.SetHeightMM( height );
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


void PCB_IO_KICAD_SEXPR_PARSER::parseTITLE_BLOCK()
{
    wxCHECK_RET( CurTok() == T_title_block,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as TITLE_BLOCK." ) );

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

            break;
        }

        default:
            Expecting( "title, date, rev, company, or comment" );
        }

        NeedRIGHT();
    }

    m_board->SetTitleBlock( titleBlock );
}


void PCB_IO_KICAD_SEXPR_PARSER::parseLayer( LAYER* aLayer )
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
    int layer_num = parseInt( "layer index" );

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

    aLayer->m_type    = LAYER::ParseType( type.c_str() );
    aLayer->m_number  = layer_num;
    aLayer->m_visible = isVisible;

    if( m_requiredVersion >= 20200922 )
    {
        aLayer->m_userName = From_UTF8( userName.c_str() );
        aLayer->m_name = From_UTF8( name.c_str() );
    }
    else // Older versions didn't have a dedicated user name field
    {
        aLayer->m_name = aLayer->m_userName = From_UTF8( name.c_str() );
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseBoardStackup()
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
        else if( !( layerId & 1 ) )
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
        {
            Expecting( "layer_name" );
        }

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
                        name = FromUTF8();

                        // Older versions didn't store opacity with custom colors
                        if( name.StartsWith( wxT( "#" ) ) && m_requiredVersion < 20210824 )
                        {
                            KIGFX::COLOR4D color( name );

                            if( item->GetType() == BS_ITEM_TYPE_SOLDERMASK )
                                color = color.WithAlpha( DEFAULT_SOLDERMASK_OPACITY );
                            else
                                color = color.WithAlpha( 1.0 );

                            wxColour wx_color = color.ToColour();

                            // Open-code wxColour::GetAsString() because 3.0 doesn't handle rgba
                            name.Printf( wxT("#%02X%02X%02X%02X" ),
                                         wx_color.Red(),
                                         wx_color.Green(),
                                         wx_color.Blue(),
                                         wx_color.Alpha() );
                        }

                        item->SetColor( name, sublayer_idx );
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


void PCB_IO_KICAD_SEXPR_PARSER::createOldLayerMapping( std::unordered_map< std::string, std::string >& aMap )
{
    // N.B. This mapping only includes Italian, Polish and French as they were the only languages
    // that mapped the layer names as of cc2022b1ac739aa673d2a0b7a2047638aa7a47b3 (kicad-i18n)
    // when the bug was fixed in KiCad source.

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


void PCB_IO_KICAD_SEXPR_PARSER::parseLayers()
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
        for( size_t i = 1; i < cu.size() - 1; i++ )
        {
            int tmpLayer = LSET::NameToLayer( cu[i].m_name );

            if( tmpLayer < 0 )
                tmpLayer = ( i + 1 ) * 2;

            cu[i].m_number = tmpLayer;
        }

        cu[0].m_number = F_Cu;
        cu[cu.size()-1].m_number = B_Cu;

        for( auto& cu_layer : cu )
        {
            enabledLayers.set( cu_layer.m_number );

            if( cu_layer.m_visible )
                visibleLayers.set( cu_layer.m_number );
            else
                anyHidden = true;

            m_board->SetLayerDescr( PCB_LAYER_ID( cu_layer.m_number ), cu_layer );

            UTF8 name = cu_layer.m_name;

            m_layerIndices[ name ] = PCB_LAYER_ID( cu_layer.m_number );
            m_layerMasks[ name ] = LSET( { PCB_LAYER_ID( cu_layer.m_number ) } );
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

            // If we are here, then we have found a translated layer name.  Put it in the maps
            // so that items on this layer get the appropriate layer ID number.
            m_layerIndices[ UTF8( layer.m_name ) ] = it->second;
            m_layerMasks[   UTF8( layer.m_name ) ] = LSET( { it->second } );
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
        wxString err = wxString::Format( _( "%d is not a valid layer count" ), copperLayerCount );

        THROW_PARSE_ERROR( err, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
    }

    m_board->SetCopperLayerCount( copperLayerCount );
    m_board->SetEnabledLayers( enabledLayers );

    // Only set this if any layers were explicitly marked as hidden.  Otherwise, we want to leave
    // this alone; default visibility will show everything
    if( anyHidden )
        m_board->m_LegacyVisibleLayers = visibleLayers;
}


LSET PCB_IO_KICAD_SEXPR_PARSER::lookUpLayerSet( const LSET_MAP& aMap )
{
    LSET_MAP::const_iterator it = aMap.find( curText );

    if( it == aMap.end() )
        return LSET( { Rescue } );

    return it->second;
}


PCB_LAYER_ID PCB_IO_KICAD_SEXPR_PARSER::lookUpLayer( const LAYER_ID_MAP& aMap )
{
    // avoid constructing another std::string, use lexer's directly
    LAYER_ID_MAP::const_iterator it = aMap.find( curText );

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


PCB_LAYER_ID PCB_IO_KICAD_SEXPR_PARSER::parseBoardItemLayer()
{
    wxCHECK_MSG( CurTok() == T_layer, UNDEFINED_LAYER,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as layer." ) );

    NextTok();

    PCB_LAYER_ID layerIndex = lookUpLayer( m_layerIndices );

    // Handle closing ) in object parser.

    return layerIndex;
}


LSET PCB_IO_KICAD_SEXPR_PARSER::parseBoardItemLayersAsMask()
{
    wxCHECK_MSG( CurTok() == T_layers, LSET(),
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as item layers." ) );

    LSET layerMask;

    for( T token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        layerMask |= lookUpLayerSet( m_layerMasks );
    }

    return layerMask;
}


LSET PCB_IO_KICAD_SEXPR_PARSER::parseLayersForCuItemWithSoldermask()
{
    LSET layerMask = parseBoardItemLayersAsMask();

    if( ( layerMask & LSET::AllCuMask() ).count() != 1 )
          Expecting( "single copper layer" );

    if( ( layerMask & LSET( { F_Mask, B_Mask } ) ).count() > 1 )
          Expecting( "max one soldermask layer" );

    if( ( ( layerMask & LSET::InternalCuMask() ).any()
              && ( layerMask & LSET( { F_Mask, B_Mask } ) ).any() ) )
    {
        Expecting( "no mask layer when track is on internal layer" );
    }

    if( ( layerMask & LSET( { F_Cu, B_Mask } ) ).count() > 1 )
          Expecting( "copper and mask on the same side" );

    if( ( layerMask & LSET( { B_Cu, F_Mask } ) ).count() > 1 )
          Expecting( "copper and mask on the same side" );

    return layerMask;
}


void PCB_IO_KICAD_SEXPR_PARSER::parseSetup()
{
    wxCHECK_RET( CurTok() == T_setup,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as setup." ) );

    BOARD_DESIGN_SETTINGS&     bds = m_board->GetDesignSettings();
    const std::shared_ptr<NETCLASS>& defaultNetClass = bds.m_NetSettings->GetDefaultNetclass();
    ZONE_SETTINGS&             zoneSettings = bds.GetDefaultZoneSettings();

    // Missing soldermask min width value means that the user has set the value to 0 and
    // not the default value (0.25mm)
    bds.m_SolderMaskMinWidth = 0;

    for( T token = NextTok();  token != T_RIGHT;  token = NextTok() )
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
            if( bds.m_TrackWidthList.empty() )
                bds.m_TrackWidthList.emplace_back( 0 );

            int trackWidth = parseBoardUnits( T_user_trace_width );

            if( !m_appendToExisting || !alg::contains( bds.m_TrackWidthList, trackWidth ) )
                bds.m_TrackWidthList.push_back( trackWidth );

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

        case T_zone_45_only:    // legacy setting
            /* zoneSettings.m_Zone_45_Only = */ parseBool();
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_clearance_min:
            bds.m_MinClearance = parseBoardUnits( T_clearance_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_trace_min:
            bds.m_TrackMinWidth = parseBoardUnits( T_trace_min );
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
            bds.m_ViasMinAnnularWidth = parseBoardUnits( T_via_min_annulus );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_via_min_size:
            bds.m_ViasMinSize = parseBoardUnits( T_via_min_size );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_through_hole_min:
            bds.m_MinThroughDrill = parseBoardUnits( T_through_hole_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        // Legacy token for T_through_hole_min
        case T_via_min_drill:
            bds.m_MinThroughDrill = parseBoardUnits( T_via_min_drill );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_hole_to_hole_min:
            bds.m_HoleToHoleMin = parseBoardUnits( T_hole_to_hole_min );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_user_via:
        {
            int           viaSize = parseBoardUnits( "user via size" );
            int           viaDrill = parseBoardUnits( "user via drill" );
            VIA_DIMENSION via( viaSize, viaDrill );

            // Make room for the netclass value
            if( bds.m_ViasDimensionsList.empty() )
                bds.m_ViasDimensionsList.emplace_back( VIA_DIMENSION( 0, 0 ) );

            if( !m_appendToExisting || !alg::contains( bds.m_ViasDimensionsList, via ) )
                bds.m_ViasDimensionsList.emplace_back( via );

            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

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
            parseBool();
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_blind_buried_vias_allowed:
            parseBool();
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_uvia_min_size:
            bds.m_MicroViasMinSize = parseBoardUnits( T_uvia_min_size );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_uvia_min_drill:
            bds.m_MicroViasMinDrill = parseBoardUnits( T_uvia_min_drill );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_user_diff_pair:
        {
            int                 width = parseBoardUnits( "user diff-pair width" );
            int                 gap = parseBoardUnits( "user diff-pair gap" );
            int                 viaGap = parseBoardUnits( "user diff-pair via gap" );
            DIFF_PAIR_DIMENSION diffPair( width, gap, viaGap );

            if( !m_appendToExisting || !alg::contains( bds.m_DiffPairDimensionsList, diffPair ) )
                bds.m_DiffPairDimensionsList.emplace_back( diffPair );

            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

        case T_segment_width:   // note: legacy (pre-6.0) token
            bds.m_LineThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( T_segment_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_edge_width:      // note: legacy (pre-6.0) token
            bds.m_LineThickness[ LAYER_CLASS_EDGES ] = parseBoardUnits( T_edge_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_mod_edge_width:  // note: legacy (pre-6.0) token
            bds.m_LineThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( T_mod_edge_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_pcb_text_width:  // note: legacy (pre-6.0) token
            bds.m_TextThickness[ LAYER_CLASS_COPPER ] = parseBoardUnits( T_pcb_text_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_mod_text_width:  // note: legacy (pre-6.0) token
            bds.m_TextThickness[ LAYER_CLASS_SILK ] = parseBoardUnits( T_mod_text_width );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_pcb_text_size:   // note: legacy (pre-6.0) token
            bds.m_TextSize[ LAYER_CLASS_COPPER ].x = parseBoardUnits( "pcb text width" );
            bds.m_TextSize[ LAYER_CLASS_COPPER ].y = parseBoardUnits( "pcb text height" );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_mod_text_size:   // note: legacy (pre-6.0) token
            bds.m_TextSize[ LAYER_CLASS_SILK ].x = parseBoardUnits( "footprint text width" );
            bds.m_TextSize[ LAYER_CLASS_SILK ].y = parseBoardUnits( "footprint text height" );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_defaults:
            parseDefaults( bds );
            m_board->m_LegacyDesignSettingsLoaded = true;
            break;

        case T_pad_size:
        {
            VECTOR2I sz;
            sz.x = parseBoardUnits( "master pad width" );
            sz.y = parseBoardUnits( "master pad height" );
            bds.m_Pad_Master->SetSize( PADSTACK::ALL_LAYERS, sz );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

        case T_pad_drill:
        {
            int drillSize = parseBoardUnits( T_pad_drill );
            bds.m_Pad_Master->SetDrillSize( VECTOR2I( drillSize, drillSize ) );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

        case T_pad_to_mask_clearance:
            bds.m_SolderMaskExpansion = parseBoardUnits( T_pad_to_mask_clearance );
            NeedRIGHT();
            break;

        case T_solder_mask_min_width:
            bds.m_SolderMaskMinWidth = parseBoardUnits( T_solder_mask_min_width );
            NeedRIGHT();
            break;

        case T_pad_to_paste_clearance:
            bds.m_SolderPasteMargin = parseBoardUnits( T_pad_to_paste_clearance );
            NeedRIGHT();
            break;

        case T_pad_to_paste_clearance_ratio:
            bds.m_SolderPasteMarginRatio = parseDouble( T_pad_to_paste_clearance_ratio );
            NeedRIGHT();
            break;

        case T_allow_soldermask_bridges_in_footprints:
            bds.m_AllowSoldermaskBridgesInFPs = parseBool();
            NeedRIGHT();
            break;

        case T_tenting:
        {
            auto [front, back] = parseFrontBackOptBool( true );
            bds.m_TentViasFront = front.value_or( false );
            bds.m_TentViasBack = back.value_or( false );
            break;
        }

        case T_covering:
        {
            auto [front, back] = parseFrontBackOptBool();
            bds.m_CoverViasFront = front.value_or( false );
            bds.m_CoverViasBack = back.value_or( false );
            break;
        }

        case T_plugging:
        {
            auto [front, back] = parseFrontBackOptBool();
            bds.m_PlugViasFront = front.value_or( false );
            bds.m_PlugViasBack = back.value_or( false );
            break;
        }

        case T_capping:
        {
            bds.m_CapVias = parseBool();
            NeedRIGHT();
            break;
        }

        case T_filling:
        {
            bds.m_FillVias = parseBool();
            NeedRIGHT();
            break;
        }

        case T_aux_axis_origin:
        {
            int x = parseBoardUnits( "auxiliary origin X" );
            int y = parseBoardUnits( "auxiliary origin Y" );
            bds.SetAuxOrigin( VECTOR2I( x, y ) );

            // Aux origin still stored in board for the moment
            //m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

        case T_grid_origin:
        {
            int x = parseBoardUnits( "grid origin X" );
            int y = parseBoardUnits( "grid origin Y" );
            bds.SetGridOrigin( VECTOR2I( x, y ) );
            // Grid origin still stored in board for the moment
            //m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;
        }

        // Stored in board prior to 6.0
        case T_visible_elements:
        {
            // Make sure to start with DefaultVisible so all new layers are set
            m_board->m_LegacyVisibleItems = GAL_SET::DefaultVisible();

            int visible = parseHex() | MIN_VISIBILITY_MASK;

            for( size_t i = 0; i < sizeof( int ) * CHAR_BIT; i++ )
                m_board->m_LegacyVisibleItems.set( i, visible & ( 1u << i ) );

            NeedRIGHT();
            break;
        }

        case T_max_error:
            bds.m_MaxError = parseBoardUnits( T_max_error );
            m_board->m_LegacyDesignSettingsLoaded = true;
            NeedRIGHT();
            break;

        case T_filled_areas_thickness:
            // Ignore this value, it is not used anymore
            parseBool();
            NeedRIGHT();
            break;

        case T_pcbplotparams:
        {
            PCB_PLOT_PARAMS        plotParams;
            PCB_PLOT_PARAMS_PARSER parser( reader, m_requiredVersion );
            // parser must share the same current line as our current PCB parser
            // synchronize it.
            parser.SyncLineReaderWith( *this );

            plotParams.Parse( &parser );
            SyncLineReaderWith( parser );

            m_board->SetPlotOptions( plotParams );

            if( plotParams.GetLegacyPlotViaOnMaskLayer().has_value() )
            {
                bool tent = !( *plotParams.GetLegacyPlotViaOnMaskLayer() );
                m_board->GetDesignSettings().m_TentViasFront = tent;
                m_board->GetDesignSettings().m_TentViasBack = tent;
            }

            break;
        }
        case T_zone_defaults:
            parseZoneDefaults( bds.GetDefaultZoneSettings() );
            break;

        default:
            Unexpected( CurText() );
        }
    }

    // Set up a default stackup in case the file doesn't define one, and now we know
    // the enabled layers
    if( ! m_board->GetDesignSettings().m_HasStackup )
    {
        BOARD_STACKUP& stackup = bds.GetStackupDescriptor();
        stackup.RemoveAll();
        stackup.BuildDefaultStackupList( &bds, m_board->GetCopperLayerCount() );
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseZoneDefaults( ZONE_SETTINGS& aZoneSettings )
{
    T token;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
        {
            Expecting( T_LEFT );
        }

        token = NextTok();

        switch( token )
        {
        case T_property:
            parseZoneLayerProperty( aZoneSettings.m_LayerProperties );
            break;
        default:
            Unexpected( CurText() );
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseZoneLayerProperty(
        std::map<PCB_LAYER_ID, ZONE_LAYER_PROPERTIES>& aProperties )
{
    T token;

    PCB_LAYER_ID          layer = UNDEFINED_LAYER;
    ZONE_LAYER_PROPERTIES properties;

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
        {
            Expecting( T_LEFT );
        }

        token = NextTok();

        switch( token )
        {
        case T_layer:
            layer = parseBoardItemLayer();
            NeedRIGHT();
            break;
        case T_hatch_position:
        {
            properties.hatching_offset = parseXY();
            NeedRIGHT();
            break;
        }
        default:
            Unexpected( CurText() );
            break;
        }
    }

    aProperties.emplace( layer, properties );
}


void PCB_IO_KICAD_SEXPR_PARSER::parseDefaults( BOARD_DESIGN_SETTINGS& designSettings )
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
            designSettings.m_DimensionPrecision =
                    static_cast<DIM_PRECISION>( parseInt( "dimension precision" ) );
            NeedRIGHT();
            break;

        default:
            Unexpected( CurText() );
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseDefaultTextDims( BOARD_DESIGN_SETTINGS& aSettings, int aLayer )
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


void PCB_IO_KICAD_SEXPR_PARSER::parseNETINFO_ITEM()
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
        m_board->Add( net, ADD_MODE::INSERT, true );

        // Store the new code mapping
        pushValueIntoMap( netCode, net->GetNetCode() );
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseNETCLASS()
{
    wxCHECK_RET( CurTok() == T_net_class,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as net class." ) );

    T token;

    std::shared_ptr<NETCLASS> nc = std::make_shared<NETCLASS>( wxEmptyString );

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
        {
            NeedSYMBOLorNUMBER();

            wxString netName = FromUTF8();

            // Convert overbar syntax from `~...~` to `~{...}`.  These were left out of the
            // first merge so the version is a bit later.
            if( m_requiredVersion < 20210606 )
                netName = ConvertToNewOverbarNotation( FromUTF8() );

            m_board->GetDesignSettings().m_NetSettings->SetNetclassPatternAssignment(
                    netName, nc->GetName() );

            break;
        }

        default:
            Expecting( "clearance, trace_width, via_dia, via_drill, uvia_dia, uvia_drill, "
                       "diff_pair_width, diff_pair_gap or add_net" );
        }

        NeedRIGHT();
    }

    std::shared_ptr<NET_SETTINGS>& netSettings = m_board->GetDesignSettings().m_NetSettings;

    if( netSettings->HasNetclass( nc->GetName() ) )
    {
        // Must have been a name conflict, this is a bad board file.
        // User may have done a hand edit to the file.
        wxString error;
        error.Printf( _( "Duplicate NETCLASS name '%s' in file '%s' at line %d, offset %d." ),
                      nc->GetName().GetData(), CurSource().GetData(), CurLineNumber(),
                      CurOffset() );
        THROW_IO_ERROR( error );
    }
    else if( nc->GetName() == netSettings->GetDefaultNetclass()->GetName() )
    {
        netSettings->SetDefaultNetclass( nc );
    }
    else
    {
        netSettings->SetNetclass( nc->GetName(), nc );
    }
}


PCB_SHAPE* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_SHAPE( BOARD_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_fp_arc || CurTok() == T_fp_circle || CurTok() == T_fp_curve ||
                 CurTok() == T_fp_rect || CurTok() == T_fp_line || CurTok() == T_fp_poly ||
                 CurTok() == T_gr_arc || CurTok() == T_gr_circle || CurTok() == T_gr_curve ||
                 CurTok() == T_gr_rect || CurTok() == T_gr_bbox || CurTok() == T_gr_line ||
                 CurTok() == T_gr_poly || CurTok() == T_gr_vector, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_SHAPE." ) );

    T                          token;
    VECTOR2I                   pt;
    STROKE_PARAMS              stroke( 0, LINE_STYLE::SOLID );
    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( aParent );

    switch( CurTok() )
    {
    case T_gr_arc:
    case T_fp_arc:
        shape->SetShape( SHAPE_T::ARC );
        token = NextTok();

        if( token == T_locked )
        {
            shape->SetLocked( true );
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        if( m_requiredVersion <= LEGACY_ARC_FORMATTING )
        {
            // In legacy files the start keyword actually gives the arc center...
            if( token != T_start )
                Expecting( T_start );

            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            shape->SetCenter( pt );
            NeedRIGHT();
            NeedLEFT();
            token = NextTok();

            // ... and the end keyword gives the start point of the arc
            if( token != T_end )
                Expecting( T_end );

            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            shape->SetStart( pt );
            NeedRIGHT();
            NeedLEFT();
            token = NextTok();

            if( token != T_angle )
                Expecting( T_angle );

            shape->SetArcAngleAndEnd( EDA_ANGLE( parseDouble( "arc angle" ), DEGREES_T ), true );
            NeedRIGHT();
        }
        else
        {
            VECTOR2I arc_start, arc_mid, arc_end;

            if( token != T_start )
                Expecting( T_start );

            arc_start.x = parseBoardUnits( "X coordinate" );
            arc_start.y = parseBoardUnits( "Y coordinate" );
            NeedRIGHT();
            NeedLEFT();
            token = NextTok();

            if( token != T_mid )
                Expecting( T_mid );

            arc_mid.x = parseBoardUnits( "X coordinate" );
            arc_mid.y = parseBoardUnits( "Y coordinate" );
            NeedRIGHT();
            NeedLEFT();
            token = NextTok();

            if( token != T_end )
                Expecting( T_end );

            arc_end.x = parseBoardUnits( "X coordinate" );
            arc_end.y = parseBoardUnits( "Y coordinate" );
            NeedRIGHT();

            shape->SetArcGeometry( arc_start, arc_mid, arc_end );
        }

        break;

    case T_gr_circle:
    case T_fp_circle:
        shape->SetShape( SHAPE_T::CIRCLE );
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

    case T_gr_curve:
    case T_fp_curve:
        shape->SetShape( SHAPE_T::BEZIER );
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
        shape->SetBezierC1( parseXY());
        shape->SetBezierC2( parseXY());
        shape->SetEnd( parseXY() );

        if( m_board )
            shape->RebuildBezierToSegmentsPointsList( m_board->GetDesignSettings().m_MaxError );
        else
            shape->RebuildBezierToSegmentsPointsList( ARC_HIGH_DEF );

        NeedRIGHT();
        break;

    case T_gr_bbox:
    case T_gr_rect:
    case T_fp_rect:
        shape->SetShape( SHAPE_T::RECTANGLE );
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

        if( aParent && aParent->Type() == PCB_FOOTPRINT_T )
        {
            // Footprint shapes are stored in board-relative coordinates, but we want the
            // normalization to remain in footprint-relative coordinates.
        }
        else
        {
            shape->Normalize();
        }

        NeedRIGHT();
        break;

    case T_gr_vector:
    case T_gr_line:
    case T_fp_line:
        shape->SetShape( SHAPE_T::SEGMENT );
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
    case T_fp_poly:
    {
        shape->SetShape( SHAPE_T::POLY );
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

        break;
    }

    default:
        if( aParent && aParent->Type() == PCB_FOOTPRINT_T )
        {
            Expecting( "fp_arc, fp_circle, fp_curve, fp_line, fp_poly or fp_rect" );
        }
        else
        {
            Expecting( "gr_arc, gr_circle, gr_curve, gr_vector, gr_line, gr_poly, gr_rect or "
                       "gr_bbox" );
        }
    }

    bool foundFill = false;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_angle:       // legacy token; ignore value
            parseDouble( "arc angle" );
            NeedRIGHT();
            break;

        case T_layer:
            shape->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_layers:
            shape->SetLayerSet( parseBoardItemLayersAsMask() );
            break;

        case T_solder_mask_margin:
            shape->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();
            break;

        case T_width:       // legacy token
            stroke.SetWidth( parseBoardUnits( T_width ) );
            NeedRIGHT();
            break;

        case T_stroke:
        {
            STROKE_PARAMS_PARSER strokeParser( reader, pcbIUScale.IU_PER_MM );
            strokeParser.SyncLineReaderWith( *this );

            strokeParser.ParseStroke( stroke );
            SyncLineReaderWith( strokeParser );
            break;
        }

        case T_tstamp:
        case T_uuid:
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
                // T_yes was used to indicate filling when first introduced, so treat it like a
                // solid fill since that was the only fill available at the time.
                case T_yes:
                case T_solid:         shape->SetFillMode( FILL_T::FILLED_SHAPE );  break;

                case T_none:
                case T_no:            shape->SetFillMode( FILL_T::NO_FILL );       break;

                case T_hatch:         shape->SetFillMode( FILL_T::HATCH );         break;
                case T_reverse_hatch: shape->SetFillMode( FILL_T::REVERSE_HATCH ); break;
                case T_cross_hatch:   shape->SetFillMode( FILL_T::CROSS_HATCH );   break;

                default: Expecting( "yes, no, solid, none, hatch, reverse_hatch or cross_hatch" );
                }
            }

            break;

        case T_status:      // legacy token; ignore value
            parseHex();
            NeedRIGHT();
            break;

        // Handle "(locked)" from 5.99 development, and "(locked yes)" from modern times
        case T_locked:
            shape->SetLocked( parseMaybeAbsentBool( true ) );
            break;

        case T_net:
            if( !shape->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
            {
                wxLogError( _( "Invalid net ID in\nfile: '%s'\nline: %d\noffset: %d." ),
                            CurSource(), CurLineNumber(), CurOffset() );
            }

            NeedRIGHT();
            break;

        default:
            Expecting( "layer, width, fill, tstamp, uuid, locked, net, status, "
                       "or solder_mask_margin" );
        }
    }

    if( !foundFill )
    {
        // Legacy versions didn't have a filled flag but allowed some shapes to indicate they
        // should be filled by specifying a 0 stroke-width.
        if( stroke.GetWidth() == 0
            && ( shape->GetShape() == SHAPE_T::RECTANGLE || shape->GetShape() == SHAPE_T::CIRCLE ) )
        {
            shape->SetFilled( true );
        }
        else if( shape->GetShape() == SHAPE_T::POLY && shape->GetLayer() != Edge_Cuts )
        {
            // Polygons on non-Edge_Cuts layers were always filled.
            shape->SetFilled( true );
        }
    }

    // Only filled shapes may have a zero line-width.  This is not permitted in KiCad but some
    // external tools can generate invalid files.
    if( stroke.GetWidth() <= 0 && !shape->IsAnyFill() )
    {
        stroke.SetWidth( pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ) );
    }

    shape->SetStroke( stroke );

    if( FOOTPRINT* parentFP = shape->GetParentFootprint() )
    {
        shape->Rotate( { 0, 0 }, parentFP->GetOrientation() );
        shape->Move( parentFP->GetPosition() );
    }

    return shape.release();
}


PCB_REFERENCE_IMAGE* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_REFERENCE_IMAGE( BOARD_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_image, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a reference image." ) );

    T token;
    std::unique_ptr<PCB_REFERENCE_IMAGE> bitmap = std::make_unique<PCB_REFERENCE_IMAGE>( aParent );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_at:
        {
            VECTOR2I pos;
            pos.x = parseBoardUnits( "X coordinate" );
            pos.y = parseBoardUnits( "Y coordinate" );
            bitmap->SetPosition( pos );
            NeedRIGHT();
            break;
        }

        case T_layer:
            bitmap->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_scale:
        {
            REFERENCE_IMAGE& refImage = bitmap->GetReferenceImage();
            refImage.SetImageScale( parseDouble( "image scale factor" ) );

            if( !std::isnormal( refImage.GetImageScale() ) )
                refImage.SetImageScale( 1.0 );

            NeedRIGHT();
            break;
        }
        case T_data:
        {
            token = NextTok();

            wxString data;

            // Reserve 512K because most image files are going to be larger than the default
            // 1K that wxString reserves.
            data.reserve( 1 << 19 );

            while( token != T_RIGHT )
            {
                if( !IsSymbol( token ) )
                    Expecting( "base64 image data" );

                data += FromUTF8();
                token = NextTok();
            }

            wxMemoryBuffer buffer = wxBase64Decode( data );

            REFERENCE_IMAGE& refImage = bitmap->GetReferenceImage();
            if( !refImage.ReadImageFile( buffer ) )
                THROW_IO_ERROR( _( "Failed to read image data." ) );

            break;
        }

        case T_locked:
        {
            // This has only ever been (locked yes) format
            const bool locked = parseBool();
            bitmap->SetLocked( locked );

            NeedRIGHT();
            break;
        }

        case T_uuid:
        {
            NextTok();
            const_cast<KIID&>( bitmap->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;
        }

        default:
            Expecting( "at, layer, scale, data, locked or uuid" );
        }
    }

    return bitmap.release();
}


PCB_TEXT* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TEXT( BOARD_ITEM* aParent, PCB_TEXT* aBaseText )
{
    wxCHECK_MSG( CurTok() == T_gr_text || CurTok() == T_fp_text, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TEXT." ) );

    FOOTPRINT*                parentFP = dynamic_cast<FOOTPRINT*>( aParent );
    std::unique_ptr<PCB_TEXT> text;

    T token = NextTok();

    // If a base text is provided, we have a derived text already parsed and just need to update it
    if( aBaseText )
    {
        text = std::unique_ptr<PCB_TEXT>( aBaseText );
    }
    else if( parentFP )
    {
        switch( token )
        {
        case T_reference:
            text = std::make_unique<PCB_FIELD>( parentFP, FIELD_T::REFERENCE );
            break;

        case T_value:
            text = std::make_unique<PCB_FIELD>( parentFP, FIELD_T::VALUE );
            break;

        case T_user:
            text = std::make_unique<PCB_TEXT>( parentFP );
            break;

        default:
            THROW_IO_ERROR( wxString::Format( _( "Cannot handle footprint text type %s" ),
                                              FromUTF8() ) );
        }

        token = NextTok();
    }
    else
    {
        text = std::make_unique<PCB_TEXT>( aParent );
    }

    // Legacy bare locked token
    if( token == T_locked )
    {
        text->SetLocked( true );
        token = NextTok();
    }

    if( !IsSymbol( token ) && (int) token != DSN_NUMBER )
        Expecting( "text value" );

    wxString value = FromUTF8();
    value.Replace( wxT( "%V" ), wxT( "${VALUE}" ) );
    value.Replace( wxT( "%R" ), wxT( "${REFERENCE}" ) );
    text->SetText( value );

    NeedLEFT();

    parsePCB_TEXT_effects( text.get(), aBaseText );

    if( parentFP )
    {
        // Convert hidden footprint text (which is no longer supported) into a hidden field
        if( !text->IsVisible() && text->Type() == PCB_TEXT_T )
            return new PCB_FIELD( *text.get(), FIELD_T::USER );
    }
    else
    {
        // Hidden PCB text is no longer supported
        text->SetVisible( true );
    }

    return text.release();
}


void PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TEXT_effects( PCB_TEXT* aText, PCB_TEXT* aBaseText )
{
    FOOTPRINT* parentFP = dynamic_cast<FOOTPRINT*>( aText->GetParent() );
    bool hasAngle       = false;    // Old files do not have a angle specified.
                                    // in this case it is 0 expected to be 0
    bool hasPos         = false;

    // By default, texts in footprints have a locked rotation (i.e. rot = -90 ... 90 deg)
    if( parentFP )
        aText->SetKeepUpright( true );

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token == T_LEFT )
            token = NextTok();

        switch( token )
        {
        case T_at:
        {
            VECTOR2I pt;

            hasPos = true;
            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            aText->SetTextPos( pt );
            token = NextTok();

            if( CurTok() == T_NUMBER )
            {
                aText->SetTextAngle( EDA_ANGLE( parseDouble(), DEGREES_T ) );
                hasAngle = true;
                token = NextTok();
            }

            // Legacy location of this token; presence implies true
            if( parentFP && CurTok() == T_unlocked )
            {
                aText->SetKeepUpright( false );
                token = NextTok();
            }

            if( (int) token != DSN_RIGHT )
                Expecting( DSN_RIGHT );

            break;
        }

        case T_layer:
            aText->SetLayer( parseBoardItemLayer() );

            token = NextTok();

            if( token == T_knockout )
            {
                aText->SetIsKnockout( true );
                token = NextTok();
            }

            if( (int) token != DSN_RIGHT )
                Expecting( DSN_RIGHT );

            break;

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( aText->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_hide:
        {
            // In older files, the hide token appears bare, and indicates hide==true.
            // In newer files, it will be an explicit bool in a list like (hide yes)
            bool hide = parseMaybeAbsentBool( true );

            if( parentFP )
                aText->SetVisible( !hide );
            else
                Expecting( "layer, effects, locked, render_cache, uuid or tstamp" );

            break;
        }

        case T_locked:
            // Newer list-enclosed locked
            aText->SetLocked( parseBool() );
            NeedRIGHT();
            break;

        // Confusingly, "unlocked" is not the opposite of "locked", but refers to "keep upright"
        case T_unlocked:
            if( parentFP )
                aText->SetKeepUpright( !parseBool() );
            else
                Expecting( "layer, effects, locked, render_cache or tstamp" );

            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( aText ) );
            break;

        case T_render_cache:
            parseRenderCache( static_cast<EDA_TEXT*>( aText ) );
            break;

        default:
            if( parentFP )
                Expecting( "layer, hide, effects, locked, render_cache or tstamp" );
            else
                Expecting( "layer, effects, locked, render_cache or tstamp" );
        }
    }

    // If there is no orientation defined, then it is the default value of 0 degrees.
    if( !hasAngle )
        aText->SetTextAngle( ANGLE_0 );

    if( parentFP && !dynamic_cast<PCB_DIMENSION_BASE*>( aBaseText ) )
    {
        // make PCB_TEXT rotation relative to the parent footprint.
        // It was read as absolute rotation from file
        // Note: this is not rue for PCB_DIMENSION items that use the board
        // coordinates
        aText->SetTextAngle( aText->GetTextAngle() - parentFP->GetOrientation() );

        // Move and rotate the text to its board coordinates
        aText->Rotate( { 0, 0 }, parentFP->GetOrientation() );

        // Only move offset from parent position if we read a position from the file.
        // These positions are relative to the parent footprint. If we don't have a position
        // then the text defaults to the parent position and moving again will double it.
        if (hasPos)
            aText->Move( parentFP->GetPosition() );
    }
}


PCB_TEXTBOX* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TEXTBOX( BOARD_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_gr_text_box || CurTok() == T_fp_text_box, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TEXTBOX." ) );

    std::unique_ptr<PCB_TEXTBOX> textbox = std::make_unique<PCB_TEXTBOX>( aParent );

    parseTextBoxContent( textbox.get() );

    return textbox.release();
}


PCB_TABLECELL* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TABLECELL( BOARD_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_table_cell, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a table cell." ) );

    std::unique_ptr<PCB_TABLECELL> cell = std::make_unique<PCB_TABLECELL>( aParent );

    parseTextBoxContent( cell.get() );

    return cell.release();
}


void PCB_IO_KICAD_SEXPR_PARSER::parseTextBoxContent( PCB_TEXTBOX* aTextBox )
{
    int           left;
    int           top;
    int           right;
    int           bottom;
    STROKE_PARAMS stroke( -1, LINE_STYLE::SOLID );
    bool          foundMargins = false;

    T token = NextTok();

    // Legacy locked
    if( token == T_locked )
    {
        aTextBox->SetLocked( true );
        token = NextTok();
    }

    if( !IsSymbol( token ) && (int) token != DSN_NUMBER )
        Expecting( "text value" );

    aTextBox->SetText( FromUTF8() );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_locked:
            aTextBox->SetLocked( parseMaybeAbsentBool( true ) );
            break;

        case T_start:
        {
            int x = parseBoardUnits( "X coordinate" );
            int y = parseBoardUnits( "Y coordinate" );
            aTextBox->SetStart( VECTOR2I( x, y ) );
            NeedRIGHT();

            NeedLEFT();
            token = NextTok();

            if( token != T_end )
                Expecting( T_end );

            x = parseBoardUnits( "X coordinate" );
            y = parseBoardUnits( "Y coordinate" );
            aTextBox->SetEnd( VECTOR2I( x, y ) );
            NeedRIGHT();
            break;
        }

        case T_pts:
        {
            aTextBox->SetShape( SHAPE_T::POLY );
            aTextBox->GetPolyShape().RemoveAllContours();
            aTextBox->GetPolyShape().NewOutline();

            while( (token = NextTok() ) != T_RIGHT )
                parseOutlinePoints( aTextBox->GetPolyShape().Outline( 0 ) );

            break;
        }

        case T_angle:
            // Set the angle of the text only, the coordinates of the box (a polygon) are
            // already at the right position, and must not be rotated
            aTextBox->EDA_TEXT::SetTextAngle( EDA_ANGLE( parseDouble( "text box angle" ), DEGREES_T ) );
            NeedRIGHT();
            break;

        case T_stroke:
        {
            STROKE_PARAMS_PARSER strokeParser( reader, pcbIUScale.IU_PER_MM );
            strokeParser.SyncLineReaderWith( *this );

            strokeParser.ParseStroke( stroke );
            SyncLineReaderWith( strokeParser );
            break;
        }

        case T_border:
            aTextBox->SetBorderEnabled( parseBool() );
            NeedRIGHT();
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

        case T_layer:
            aTextBox->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_knockout:
            if( [[maybe_unused]] PCB_TABLECELL* cell = dynamic_cast<PCB_TABLECELL*>( aTextBox ) )
            {
                Expecting( "locked, start, pts, angle, width, margins, layer, effects, span, "
                           "render_cache, uuid or tstamp" );
            }
            else
            {
                aTextBox->SetIsKnockout( parseBool() );
            }

            NeedRIGHT();
            break;

        case T_span:
            if( PCB_TABLECELL* cell = dynamic_cast<PCB_TABLECELL*>( aTextBox ) )
            {
                cell->SetColSpan( parseInt( "column span" ) );
                cell->SetRowSpan( parseInt( "row span" ) );
            }
            else
            {
                Expecting( "locked, start, pts, angle, width, stroke, border, margins, knockout, "
                           "layer, effects, render_cache, uuid or tstamp" );
            }

            NeedRIGHT();
            break;

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( aTextBox->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_effects:
            parseEDA_TEXT( static_cast<EDA_TEXT*>( aTextBox ) );
            break;

        case T_render_cache:
            parseRenderCache( static_cast<EDA_TEXT*>( aTextBox ) );
            break;

        default:
            if( dynamic_cast<PCB_TABLECELL*>( aTextBox ) != nullptr )
            {
                Expecting( "locked, start, pts, angle, width, margins, layer, effects, span, "
                           "render_cache, uuid or tstamp" );
            }
            else
            {
                Expecting( "locked, start, pts, angle, width, stroke, border, margins, knockout,"
                           "layer, effects, render_cache, uuid or tstamp" );
            }
        }
    }

    aTextBox->SetStroke( stroke );

    if( m_requiredVersion < 20230825 ) // compat, we move to an explicit flag
        aTextBox->SetBorderEnabled( stroke.GetWidth() >= 0 );

    if( !foundMargins )
    {
        int margin = aTextBox->GetLegacyTextMargin();
        aTextBox->SetMarginLeft( margin );
        aTextBox->SetMarginTop( margin );
        aTextBox->SetMarginRight( margin );
        aTextBox->SetMarginBottom( margin );
    }

    if( FOOTPRINT* parentFP = aTextBox->GetParentFootprint() )
    {
        aTextBox->Rotate( { 0, 0 }, parentFP->GetOrientation() );
        aTextBox->Move( parentFP->GetPosition() );
    }
}


PCB_TABLE* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TABLE( BOARD_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_table, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as a table." ) );

    T             token;
    STROKE_PARAMS borderStroke( -1, LINE_STYLE::SOLID );
    STROKE_PARAMS separatorsStroke( -1, LINE_STYLE::SOLID );
    std::unique_ptr<PCB_TABLE> table = std::make_unique<PCB_TABLE>( aParent, -1 );

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

        case T_locked:
            table->SetLocked( parseBool() );
            NeedRIGHT();
            break;

        case T_angle:   // legacy token no longer used
            NeedRIGHT();
            break;

        case T_layer:
            table->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_column_widths:
        {
            int col = 0;

            while( ( token = NextTok() ) != T_RIGHT )
                table->SetColWidth( col++, parseBoardUnits() );

            break;
        }

        case T_row_heights:
        {
            int row = 0;

            while( ( token = NextTok() ) != T_RIGHT )
                table->SetRowHeight( row++, parseBoardUnits() );

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

                table->AddCell( parsePCB_TABLECELL( table.get() ) );
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
                {
                    STROKE_PARAMS_PARSER strokeParser( reader, pcbIUScale.IU_PER_MM );
                    strokeParser.SyncLineReaderWith( *this );

                    strokeParser.ParseStroke( borderStroke );
                    SyncLineReaderWith( strokeParser );

                    table->SetBorderStroke( borderStroke );
                    break;
                }

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
                {
                    STROKE_PARAMS_PARSER strokeParser( reader, pcbIUScale.IU_PER_MM );
                    strokeParser.SyncLineReaderWith( *this );

                    strokeParser.ParseStroke( separatorsStroke );
                    SyncLineReaderWith( strokeParser );

                    table->SetSeparatorsStroke( separatorsStroke );
                    break;
                }

                default:
                    Expecting( "rows, cols, or stroke" );
                    break;
                }
            }

            break;

        default:
            Expecting( "columns, layer, col_widths, row_heights, border, separators, header or "
                       "cells" );
        }
    }

    return table.release();
}


PCB_DIMENSION_BASE* PCB_IO_KICAD_SEXPR_PARSER::parseDIMENSION( BOARD_ITEM* aParent )
{
    wxCHECK_MSG( CurTok() == T_dimension, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as DIMENSION." ) );

    T token;
    bool locked = false;
    std::unique_ptr<PCB_DIMENSION_BASE> dim;

    token = NextTok();

    // Free 'locked' token from 6.0/7.0 formats
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
    bool isStyleKnown = false;

    // Old format
    if( token == T_width )
    {
        isLegacyDimension = true;
        dim = std::make_unique<PCB_DIM_ALIGNED>( aParent );
        dim->SetLineThickness( parseBoardUnits( "dimension width value" ) );
        NeedRIGHT();
    }
    else
    {
        if( token != T_type )
            Expecting( T_type );

        switch( NextTok() )
        {
        case T_aligned:    dim = std::make_unique<PCB_DIM_ALIGNED>( aParent );    break;
        case T_orthogonal: dim = std::make_unique<PCB_DIM_ORTHOGONAL>( aParent ); break;
        case T_leader:     dim = std::make_unique<PCB_DIM_LEADER>( aParent );     break;
        case T_center:     dim = std::make_unique<PCB_DIM_CENTER>( aParent );     break;
        case T_radial:     dim = std::make_unique<PCB_DIM_RADIAL>( aParent );     break;
        default:           wxFAIL_MSG( wxT( "Cannot parse unknown dimension type " )
                                       + GetTokenString( CurTok() ) );
        }

        NeedRIGHT();

        // Before parsing further, set default properites for old KiCad file
        // versions that didnt have these properties:
        dim->SetArrowDirection( DIM_ARROW_DIRECTION::OUTWARD );
    }

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_layer:
            dim->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( dim->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_gr_text:
        {
            // In old pcb files, when parsing the text we do not yet know
            // if the text is kept aligned or not, and its DIM_TEXT_POSITION option.
            // Leave the text not aligned for now to read the text angle, and no
            // constraint for DIM_TEXT_POSITION in this case.
            // It will be set aligned (or not) later
            bool is_aligned = dim->GetKeepTextAligned();
            DIM_TEXT_POSITION t_dim_pos = dim->GetTextPositionMode();

            if( !isStyleKnown )
            {
                dim->SetTextPositionMode( DIM_TEXT_POSITION::MANUAL );
                dim->SetKeepTextAligned( false );
            }

            parsePCB_TEXT( m_board, dim.get() );

            if( isLegacyDimension )
            {
                EDA_UNITS units = EDA_UNITS::MM;

                if( !EDA_UNIT_UTILS::FetchUnitsFromString( dim->GetText(), units ) )
                    dim->SetAutoUnits( true ); //Not determined => use automatic units

                dim->SetUnits( units );
            }

            if( !isStyleKnown )
            {
                dim->SetKeepTextAligned( is_aligned );
                dim->SetTextPositionMode( t_dim_pos );
            }
            break;
        }

        // New format: feature points
        case T_pts:
        {
            VECTOR2I point;

            parseXY( &point.x, &point.y );
            dim->SetStart( point );
            parseXY( &point.x, &point.y );
            dim->SetEnd( point );

            NeedRIGHT();
            break;
        }

        case T_height:
        {
            int height = parseBoardUnits( "dimension height value" );
            NeedRIGHT();

            if( dim->Type() == PCB_DIM_ORTHOGONAL_T || dim->Type() == PCB_DIM_ALIGNED_T )
            {
                PCB_DIM_ALIGNED* aligned = static_cast<PCB_DIM_ALIGNED*>( dim.get() );
                aligned->SetHeight( height );
            }

            break;
        }

        case T_leader_length:
        {
            int length = parseBoardUnits( "leader length value" );
            NeedRIGHT();

            if( dim->Type() == PCB_DIM_RADIAL_T )
            {
                PCB_DIM_RADIAL* radial = static_cast<PCB_DIM_RADIAL*>( dim.get() );
                radial->SetLeaderLength( length );
            }

            break;
        }

        case T_orientation:
        {
            int orientation = parseInt( "orthogonal dimension orientation" );
            NeedRIGHT();

            if( dim->Type() == PCB_DIM_ORTHOGONAL_T )
            {
                PCB_DIM_ORTHOGONAL* ortho = static_cast<PCB_DIM_ORTHOGONAL*>( dim.get() );
                orientation = std::clamp( orientation, 0, 1 );
                ortho->SetOrientation( static_cast<PCB_DIM_ORTHOGONAL::DIR>( orientation ) );
            }

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
                    dim->SetPrefix( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_suffix:
                    NeedSYMBOLorNUMBER();
                    dim->SetSuffix( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_units:
                {
                    int mode = parseInt( "dimension units mode" );
                    mode = std::max( 0, std::min( 4, mode ) );
                    dim->SetUnitsMode( static_cast<DIM_UNITS_MODE>( mode ) );
                    NeedRIGHT();
                    break;
                }

                case T_units_format:
                {
                    int format = parseInt( "dimension units format" );
                    format = std::clamp( format, 0, 3 );
                    dim->SetUnitsFormat( static_cast<DIM_UNITS_FORMAT>( format ) );
                    NeedRIGHT();
                    break;
                }

                case T_precision:
                    dim->SetPrecision( static_cast<DIM_PRECISION>( parseInt( "dimension precision" ) ) );
                    NeedRIGHT();
                    break;

                case T_override_value:
                    NeedSYMBOLorNUMBER();
                    dim->SetOverrideTextEnabled( true );
                    dim->SetOverrideText( FromUTF8() );
                    NeedRIGHT();
                    break;

                case T_suppress_zeroes:
                    dim->SetSuppressZeroes( parseMaybeAbsentBool( true ) );
                    break;

                default:
                    std::cerr << "Unknown format token: " << GetTokenString( token ) << std::endl;
                    Expecting( "prefix, suffix, units, units_format, precision, override_value, "
                               "suppress_zeroes" );
                }
            }
            break;
        }

        case T_style:
        {
            isStyleKnown = true;

            // new format: default to keep text aligned off unless token is present
            dim->SetKeepTextAligned( false );

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                switch( token )
                {
                case T_LEFT:
                    continue;

                case T_thickness:
                    dim->SetLineThickness( parseBoardUnits( "extension line thickness value" ) );
                    NeedRIGHT();
                    break;

                case T_arrow_direction:
                {
                    token = NextTok();

                    if( token == T_inward )
                        dim->ChangeArrowDirection( DIM_ARROW_DIRECTION::INWARD );
                    else if( token == T_outward )
                        dim->ChangeArrowDirection( DIM_ARROW_DIRECTION::OUTWARD );
                    else
                        Expecting( "inward or outward" );

                    NeedRIGHT();
                    break;
                }
                case T_arrow_length:

                    dim->SetArrowLength( parseBoardUnits( "arrow length value" ) );
                    NeedRIGHT();
                    break;

                case T_text_position_mode:
                {
                    int mode = parseInt( "text position mode" );
                    mode = std::max( 0, std::min( 3, mode ) );
                    dim->SetTextPositionMode( static_cast<DIM_TEXT_POSITION>( mode ) );
                    NeedRIGHT();
                    break;
                }

                case T_extension_height:
                {
                    PCB_DIM_ALIGNED* aligned = dynamic_cast<PCB_DIM_ALIGNED*>( dim.get() );
                    wxCHECK_MSG( aligned, nullptr, wxT( "Invalid extension_height token" ) );
                    aligned->SetExtensionHeight( parseBoardUnits( "extension height value" ) );
                    NeedRIGHT();
                    break;
                }

                case T_extension_offset:
                    dim->SetExtensionOffset( parseBoardUnits( "extension offset value" ) );
                    NeedRIGHT();
                    break;

                case T_keep_text_aligned:
                    dim->SetKeepTextAligned( parseMaybeAbsentBool( true ) );
                    break;

                case T_text_frame:
                {
                    wxCHECK_MSG( dim->Type() == PCB_DIM_LEADER_T, nullptr,
                                 wxT( "Invalid text_frame token" ) );

                    PCB_DIM_LEADER* leader = static_cast<PCB_DIM_LEADER*>( dim.get() );

                    int textFrame = parseInt( "text frame mode" );
                    textFrame = std::clamp( textFrame, 0, 3 );
                    leader->SetTextBorder( static_cast<DIM_TEXT_BORDER>( textFrame ));
                    NeedRIGHT();
                    break;
                }

                default:
                    Expecting( "thickness, arrow_length, arrow_direction, text_position_mode, "
                               "extension_height, extension_offset" );
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

            VECTOR2I point;

            parseXY( &point.x, &point.y );
            dim->SetStart( point );

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

            VECTOR2I point;

            parseXY( &point.x, &point.y );
            dim->SetEnd( point );

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
                // If we have a crossbar, we know we're an old aligned dim
                PCB_DIM_ALIGNED* aligned = static_cast<PCB_DIM_ALIGNED*>( dim.get() );

                // Old style: calculate height from crossbar
                VECTOR2I point1, point2;
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

        // Handle (locked yes) from modern times
        case T_locked:
        {
            // Unsure if we ever wrote out (locked) for dimensions, so use maybeAbsent just in case
            bool isLocked = parseMaybeAbsentBool( true );
            dim->SetLocked( isLocked );
            break;
        }

        default:
            Expecting( "layer, tstamp, uuid, gr_text, feature1, feature2, crossbar, arrow1a, "
                       "arrow1b, arrow2a, or arrow2b" );
        }
    }

    if( locked )
        dim->SetLocked( true );

    dim->Update();

    return dim.release();
}


FOOTPRINT* PCB_IO_KICAD_SEXPR_PARSER::parseFOOTPRINT( wxArrayString* aInitialComments )
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


FOOTPRINT* PCB_IO_KICAD_SEXPR_PARSER::parseFOOTPRINT_unchecked( wxArrayString* aInitialComments )
{
    wxCHECK_MSG( CurTok() == T_module || CurTok() == T_footprint, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as FOOTPRINT." ) );

    wxString name;
    VECTOR2I pt;
    T        token;
    LIB_ID   fpid;
    int      attributes = 0;

    std::unique_ptr<FOOTPRINT> footprint = std::make_unique<FOOTPRINT>( m_board );

    footprint->SetInitialComments( aInitialComments );

    if( m_board )
    {
        footprint->SetStaticComponentClass(
                m_board->GetComponentClassManager().GetNoneComponentClass() );
    }

    token = NextTok();

    if( !IsSymbol( token ) && token != T_NUMBER )
        Expecting( "symbol|number" );

    name = FromUTF8();

    if( !name.IsEmpty() && fpid.Parse( name, true ) >= 0 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid footprint ID in\nfile: %s\nline: %d\n"
                                             "offset: %d." ),
                                          CurSource(), CurLineNumber(), CurOffset() ) );
    }

    auto checkVersion =
            [&]()
            {
                if( m_requiredVersion > SEXPR_BOARD_FILE_VERSION )
                {
                    throw FUTURE_FORMAT_ERROR( fmt::format( "{}", m_requiredVersion ),
                                               m_generatorVersion );
                }
            };

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
            SetKnowsBar( m_requiredVersion >= 20240706 ); // Bar token is known from this version
            footprint->SetFileFormatVersionAtLoad( this_version );
            break;
        }

        case T_generator:
            // We currently ignore the generator when parsing. It is included in the file for manual
            // indication of where the footprint came from.
            NeedSYMBOL();
            NeedRIGHT();
            break;

        case T_generator_version:
        {
            NeedSYMBOL();
            m_generatorVersion = FromUTF8();
            NeedRIGHT();

            // If the format includes a generator version, by this point we have enough info to
            // do the version check here
            checkVersion();

            break;
        }

        case T_locked:
            footprint->SetLocked( parseMaybeAbsentBool( true ) );
            break;

        case T_placed:
            footprint->SetIsPlaced( parseMaybeAbsentBool( true ) );
            break;

        case T_layer:
        {
            // Footprints can be only on the front side or the back side.
            // but because we can find some stupid layer in file, ensure a
            // acceptable layer is set for the footprint
            PCB_LAYER_ID layer = parseBoardItemLayer();
            footprint->SetLayer( layer == B_Cu ? B_Cu : F_Cu );
            NeedRIGHT();
            break;
        }

        case T_tedit:
            parseHex();
            NeedRIGHT();
            break;

        case T_tstamp:
        case T_uuid:
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
                footprint->SetOrientation( EDA_ANGLE( parseDouble(), DEGREES_T ) );
                NeedRIGHT();
            }
            else if( token != T_RIGHT )
            {
                Expecting( T_RIGHT );
            }

            break;

        case T_descr:
            NeedSYMBOLorNUMBER(); // some symbols can be 0508, so a number is also a symbol here
            footprint->SetLibDescription( FromUTF8() );
            NeedRIGHT();
            break;

        case T_tags:
            NeedSYMBOLorNUMBER(); // some symbols can be 0508, so a number is also a symbol here
            footprint->SetKeywords( FromUTF8() );
            NeedRIGHT();
            break;

        case T_property:
        {
            NeedSYMBOL();
            wxString pName = FromUTF8();
            NeedSYMBOL();
            wxString pValue = FromUTF8();

            // Prior to PCB fields, we used to use properties for special values instead of
            // using (keyword_example "value")
            if( m_requiredVersion < 20230620 )
            {
                // Skip legacy non-field properties sent from symbols that should not be kept
                // in footprints.
                if( pName == "ki_keywords" || pName == "ki_locked" )
                {
                    NeedRIGHT();
                    break;
                }

                // Description from symbol (not the fooprint library description stored in (descr) )
                // used to be stored as a reserved key value
                if( pName == "ki_description" )
                {
                    footprint->GetField( FIELD_T::DESCRIPTION )->SetText( pValue );
                    NeedRIGHT();
                    break;
                }

                // Sheet file and name used to be stored as properties invisible to the user
                if( pName == "Sheetfile" || pName == "Sheet file" )
                {
                    footprint->SetSheetfile( pValue );
                    NeedRIGHT();
                    break;
                }

                if( pName == "Sheetname" || pName == "Sheet name" )
                {
                    footprint->SetSheetname( pValue );
                    NeedRIGHT();
                    break;
                }
            }

            PCB_FIELD* field = nullptr;
            std::unique_ptr<PCB_FIELD> unusedField;

            // 8.0.0rc3 had a bug where these properties were mistakenly added to the footprint as
            // fields, this will remove them as fields but still correctly set the footprint filters
            if( pName == "ki_fp_filters" )
            {
                footprint->SetFilters( pValue );

                // Use the text effect parsing function because it will handle ki_fp_filters as a
                // property with no text effects, but will also handle parsing the text effects.
                // We just drop the effects if they're present.
                unusedField = std::make_unique<PCB_FIELD>( footprint.get(), FIELD_T::USER );
                field = unusedField.get();
            }
            else if( pName == "Footprint" )
            {
                // Until V9, footprints had a Footprint field that usually (but not always)
                // duplicated the footprint's LIB_ID.  In V9 this was removed.  Parse it
                // like any other, but don't add it to anything.
                unusedField = std::make_unique<PCB_FIELD>( footprint.get(), FIELD_T::FOOTPRINT );
                field = unusedField.get();
            }
            else if( footprint->HasField( pName ) )
            {
                field = footprint->GetField( pName );
                field->SetText( pValue );
            }
            else
            {
                field = new PCB_FIELD( footprint.get(), FIELD_T::USER, pName );
                footprint->Add( field );

                field->SetText( pValue );
                field->SetLayer( footprint->GetLayer() == F_Cu ? F_Fab : B_Fab );

                if( m_board )   // can be null when reading a lib
                    field->StyleFromSettings( m_board->GetDesignSettings() );
            }

            // Hide the field by default if it is a legacy field that did not have
            // text effects applied, since hide is a negative effect
            if( m_requiredVersion < 20230620 )
                field->SetVisible( false );
            else
                field->SetVisible( true );

            parsePCB_TEXT_effects( field );
        }
            break;

        case T_path:
            NeedSYMBOLorNUMBER(); // Paths can be numerical so a number is also a symbol here
            footprint->SetPath( KIID_PATH( FromUTF8() ) );
            NeedRIGHT();
            break;

        case T_sheetname:
            NeedSYMBOL();
            footprint->SetSheetname( FromUTF8() );
            NeedRIGHT();
            break;

        case T_sheetfile:
            NeedSYMBOL();
            footprint->SetSheetfile( FromUTF8() );
            NeedRIGHT();
            break;

        case T_autoplace_cost90:
        case T_autoplace_cost180:
            parseInt( "legacy auto-place cost" );
            NeedRIGHT();
            break;

        case T_private_layers:
        {
            LSET privateLayers;

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                auto it = m_layerIndices.find( CurStr() );

                if( it != m_layerIndices.end() )
                    privateLayers.set( it->second );
                else
                    Expecting( "layer name" );
            }

            if( m_requiredVersion < 20220427 )
            {
                privateLayers.set( Edge_Cuts, false );
                privateLayers.set( Margin, false );
            }

            footprint->SetPrivateLayers( privateLayers );
            break;
        }

        case T_net_tie_pad_groups:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                footprint->AddNetTiePadGroup( CurStr() );

            break;

        case T_duplicate_pad_numbers_are_jumpers:
            footprint->SetDuplicatePadNumbersAreJumpers( parseBool() );
            NeedRIGHT();
            break;

        case T_jumper_pad_groups:
        {
            // This should only be formatted if there is at least one group
            std::vector<std::set<wxString>>& groups = footprint->JumperPadGroups();
            std::set<wxString>* currentGroup = nullptr;

            for( token = NextTok(); currentGroup || token != T_RIGHT; token = NextTok() )
            {
                switch( static_cast<int>( token ) )
                {
                case T_LEFT:
                    currentGroup = &groups.emplace_back();
                    break;

                case DSN_STRING:
                    if( currentGroup )
                        currentGroup->insert( FromUTF8() );

                    break;

                case T_RIGHT:
                    currentGroup = nullptr;
                    break;

                default:
                    Expecting( "list of pad names" );
                }
            }

            break;
        }

        case T_solder_mask_margin:
            footprint->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && footprint->GetLocalSolderMaskMargin() == 0 )
                footprint->SetLocalSolderMaskMargin( {} );

            break;

        case T_solder_paste_margin:
            footprint->SetLocalSolderPasteMargin( parseBoardUnits( "local solder paste margin value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && footprint->GetLocalSolderPasteMargin() == 0 )
                footprint->SetLocalSolderPasteMargin( {} );

            break;

        case T_solder_paste_ratio:          // legacy token
        case T_solder_paste_margin_ratio:
            footprint->SetLocalSolderPasteMarginRatio( parseDouble( "local solder paste margin ratio value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && footprint->GetLocalSolderPasteMarginRatio() == 0 )
                footprint->SetLocalSolderPasteMarginRatio( {} );

            break;

        case T_clearance:
            footprint->SetLocalClearance( parseBoardUnits( "local clearance value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && footprint->GetLocalClearance() == 0 )
                footprint->SetLocalClearance( {} );

            break;

        case T_zone_connect:
            footprint->SetLocalZoneConnection((ZONE_CONNECTION) parseInt( "zone connection value" ) );
            NeedRIGHT();
            break;

        case T_thermal_width:
        case T_thermal_gap:
            // Interestingly, these have never been exposed in the GUI
            parseBoardUnits( token );
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

                case T_allow_missing_courtyard:
                    footprint->SetAllowMissingCourtyard( true );
                    break;

                case T_dnp:
                    attributes |= FP_DNP;
                    break;

                case T_allow_soldermask_bridges:
                    footprint->SetAllowSolderMaskBridges( true );
                    break;

                default:
                    Expecting( "through_hole, smd, virtual, board_only, exclude_from_pos_files, "
                               "exclude_from_bom or allow_solder_mask_bridges" );
                }
            }

            break;

        case T_fp_text:
        {
            PCB_TEXT* text = parsePCB_TEXT( footprint.get() );

            if( PCB_FIELD* field = dynamic_cast<PCB_FIELD*>( text ) )
            {
                switch( field->GetId() )
                {
                case FIELD_T::REFERENCE:
                    footprint->Reference() = PCB_FIELD( *text, FIELD_T::REFERENCE );
                    const_cast<KIID&>( footprint->Reference().m_Uuid ) = text->m_Uuid;
                    delete text;
                    break;

                case FIELD_T::VALUE:
                    footprint->Value() = PCB_FIELD( *text, FIELD_T::VALUE );
                    const_cast<KIID&>( footprint->Value().m_Uuid ) = text->m_Uuid;
                    delete text;
                    break;

                default:
                    // Fields other than reference and value weren't historically
                    // stored in fp_texts so we don't need to handle them here
                    break;
                }
            }
            else
            {
                footprint->Add( text, ADD_MODE::APPEND, true );
            }

            break;
        }

        case T_fp_text_box:
        {
            PCB_TEXTBOX* textbox = parsePCB_TEXTBOX( footprint.get() );
            footprint->Add( textbox, ADD_MODE::APPEND, true );
            break;
        }

        case T_table:
        {
            PCB_TABLE* table = parsePCB_TABLE( footprint.get() );
            footprint->Add( table, ADD_MODE::APPEND, true );
            break;
        }

        case T_fp_arc:
        case T_fp_circle:
        case T_fp_curve:
        case T_fp_rect:
        case T_fp_line:
        case T_fp_poly:
        {
            PCB_SHAPE* shape = parsePCB_SHAPE( footprint.get() );
            footprint->Add( shape, ADD_MODE::APPEND, true );
            break;
        }

        case T_image:
        {
            PCB_REFERENCE_IMAGE* image = parsePCB_REFERENCE_IMAGE( footprint.get() );
            footprint->Add( image, ADD_MODE::APPEND, true );
            break;
        }

        case T_dimension:
        {
            PCB_DIMENSION_BASE* dimension = parseDIMENSION( footprint.get() );
            footprint->Add( dimension, ADD_MODE::APPEND, true );
            break;
        }

        case T_pad:
        {
            PAD* pad = parsePAD( footprint.get() );
            footprint->Add( pad, ADD_MODE::APPEND, true );
            break;
        }

        case T_model:
        {
            FP_3DMODEL* model = parse3DModel();
            footprint->Add3DModel( model );
            delete model;
            break;
        }

        case T_zone:
        {
            ZONE* zone = parseZONE( footprint.get() );
            footprint->Add( zone, ADD_MODE::APPEND, true );
            break;
        }

        case T_group:
            parseGROUP( footprint.get() );
            break;

        case T_embedded_fonts:
        {
            footprint->GetEmbeddedFiles()->SetAreFontsEmbedded( parseBool() );
            NeedRIGHT();
            break;
        }

        case T_embedded_files:
        {
            EMBEDDED_FILES_PARSER embeddedFilesParser( reader );
            embeddedFilesParser.SyncLineReaderWith( *this );

            try
            {
                embeddedFilesParser.ParseEmbedded( footprint->GetEmbeddedFiles() );
            }
            catch( const PARSE_ERROR& e )
            {
                wxLogError( e.What() );
            }

            SyncLineReaderWith( embeddedFilesParser );
            break;
        }

        case T_component_classes:
        {
            std::unordered_set<wxString> componentClassNames;

            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                if( ( token = NextTok() ) != T_class )
                    Expecting( T_class );

                NeedSYMBOLorNUMBER();
                componentClassNames.insert( From_UTF8( CurText() ) );
                NeedRIGHT();
            }

            footprint->SetTransientComponentClassNames( componentClassNames );

            if( m_board )
                footprint->ResolveComponentClassNames( m_board, componentClassNames );

            break;
        }

        default:
            Expecting( "at, descr, locked, placed, tedit, tstamp, uuid, "
                       "autoplace_cost90, autoplace_cost180, attr, clearance, "
                       "embedded_files, fp_arc, fp_circle, fp_curve, fp_line, fp_poly, "
                       "fp_rect, fp_text, pad, group, generator, model, path, solder_mask_margin, "
                       "solder_paste_margin, solder_paste_margin_ratio, tags, thermal_gap, "
                       "version, zone, zone_connect, or component_classes" );
        }
    }

    // In legacy files the lack of attributes indicated a through-hole component which was by
    // default excluded from pos files.  However there was a hack to look for SMD pads and
    // consider those "mislabeled through-hole components" and therefore include them in place
    // files.  We probably don't want to get into that game so we'll just include them by
    // default and let the user change it if required.
    if( m_requiredVersion < 20200826 && attributes == 0 )
        attributes |= FP_THROUGH_HOLE;

    if( m_requiredVersion <= LEGACY_NET_TIES )
    {
        if( footprint->GetKeywords().StartsWith( wxT( "net tie" ) ) )
        {
            wxString padGroup;

            for( PAD* pad : footprint->Pads() )
            {
                if( !padGroup.IsEmpty() )
                    padGroup += wxS( ", " );

                padGroup += pad->GetNumber();
            }

            if( !padGroup.IsEmpty() )
                footprint->AddNetTiePadGroup( padGroup );
        }
    }

    footprint->SetAttributes( attributes );

    footprint->SetFPID( fpid );

    return footprint.release();
}


PAD* PCB_IO_KICAD_SEXPR_PARSER::parsePAD( FOOTPRINT* aParent )
{
    wxCHECK_MSG( CurTok() == T_pad, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PAD." ) );

    VECTOR2I sz;
    VECTOR2I pt;
    bool     foundNet = false;

    std::unique_ptr<PAD> pad = std::make_unique<PAD>( aParent );

    NeedSYMBOLorNUMBER();
    pad->SetNumber( FromUTF8() );

    T token = NextTok();

    switch( token )
    {
    case T_thru_hole:
        pad->SetAttribute( PAD_ATTRIB::PTH );

        // The drill token is usually missing if 0 drill size is specified.
        // Emulate it using 1 nm drill size to avoid errors.
        // Drill size cannot be set to 0 in newer versions.
        pad->SetDrillSize( VECTOR2I( 1, 1 ) );
        break;

    case T_smd:
        pad->SetAttribute( PAD_ATTRIB::SMD );

        // Default PAD object is thru hole with drill.
        // SMD pads have no hole
        pad->SetDrillSize( VECTOR2I( 0, 0 ) );
        break;

    case T_connect:
        pad->SetAttribute( PAD_ATTRIB::CONN );

        // Default PAD object is thru hole with drill.
        // CONN pads have no hole
        pad->SetDrillSize( VECTOR2I( 0, 0 ) );
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
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        break;

    case T_rect:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
        break;

    case T_oval:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
        break;

    case T_trapezoid:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::TRAPEZOID );
        break;

    case T_roundrect:
        // Note: the shape can be PAD_SHAPE::ROUNDRECT or PAD_SHAPE::CHAMFERED_RECT
        // (if chamfer parameters are found later in pad descr.)
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::ROUNDRECT );
        break;

    case T_custom:
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CUSTOM );
        break;

    default:
        Expecting( "circle, rectangle, roundrect, oval, trapezoid or custom" );
    }

    std::optional<EDA_ANGLE> thermalBrAngleOverride;

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        if( token == T_locked )
        {
            // Pad locking is now a session preference
            token = NextTok();
        }

        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_size:
            sz.x = parseBoardUnits( "width value" );
            sz.y = parseBoardUnits( "height value" );
            pad->SetSize( PADSTACK::ALL_LAYERS, sz );
            NeedRIGHT();
            break;

        case T_at:
            pt.x = parseBoardUnits( "X coordinate" );
            pt.y = parseBoardUnits( "Y coordinate" );
            pad->SetFPRelativePosition( pt );
            token = NextTok();

            if( token == T_NUMBER )
            {
                pad->SetOrientation( EDA_ANGLE( parseDouble(), DEGREES_T ) );
                NeedRIGHT();
            }
            else if( token != T_RIGHT )
            {
                Expecting( ") or angle value" );
            }

            break;

        case T_rect_delta:
        {
            VECTOR2I delta;
            delta.x = parseBoardUnits( "rectangle delta width" );
            delta.y = parseBoardUnits( "rectangle delta height" );
            pad->SetDelta( PADSTACK::ALL_LAYERS, delta );
            NeedRIGHT();
            break;
        }

        case T_drill:
        {
            bool     haveWidth = false;
            VECTOR2I drillSize = pad->GetDrillSize();

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_oval: pad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG ); break;

                case T_NUMBER:
                {
                    if( !haveWidth )
                    {
                        drillSize.x = parseBoardUnits();

                        // If height is not defined the width and height are the same.
                        drillSize.y = drillSize.x;
                        haveWidth = true;
                    }
                    else
                    {
                        drillSize.y = parseBoardUnits();
                    }
                }

                break;

                case T_offset:
                    pt.x = parseBoardUnits( "drill offset x" );
                    pt.y = parseBoardUnits( "drill offset y" );
                    pad->SetOffset( PADSTACK::ALL_LAYERS, pt );
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
            if( pad->GetAttribute() != PAD_ATTRIB::SMD && pad->GetAttribute() != PAD_ATTRIB::CONN )
                pad->SetDrillSize( drillSize );
            else
                pad->SetDrillSize( VECTOR2I( 0, 0 ) );

            break;
        }

        case T_layers:
        {
            LSET layerMask = parseBoardItemLayersAsMask();

            // We force this mask to include all copper layers if the pad is a PTH pad.
            // This is because PTH pads are always drawn on all copper layers, even if the
            // padstack has inner layers that are smaller than the hole.  There was a corner
            // case in the past where a PTH pad was defined with NPTH layer set (F&B.Cu) and
            // could not be reset without effort
            if( pad->GetAttribute() == PAD_ATTRIB::PTH && m_board )
                layerMask |= LSET::AllCuMask( m_board->GetCopperLayerCount() );

            pad->SetLayerSet( layerMask );
            break;
        }

        case T_net:
            foundNet = true;

            if( ! pad->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
            {
                wxLogError( _( "Invalid net ID in\nfile: %s\nline: %d offset: %d" ),
                            CurSource(), CurLineNumber(), CurOffset() );
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
                                CurSource(), CurLineNumber(), CurOffset() );
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

        case T_die_delay:
            pad->SetPadToDieDelay( parseBoardUnits( T_die_delay ) );
            NeedRIGHT();
            break;

        case T_solder_mask_margin:
            pad->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && pad->GetLocalSolderMaskMargin() == 0 )
                pad->SetLocalSolderMaskMargin( {} );

            break;

        case T_solder_paste_margin:
            pad->SetLocalSolderPasteMargin( parseBoardUnits( "local solder paste margin value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && pad->GetLocalSolderPasteMargin() == 0 )
                pad->SetLocalSolderPasteMargin( {} );

            break;

        case T_solder_paste_margin_ratio:
            pad->SetLocalSolderPasteMarginRatio( parseDouble( "local solder paste margin ratio value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && pad->GetLocalSolderPasteMarginRatio() == 0 )
                pad->SetLocalSolderPasteMarginRatio( {} );

            break;

        case T_clearance:
            pad->SetLocalClearance( parseBoardUnits( "local clearance value" ) );
            NeedRIGHT();

            // In pre-9.0 files "0" meant inherit.
            if( m_requiredVersion <= 20240201 && pad->GetLocalClearance() == 0 )
                pad->SetLocalClearance( {} );

            break;

        case T_teardrops:
            parseTEARDROP_PARAMETERS( &pad->GetTeardropParams() );
            break;

        case T_zone_connect:
            pad->SetLocalZoneConnection( (ZONE_CONNECTION) parseInt( "zone connection value" ) );
            NeedRIGHT();
            break;

        case T_thermal_width:       // legacy token
        case T_thermal_bridge_width:
            pad->SetLocalThermalSpokeWidthOverride( parseBoardUnits( token ) );
            NeedRIGHT();
            break;

        case T_thermal_bridge_angle:
            thermalBrAngleOverride = EDA_ANGLE( parseDouble( "thermal spoke angle" ), DEGREES_T );
            NeedRIGHT();
            break;


        case T_thermal_gap:
            pad->SetThermalGap( parseBoardUnits( "thermal relief gap value" ) );
            NeedRIGHT();
            break;

        case T_roundrect_rratio:
            pad->SetRoundRectRadiusRatio( PADSTACK::ALL_LAYERS,
                                          parseDouble( "roundrect radius ratio" ) );
            NeedRIGHT();
            break;

        case T_chamfer_ratio:
            pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, parseDouble( "chamfer ratio" ) );

            if( pad->GetChamferRectRatio( PADSTACK::ALL_LAYERS ) > 0 )
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CHAMFERED_RECT );

            NeedRIGHT();
            break;

        case T_chamfer:
        {
            int  chamfers = 0;
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
                    pad->SetChamferPositions( PADSTACK::ALL_LAYERS, chamfers );
                    end_list = true;
                    break;

                default:
                    Expecting( "chamfer_top_left chamfer_top_right chamfer_bottom_left or "
                               "chamfer_bottom_right" );
                }
            }

            if( pad->GetChamferPositions( PADSTACK::ALL_LAYERS ) != RECT_NO_CHAMFER )
                pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CHAMFERED_RECT );

            break;
        }

        case T_property:
            while( token != T_RIGHT )
            {
                token = NextTok();

                switch( token )
                {
                case T_pad_prop_bga:           pad->SetProperty( PAD_PROP::BGA );            break;
                case T_pad_prop_fiducial_glob: pad->SetProperty( PAD_PROP::FIDUCIAL_GLBL );  break;
                case T_pad_prop_fiducial_loc:  pad->SetProperty( PAD_PROP::FIDUCIAL_LOCAL ); break;
                case T_pad_prop_testpoint:     pad->SetProperty( PAD_PROP::TESTPOINT );      break;
                case T_pad_prop_castellated:   pad->SetProperty( PAD_PROP::CASTELLATED );    break;
                case T_pad_prop_heatsink:      pad->SetProperty( PAD_PROP::HEATSINK );       break;
                case T_pad_prop_mechanical:    pad->SetProperty( PAD_PROP::MECHANICAL );     break;
                case T_none:                   pad->SetProperty( PAD_PROP::NONE );           break;
                case T_RIGHT:                                                                break;

                default:
#if 0   // Currently: skip unknown property
                    Expecting( "pad_prop_bga pad_prop_fiducial_glob pad_prop_fiducial_loc"
                               " pad_prop_heatsink or pad_prop_castellated" );
#endif
                    break;
                }
            }

            break;

        case T_options:
            parsePAD_option( pad.get() );
            break;

        case T_padstack:
            parsePadstack( pad.get() );
            break;

        case T_primitives:
            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_gr_arc:
                case T_gr_line:
                case T_gr_circle:
                case T_gr_rect:
                case T_gr_poly:
                case T_gr_curve:
                    pad->AddPrimitive( PADSTACK::ALL_LAYERS, parsePCB_SHAPE( nullptr ) );
                    break;

                case T_gr_bbox:
                {
                    PCB_SHAPE* numberBox = parsePCB_SHAPE( nullptr );
                    numberBox->SetIsProxyItem();
                    pad->AddPrimitive( PADSTACK::ALL_LAYERS, numberBox );
                    break;
                }

                case T_gr_vector:
                {
                    PCB_SHAPE* spokeTemplate = parsePCB_SHAPE( nullptr );
                    spokeTemplate->SetIsProxyItem();
                    pad->AddPrimitive( PADSTACK::ALL_LAYERS, spokeTemplate );
                    break;
                }

                default:
                    Expecting( "gr_line, gr_arc, gr_circle, gr_curve, gr_rect, gr_bbox or gr_poly" );
                    break;
                }
            }

            break;

        case T_remove_unused_layers:
        {
            bool remove = parseMaybeAbsentBool( true );
            pad->SetRemoveUnconnected( remove );
            break;
        }

        case T_keep_end_layers:
        {
            bool keep = parseMaybeAbsentBool( true );
            pad->SetKeepTopBottom( keep );
            break;
        }

        case T_tenting:
        {
            auto [front, back] = parseFrontBackOptBool( true );
            pad->Padstack().FrontOuterLayers().has_solder_mask = front;
            pad->Padstack().BackOuterLayers().has_solder_mask = back;
            break;
        }

        case T_zone_layer_connections:
        {
            LSET cuLayers = pad->GetLayerSet() & LSET::AllCuMask();

            for( PCB_LAYER_ID layer : cuLayers )
                pad->SetZoneLayerOverride( layer, ZLO_FORCE_NO_ZONE_CONNECTION );

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                PCB_LAYER_ID layer = lookUpLayer( m_layerIndices );

                if( !IsCopperLayer( layer ) )
                    Expecting( "copper layer name" );

                pad->SetZoneLayerOverride( layer, ZLO_FORCE_FLASHED );
            }

            break;
        }

        // Continue to process "(locked)" format which was output during 5.99 development
        case T_locked:
            // Pad locking is now a session preference
            parseMaybeAbsentBool( true );
            break;

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( pad->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "at, locked, drill, layers, net, die_length, roundrect_rratio, "
                       "solder_mask_margin, solder_paste_margin, solder_paste_margin_ratio, uuid, "
                       "clearance, tstamp, primitives, remove_unused_layers, keep_end_layers, "
                       "pinfunction, pintype, zone_connect, thermal_width, thermal_gap, padstack or "
                       "teardrops" );
        }
    }

    if( !foundNet )
    {
        // Make sure default netclass is correctly assigned to pads that don't define a net.
        pad->SetNetCode( 0, /* aNoAssert */ true );
    }

    if( thermalBrAngleOverride )
    {
        pad->SetThermalSpokeAngle( *thermalBrAngleOverride );
    }
    else
    {
        // This is here because custom pad anchor shape isn't known before reading (options
        if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE )
        {
            pad->SetThermalSpokeAngle( ANGLE_45 );
        }
        else if( pad->GetShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CUSTOM
                 && pad->GetAnchorPadShape( PADSTACK::ALL_LAYERS ) == PAD_SHAPE::CIRCLE )
        {
            if( m_requiredVersion <= 20211014 ) // 6.0
                pad->SetThermalSpokeAngle( ANGLE_90 );
            else
                pad->SetThermalSpokeAngle( ANGLE_45 );
        }
        else
        {
            pad->SetThermalSpokeAngle( ANGLE_90 );
        }
    }

    if( !pad->CanHaveNumber() )
    {
        // At some point it was possible to assign a number to aperture pads so we need to clean
        // those out here.
        pad->SetNumber( wxEmptyString );
    }

    // Zero-sized pads are likely algorithmically unsafe.
    if( pad->GetSizeX() <= 0 || pad->GetSizeY() <= 0 )
    {
        pad->SetSize( PADSTACK::ALL_LAYERS,
                      VECTOR2I( pcbIUScale.mmToIU( 0.001 ), pcbIUScale.mmToIU( 0.001 ) ) );

        wxLogWarning( _( "Invalid zero-sized pad pinned to %s in\nfile: %s\nline: %d\noffset: %d" ),
                      wxT( "1µm" ), CurSource(), CurLineNumber(), CurOffset() );
    }

    return pad.release();
}


bool PCB_IO_KICAD_SEXPR_PARSER::parsePAD_option( PAD* aPad )
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
            // circle and rect.
            switch( token )
            {
                case T_circle:
                    aPad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
                    break;

                case T_rect:
                    aPad->SetAnchorPadShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
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
                aPad->SetCustomShapeInZoneOpt( PADSTACK::CUSTOM_SHAPE_ZONE_MODE::OUTLINE );
                break;

            case T_convexhull:
                aPad->SetCustomShapeInZoneOpt( PADSTACK::CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL );
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


void PCB_IO_KICAD_SEXPR_PARSER::parsePadstack( PAD* aPad )
{
    PADSTACK& padstack = aPad->Padstack();

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_mode:
            token = NextTok();

            switch( token )
            {
            case T_front_inner_back:
                padstack.SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
                break;

            case T_custom:
                padstack.SetMode( PADSTACK::MODE::CUSTOM );
                break;

            default:
                Expecting( "front_inner_back or custom" );
            }

            NeedRIGHT();
            break;

        case T_layer:
        {
            NextTok();
            PCB_LAYER_ID curLayer = UNDEFINED_LAYER;

            if( curText == "Inner" )
            {
                if( padstack.Mode() != PADSTACK::MODE::FRONT_INNER_BACK )
                {
                    THROW_IO_ERROR( wxString::Format( _( "Invalid padstack layer in\nfile: %s\n"
                                                         "line: %d\noffset: %d." ),
                                                      CurSource(), CurLineNumber(), CurOffset() ) );
                }

                curLayer = PADSTACK::INNER_LAYERS;
            }
            else
            {
                curLayer = lookUpLayer( m_layerIndices );
            }

            if( !IsCopperLayer( curLayer ) )
            {
                wxString error;
                error.Printf( _( "Invalid padstack layer '%s' in file '%s' at line %d, offset %d." ),
                              curText, CurSource().GetData(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {
                case T_shape:
                    token = NextTok();

                    switch( token )
                    {
                    case T_circle:
                        aPad->SetShape( curLayer, PAD_SHAPE::CIRCLE );
                        break;

                    case T_rect:
                        aPad->SetShape( curLayer, PAD_SHAPE::RECTANGLE );
                        break;

                    case T_oval:
                        aPad->SetShape( curLayer, PAD_SHAPE::OVAL );
                        break;

                    case T_trapezoid:
                        aPad->SetShape( curLayer, PAD_SHAPE::TRAPEZOID );
                        break;

                    case T_roundrect:
                        // Note: the shape can be PAD_SHAPE::ROUNDRECT or PAD_SHAPE::CHAMFERED_RECT
                        // (if chamfer parameters are found later in pad descr.)
                        aPad->SetShape( curLayer, PAD_SHAPE::ROUNDRECT );
                        break;

                    case T_custom:
                        aPad->SetShape( curLayer, PAD_SHAPE::CUSTOM );
                        break;

                    default:
                        Expecting( "circle, rectangle, roundrect, oval, trapezoid or custom" );
                    }

                    NeedRIGHT();
                    break;

                case T_size:
                {
                    VECTOR2I sz;
                    sz.x = parseBoardUnits( "width value" );
                    sz.y = parseBoardUnits( "height value" );
                    aPad->SetSize( curLayer, sz );
                    NeedRIGHT();
                    break;
                }

                case T_offset:
                {
                    VECTOR2I pt;
                    pt.x = parseBoardUnits( "drill offset x" );
                    pt.y = parseBoardUnits( "drill offset y" );
                    aPad->SetOffset( curLayer, pt );
                    NeedRIGHT();
                    break;
                }

                case T_rect_delta:
                {
                    VECTOR2I delta;
                    delta.x = parseBoardUnits( "rectangle delta width" );
                    delta.y = parseBoardUnits( "rectangle delta height" );
                    aPad->SetDelta( curLayer, delta );
                    NeedRIGHT();
                    break;
                }

                case T_roundrect_rratio:
                    aPad->SetRoundRectRadiusRatio( curLayer,
                                                   parseDouble( "roundrect radius ratio" ) );
                    NeedRIGHT();
                    break;

                case T_chamfer_ratio:
                {
                    double ratio = parseDouble( "chamfer ratio" );
                    aPad->SetChamferRectRatio( curLayer, ratio );

                    if( ratio > 0 )
                        aPad->SetShape( curLayer, PAD_SHAPE::CHAMFERED_RECT );

                    NeedRIGHT();
                    break;
                }

                case T_chamfer:
                {
                    int  chamfers = 0;
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
                            aPad->SetChamferPositions( curLayer, chamfers );
                            end_list = true;
                            break;

                        default:
                            Expecting( "chamfer_top_left chamfer_top_right chamfer_bottom_left or "
                                       "chamfer_bottom_right" );
                        }
                    }

                    if( end_list && chamfers != RECT_NO_CHAMFER )
                        aPad->SetShape( curLayer, PAD_SHAPE::CHAMFERED_RECT );

                    break;
                }

                case T_thermal_bridge_width:
                    padstack.ThermalSpokeWidth( curLayer ) =
                            parseBoardUnits( "thermal relief spoke width" );
                    NeedRIGHT();
                    break;

                case T_thermal_gap:
                    padstack.ThermalGap( curLayer ) = parseBoardUnits( "thermal relief gap value" );
                    NeedRIGHT();
                    break;

                case T_thermal_bridge_angle:
                    padstack.SetThermalSpokeAngle(
                            EDA_ANGLE( parseDouble( "thermal spoke angle" ), DEGREES_T ) );
                    NeedRIGHT();
                    break;

                case T_zone_connect:
                    padstack.ZoneConnection( curLayer ) = magic_enum::enum_cast<ZONE_CONNECTION>(
                            parseInt( "zone connection value" ) );
                    NeedRIGHT();
                    break;

                case T_clearance:
                    padstack.Clearance( curLayer ) = parseBoardUnits( "local clearance value" );
                    NeedRIGHT();
                    break;

                case T_tenting:
                {
                    auto [front, back] = parseFrontBackOptBool( true );
                    padstack.FrontOuterLayers().has_solder_mask = front;
                    padstack.BackOuterLayers().has_solder_mask = back;
                    break;
                }

                // TODO: refactor parsePAD_options to work on padstacks too
                case T_options:
                {
                     for( token = NextTok(); token != T_RIGHT; token = NextTok() )
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
                             // circle and rect.
                             switch( token )
                             {
                             case T_circle:
                                 padstack.SetAnchorShape( PAD_SHAPE::CIRCLE, curLayer );
                                 break;

                             case T_rect:
                                 padstack.SetAnchorShape( PAD_SHAPE::RECTANGLE, curLayer );
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
                             // TODO: m_customShapeInZoneMode is not per-layer at the moment
                             NeedRIGHT();
                             break;

                         default:
                             // Currently, because pad options is a moving target
                             // just skip unknown keywords
                             while( ( token = NextTok() ) != T_RIGHT )
                             {
                             }

                             break;
                         }
                     }

                     break;
                }

                // TODO: deduplicate with non-padstack parser
                case T_primitives:
                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                    {
                        if( token == T_LEFT )
                            token = NextTok();

                        switch( token )
                        {
                        case T_gr_arc:
                        case T_gr_line:
                        case T_gr_circle:
                        case T_gr_rect:
                        case T_gr_poly:
                        case T_gr_curve:
                            padstack.AddPrimitive( parsePCB_SHAPE( nullptr ), curLayer );
                            break;

                        case T_gr_bbox:
                        {
                            PCB_SHAPE* numberBox = parsePCB_SHAPE( nullptr );
                            numberBox->SetIsProxyItem();
                            padstack.AddPrimitive( numberBox, curLayer );
                            break;
                        }

                        case T_gr_vector:
                        {
                            PCB_SHAPE* spokeTemplate = parsePCB_SHAPE( nullptr );
                            spokeTemplate->SetIsProxyItem();
                            padstack.AddPrimitive( spokeTemplate, curLayer );
                            break;
                        }

                        default:
                            Expecting( "gr_line, gr_arc, gr_circle, gr_curve, gr_rect, gr_bbox or gr_poly" );
                            break;
                        }
                    }

                    break;

                default:
                    // Not strict-parsing padstack layers yet
                    continue;
                }
            }

            break;
        }

        default:
            Expecting( "mode or layer" );
            break;
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseGROUP_members( GROUP_INFO& aGroupInfo )
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


void PCB_IO_KICAD_SEXPR_PARSER::parseGROUP( BOARD_ITEM* aParent )
{
    wxCHECK_RET( CurTok() == T_group,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_GROUP." ) );

    T token;

    m_groupInfos.push_back( GROUP_INFO() );
    GROUP_INFO& groupInfo = m_groupInfos.back();
    groupInfo.parent = aParent;

    while( ( token = NextTok() ) != T_LEFT )
    {
        if( token == T_STRING )
            groupInfo.name = FromUTF8();
        else if( token == T_locked )
            groupInfo.locked = true;
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
         // From formats [20200811, 20231215), 'id' was used instead of 'uuid'
        case T_id:
        case T_uuid:
            NextTok();
            groupInfo.uuid = CurStrToKIID();
            NeedRIGHT();
            break;

        case T_lib_id:
        {
            token = NextTok();

            if( !IsSymbol( token ) && token != T_NUMBER )
                Expecting( "symbol|number" );

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
                    wxString msg = wxString::Format( _( "Group library link %s contains invalid character '%c'" ),
                                                     name,
                                                     name[bad_pos] );

                    THROW_PARSE_ERROR( msg, CurSource(), CurLine(), CurLineNumber(), CurOffset() );
                }

                THROW_PARSE_ERROR( _( "Invalid library ID" ), CurSource(), CurLine(), CurLineNumber(), CurOffset() );
            }

            NeedRIGHT();
            break;
        }

        case T_locked:
            groupInfo.locked = parseBool();
            NeedRIGHT();
            break;

        case T_members:
        {
            parseGROUP_members( groupInfo );
            break;
        }

        default:
            Expecting( "uuid, locked, lib_id, or members" );
        }
    }
}


void PCB_IO_KICAD_SEXPR_PARSER::parseGENERATOR( BOARD_ITEM* aParent )
{
    wxCHECK_RET( CurTok() == T_generated,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_GENERATOR." ) );

    T token;

    m_generatorInfos.push_back( GENERATOR_INFO() );
    GENERATOR_INFO& genInfo = m_generatorInfos.back();

    genInfo.layer = F_Cu;
    genInfo.parent = aParent;
    genInfo.properties = STRING_ANY_MAP( pcbIUScale.IU_PER_MM );

    NeedLEFT();
    token = NextTok();

    // For formats [20231007, 20231215), 'id' was used instead of 'uuid'
    if( token != T_uuid && token != T_id )
        Expecting( T_uuid );

    NextTok();
    genInfo.uuid = CurStrToKIID();
    NeedRIGHT();

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_type:
            NeedSYMBOL();
            genInfo.genType = FromUTF8();
            NeedRIGHT();
            break;

        case T_name:
            NeedSYMBOL();
            genInfo.name = FromUTF8();
            NeedRIGHT();
            break;

        case T_locked:
            token = NextTok();
            genInfo.locked = token == T_yes;
            NeedRIGHT();
            break;

        case T_layer:
            genInfo.layer = parseBoardItemLayer();
            NeedRIGHT();
            break;

        case T_members:
            parseGROUP_members( genInfo );
            break;

        default:
        {
            wxString pName = FromUTF8();
            T        tok1 = NextTok();

            switch( tok1 )
            {
            case T_yes:
            {
                genInfo.properties.emplace( pName, wxAny( true ) );
                NeedRIGHT();
                break;
            }
            case T_no:
            {
                genInfo.properties.emplace( pName, wxAny( false ) );
                NeedRIGHT();
                break;
            }
            case T_NUMBER:
            {
                double pValue = parseDouble();
                genInfo.properties.emplace( pName, wxAny( pValue ) );
                NeedRIGHT();
                break;
            }
            case T_STRING: // Quoted string
            {
                wxString pValue = FromUTF8();
                genInfo.properties.emplace( pName, pValue );
                NeedRIGHT();
                break;
            }
            case T_LEFT:
            {
                NeedSYMBOL();
                T tok2 = CurTok();

                switch( tok2 )
                {
                case T_xy:
                {
                    VECTOR2I pt;

                    pt.x = parseBoardUnits( "X coordinate" );
                    pt.y = parseBoardUnits( "Y coordinate" );

                    genInfo.properties.emplace( pName, wxAny( pt ) );
                    NeedRIGHT();
                    NeedRIGHT();

                    break;
                }
                case T_pts:
                {
                    SHAPE_LINE_CHAIN chain;

                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                        parseOutlinePoints( chain );

                    NeedRIGHT();

                    genInfo.properties.emplace( pName, wxAny( chain ) );
                    break;
                }
                default: Expecting( "xy or pts" );
                }

                break;
            }
            default: Expecting( "a number, symbol, string or (" );
            }

            break;
        }
        }
    }

    // Previous versions had bugs which could save ghost tuning patterns.  Ignore them.
    if( genInfo.genType == wxT( "tuning_pattern" ) && genInfo.memberUuids.empty() )
        m_generatorInfos.pop_back();
}


PCB_ARC* PCB_IO_KICAD_SEXPR_PARSER::parseARC()
{
    wxCHECK_MSG( CurTok() == T_arc, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as ARC." ) );

    VECTOR2I pt;
    T        token;

    std::unique_ptr<PCB_ARC> arc = std::make_unique<PCB_ARC>( m_board );

    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        // Legacy locked
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
            NeedRIGHT();
            break;

        case T_mid:
            pt.x = parseBoardUnits( "mid x" );
            pt.y = parseBoardUnits( "mid y" );
            arc->SetMid( pt );
            NeedRIGHT();
            break;

        case T_end:
            pt.x = parseBoardUnits( "end x" );
            pt.y = parseBoardUnits( "end y" );
            arc->SetEnd( pt );
            NeedRIGHT();
            break;

        case T_width:
            arc->SetWidth( parseBoardUnits( "width" ) );
            NeedRIGHT();
            break;

        case T_layer:
            arc->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_layers:
            arc->SetLayerSet( parseLayersForCuItemWithSoldermask() );
            break;

        case T_solder_mask_margin:
            arc->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();
            break;

        case T_net:
            if( !arc->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
            {
                wxLogError( _( "Invalid net ID in\nfile: %s\nline: %d\noffset: %d." ),
                            CurSource(), CurLineNumber(), CurOffset() );
            }
            NeedRIGHT();
            break;

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( arc->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            parseHex();
            NeedRIGHT();
            break;

        case T_locked:
            arc->SetLocked( parseMaybeAbsentBool( true ) );
            break;

        default:
            Expecting( "start, mid, end, width, layer, solder_mask_margin, net, tstamp, uuid, "
                       "or status" );
        }
    }

    if( !IsCopperLayer( arc->GetLayer() ) )
    {
        // No point in asserting; these usually come from hand-edited boards
        return nullptr;
    }

    return arc.release();
}


PCB_TRACK* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TRACK()
{
    wxCHECK_MSG( CurTok() == T_segment, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TRACK." ) );

    VECTOR2I pt;
    T        token;

    std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( m_board );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        // Legacy locked flag
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
            NeedRIGHT();
            break;

        case T_end:
            pt.x = parseBoardUnits( "end x" );
            pt.y = parseBoardUnits( "end y" );
            track->SetEnd( pt );
            NeedRIGHT();
            break;

        case T_width:
            track->SetWidth( parseBoardUnits( "width" ) );
            NeedRIGHT();
            break;

        case T_layer:
            track->SetLayer( parseBoardItemLayer() );
            NeedRIGHT();
            break;

        case T_layers:
            track->SetLayerSet( parseLayersForCuItemWithSoldermask() );
            break;

        case T_solder_mask_margin:
            track->SetLocalSolderMaskMargin( parseBoardUnits( "local solder mask margin value" ) );
            NeedRIGHT();
            break;

        case T_net:
            if( !track->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
            {
                wxLogError( _( "Invalid net ID in\nfile: '%s'\nline: %d\noffset: %d." ),
                            CurSource(), CurLineNumber(), CurOffset() );
            }
            NeedRIGHT();
            break;

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( track->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            parseHex();
            NeedRIGHT();
            break;

        case T_locked:
            track->SetLocked( parseMaybeAbsentBool( true ) );
            break;

        default:
            Expecting( "start, end, width, layer, solder_mask_margin, net, tstamp, uuid, "
                       "or locked" );
        }
    }

    if( !IsCopperLayer( track->GetLayer() ) )
    {
        // No point in asserting; these usually come from hand-edited boards
        return nullptr;
    }

    return track.release();
}


PCB_VIA* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_VIA()
{
    wxCHECK_MSG( CurTok() == T_via, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_VIA." ) );

    VECTOR2I pt;
    T        token;

    std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( m_board );

    // File format default is no-token == no-feature.
    via->Padstack().SetUnconnectedLayerMode( PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        // Legacy locked
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
            via->SetWidth( PADSTACK::ALL_LAYERS, parseBoardUnits( "via width" ) );
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
            layer1 = lookUpLayer( m_layerIndices );
            NextTok();
            layer2 = lookUpLayer( m_layerIndices );
            via->SetLayerPair( layer1, layer2 );

            if( layer1 == UNDEFINED_LAYER || layer2 == UNDEFINED_LAYER )
                Expecting( "layer name" );

            NeedRIGHT();
            break;
        }

        case T_net:
            if( !via->SetNetCode( getNetCode( parseInt( "net number" ) ), /* aNoAssert */ true ) )
            {
                wxLogError( _( "Invalid net ID in\nfile: %s\nline: %d\noffset: %d" ),
                            CurSource(), CurLineNumber(), CurOffset() );
            }

            NeedRIGHT();
            break;

        case T_remove_unused_layers:
        {
            bool remove = parseMaybeAbsentBool( true );
            via->SetRemoveUnconnected( remove );
            break;
        }

        case T_keep_end_layers:
        {
            bool keep = parseMaybeAbsentBool( true );
            via->SetKeepStartEnd( keep );
            break;
        }

        case T_start_end_only:
        {
            if( parseMaybeAbsentBool( true ) )
                via->Padstack().SetUnconnectedLayerMode( PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY );

            break;
        }

        case T_zone_layer_connections:
        {
            // Ensure only copper layers are stored int ZoneLayerOverride array
            LSET cuLayers = via->GetLayerSet() & LSET::AllCuMask();

            for( PCB_LAYER_ID layer : cuLayers )
            {
                via->SetZoneLayerOverride( layer, ZLO_FORCE_NO_ZONE_CONNECTION );
            }

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                PCB_LAYER_ID layer = lookUpLayer( m_layerIndices );

                if( !IsCopperLayer( layer ) )
                    Expecting( "copper layer name" );

                via->SetZoneLayerOverride( layer, ZLO_FORCE_FLASHED );
            }
        }
            break;

        case T_padstack:
            parseViastack( via.get() );
            break;

        case T_teardrops:
            parseTEARDROP_PARAMETERS( &via->GetTeardropParams() );
            break;

        case T_tenting:
        {
            auto [front, back] = parseFrontBackOptBool( true );
            via->Padstack().FrontOuterLayers().has_solder_mask = front;
            via->Padstack().BackOuterLayers().has_solder_mask = back;
            break;
        }
        case T_covering:
        {
            auto [front, back] = parseFrontBackOptBool();
            via->Padstack().FrontOuterLayers().has_covering = front;
            via->Padstack().BackOuterLayers().has_covering = back;
            break;
        }
        case T_plugging:
        {
            auto [front, back] = parseFrontBackOptBool();
            via->Padstack().FrontOuterLayers().has_plugging = front;
            via->Padstack().BackOuterLayers().has_plugging = back;
            break;
        }
        case T_filling:
        {
            via->Padstack().Drill().is_filled = parseOptBool();
            NeedRIGHT();
            break;
        }
        case T_capping:
        {
            via->Padstack().Drill().is_capped = parseOptBool();
            NeedRIGHT();
            break;
        }

        case T_tstamp:
        case T_uuid:
            NextTok();
            const_cast<KIID&>( via->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        // We continue to parse the status field but it is no longer written
        case T_status:
            parseHex();
            NeedRIGHT();
            break;

        case T_locked:
            via->SetLocked( parseMaybeAbsentBool( true ) );
            break;

        case T_free:
            via->SetIsFree( parseMaybeAbsentBool( true ) );
            break;

        default:
            Expecting( "blind, micro, at, size, drill, layers, net, free, tstamp, uuid, status or "
                       "teardrops" );
        }
    }

    return via.release();
}


std::pair<std::optional<bool>, std::optional<bool>>
PCB_IO_KICAD_SEXPR_PARSER::parseFrontBackOptBool( bool aLegacy )
{
    T token = NextTok();

    std::optional<bool> front{ std::nullopt };
    std::optional<bool> back{ std::nullopt };

    if( token != T_LEFT && aLegacy )
    {
        // legacy format for tenting.
        if( token == T_front || token == T_back || token == T_none )
        {
            while( token != T_RIGHT )
            {
                if( token == T_front )
                {
                    front = true;
                }
                else if( token == T_back )
                {
                    back = true;
                }
                else if( token == T_none )
                {
                    front.reset();
                    back.reset();
                }
                else
                {
                    Expecting( "front, back or none" );
                }

                token = NextTok();
            }

            return { front, back };
        }
    }

    while( token != T_RIGHT )
    {
        if( token != T_LEFT )
            Expecting( "(" );

        token = NextTok();

        if( token == T_front )
            front = parseOptBool();
        else if( token == T_back )
            back = parseOptBool();
        else
            Expecting( "front or back" );

        NeedRIGHT();

        token = NextTok();
    }

    return { front, back };
}


void PCB_IO_KICAD_SEXPR_PARSER::parseViastack( PCB_VIA* aVia )
{
    PADSTACK& padstack = aVia->Padstack();

    for( T token = NextTok(); token != T_RIGHT; token = NextTok() )
    {
        if( token != T_LEFT )
            Expecting( T_LEFT );

        token = NextTok();

        switch( token )
        {
        case T_mode:
            token = NextTok();

            switch( token )
            {
            case T_front_inner_back:
                padstack.SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
                break;

            case T_custom:
                padstack.SetMode( PADSTACK::MODE::CUSTOM );
                break;

            default:
                Expecting( "front_inner_back or custom" );
            }

            NeedRIGHT();
            break;

        case T_layer:
        {
            NextTok();
            PCB_LAYER_ID curLayer = UNDEFINED_LAYER;

            if( curText == "Inner" )
            {
                if( padstack.Mode() != PADSTACK::MODE::FRONT_INNER_BACK )
                {
                    THROW_IO_ERROR( wxString::Format( _( "Invalid padstack layer in\nfile: %s\n"
                                                         "line: %d\noffset: %d." ),
                                                      CurSource(), CurLineNumber(), CurOffset() ) );
                }

                curLayer = PADSTACK::INNER_LAYERS;
            }
            else
            {
                curLayer = lookUpLayer( m_layerIndices );
            }

            if( !IsCopperLayer( curLayer ) )
            {
                wxString error;
                error.Printf( _( "Invalid padstack layer '%s' in file '%s' at line %d, offset %d." ),
                              curText, CurSource().GetData(), CurLineNumber(), CurOffset() );
                THROW_IO_ERROR( error );
            }

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                switch( token )
                {

                case T_size:
                {
                    int diameter = parseBoardUnits( "via width" );
                    padstack.SetSize( { diameter, diameter }, curLayer );
                    NeedRIGHT();
                    break;
                }

                default:
                    // Currently only supporting custom via diameter per layer, not other properties
                    Expecting( "size" );
                }
            }

            break;
        }

        default:
            Expecting( "mode or layer" );
            break;
        }
    }
}


ZONE* PCB_IO_KICAD_SEXPR_PARSER::parseZONE( BOARD_ITEM_CONTAINER* aParent )
{
    wxCHECK_MSG( CurTok() == T_zone, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as ZONE." ) );

    ZONE_BORDER_DISPLAY_STYLE hatchStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;

    int      hatchPitch = ZONE::GetDefaultHatchPitch();
    T        token;
    int      tmp;
    wxString netnameFromfile;    // the zone net name find in file

    // bigger scope since each filled_polygon is concatenated in here
    std::map<PCB_LAYER_ID, SHAPE_POLY_SET> pts;
    std::map<PCB_LAYER_ID, std::vector<SEG>> legacySegs;
    PCB_LAYER_ID filledLayer;
    bool         addedFilledPolygons = false;

    // This hasn't been supported since V6 or so, but we only stopped writing out the token
    // in V10.
    bool isStrokedFill = m_requiredVersion < 20250210;

    std::unique_ptr<ZONE> zone = std::make_unique<ZONE>( aParent );

    zone->SetAssignedPriority( 0 );

    // This is the default for board files:
    zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::ALWAYS );

    for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
    {
        // legacy locked
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
            {
                wxLogError( _( "Invalid net ID in\nfile: %s;\nline: %d\noffset: %d." ),
                            CurSource(), CurLineNumber(), CurOffset() );
            }

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

        case T_property:
            parseZoneLayerProperty( zone->LayerProperties() );
            break;

        case T_tstamp:
        case T_uuid:
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
            zone->SetAssignedPriority( parseInt( "zone priority" ) );
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
            // A new zone fill strategy was added in v6, so we need to know if we're parsing
            // a zone that was filled before that. Note that the change was implemented as
            // a new parameter, so we need to check for the  presence of filled_areas_thickness
            // instead of just its value.

            if( !parseBool() )
                isStrokedFill = false;

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

                    switch( token )
                    {
                    case T_hatch:
                        zone->SetFillMode( ZONE_FILL_MODE::HATCH_PATTERN );
                        break;

                    case T_segment: // deprecated, convert to polygons
                    case T_polygon:
                    default:
                        zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
                        break;
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
                {
                    EDA_ANGLE orientation( parseDouble( T_hatch_orientation ), DEGREES_T );
                    zone->SetHatchOrientation( orientation );
                    NeedRIGHT();
                    break;
                }

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
                    ignore_unused( parseInt( "arc segment count" ) );
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
                    zone->SetMinIslandArea( area * pcbIUScale.IU_PER_MM );
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

        case T_placement:
            zone->SetIsRuleArea( true );

            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_sheetname:
                {
                    zone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::SHEETNAME );
                    NeedSYMBOL();
                    zone->SetPlacementAreaSource( FromUTF8() );
                    break;
                }
                case T_component_class:
                {
                    zone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::COMPONENT_CLASS );
                    NeedSYMBOL();
                    zone->SetPlacementAreaSource( FromUTF8() );
                    break;
                }
                case T_group:
                {
                    zone->SetPlacementAreaSourceType( PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
                    NeedSYMBOL();
                    zone->SetPlacementAreaSource( FromUTF8() );
                    break;
                }
                case T_enabled:
                {
                    token = NextTok();

                    if( token == T_yes )
                        zone->SetPlacementAreaEnabled( true );
                    else if( token == T_no )
                        zone->SetPlacementAreaEnabled( false );
                    else
                        Expecting( "yes or no" );

                    break;
                }
                default:
                {
                    Expecting( "enabled, sheetname, component_class, or group" );
                    break;
                }
                }

                NeedRIGHT();
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

                    zone->SetDoNotAllowZoneFills( token == T_not_allowed );
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
            break;
        }

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
                    // for legacy, single-layer zones
                    filledLayer = zone->GetFirstLayer();
                }

                bool island = false;

                if( token == T_island )
                {
                    island = parseMaybeAbsentBool( true );
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
            // Legacy segment fill

            for( token = NextTok(); token != T_RIGHT; token = NextTok() )
            {
                if( token != T_LEFT )
                    Expecting( T_LEFT );

                token = NextTok();

                if( token != T_pts )
                    Expecting( T_pts );

                // Legacy zones only had one layer
                filledLayer = zone->GetFirstLayer();

                SEG fillSegment;

                fillSegment.A = parseXY();
                fillSegment.B = parseXY();

                legacySegs[filledLayer].push_back( fillSegment );

                NeedRIGHT();
            }

            break;
        }

        case T_name:
            NextTok();
            zone->SetZoneName( FromUTF8() );
            NeedRIGHT();
            break;

        case T_attr:
            for( token = NextTok();  token != T_RIGHT;  token = NextTok() )
            {
                if( token == T_LEFT )
                    token = NextTok();

                switch( token )
                {
                case T_teardrop:
                    for( token = NextTok(); token != T_RIGHT; token = NextTok() )
                    {
                        if( token == T_LEFT )
                            token = NextTok();

                        switch( token )
                        {
                        case T_type:
                            token = NextTok();

                            if( token == T_padvia )
                                zone->SetTeardropAreaType( TEARDROP_TYPE::TD_VIAPAD );
                            else if( token == T_track_end )
                                zone->SetTeardropAreaType( TEARDROP_TYPE::TD_TRACKEND );
                            else
                                Expecting( "padvia or track_end" );

                            NeedRIGHT();
                            break;

                        default:
                            Expecting( "type" );
                        }
                    }

                    break;

                default:
                    Expecting( "teardrop" );
                }
            }
            break;

        case T_locked:
            zone->SetLocked( parseBool() );
            NeedRIGHT();
            break;

        default:
            Expecting( "net, layer/layers, tstamp, hatch, priority, connect_pads, min_thickness, "
                       "fill, polygon, filled_polygon, fill_segments, attr, locked, uuid, or name" );
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
        if( isStrokedFill && !zone->GetIsRuleArea() )
        {
            if( m_showLegacy5ZoneWarning )
            {
                wxLogWarning( _( "Legacy zone fill strategy is not supported anymore.\n"
                                 "Zone fills will be converted on best-effort basis." ) );

                m_showLegacy5ZoneWarning = false;
            }

            if( zone->GetMinThickness() > 0 )
            {
                for( auto& [layer, polyset] : pts )
                {
                    polyset.InflateWithLinkedHoles( zone->GetMinThickness() / 2,
                                                    CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                                    ARC_HIGH_DEF / 2 );
                }
            }
        }

        for( auto& [layer, polyset] : pts )
            zone->SetFilledPolysList( layer, polyset );

        zone->CalculateFilledArea();
    }
    else if( legacySegs.size() > 0 )
    {
        // No polygons, just segment fill?
        // Note RFB: This code might be removed if turns out this never existed for sexpr file
        // format or otherwise we should add a test case to the qa folder

        if( m_showLegacySegmentZoneWarning )
        {
            wxLogWarning( _( "The legacy segment zone fill mode is no longer supported.\n"
                             "Zone fills will be converted on a best-effort basis." ) );

            m_showLegacySegmentZoneWarning = false;
        }


        for( const auto& [layer, segments] : legacySegs )
        {
            SHAPE_POLY_SET layerFill;

            if( zone->HasFilledPolysForLayer( layer ) )
                layerFill = SHAPE_POLY_SET( *zone->GetFill( layer ) );

            for( const auto& seg : segments )
            {
                SHAPE_POLY_SET segPolygon;

                TransformOvalToPolygon( segPolygon, seg.A, seg.B, zone->GetMinThickness(),
                                        ARC_HIGH_DEF, ERROR_OUTSIDE );

                layerFill.BooleanAdd( segPolygon );
            }


            zone->SetFilledPolysList( layer, layerFill );
            zone->CalculateFilledArea();
        }
    }


    // Ensure keepout and non copper zones do not have a net
    // (which have no sense for these zones)
    // the netcode 0 is used for these zones
    bool zone_has_net = zone->IsOnCopperLayer() && !zone->GetIsRuleArea();

    if( !zone_has_net )
        zone->SetNetCode( NETINFO_LIST::UNCONNECTED );

    // Ensure the zone net name is valid, and matches the net code, for copper zones
    if( zone_has_net
        && ( !zone->GetNet() || zone->GetNet()->GetNetname() != netnameFromfile ) )
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
            m_board->Add( net, ADD_MODE::INSERT, true );

            // Store the new code mapping
            pushValueIntoMap( newnetcode, net->GetNetCode() );

            // and update the zone netcode
            zone->SetNetCode( net->GetNetCode() );
        }
    }

    if( zone->IsTeardropArea() && m_requiredVersion < 20230517 )
        m_board->SetLegacyTeardrops( true );

    // Clear flags used in zone edition:
    zone->SetNeedRefill( false );

    return zone.release();
}


PCB_TARGET* PCB_IO_KICAD_SEXPR_PARSER::parsePCB_TARGET()
{
    wxCHECK_MSG( CurTok() == T_target, nullptr,
                 wxT( "Cannot parse " ) + GetTokenString( CurTok() ) + wxT( " as PCB_TARGET." ) );

    VECTOR2I pt;
    T        token;

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
        case T_uuid:
            NextTok();
            const_cast<KIID&>( target->m_Uuid ) = CurStrToKIID();
            NeedRIGHT();
            break;

        default:
            Expecting( "x, plus, at, size, width, layer, uuid, or tstamp" );
        }
    }

    return target.release();
}


KIID PCB_IO_KICAD_SEXPR_PARSER::CurStrToKIID()
{
    KIID aId;
    std::string idStr( CurStr() );

    // Older files did not quote UUIDs
    if( *idStr.begin() == '"' && *idStr.rbegin() == '"' )
        idStr = idStr.substr( 1, idStr.length() - 1 );

    if( m_appendToExisting )
    {
        aId = KIID();
        m_resetKIIDMap.insert( std::make_pair( idStr, aId ) );
    }
    else
    {
        aId = KIID( idStr );
    }

    return aId;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_zone.cpp
 * @brief Implementation of class to handle copper zones.
 */

#include "fctsys.h"
#include "wxstruct.h"
#include "trigo.h"
#include "class_pcb_screen.h"
#include "class_drawpanel.h"
#include "kicad_string.h"
#include "pcbcommon.h"
#include "colors_selection.h"
#include "richio.h"
#include "macros.h"
#include "wxBasePcbFrame.h"

#include "protos.h"
#include "class_board.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "zones.h"


ZONE_CONTAINER::ZONE_CONTAINER( BOARD* parent ) :
    BOARD_CONNECTED_ITEM( parent, PCB_ZONE_AREA_T )
{
    m_NetCode = -1;                             // Net number for fast comparisons
    m_CornerSelection = -1;
    m_IsFilled = false;                         // fill status : true when the zone is filled
    m_FillMode = 0;                             // How to fill areas: 0 = use filled polygons, != 0 fill with segments
    smoothedPoly = NULL;
    cornerSmoothingType = ZONE_SETTING::SMOOTHING_NONE;
    cornerRadius = 0;
    utility    = 0;                             // flags used in polygon calculations
    utility2   = 0;                             // flags used in polygon calculations
    m_Poly     = new CPolyLine();               // Outlines
    g_Zone_Default_Setting.ExportSetting( *this );
}


ZONE_CONTAINER::~ZONE_CONTAINER()
{
    delete m_Poly;
    m_Poly = NULL;
}


bool ZONE_CONTAINER::UnFill()
{
    bool change = ( m_FilledPolysList.size() > 0 ) || ( m_FillSegmList.size() > 0 );

    m_FilledPolysList.clear();
    m_FillSegmList.clear();
    m_IsFilled = false;

    return change;
}


const wxPoint ZONE_CONTAINER::GetPosition() const
{
    return m_Poly? GetCornerPosition( 0 ) : wxPoint( 0, 0 );
}


void ZONE_CONTAINER::SetPosition( const wxPoint& aPos ) {}


void ZONE_CONTAINER::SetNet( int aNetCode )
{
    m_NetCode = aNetCode;

    if( aNetCode < 0 )
        return;

    BOARD* board = GetBoard();

    if( board )
    {
        NETINFO_ITEM* net = board->FindNet( aNetCode );

        if( net )
            m_Netname = net->GetNetname();
        else
            m_Netname.Empty();
    }
    else
    {
        m_Netname.Empty();
    }
}


bool ZONE_CONTAINER::Save( FILE* aFile ) const
{
    unsigned item_pos;
    int      ret;
    unsigned corners_count = m_Poly->corner.size();
    int      outline_hatch;
    char     padoption;

    fprintf( aFile, "$CZONE_OUTLINE\n" );

    // Save the outline main info
    ret = fprintf( aFile, "ZInfo %8.8lX %d %s\n",
                   m_TimeStamp, m_NetCode,
                   EscapedUTF8( m_Netname ).c_str() );

    if( ret < 3 )
        return false;

    // Save the outline layer info
    ret = fprintf( aFile, "ZLayer %d\n", m_Layer );

    if( ret < 1 )
        return false;

    // Save the outline aux info
    switch( m_Poly->GetHatchStyle() )
    {
    default:
    case CPolyLine::NO_HATCH:
        outline_hatch = 'N';
        break;

    case CPolyLine::DIAGONAL_EDGE:
        outline_hatch = 'E';
        break;

    case CPolyLine::DIAGONAL_FULL:
        outline_hatch = 'F';
        break;
    }

    ret = fprintf( aFile, "ZAux %d %c\n", corners_count, outline_hatch );

    if( ret < 2 )
        return false;

    // Save pad option and clearance
    switch( m_PadOption )
    {
    default:
    case PAD_IN_ZONE:
        padoption = 'I';
        break;

    case THERMAL_PAD:
        padoption = 'T';
        break;

    case PAD_NOT_IN_ZONE:
        padoption = 'X';
        break;
    }

    ret = fprintf( aFile, "ZClearance %d %c\n", m_ZoneClearance, padoption );

    if( ret < 2 )
        return false;

    ret = fprintf( aFile, "ZMinThickness %d\n", m_ZoneMinThickness );

    if( ret < 1 )
        return false;

    ret = fprintf( aFile,
                   "ZOptions %d %d %c %d %d\n",
                   m_FillMode,
                   m_ArcToSegmentsCount,
                   m_IsFilled ? 'S' : 'F',
                   m_ThermalReliefGap,
                   m_ThermalReliefCopperBridge );

    if( ret < 3 )
        return false;

    ret = fprintf( aFile,
                   "ZSmoothing %d %d\n",
                   cornerSmoothingType, cornerRadius );

    if( ret < 2 )
        return false;

    // Save the corner list
    for( item_pos = 0; item_pos < corners_count; item_pos++ )
    {
        ret = fprintf( aFile, "ZCorner %d %d %d\n",
                       m_Poly->corner[item_pos].x, m_Poly->corner[item_pos].y,
                       m_Poly->corner[item_pos].end_contour );

        if( ret < 3 )
            return false;
    }

    // Save the PolysList
    if( m_FilledPolysList.size() )
    {
        fprintf( aFile, "$POLYSCORNERS\n" );

        for( unsigned ii = 0; ii < m_FilledPolysList.size(); ii++ )
        {
            const CPolyPt* corner = &m_FilledPolysList[ii];
            ret = fprintf( aFile,
                           "%d %d %d %d\n",
                           corner->x,
                           corner->y,
                           corner->end_contour,
                           corner->utility );

            if( ret < 4 )
                return false;
        }

        fprintf( aFile, "$endPOLYSCORNERS\n" );
    }

    // Save the filling segments list
    if( m_FillSegmList.size() )
    {
        fprintf( aFile, "$FILLSEGMENTS\n" );

        for( unsigned ii = 0; ii < m_FillSegmList.size(); ii++ )
        {
            ret = fprintf( aFile, "%d %d %d %d\n",
                           m_FillSegmList[ii].m_Start.x, m_FillSegmList[ii].m_Start.y,
                           m_FillSegmList[ii].m_End.x, m_FillSegmList[ii].m_End.y );

            if( ret < 4 )
                return false;
        }

        fprintf( aFile, "$endFILLSEGMENTS\n" );
    }

    fprintf( aFile, "$endCZONE_OUTLINE\n" );

    return true;
}


int ZONE_CONTAINER::ReadDescr( LINE_READER* aReader )
{
    char* Line, * text;
    char  netname_buffer[1024];
    int   ret;
    int   outline_hatch = CPolyLine::NO_HATCH;
    bool  error = false, has_corner = false;

    netname_buffer[0] = 0;

    while( aReader->ReadLine() )
    {
        Line = aReader->Line();

        if( strnicmp( Line, "ZCorner", 7 ) == 0 ) // new corner found
        {
            int x;
            int y;
            int flag;

            text = Line + 7;
            ret  = sscanf( text, "%d %d %d", &x, &y, &flag );

            if( ret < 3 )
            {
                error = true;
            }
            else
            {
                if( !has_corner )
                    m_Poly->Start( m_Layer, x, y, outline_hatch );
                else
                    AppendCorner( wxPoint( x, y ) );

                has_corner = true;

                if( flag )
                    m_Poly->Close();
            }
        }
        else if( strnicmp( Line, "ZInfo", 5 ) == 0 )   // general info found
        {
            int ts;
            int netcode;

            text = Line + 5;
            ret  = sscanf( text, "%X %d %s", &ts, &netcode, netname_buffer );

            if( ret < 3 )
            {
                error = true;
            }
            else
            {
                m_TimeStamp = ts;
                m_NetCode   = netcode;
                ReadDelimitedText( netname_buffer, netname_buffer, 1024 );
                m_Netname = FROM_UTF8( netname_buffer );
            }
        }
        else if( strnicmp( Line, "ZLayer", 6 ) == 0 )  // layer found
        {
            int x;

            text = Line + 6;
            ret  = sscanf( text, "%d", &x );

            if( ret < 1 )
                error = true;
            else
                m_Layer = x;
        }
        else if( strnicmp( Line, "ZAux", 4 ) == 0 )    // aux info found
        {
            int  x;
            char hopt[10];

            text = Line + 4;
            ret  = sscanf( text, "%d %c", &x, hopt );

            if( ret < 2 )
            {
                error = true;
            }
            else
            {
                switch( hopt[0] )
                {
                case 'n':
                case 'N':
                    outline_hatch = CPolyLine::NO_HATCH;
                    break;

                case 'e':
                case 'E':
                    outline_hatch = CPolyLine::DIAGONAL_EDGE;
                    break;

                case 'f':
                case 'F':
                    outline_hatch = CPolyLine::DIAGONAL_FULL;
                    break;
                }
            }
            /* Set hatch mode later, after reading outlines corners data */
        }
        else if( strnicmp( Line, "ZSmoothing", 10 ) == 0 )
        {
            int tempSmoothingType;
            int tempCornerRadius;
            text = Line + 10;
            ret  = sscanf( text, "%d %d", &tempSmoothingType, &tempCornerRadius );

            if( ret < 2 )
                return false;

            if( tempSmoothingType >= ZONE_SETTING::SMOOTHING_LAST)
                return false;

            if( tempSmoothingType < 0 )
                return false;

            cornerSmoothingType = tempSmoothingType;
            SetCornerRadius( tempCornerRadius );
        }
        else if( strnicmp( Line, "ZOptions", 8 ) == 0 )    // Options info found
        {
            int  fillmode = 1;
            int  arcsegmentcount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;
            char fillstate = 'F';
            text = Line + 8;
            ret  = sscanf( text, "%d %d %c %d %d", &fillmode, &arcsegmentcount, &fillstate,
                           &m_ThermalReliefGap, &m_ThermalReliefCopperBridge );

            if( ret < 1 )  // Must find 1 or more args.
                return false;
            else
                m_FillMode = fillmode ? 1 : 0;

            if( arcsegmentcount >= ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF )
                m_ArcToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF;

            m_IsFilled = (fillstate == 'S') ? true : false;
        }
        else if( strnicmp( Line, "ZClearance", 10 ) == 0 ) // Clearance and pad options info found
        {
            int  clearance = 200;
            char padoption;
            text = Line + 10;
            ret  = sscanf( text, "%d %1c", &clearance, &padoption );

            if( ret < 2 )
            {
                error = true;
            }
            else
            {
                m_ZoneClearance = clearance;

                switch( padoption )
                {
                case 'i':
                case 'I':
                    m_PadOption = PAD_IN_ZONE;
                    break;

                case 't':
                case 'T':
                    m_PadOption = THERMAL_PAD;
                    break;

                case 'x':
                case 'X':
                    m_PadOption = PAD_NOT_IN_ZONE;
                    break;
                }
            }
        }
        else if( strnicmp( Line, "ZMinThickness", 13 ) == 0 )    // Min Thickness info found
        {
            int thickness;
            text = Line + 13;
            ret  = sscanf( text, "%d", &thickness );

            if( ret < 1 )
                error = true;
            else
                m_ZoneMinThickness = thickness;
        }
        else if( strnicmp( Line, "$POLYSCORNERS", 13 ) == 0  )  // Read the PolysList (polygons used for fill areas in the zone)
        {
            while( aReader->ReadLine() )
            {
                Line = aReader->Line();

                if( strnicmp( Line, "$endPOLYSCORNERS", 4 ) == 0  )
                    break;

                CPolyPt corner;
                int     end_contour, utility;
                utility = 0;
                ret     = sscanf( Line,
                                  "%d %d %d %d",
                                  &corner.x,
                                  &corner.y,
                                  &end_contour,
                                  &utility );
                if( ret < 4 )
                    return false;

                corner.end_contour = end_contour ? true : false;
                corner.utility     = utility;
                m_FilledPolysList.push_back( corner );
            }
        }
        else if( strnicmp( Line, "$FILLSEGMENTS", 13 ) == 0  )
        {
            SEGMENT segm;
            while( aReader->ReadLine() )
            {
                Line = aReader->Line();

                if( strnicmp( Line, "$endFILLSEGMENTS", 4 ) == 0  )
                    break;

                ret = sscanf( Line,
                              "%d %d %d %d",
                              &segm.m_Start.x,
                              &segm.m_Start.y,
                              &segm.m_End.x,
                              &segm.m_End.y );
                if( ret < 4 )
                    return false;

                m_FillSegmList.push_back( segm );
            }
        }
        else if( strnicmp( Line, "$end", 4 ) == 0 )    // end of description
        {
            break;
        }
    }

    if( !IsOnCopperLayer() )
    {
        m_FillMode = 0;
        SetNet( 0 );
    }

    /* Set hatch here, when outlines corners are read */
    m_Poly->SetHatch( outline_hatch );

    return error ? 0 : 1;
}


void ZONE_CONTAINER::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int aDrawMode, const wxPoint& offset )
{
    if( DC == NULL )
        return;

    wxPoint seg_start, seg_end;
    int     curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;

    BOARD*  brd   = GetBoard();
    int     color = brd->GetLayerColor( m_Layer );

    if( brd->IsLayerVisible( m_Layer ) == false && ( color & HIGHLIGHT_FLAG ) != HIGHLIGHT_FLAG )
        return;

    GRSetDrawMode( DC, aDrawMode );

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( aDrawMode & GR_HIGHLIGHT )
    {
        if( aDrawMode & GR_AND )
            color &= ~HIGHLIGHT_FLAG;
        else
            color |= HIGHLIGHT_FLAG;
    }

    if( color & HIGHLIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    SetAlpha( &color, 150 );

    // draw the lines
    int i_start_contour = 0;
    std::vector<wxPoint> lines;
    lines.reserve( (GetNumCorners() * 2) + 2 );

    for( int ic = 0; ic < GetNumCorners(); ic++ )
    {
        seg_start = GetCornerPosition( ic ) + offset;

        if( m_Poly->corner[ic].end_contour == false && ic < GetNumCorners() - 1 )
        {
            seg_end = GetCornerPosition( ic + 1 ) + offset;
        }
        else
        {
            seg_end = GetCornerPosition( i_start_contour ) + offset;
            i_start_contour = ic + 1;
        }

        lines.push_back( seg_start );
        lines.push_back( seg_end );
    }

    GRLineArray( &panel->m_ClipBox, DC, lines, 0, color );

    // draw hatches
    lines.clear();
    lines.reserve( (m_Poly->m_HatchLines.size() * 2) + 2 );

    for( unsigned ic = 0; ic < m_Poly->m_HatchLines.size(); ic++ )
    {
        seg_start.x = m_Poly->m_HatchLines[ic].xi + offset.x;
        seg_start.y = m_Poly->m_HatchLines[ic].yi + offset.y;
        seg_end.x   = m_Poly->m_HatchLines[ic].xf + offset.x;
        seg_end.y   = m_Poly->m_HatchLines[ic].yf + offset.y;
        lines.push_back( seg_start );
        lines.push_back( seg_end );
    }

    GRLineArray( &panel->m_ClipBox, DC, lines, 0, color );
}


void ZONE_CONTAINER::DrawFilledArea( EDA_DRAW_PANEL* panel,
                                     wxDC* DC, int aDrawMode, const wxPoint& offset )
{
    static vector <char>    CornersTypeBuffer;
    static vector <wxPoint> CornersBuffer;

    // outline_mode is false to show filled polys,
    // and true to show polygons outlines only (test and debug purposes)
    bool outline_mode = DisplayOpt.DisplayZonesMode == 2 ? true : false;

    if( DC == NULL )
        return;

    if( DisplayOpt.DisplayZonesMode == 1 )     // Do not show filled areas
        return;

    if( m_FilledPolysList.size() == 0 )  // Nothing to draw
        return;

    BOARD* brd = GetBoard();
    int    curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    int    color = brd->GetLayerColor( m_Layer );

    if( brd->IsLayerVisible( m_Layer ) == false && ( color & HIGHLIGHT_FLAG ) != HIGHLIGHT_FLAG )
        return;

    GRSetDrawMode( DC, aDrawMode );

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    if( aDrawMode & GR_HIGHLIGHT )
    {
        if( aDrawMode & GR_AND )
            color &= ~HIGHLIGHT_FLAG;
        else
            color |= HIGHLIGHT_FLAG;
    }

    if( color & HIGHLIGHT_FLAG )
        color = ColorRefs[color & MASKCOLOR].m_LightColor;

    SetAlpha( &color, 150 );

    CornersTypeBuffer.clear();
    CornersBuffer.clear();

    // Draw all filled areas
    int imax = m_FilledPolysList.size() - 1;

    for( int ic = 0; ic <= imax; ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];

        wxPoint  coord( corner->x + offset.x, corner->y + offset.y );

        CornersBuffer.push_back( coord );

        CornersTypeBuffer.push_back( (char) corner->utility );

        if( (corner->end_contour) || (ic == imax) ) // the last corner of a filled area is found: draw it
        {
            /* Draw the current filled area: draw segments outline first
             * Curiously, draw segments outline first and after draw filled polygons
             * with outlines thickness = 0 is a faster than
             * just draw filled polygons but with outlines thickness = m_ZoneMinThickness
             * So DO NOT use draw filled polygons with outlines having a thickness  > 0
             * Note: Extra segments ( added by kbool to joint holes with external outline) are not drawn
             */
            {
                // Draw outlines:
                if( (m_ZoneMinThickness > 1) || outline_mode )
                {
                    int ilim = CornersBuffer.size() - 1;

                    for(  int is = 0, ie = ilim; is <= ilim; ie = is, is++ )
                    {
                        int x0 = CornersBuffer[is].x;
                        int y0 = CornersBuffer[is].y;
                        int x1 = CornersBuffer[ie].x;
                        int y1 = CornersBuffer[ie].y;

                        if( CornersTypeBuffer[ie] == 0 )   // Draw only basic outlines, not extra segments
                        {
                            if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
                                GRCSegm( &panel->m_ClipBox, DC,
                                         x0, y0, x1, y1,
                                         m_ZoneMinThickness, color );
                            else
                                GRFillCSegm( &panel->m_ClipBox, DC,
                                             x0, y0, x1, y1,
                                             m_ZoneMinThickness, color );
                        }
                    }
                }

                // Draw areas:
                if( m_FillMode==0  && !outline_mode )
                    GRPoly( &panel->m_ClipBox, DC, CornersBuffer.size(), &CornersBuffer[0],
                            true, 0, color, color );
            }

            CornersTypeBuffer.clear();
            CornersBuffer.clear();
        }
    }

    if( m_FillMode == 1  && !outline_mode )     // filled with segments
    {
        for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
        {
            wxPoint start = m_FillSegmList[ic].m_Start + offset;
            wxPoint end   = m_FillSegmList[ic].m_End + offset;

            if( !DisplayOpt.DisplayPcbTrackFill || GetState( FORCE_SKETCH ) )
                GRCSegm( &panel->m_ClipBox, DC, start.x, start.y, end.x, end.y,
                         m_ZoneMinThickness, color );
            else
                GRFillCSegm( &panel->m_ClipBox, DC, start.x, start.y, end.x, end.y,
                             m_ZoneMinThickness, color );
        }
    }
}


EDA_RECT ZONE_CONTAINER::GetBoundingBox() const
{
    const int PRELOAD = 0x7FFFFFFF;     // Biggest integer (32 bits)

    int       ymax = -PRELOAD;
    int       ymin = PRELOAD;
    int       xmin = PRELOAD;
    int       xmax = -PRELOAD;

    int       count = GetNumCorners();

    for( int i = 0; i<count; ++i )
    {
        wxPoint corner = GetCornerPosition( i );

        ymax = MAX( ymax, corner.y );
        xmax = MAX( xmax, corner.x );
        ymin = MIN( ymin, corner.y );
        xmin = MIN( xmin, corner.x );
    }

    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


void ZONE_CONTAINER::DrawWhileCreateOutline( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode )
{
    int     current_gr_mode  = draw_mode;
    bool    is_close_segment = false;
    wxPoint seg_start, seg_end;

    if( DC == NULL )
        return;

    int    curr_layer = ( (PCB_SCREEN*) panel->GetScreen() )->m_Active_Layer;
    BOARD* brd   = GetBoard();
    int    color = brd->GetLayerColor( m_Layer ) & MASKCOLOR;

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( !IsOnLayer( curr_layer ) )
        {
            color &= ~MASKCOLOR;
            color |= DARKDARKGRAY;
        }
    }

    // draw the lines
    wxPoint start_contour_pos = GetCornerPosition( 0 );
    int     icmax = GetNumCorners() - 1;

    for( int ic = 0; ic <= icmax; ic++ )
    {
        int xi = GetCornerPosition( ic ).x;
        int yi = GetCornerPosition( ic ).y;
        int xf, yf;

        if( m_Poly->corner[ic].end_contour == false && ic < icmax )
        {
            is_close_segment = false;
            xf = GetCornerPosition( ic + 1 ).x;
            yf = GetCornerPosition( ic + 1 ).y;

            if( (m_Poly->corner[ic + 1].end_contour) || (ic == icmax - 1) )
                current_gr_mode = GR_XOR;
            else
                current_gr_mode = draw_mode;
        }
        else    // Draw the line from last corner to the first corner of the current contour
        {
            is_close_segment = true;
            current_gr_mode  = GR_XOR;
            xf = start_contour_pos.x;
            yf = start_contour_pos.y;

            // Prepare the next contour for drawing, if exists
            if( ic < icmax )
                start_contour_pos = GetCornerPosition( ic + 1 );
        }

        GRSetDrawMode( DC, current_gr_mode );

        if( is_close_segment )
            GRLine( &panel->m_ClipBox, DC, xi, yi, xf, yf, 0, WHITE );
        else
            GRLine( &panel->m_ClipBox, DC, xi, yi, xf, yf, 0, color );
    }
}


bool ZONE_CONTAINER::HitTest( const wxPoint& refPos )
{
    if( HitTestForCorner( refPos ) )
        return true;

    if( HitTestForEdge( refPos ) )
        return true;

    return false;
}


bool ZONE_CONTAINER::HitTestForCorner( const wxPoint& refPos )
{
    m_CornerSelection = -1;         // Set to not found

    // distance (in internal units) to detect a corner in a zone outline.
    #define CORNER_MIN_DIST 100

    int min_dist = CORNER_MIN_DIST + 1;

    if( GetBoard() && GetBoard()->m_PcbFrame )
    {
        // Use grid size because it is known
        wxRealPoint grid = GetBoard()->m_PcbFrame->DrawPanel->GetGrid();
        min_dist = wxRound( MIN( grid.x, grid.y ) );
    }

    wxPoint delta;
    unsigned lim = m_Poly->corner.size();

    for( unsigned item_pos = 0; item_pos < lim; item_pos++ )
    {
        delta.x = refPos.x - m_Poly->corner[item_pos].x;
        delta.y = refPos.y - m_Poly->corner[item_pos].y;
        // Calculate a distance:
        int dist = MAX( abs( delta.x ), abs( delta.y ) );

        if( dist < min_dist )  // this corner is a candidate:
        {
            m_CornerSelection = item_pos;
            min_dist = dist;
        }
    }

    return m_CornerSelection >= 0;
}


bool ZONE_CONTAINER::HitTestForEdge( const wxPoint& refPos )
{
    unsigned lim = m_Poly->corner.size();

    m_CornerSelection = -1;     // Set to not found

    #define EDGE_MIN_DIST 200   // distance (in internal units) to detect a zone outline
    int min_dist = EDGE_MIN_DIST+1;

    if( GetBoard() && GetBoard()->m_PcbFrame )
    {
        // Use grid size because it is known
        wxRealPoint grid = GetBoard()->m_PcbFrame->DrawPanel->GetGrid();
        min_dist = wxRound( MIN( grid.x, grid.y ) );
    }

    unsigned first_corner_pos = 0;

    for( unsigned item_pos = 0; item_pos < lim; item_pos++ )
    {
        unsigned end_segm = item_pos + 1;

        /* the last corner of the current outline is tested
         * the last segment of the current outline starts at current corner, and ends
         * at the first corner of the outline
         */
        if( m_Poly->corner[item_pos].end_contour || end_segm >= lim )
        {
            unsigned tmp = first_corner_pos;
            first_corner_pos = end_segm;    // first_corner_pos is now the beginning of the next outline
            end_segm = tmp;                 // end_segm is the beginning of the current outline
        }

        /* test the dist between segment and ref point */
        int dist = (int) GetPointToLineSegmentDistance( refPos.x,
                                                        refPos.y,
                                                        m_Poly->corner[item_pos].x,
                                                        m_Poly->corner[item_pos].y,
                                                        m_Poly->corner[end_segm].x,
                                                        m_Poly->corner[end_segm].y );

        if( dist < min_dist )
        {
            m_CornerSelection = item_pos;
            min_dist = dist;
        }
    }

    return m_CornerSelection >= 0;
}


bool ZONE_CONTAINER::HitTest( EDA_RECT& refArea )
{
    bool  is_out_of_box = false;

    CRect rect = m_Poly->GetCornerBounds();

    if( rect.left < refArea.GetX() )
        is_out_of_box = true;

    if( rect.top < refArea.GetY() )
        is_out_of_box = true;

    if( rect.right > refArea.GetRight() )
        is_out_of_box = true;

    if( rect.bottom > refArea.GetBottom() )
        is_out_of_box = true;

    return is_out_of_box ? false : true;
}


int ZONE_CONTAINER::GetClearance( BOARD_CONNECTED_ITEM* aItem ) const
{
    int         myClearance = m_ZoneClearance;

#if 0   // Maybe the netclass clearance should not come into play for a zone?
        // At least the policy decision can be controlled by the zone
        // itself, i.e. here.  On reasons of insufficient documentation,
        // the user will be less bewildered if we simply respect the
        // "zone clearance" setting in the zone properties dialog.  (At least
        // until there is a UI boolean for this.)

    NETCLASS*   myClass  = GetNetClass();

    if( myClass )
        myClearance = std::max( myClearance, myClass->GetClearance() );
#endif

    if( aItem )
    {
        int hisClearance = aItem->GetClearance( NULL );
        myClearance = max( hisClearance, myClearance );
    }

    return myClearance;
}


bool ZONE_CONTAINER::HitTestFilledArea( const wxPoint& aRefPos )
{
    unsigned indexstart = 0, indexend;
    bool     inside     = false;

    for( indexend = 0; indexend < m_FilledPolysList.size(); indexend++ )
    {
        if( m_FilledPolysList[indexend].end_contour )       // end of a filled sub-area found
        {
            if( TestPointInsidePolygon( m_FilledPolysList, indexstart, indexend, aRefPos.x,
                                        aRefPos.y ) )
            {
                inside = true;
                break;
            }

            // Prepare test of next area which starts after the current index end (if exists)
            indexstart = indexend + 1;
        }
    }

    return inside;
}


void ZONE_CONTAINER::DisplayInfo( EDA_DRAW_FRAME* frame )
{
    wxString msg;

    BOARD*   board = (BOARD*) m_Parent;

    wxASSERT( board );

    frame->ClearMsgPanel();

    msg = _( "Zone Outline" );

    int ncont = m_Poly->GetContour( m_CornerSelection );

    if( ncont )
        msg << wxT( " " ) << _( "(Cutout)" );

    frame->AppendMsgPanel( _( "Type" ), msg, DARKCYAN );

    if( IsOnCopperLayer() )
    {
        if( GetNet() >= 0 )
        {
            NETINFO_ITEM* equipot = ( (PCB_BASE_FRAME*) frame )->GetBoard()->FindNet( GetNet() );

            if( equipot )
                msg = equipot->GetNetname();
            else
                msg = wxT( "<noname>" );
        }
        else // a netcode < 0 is an error
        {
            msg = wxT( " [" );
            msg << m_Netname + wxT( "]" );
            msg << wxT( " <" ) << _( "Not Found" ) << wxT( ">" );
        }

        frame->AppendMsgPanel( _( "NetName" ), msg, RED );
    }
    else
    {
        frame->AppendMsgPanel( _( "Non Copper Zone" ), wxEmptyString, RED );
    }

    /* Display net code : (useful in test or debug) */
    msg.Printf( wxT( "%d" ), GetNet() );
    frame->AppendMsgPanel( _( "NetCode" ), msg, RED );

    msg = board->GetLayerName( m_Layer );
    frame->AppendMsgPanel( _( "Layer" ), msg, BROWN );

    msg.Printf( wxT( "%d" ), m_Poly->corner.size() );
    frame->AppendMsgPanel( _( "Corners" ), msg, BLUE );

    if( m_FillMode )
        msg.Printf( _( "Segments" ), m_FillMode );
    else
        msg = _( "Polygons" );

    frame->AppendMsgPanel( _( "Fill mode" ), msg, BROWN );

    // Useful for statistics :
    msg.Printf( wxT( "%d" ), m_Poly->m_HatchLines.size() );
    frame->AppendMsgPanel( _( "Hatch lines" ), msg, BLUE );

    if( m_FilledPolysList.size() )
    {
        msg.Printf( wxT( "%d" ), m_FilledPolysList.size() );
        frame->AppendMsgPanel( _( "Corners in DrawList" ), msg, BLUE );
    }
}


/* Geometric transforms: */

void ZONE_CONTAINER::Move( const wxPoint& offset )
{
    /* move outlines */
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        SetCornerPosition( ii, GetCornerPosition( ii ) + offset );
    }

    m_Poly->Hatch();

    /* move filled areas: */
    for( unsigned ic = 0; ic < m_FilledPolysList.size(); ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        corner->x += offset.x;
        corner->y += offset.y;
    }

    for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
    {
        m_FillSegmList[ic].m_Start += offset;
        m_FillSegmList[ic].m_End   += offset;
    }
}


void ZONE_CONTAINER::MoveEdge( const wxPoint& offset )
{
    int ii = m_CornerSelection;

    // Move the start point of the selected edge:
    SetCornerPosition( ii, GetCornerPosition( ii ) + offset );

    // Move the end point of the selected edge:
    if( m_Poly->corner[ii].end_contour || ii == GetNumCorners() - 1 )
    {
        int icont = m_Poly->GetContour( ii );
        ii = m_Poly->GetContourStart( icont );
    }
    else
    {
        ii++;
    }

    SetCornerPosition( ii, GetCornerPosition( ii ) + offset );

    m_Poly->Hatch();
}


void ZONE_CONTAINER::Rotate( const wxPoint& centre, int angle )
{
    wxPoint pos;

    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        pos.x = m_Poly->corner[ii].x;
        pos.y = m_Poly->corner[ii].y;
        RotatePoint( &pos, centre, angle );
        m_Poly->corner[ii].x = pos.x;
        m_Poly->corner[ii].y = pos.y;
    }

    m_Poly->Hatch();

    /* rotate filled areas: */
    for( unsigned ic = 0; ic < m_FilledPolysList.size(); ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        pos.x = corner->x;
        pos.y = corner->y;
        RotatePoint( &pos, centre, angle );
        corner->x = pos.x;
        corner->y = pos.y;
    }

    for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
    {
        RotatePoint( &m_FillSegmList[ic].m_Start, centre, angle );
        RotatePoint( &m_FillSegmList[ic].m_End, centre, angle );
    }
}


void ZONE_CONTAINER::Flip( const wxPoint& aCentre )
{
    Mirror( aCentre );
    SetLayer( ChangeSideNumLayer( GetLayer() ) );
}


void ZONE_CONTAINER::Mirror( const wxPoint& mirror_ref )
{
    for( unsigned ii = 0; ii < m_Poly->corner.size(); ii++ )
    {
        m_Poly->corner[ii].y -= mirror_ref.y;
        NEGATE( m_Poly->corner[ii].y );
        m_Poly->corner[ii].y += mirror_ref.y;
    }

    m_Poly->Hatch();

    /* mirror filled areas: */
    for( unsigned ic = 0; ic < m_FilledPolysList.size(); ic++ )
    {
        CPolyPt* corner = &m_FilledPolysList[ic];
        corner->y -= mirror_ref.y;
        NEGATE( corner->y );
        corner->y += mirror_ref.y;
    }

    for( unsigned ic = 0; ic < m_FillSegmList.size(); ic++ )
    {
        m_FillSegmList[ic].m_Start.y -= mirror_ref.y;
        NEGATE( m_FillSegmList[ic].m_Start.y );
        m_FillSegmList[ic].m_Start.y += mirror_ref.y;
        m_FillSegmList[ic].m_End.y   -= mirror_ref.y;
        NEGATE( m_FillSegmList[ic].m_End.y );
        m_FillSegmList[ic].m_End.y += mirror_ref.y;
    }
}


void ZONE_CONTAINER::Copy( ZONE_CONTAINER* src )
{
    m_Parent = src->m_Parent;
    m_Layer  = src->m_Layer;
    SetNet( src->GetNet() );
    m_TimeStamp = src->m_TimeStamp;
    m_Poly->RemoveAllContours();
    m_Poly->Copy( src->m_Poly );                // copy outlines
    m_CornerSelection  = -1;                    // For corner moving, corner index to drag, or -1 if no selection
    m_ZoneClearance    = src->m_ZoneClearance;  // clearance value
    m_ZoneMinThickness = src->m_ZoneMinThickness;
    m_FillMode = src->m_FillMode;               // Filling mode (segments/polygons)
    m_ArcToSegmentsCount = src->m_ArcToSegmentsCount;
    m_PadOption = src->m_PadOption;
    m_ThermalReliefGap = src->m_ThermalReliefGap;
    m_ThermalReliefCopperBridge = src->m_ThermalReliefCopperBridge;
    m_Poly->m_HatchStyle = src->m_Poly->GetHatchStyle();
    m_Poly->m_HatchLines = src->m_Poly->m_HatchLines;   // Copy vector <CSegment>
    m_FilledPolysList.clear();
    m_FilledPolysList = src->m_FilledPolysList;
    m_FillSegmList.clear();
    m_FillSegmList = src->m_FillSegmList;
}


bool ZONE_CONTAINER::SetNetNameFromNetCode( void )
{
    NETINFO_ITEM* net;

    if( m_Parent && ( net = ( (BOARD*) m_Parent )->FindNet( GetNet() ) ) )
    {
        m_Netname = net->GetNetname();
        return true;
    }

    return false;
}


wxString ZONE_CONTAINER::GetSelectMenuText() const
{
    wxString text;
    NETINFO_ITEM* net;
    BOARD* board = GetBoard();

    text = _( "Zone Outline" );

    int ncont = m_Poly->GetContour( m_CornerSelection );

    if( ncont )
        text << wxT( " " ) << _( "(Cutout)" );

    text << wxT( " " );
    text << wxString::Format( wxT( "(%8.8X)" ), m_TimeStamp );

    if ( !IsOnCopperLayer() )
    {
        text << wxT( " [" ) << _( "Not on copper layer" ) << wxT( "]" );
    }
    else if( GetNet() >= 0 )
    {
        if( board )
        {
            net = board->FindNet( GetNet() );

            if( net )
            {
                text << wxT( " [" ) << net->GetNetname() << wxT( "]" );
            }
        }
        else
        {
            text << _( "** NO BOARD DEFINED **" );
        }
    }
    else    // A netcode < 0 is an error flag (Netname not found or area not initialised)
    {
        text << wxT( " [" ) << m_Netname << wxT( "]" );
        text << wxT( " <" ) << _( "Not Found" ) << wxT( ">" );
    }

    text << _( " on " ) << GetLayerName();

    return text;
}

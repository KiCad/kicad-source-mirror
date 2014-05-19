/**
 * @file plot_brditems_plotter.cpp
 * @brief basic plot functions to plot board items, or a group of board items.
 */


/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <base_struct.h>
#include <drawtxt.h>
#include <trigo.h>
#include <macros.h>
#include <wxBasePcbFrame.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_mire.h>
#include <class_dimension.h>

#include <pcbnew.h>
#include <pcbplot.h>

/* class BRDITEMS_PLOTTER is a helper class to plot board items
 * and a group of board items
 */


EDA_COLOR_T BRDITEMS_PLOTTER::getColor( LAYER_NUM aLayer )
{
    EDA_COLOR_T color = m_board->GetLayerColor( aLayer );
    if (color == WHITE)
        color = LIGHTGRAY;
    return color;
}


void BRDITEMS_PLOTTER::PlotPad( D_PAD* aPad, EDA_COLOR_T aColor, EDA_DRAW_MODE_T aPlotMode )
{
    wxPoint shape_pos = aPad->ShapePos();

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( aColor != WHITE ? aColor : LIGHTGRAY);

    switch( aPad->GetShape() )
    {
    case PAD_CIRCLE:
        m_plotter->FlashPadCircle( shape_pos, aPad->GetSize().x, aPlotMode );
        break;

    case PAD_OVAL:
        m_plotter->FlashPadOval( shape_pos, aPad->GetSize(),
                                 aPad->GetOrientation(), aPlotMode );
        break;

    case PAD_TRAPEZOID:
        {
            wxPoint coord[4];
            aPad->BuildPadPolygon( coord, wxSize(0,0), 0 );
            m_plotter->FlashPadTrapez( shape_pos, coord,
                                       aPad->GetOrientation(), aPlotMode );
        }
        break;

    case PAD_RECT:
    default:
        m_plotter->FlashPadRect( shape_pos, aPad->GetSize(),
                                 aPad->GetOrientation(), aPlotMode );
        break;
    }
}


bool BRDITEMS_PLOTTER::PlotAllTextsModule( MODULE* aModule )
{
    // see if we want to plot VALUE and REF fields
    bool trace_val = GetPlotValue();
    bool trace_ref = GetPlotReference();

    TEXTE_MODULE* textModule = &aModule->Reference();
    LAYER_NUM     textLayer = textModule->GetLayer();

    if( textLayer >= NB_LAYERS )
        return false;

    if( ( GetLayerMask( textLayer ) & m_layerMask ) == 0 )
        trace_ref = false;

    if( !textModule->IsVisible() && !GetPlotInvisibleText() )
        trace_ref = false;

    textModule = &aModule->Value();
    textLayer = textModule->GetLayer();

    if( textLayer > NB_LAYERS )
        return false;

    if( ( GetLayerMask( textLayer ) & m_layerMask ) == 0 )
        trace_val = false;

    if( !textModule->IsVisible() && !GetPlotInvisibleText() )
        trace_val = false;

    // Plot text fields, if allowed
    if( trace_ref )
    {
        if( GetReferenceColor() == UNSPECIFIED_COLOR )
            PlotTextModule( &aModule->Reference(), getColor( textLayer ) );
        else
            PlotTextModule( &aModule->Reference(), GetReferenceColor() );
    }

    if( trace_val )
    {
        if( GetValueColor() == UNSPECIFIED_COLOR )
            PlotTextModule( &aModule->Value(), getColor( textLayer ) );
        else
            PlotTextModule( &aModule->Value(), GetValueColor() );
    }

    for( BOARD_ITEM *item = aModule->GraphicalItems().GetFirst();
         item != NULL; item = item->Next() )
    {
        textModule = dynamic_cast<TEXTE_MODULE*>( item );
        if( !textModule )
            continue;

        if( !textModule->IsVisible() )
            continue;

        textLayer = textModule->GetLayer();

        if( textLayer >= NB_LAYERS )
            return false;

        if( !( GetLayerMask( textLayer ) & m_layerMask ) )
            continue;

        PlotTextModule( textModule, getColor( textLayer ) );
    }

    return true;
}


// plot items like text and graphics, but not tracks and module
void BRDITEMS_PLOTTER::PlotBoardGraphicItems()
{
    for( BOARD_ITEM* item = m_board->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_LINE_T:
            PlotDrawSegment( (DRAWSEGMENT*) item);
            break;

        case PCB_TEXT_T:
            PlotTextePcb( (TEXTE_PCB*) item );
            break;

        case PCB_DIMENSION_T:
            PlotDimension( (DIMENSION*) item );
            break;

        case PCB_TARGET_T:
            PlotPcbTarget( (PCB_TARGET*) item );
            break;

        case PCB_MARKER_T:
        default:
            break;
        }
    }
}

void BRDITEMS_PLOTTER::PlotTextModule( TEXTE_MODULE* pt_texte, EDA_COLOR_T aColor )
{
    wxSize  size;
    wxPoint pos;
    double  orient;
    int     thickness;

    if( aColor == WHITE )
        aColor = LIGHTGRAY;

    m_plotter->SetColor( aColor );

    // calculate some text parameters :
    size = pt_texte->GetSize();
    pos  = pt_texte->GetTextPosition();

    orient = pt_texte->GetDrawRotation();

    thickness = pt_texte->GetThickness();

    if( GetMode() == LINE )
        thickness = -1;

    if( pt_texte->IsMirrored() )
        NEGATE( size.x );  // Text is mirrored

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = pt_texte->IsBold() || thickness;

    m_plotter->Text( pos, aColor,
                     pt_texte->GetText(),
                     orient, size,
                     pt_texte->GetHorizJustify(), pt_texte->GetVertJustify(),
                     thickness, pt_texte->IsItalic(), allow_bold );
}


void BRDITEMS_PLOTTER::PlotDimension( DIMENSION* aDim )
{
    if( (GetLayerMask( aDim->GetLayer() ) & m_layerMask) == 0 )
        return;

    DRAWSEGMENT draw;

    draw.SetWidth( (GetMode() == LINE) ? -1 : aDim->GetWidth() );
    draw.SetLayer( aDim->GetLayer() );

    EDA_COLOR_T color = aDim->GetBoard()->GetLayerColor( aDim->GetLayer() );
    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( color != WHITE ? color : LIGHTGRAY);

    PlotTextePcb( &aDim->Text() );

    draw.SetStart( aDim->m_crossBarO );
    draw.SetEnd( aDim->m_crossBarF );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_featureLineGO);
    draw.SetEnd( aDim->m_featureLineGF );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_featureLineDO );
    draw.SetEnd( aDim->m_featureLineDF );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarF );
    draw.SetEnd( aDim->m_arrowD1F );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarF );
    draw.SetEnd( aDim->m_arrowD2F );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarO );
    draw.SetEnd( aDim->m_arrowG1F );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarO );
    draw.SetEnd( aDim->m_arrowG2F );
    PlotDrawSegment( &draw );
}


void BRDITEMS_PLOTTER::PlotPcbTarget( PCB_TARGET* aMire )
{
    int     dx1, dx2, dy1, dy2, radius;

    if( (GetLayerMask( aMire->GetLayer() ) & m_layerMask) == 0 )
        return;

    m_plotter->SetColor( getColor( aMire->GetLayer() ) );

    DRAWSEGMENT  draw;

    draw.SetShape( S_CIRCLE );
    draw.SetWidth( ( GetMode() == LINE ) ? -1 : aMire->GetWidth() );
    draw.SetLayer( aMire->GetLayer() );
    draw.SetStart( aMire->GetPosition() );
    radius = aMire->GetSize() / 3;

    if( aMire->GetShape() )   // shape X
        radius = aMire->GetSize() / 2;

    // Draw the circle
    draw.SetEnd( wxPoint( draw.GetStart().x + radius, draw.GetStart().y ));

    PlotDrawSegment( &draw );

    draw.SetShape( S_SEGMENT );

    radius = aMire->GetSize() / 2;
    dx1    = radius;
    dy1    = 0;
    dx2    = 0;
    dy2    = radius;

    if( aMire->GetShape() )    // Shape X
    {
        dx1 = dy1 = radius;
        dx2 = dx1;
        dy2 = -dy1;
    }

    wxPoint mirePos( aMire->GetPosition() );

    // Draw the X or + shape:
    draw.SetStart( wxPoint( mirePos.x - dx1, mirePos.y - dy1 ));
    draw.SetEnd(   wxPoint( mirePos.x + dx1, mirePos.y + dy1 ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( mirePos.x - dx2, mirePos.y - dy2 ));
    draw.SetEnd(   wxPoint( mirePos.x + dx2, mirePos.y + dy2 ));
    PlotDrawSegment( &draw );
}


// Plot footprints graphic items (outlines)
void BRDITEMS_PLOTTER::Plot_Edges_Modules()
{
    for( MODULE* module = m_board->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems().GetFirst(); item; item = item->Next() )
        {
            EDGE_MODULE *edge = dynamic_cast<EDGE_MODULE*>( item );

            if( !edge || (( GetLayerMask( edge->GetLayer() ) & m_layerMask ) == 0) )
                continue;

            Plot_1_EdgeModule( edge );
        }
    }
}


//* Plot a graphic item (outline) relative to a footprint
void BRDITEMS_PLOTTER::Plot_1_EdgeModule( EDGE_MODULE* aEdge )
{
    int     type_trace;         // Type of item to plot.
    int     thickness;          // Segment thickness.
    int     radius;             // Circle radius.

    if( aEdge->Type() != PCB_MODULE_EDGE_T )
        return;

    m_plotter->SetColor( getColor( aEdge->GetLayer() ) );

    type_trace = aEdge->GetShape();
    thickness  = aEdge->GetWidth();

    wxPoint pos( aEdge->GetStart() );
    wxPoint end( aEdge->GetEnd() );

    switch( type_trace )
    {
    case S_SEGMENT:
        m_plotter->ThickSegment( pos, end, thickness, GetMode() );
        break;

    case S_CIRCLE:
        radius = KiROUND( GetLineLength( end, pos ) );
        m_plotter->ThickCircle( pos, radius * 2, thickness, GetMode() );
        break;

    case S_ARC:
    {
        radius = KiROUND( GetLineLength( end, pos ) );

        double startAngle  = ArcTangente( end.y - pos.y, end.x - pos.x );

        double endAngle = startAngle + aEdge->GetAngle();

        if ( ( GetFormat() == PLOT_FORMAT_DXF ) &&
             ( m_layerMask & ( SILKSCREEN_LAYER_BACK | DRAW_LAYER | COMMENT_LAYER ) ) )
            m_plotter->ThickArc( pos, -startAngle, -endAngle, radius, thickness, GetMode() );
        else
            m_plotter->ThickArc( pos, -endAngle, -startAngle, radius, thickness, GetMode() );
    }
    break;

    case S_POLYGON:
    {
        const std::vector<wxPoint>& polyPoints = aEdge->GetPolyPoints();

        if( polyPoints.size() <= 1 )  // Malformed polygon
            break;

        // We must compute true coordinates from m_PolyList
        // which are relative to module position, orientation 0
        MODULE* module = aEdge->GetParentModule();

        std::vector< wxPoint > cornerList;

        cornerList.reserve( polyPoints.size() );

        for( unsigned ii = 0; ii < polyPoints.size(); ii++ )
        {
            wxPoint corner = polyPoints[ii];

            if( module )
            {
                RotatePoint( &corner, module->GetOrientation() );
                corner += module->GetPosition();
            }

            cornerList.push_back( corner );
        }

        m_plotter->PlotPoly( cornerList, FILLED_SHAPE, thickness );
    }
    break;
    }
}


// Plot a PCB Text, i;e. a text found on a copper or technical layer
void BRDITEMS_PLOTTER::PlotTextePcb( TEXTE_PCB* pt_texte )
{
    double  orient;
    int     thickness;
    wxPoint pos;
    wxSize  size;

    if( pt_texte->GetText().IsEmpty() )
        return;

    if( ( GetLayerMask( pt_texte->GetLayer() ) & m_layerMask ) == 0 )
        return;

    m_plotter->SetColor( getColor( pt_texte->GetLayer() ) );

    size      = pt_texte->GetSize();
    pos       = pt_texte->GetTextPosition();
    orient    = pt_texte->GetOrientation();
    thickness = ( GetMode() == LINE ) ? -1 : pt_texte->GetThickness();

    if( pt_texte->IsMirrored() )
        size.x = -size.x;

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = pt_texte->IsBold() || thickness;

    if( pt_texte->IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString* list = wxStringSplit( pt_texte->GetText(), '\n' );
        positions.reserve( list->Count() );

        pt_texte->GetPositionsOfLinesOfMultilineText( positions, list->Count() );

        for( unsigned ii = 0; ii < list->Count(); ii++ )
        {
            wxString& txt = list->Item( ii );
            m_plotter->Text( positions[ii], UNSPECIFIED_COLOR, txt, orient, size,
                             pt_texte->GetHorizJustify(), pt_texte->GetVertJustify(),
                             thickness, pt_texte->IsItalic(), allow_bold );
        }

        delete list;
    }
    else
    {
        m_plotter->Text( pos, UNSPECIFIED_COLOR, pt_texte->GetText(), orient, size,
                         pt_texte->GetHorizJustify(), pt_texte->GetVertJustify(),
                         thickness, pt_texte->IsItalic(), allow_bold );
    }
}


/* Plot areas (given by .m_FilledPolysList member) in a zone
 */
void BRDITEMS_PLOTTER::PlotFilledAreas( ZONE_CONTAINER* aZone )
{
    const CPOLYGONS_LIST& polysList = aZone->GetFilledPolysList();
    unsigned imax = polysList.GetCornersCount();

    if( imax == 0 )  // Nothing to draw
        return;

    // We need a buffer to store corners coordinates:
    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    m_plotter->SetColor( getColor( aZone->GetLayer() ) );

    /* Plot all filled areas: filled areas have a filled area and a thick
     * outline we must plot the filled area itself ( as a filled polygon
     * OR a set of segments ) and plot the thick outline itself
     *
     * in non filled mode the outline is plotted, but not the filling items
     */
    for( unsigned ic = 0; ic < imax; ic++ )
    {
        wxPoint pos = polysList.GetPos( ic );
        cornerList.push_back( pos );

        if(  polysList.IsEndContour( ic ) )   // Plot the current filled area outline
        {
            // First, close the outline
            if( cornerList[0] != cornerList[cornerList.size() - 1] )
            {
                cornerList.push_back( cornerList[0] );
            }

            // Plot the current filled area and its outline
            if( GetMode() == FILLED )
            {
                // Plot the filled area polygon.
                // The area can be filled by segments or uses solid polygons
                if( aZone->GetFillMode() == 0 ) // We are using solid polygons
                {
                    m_plotter->PlotPoly( cornerList, FILLED_SHAPE, aZone->GetMinThickness() );
                }
                else    // We are using areas filled by segments: plot segments and outline
                {
                    for( unsigned iseg = 0; iseg < aZone->FillSegments().size(); iseg++ )
                    {
                        wxPoint start = aZone->FillSegments()[iseg].m_Start;
                        wxPoint end   = aZone->FillSegments()[iseg].m_End;
                        m_plotter->ThickSegment( start, end,
                                                 aZone->GetMinThickness(),
                                                 GetMode() );
                    }

                // Plot the area outline only
                if( aZone->GetMinThickness() > 0 )
                    m_plotter->PlotPoly( cornerList, NO_FILL, aZone->GetMinThickness() );
                }
            }
            else
            {
                if( aZone->GetMinThickness() > 0 )
                {
                    for( unsigned jj = 1; jj<cornerList.size(); jj++ )
                        m_plotter->ThickSegment( cornerList[jj -1], cornerList[jj],
                                                 ( GetMode() == LINE ) ? -1 :
                                                 aZone->GetMinThickness(),
                                                 GetMode() );
                }

                m_plotter->SetCurrentLineWidth( -1 );
            }

            cornerList.clear();
        }
    }
}


/* Plot items type DRAWSEGMENT on layers allowed by aLayerMask
 */
void BRDITEMS_PLOTTER::PlotDrawSegment(  DRAWSEGMENT* aSeg )
{
    int     thickness;
    int     radius = 0;
    double  StAngle = 0, EndAngle = 0;

    if( (GetLayerMask( aSeg->GetLayer() ) & m_layerMask) == 0 )
        return;

    if( GetMode() == LINE )
        thickness = GetLineWidth();
    else
        thickness = aSeg->GetWidth();

    m_plotter->SetColor( getColor( aSeg->GetLayer() ) );

    wxPoint start( aSeg->GetStart() );
    wxPoint end(   aSeg->GetEnd() );

    m_plotter->SetCurrentLineWidth( thickness );

    switch( aSeg->GetShape() )
    {
    case S_CIRCLE:
        radius = KiROUND( GetLineLength( end, start ) );
        m_plotter->ThickCircle( start, radius * 2, thickness, GetMode() );
        break;

    case S_ARC:
        radius = KiROUND( GetLineLength( end, start ) );
        StAngle  = ArcTangente( end.y - start.y, end.x - start.x );
        EndAngle = StAngle + aSeg->GetAngle();
        m_plotter->ThickArc( start, -EndAngle, -StAngle, radius, thickness, GetMode() );
        break;

    case S_CURVE:
        {
            const std::vector<wxPoint>& bezierPoints = aSeg->GetBezierPoints();

            for( unsigned i = 1; i < bezierPoints.size(); i++ )
                m_plotter->ThickSegment( bezierPoints[i - 1],
                                         bezierPoints[i],
                                         thickness, GetMode() );
        }
        break;

    default:
        m_plotter->ThickSegment( start, end, thickness, GetMode() );
    }
}


/** Helper function to plot a single drill mark. It compensate and clamp
 *   the drill mark size depending on the current plot options
 */
void BRDITEMS_PLOTTER::plotOneDrillMark( PAD_DRILL_SHAPE_T aDrillShape,
                           const wxPoint &aDrillPos, wxSize aDrillSize,
                           const wxSize &aPadSize,
                           double aOrientation, int aSmallDrill )
{
    // Small drill marks have no significance when applied to slots
    if( aSmallDrill && aDrillShape == PAD_DRILL_CIRCLE )
        aDrillSize.x = std::min( aSmallDrill, aDrillSize.x );

    // Round holes only have x diameter, slots have both
    aDrillSize.x -= getFineWidthAdj();
    aDrillSize.x = Clamp( 1, aDrillSize.x, aPadSize.x - 1 );

    if( aDrillShape == PAD_DRILL_OBLONG )
    {
        aDrillSize.y -= getFineWidthAdj();
        aDrillSize.y = Clamp( 1, aDrillSize.y, aPadSize.y - 1 );
        m_plotter->FlashPadOval( aDrillPos, aDrillSize, aOrientation, GetMode() );
    }
    else
        m_plotter->FlashPadCircle( aDrillPos, aDrillSize.x, GetMode() );
}


void BRDITEMS_PLOTTER::PlotDrillMarks()
{
    /* If small drills marks were requested prepare a clamp value to pass
       to the helper function */
    int small_drill = (GetDrillMarksType() == PCB_PLOT_PARAMS::SMALL_DRILL_SHAPE) ?
                        SMALL_DRILL : 0;

    /* In the filled trace mode drill marks are drawn white-on-black to scrape
       the underlying pad. This works only for drivers supporting color change,
       obviously... it means that:
       - PS, SVG and PDF output is correct (i.e. you have a 'donut' pad)
       - In HPGL you can't see them
       - In gerbers you can't see them, too. This is arguably the right thing to
         do since having drill marks and high speed drill stations is a sure
         recipe for broken tools and angry manufacturers. If you *really* want them
         you could start a layer with negative polarity to scrape the film.
       - In DXF they go into the 'WHITE' layer. This could be useful.
     */
    if( GetMode() == FILLED )
         m_plotter->SetColor( WHITE );

    for( TRACK *pts = m_board->m_Track; pts != NULL; pts = pts->Next() )
    {
        const VIA *via = dynamic_cast<const VIA*>( pts );

        if( via )
            plotOneDrillMark( PAD_DRILL_CIRCLE, via->GetStart(),
                    wxSize( via->GetDrillValue(), 0 ),
                    wxSize( via->GetWidth(), 0 ), 0, small_drill );
    }

    for( MODULE *Module = m_board->m_Modules; Module != NULL; Module = Module->Next() )
    {
        for( D_PAD *pad = Module->Pads(); pad != NULL; pad = pad->Next() )
        {
            if( pad->GetDrillSize().x == 0 )
                continue;

            plotOneDrillMark( pad->GetDrillShape(),
                              pad->GetPosition(), pad->GetDrillSize(),
                              pad->GetSize(), pad->GetOrientation(),
                              small_drill );
        }
    }

    if( GetMode() == FILLED )
        m_plotter->SetColor( GetColor() );
}

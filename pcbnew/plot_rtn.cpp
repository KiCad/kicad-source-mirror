/**
 * @file plot_rtn.cpp
 * @brief Common plot routines.
 */

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <base_struct.h>
#include <drawtxt.h>
#include <trigo.h>
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

/* Function getColor
 * return the layer colorfrom the layer id
 * White color is special: cannot be seen on a white paper
 * and in B&W mode, is plotted as white but other colors are plotted in BLACK
 * so the returned color is LIGHTGRAY when the layer color is WHITE
 */
EDA_COLOR_T BRDITEMS_PLOTTER::getColor( int aLayer )
{
    EDA_COLOR_T color = m_board->GetLayerColor( aLayer );
    if (color == WHITE)
        color = LIGHTGRAY;
    return color;
}

bool BRDITEMS_PLOTTER::PlotAllTextsModule( MODULE* aModule )
{
    // see if we want to plot VALUE and REF fields
    bool trace_val = GetPlotValue();
    bool trace_ref = GetPlotReference();

    TEXTE_MODULE* textModule = aModule->m_Reference;
    unsigned      textLayer = textModule->GetLayer();

    if( textLayer >= 32 )
        return false;

    if( ( ( 1 << textLayer ) & m_layerMask ) == 0 )
        trace_ref = false;

    if( !textModule->IsVisible() && !GetPlotInvisibleText() )
        trace_ref = false;

    textModule = aModule->m_Value;
    textLayer = textModule->GetLayer();

    if( textLayer > 32 )
        return false;

    if( ( (1 << textLayer) & m_layerMask ) == 0 )
        trace_val = false;

    if( !textModule->IsVisible() && !GetPlotInvisibleText() )
        trace_val = false;

    // Plot text fields, if allowed
    if( trace_ref )
    {
        if( GetReferenceColor() == UNSPECIFIED_COLOR )
            PlotTextModule( aModule->m_Reference, getColor( textLayer ) );
        else
            PlotTextModule( aModule->m_Reference, GetReferenceColor() );
    }

    if( trace_val )
    {
        if( GetValueColor() == UNSPECIFIED_COLOR )
            PlotTextModule( aModule->m_Value, getColor( textLayer ) );
        else
            PlotTextModule( aModule->m_Value, GetValueColor() );
    }

    for( textModule = (TEXTE_MODULE*) aModule->m_Drawings.GetFirst();
         textModule != NULL; textModule = textModule->Next() )
    {
        if( textModule->Type() != PCB_MODULE_TEXT_T )
            continue;

        if( !GetPlotOtherText() )
            continue;

        if( !textModule->IsVisible() && !GetPlotInvisibleText() )
            continue;

        textLayer = textModule->GetLayer();

        if( textLayer >= 32 )
            return false;

        if( !( ( 1 << textLayer ) & m_layerMask ) )
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


/* Creates the plot for silkscreen layers
 */
void PlotSilkScreen( BOARD *aBoard, PLOTTER* aPlotter, long aLayerMask,
                     const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerMask( aLayerMask );

    // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Plot footprint outlines :
    itemplotter.Plot_Edges_Modules();

    // Plot pads (creates pads outlines, for pads on silkscreen layers)
    int layersmask_plotpads = aLayerMask;
    // Calculate the mask layers of allowed layers for pads

    if( !aPlotOpt.GetPlotPadsOnSilkLayer() )       // Do not plot pads on silk screen layers
        layersmask_plotpads &= ~(SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT );

    if( layersmask_plotpads )
    {
        for( MODULE* Module = aBoard->m_Modules; Module; Module = Module->Next() )
        {
            for( D_PAD * pad = Module->m_Pads; pad != NULL; pad = pad->Next() )
            {
                // See if the pad is on this layer
                int masklayer = pad->GetLayerMask();
                if( (masklayer & layersmask_plotpads) == 0 )
                    continue;

                wxPoint shape_pos = pad->ReturnShapePos();

                EDA_COLOR_T color = ColorFromInt(0);
                if( (layersmask_plotpads & SILKSCREEN_LAYER_BACK) )
                   color = aBoard->GetLayerColor( SILKSCREEN_N_BACK );

                if((layersmask_plotpads & SILKSCREEN_LAYER_FRONT ) )
                    color = ColorFromInt( color | aBoard->GetLayerColor( SILKSCREEN_N_FRONT ) );

                // Set plot color (change WHITE to LIGHTGRAY because
                // the white items are not seen on a white paper or screen
                aPlotter->SetColor( color != WHITE ? color : LIGHTGRAY);

                switch( pad->GetShape() )
                {
                case PAD_CIRCLE:
                    aPlotter->FlashPadCircle( shape_pos, pad->GetSize().x, LINE );
                    break;

                case PAD_OVAL:
                    aPlotter->FlashPadOval( shape_pos, pad->GetSize(),
		                            pad->GetOrientation(), LINE );
                    break;

                case PAD_TRAPEZOID:
                    {
                        wxPoint coord[4];
                        pad->BuildPadPolygon( coord, wxSize(0,0), 0 );
                        aPlotter->FlashPadTrapez( shape_pos, coord,
			                          pad->GetOrientation(), LINE );
                    }
                    break;

                case PAD_RECT:
                default:
                    aPlotter->FlashPadRect( shape_pos, pad->GetSize(),
		                            pad->GetOrientation(), LINE );
                    break;
                }
            }
        }
    }

    // Plot footprints fields (ref, value ...)
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        if( ! itemplotter.PlotAllTextsModule( module ) )
        {
             wxLogMessage( _( "Your BOARD has a bad layer number for module %s" ),
                           GetChars( module->GetReference() ) );
        }
    }

    // Plot filled areas
    for( int ii = 0; ii < aBoard->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = aBoard->GetArea( ii );

        if( ( ( 1 << edge_zone->GetLayer() ) & aLayerMask ) == 0 )
            continue;

        itemplotter.PlotFilledAreas( edge_zone );
    }

    // Plot segments used to fill zone areas (outdated, but here for old boards
    // compatibility):
    for( SEGZONE* seg = aBoard->m_Zone; seg != NULL; seg = seg->Next() )
    {
        if( ( ( 1 << seg->GetLayer() ) & aLayerMask ) == 0 )
            continue;

        aPlotter->ThickSegment( seg->m_Start, seg->m_End, seg->m_Width,
                                itemplotter.GetMode() );
    }
}


void BRDITEMS_PLOTTER::PlotTextModule( TEXTE_MODULE* pt_texte,
                                       EDA_COLOR_T aColor )
{
    wxSize  size;
    wxPoint pos;
    int     orient, thickness;

    if( aColor == WHITE )
        aColor = LIGHTGRAY;
    m_plotter->SetColor( aColor );

    // calculate some text parameters :
    size = pt_texte->m_Size;
    pos  = pt_texte->m_Pos;

    orient = pt_texte->GetDrawRotation();

    thickness = pt_texte->m_Thickness;

    if( GetMode() == LINE )
        thickness = -1;

    if( pt_texte->m_Mirror )
        NEGATE( size.x );  // Text is mirrored

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = pt_texte->m_Bold || thickness;

    m_plotter->Text( pos, aColor,
                   pt_texte->m_Text,
                   orient, size,
                   pt_texte->m_HJustify, pt_texte->m_VJustify,
                   thickness, pt_texte->m_Italic, allow_bold );
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

    PlotTextePcb( &aDim->m_Text );

    draw.SetStart( wxPoint( aDim->m_crossBarOx, aDim->m_crossBarOy ));
    draw.SetEnd(   wxPoint( aDim->m_crossBarFx, aDim->m_crossBarFy ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( aDim->m_featureLineGOx, aDim->m_featureLineGOy ));
    draw.SetEnd(   wxPoint( aDim->m_featureLineGFx, aDim->m_featureLineGFy ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( aDim->m_featureLineDOx, aDim->m_featureLineDOy ));
    draw.SetEnd(   wxPoint( aDim->m_featureLineDFx, aDim->m_featureLineDFy ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( aDim->m_arrowD1Ox, aDim->m_arrowD1Oy ));
    draw.SetEnd(   wxPoint( aDim->m_arrowD1Fx, aDim->m_arrowD1Fy ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( aDim->m_arrowD2Ox, aDim->m_arrowD2Oy ));
    draw.SetEnd(   wxPoint( aDim->m_arrowD2Fx, aDim->m_arrowD2Fy ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( aDim->m_arrowG1Ox, aDim->m_arrowG1Oy ));
    draw.SetEnd(   wxPoint( aDim->m_arrowG1Fx, aDim->m_arrowG1Fy ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( aDim->m_arrowG2Ox, aDim->m_arrowG2Oy ));
    draw.SetEnd(   wxPoint( aDim->m_arrowG2Fx, aDim->m_arrowG2Fy ));
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
        for( EDGE_MODULE* edge = (EDGE_MODULE*) module->m_Drawings.GetFirst();
             edge; edge = edge->Next() )
        {
            if( edge->Type() != PCB_MODULE_EDGE_T )
                continue;

            if( ( GetLayerMask( edge->GetLayer() ) & m_layerMask ) == 0 )
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
        radius = (int) hypot( (double) ( end.x - pos.x ),
                              (double) ( end.y - pos.y ) );
        m_plotter->ThickCircle( pos, radius * 2, thickness, GetMode() );
        break;

    case S_ARC:
        {
            radius = (int) hypot( (double) ( end.x - pos.x ),
                                  (double) ( end.y - pos.y ) );

            double startAngle  = ArcTangente( end.y - pos.y, end.x - pos.x );

            double endAngle = startAngle + aEdge->GetAngle();

            if ( ( GetFormat() == PLOT_FORMAT_DXF ) &&
               ( m_layerMask & ( SILKSCREEN_LAYER_BACK | DRAW_LAYER | COMMENT_LAYER ) ) )
                m_plotter->ThickArc( pos, -startAngle, -endAngle, radius,
                                thickness, GetMode() );
            else
                m_plotter->ThickArc( pos, -endAngle, -startAngle, radius,
                                thickness, GetMode() );
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
    int     orient, thickness;
    wxPoint pos;
    wxSize  size;

    if( pt_texte->m_Text.IsEmpty() )
        return;

    if( ( GetLayerMask( pt_texte->GetLayer() ) & m_layerMask ) == 0 )
        return;

    m_plotter->SetColor( getColor( pt_texte->GetLayer() ) );

    size = pt_texte->m_Size;
    pos  = pt_texte->m_Pos;
    orient    = pt_texte->m_Orient;
    thickness = ( GetMode() == LINE ) ? -1 : pt_texte->m_Thickness;

    if( pt_texte->m_Mirror )
        size.x = -size.x;

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = pt_texte->m_Bold || thickness;

    if( pt_texte->m_MultilineAllowed )
    {
        wxArrayString* list = wxStringSplit( pt_texte->m_Text, '\n' );
        wxPoint        offset;

        offset.y = pt_texte->GetInterline();

        RotatePoint( &offset, orient );

        for( unsigned i = 0; i < list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            m_plotter->Text( pos, UNSPECIFIED_COLOR, txt, orient, size,
                           pt_texte->m_HJustify, pt_texte->m_VJustify,
                           thickness, pt_texte->m_Italic, allow_bold );
            pos += offset;
        }

        delete list;
    }
    else
    {
        m_plotter->Text( pos, UNSPECIFIED_COLOR, pt_texte->m_Text, orient, size,
                       pt_texte->m_HJustify, pt_texte->m_VJustify,
                       thickness, pt_texte->m_Italic, allow_bold );
    }
}


/* Plot areas (given by .m_FilledPolysList member) in a zone
 */
void BRDITEMS_PLOTTER::PlotFilledAreas( ZONE_CONTAINER* aZone )
{
    std::vector<CPolyPt> polysList = aZone->GetFilledPolysList();
    unsigned imax = polysList.size();

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
        CPolyPt* corner = &polysList[ic];
        cornerList.push_back( wxPoint( corner->x, corner->y) );

        if( corner->end_contour )   // Plot the current filled area outline
        {
            // First, close the outline
            if( cornerList[0] != cornerList[cornerList.size() - 1] )
            {
                cornerList.push_back( cornerList[0] );
            }

            // Plot the current filled area and its outline
            if( GetMode() == FILLED )
            {
                // Plot the current filled area polygon
                if( aZone->m_FillMode == 0 )    // We are using solid polygons
                {                               // (if != 0: using segments )
                    m_plotter->PlotPoly( cornerList, FILLED_SHAPE );
                }
                else                            // We are using areas filled by
                {                               // segments: plot them )
                    for( unsigned iseg = 0; iseg < aZone->m_FillSegmList.size(); iseg++ )
                    {
                        wxPoint start = aZone->m_FillSegmList[iseg].m_Start;
                        wxPoint end   = aZone->m_FillSegmList[iseg].m_End;
                        m_plotter->ThickSegment( start, end,
                                                aZone->m_ZoneMinThickness,
                                                GetMode() );
                    }
                }

                // Plot the current filled area outline
                if( aZone->m_ZoneMinThickness > 0 )
                    m_plotter->PlotPoly( cornerList, NO_FILL, aZone->m_ZoneMinThickness );
            }
            else
            {
                if( aZone->m_ZoneMinThickness > 0 )
                {
                    for( unsigned jj = 1; jj<cornerList.size(); jj++ )
                        m_plotter->ThickSegment( cornerList[jj -1], cornerList[jj],
                                                ( GetMode() == LINE ) ? -1 : aZone->m_ZoneMinThickness,
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
    int     radius = 0, StAngle = 0, EndAngle = 0;

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
        radius = (int) hypot( (double) ( end.x - start.x ),
                              (double) ( end.y - start.y ) );
        m_plotter->ThickCircle( start, radius * 2, thickness, GetMode() );
        break;

    case S_ARC:
        radius = (int) hypot( (double) ( end.x - start.x ),
                              (double) ( end.y - start.y ) );
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


void PlotOneBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, int aLayer,
                     const PCB_PLOT_PARAMS& aPlotOpt )
{
    PCB_PLOT_PARAMS plotOpt = aPlotOpt;
    // Set a default color and the text mode for this layer
    aPlotter->SetColor( aPlotOpt.GetColor() );
    aPlotter->SetTextMode( aPlotOpt.GetTextMode() );

    // Specify that the contents of the "Edges Pcb" layer are to be plotted
    // in addition to the contents of the currently specified layer.
    int layer_mask = GetLayerMask( aLayer );

    if( !aPlotOpt.GetExcludeEdgeLayer() )
        layer_mask |= EDGE_LAYER;

    switch( aLayer )
    {
    case FIRST_COPPER_LAYER:
    case LAYER_N_2:
    case LAYER_N_3:
    case LAYER_N_4:
    case LAYER_N_5:
    case LAYER_N_6:
    case LAYER_N_7:
    case LAYER_N_8:
    case LAYER_N_9:
    case LAYER_N_10:
    case LAYER_N_11:
    case LAYER_N_12:
    case LAYER_N_13:
    case LAYER_N_14:
    case LAYER_N_15:
    case LAST_COPPER_LAYER:
        // Skip NPTH pads on copper layers ( only if hole size == pad size ):
        plotOpt.SetSkipPlotNPTH_Pads( true );
        // Drill mark will be plotted,
        // if drill mark is SMALL_DRILL_SHAPE  or FULL_DRILL_SHAPE
        PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        break;

    case SOLDERMASK_N_BACK:
    case SOLDERMASK_N_FRONT:
        plotOpt.SetSkipPlotNPTH_Pads( false );
        // Disable plot pad holes
        plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );
        PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        break;

    case SOLDERPASTE_N_BACK:
    case SOLDERPASTE_N_FRONT:
        plotOpt.SetSkipPlotNPTH_Pads( false );
        // Disable plot pad holes
        plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );
        PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        break;

    case SILKSCREEN_N_FRONT:
    case SILKSCREEN_N_BACK:
        PlotSilkScreen( aBoard, aPlotter, layer_mask, plotOpt );

        // Gerber: Subtract soldermask from silkscreen if enabled
        if( aPlotter->GetPlotterType() == PLOT_FORMAT_GERBER
            && plotOpt.GetSubtractMaskFromSilk() )
        {
            plotOpt.SetPlotViaOnMaskLayer( true );
            if( aLayer == SILKSCREEN_N_FRONT )
                layer_mask = GetLayerMask( SOLDERMASK_N_FRONT );
            else
                layer_mask = GetLayerMask( SOLDERMASK_N_BACK );

            // Create the mask to substract by creating a negative layer polarity
            aPlotter->SetLayerPolarity( false );
            // Disable plot pad holes
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );
            // Plot the mask
            PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        }
        break;

    default:
        PlotSilkScreen( aBoard, aPlotter, layer_mask, plotOpt );
        break;
    }
}


/* Plot a copper layer or mask.
 * Silk screen layers are not plotted here.
 */
void PlotStandardLayer( BOARD *aBoard, PLOTTER* aPlotter,
                        long aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt )
{

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerMask( aLayerMask );

    EDA_DRAW_MODE_T plotMode = aPlotOpt.GetMode();

     // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Draw footprint shapes without pads (pads will plotted later)
    // We plot here module texts, but they are usually on silkscreen layer,
    // so they are not plot here but plot by PlotSilkScreen()
    // Plot footprints fields (ref, value ...)
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        if( ! itemplotter.PlotAllTextsModule( module ) )
        {
            wxLogMessage( _( "Your BOARD has a bad layer number for module %s" ),
                           GetChars( module->GetReference() ) );
        }
    }

    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->m_Drawings; item; item = item->Next() )
        {
            if( ! (aLayerMask & GetLayerMask( item->GetLayer() ) ) )
                continue;

            switch( item->Type() )
            {
            case PCB_MODULE_EDGE_T:
                itemplotter.Plot_1_EdgeModule( (EDGE_MODULE*) item );
                break;

            default:
                break;
            }
        }
    }

    // Plot footprint pads
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads;  pad;  pad = pad->Next() )
        {
            if( (pad->GetLayerMask() & aLayerMask) == 0 )
                continue;

            wxPoint shape_pos = pad->ReturnShapePos();

            wxSize margin;
            double width_adj = 0;

            if( aLayerMask & ALL_CU_LAYERS )
            {
                width_adj =  itemplotter.getFineWidthAdj();
            }

            switch( aLayerMask &
                   ( SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT |
                     SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT ) )
            {
            case SOLDERMASK_LAYER_FRONT:
            case SOLDERMASK_LAYER_BACK:
                margin.x = margin.y = pad->GetSolderMaskMargin();
                break;

            case SOLDERPASTE_LAYER_FRONT:
            case SOLDERPASTE_LAYER_BACK:
                margin = pad->GetSolderPasteMargin();
                break;

            default:
                break;
            }

            wxSize size;
            size.x = pad->GetSize().x + ( 2 * margin.x ) + width_adj;
            size.y = pad->GetSize().y + ( 2 * margin.y ) + width_adj;

            // Don't draw a null size item :
            if( size.x <= 0 || size.y <= 0 )
                continue;

            EDA_COLOR_T color = BLACK;

            if( (pad->GetLayerMask() & LAYER_BACK) )
               color = aBoard->GetVisibleElementColor( PAD_BK_VISIBLE );

            if((pad->GetLayerMask() & LAYER_FRONT ) )
                color = ColorFromInt( color | aBoard->GetVisibleElementColor( PAD_FR_VISIBLE ) );

            // Set plot color (change WHITE to LIGHTGRAY because
            // the white items are not seen on a white paper or screen
            aPlotter->SetColor( color != WHITE ? color : LIGHTGRAY);

            switch( pad->GetShape() )
            {
            case PAD_CIRCLE:
                if( aPlotOpt.GetSkipPlotNPTH_Pads() &&
                    (pad->GetSize() == pad->GetDrillSize()) &&
                    (pad->GetAttribute() == PAD_HOLE_NOT_PLATED) )
                    break;

                aPlotter->FlashPadCircle( shape_pos, size.x, plotMode );
                break;

            case PAD_OVAL:
                if( aPlotOpt.GetSkipPlotNPTH_Pads() &&
                    (pad->GetSize() == pad->GetDrillSize()) &&
                    (pad->GetAttribute() == PAD_HOLE_NOT_PLATED) )
                    break;

                aPlotter->FlashPadOval( shape_pos, size, pad->GetOrientation(), plotMode );
                break;

            case PAD_TRAPEZOID:
            {
                wxPoint coord[4];
                pad->BuildPadPolygon( coord, margin, 0 );
                aPlotter->FlashPadTrapez( shape_pos, coord, pad->GetOrientation(), plotMode );
            }
            break;

            case PAD_RECT:
            default:
                aPlotter->FlashPadRect( shape_pos, size, pad->GetOrientation(), plotMode );
                break;
            }
        }
    }

    // Plot vias on copper layers, and if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    // plot them on solder mask
    for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        SEGVIA* Via = (SEGVIA*) track;

        // vias are not plotted if not on selected layer, but if layer
        // is SOLDERMASK_LAYER_BACK or SOLDERMASK_LAYER_FRONT,vias are drawn,
        // only if they are on the corresponding external copper layer
        int via_mask_layer = Via->ReturnMaskLayer();

        if( aPlotOpt.GetPlotViaOnMaskLayer() )
        {
            if( via_mask_layer & LAYER_BACK )
                via_mask_layer |= SOLDERMASK_LAYER_BACK;

            if( via_mask_layer & LAYER_FRONT )
                via_mask_layer |= SOLDERMASK_LAYER_FRONT;
        }

        if( ( via_mask_layer & aLayerMask ) == 0 )
            continue;

        int via_margin = 0;
        double width_adj = 0;

        // If the current layer is a solder mask, use the global mask
        // clearance for vias
        if( ( aLayerMask & ( SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT ) ) )
            via_margin = aBoard->GetDesignSettings().m_SolderMaskMargin;

        if( aLayerMask & ALL_CU_LAYERS )
            width_adj = itemplotter.getFineWidthAdj();

        int diameter = Via->m_Width + 2 * via_margin + width_adj;

        // Don't draw a null size item :
        if( diameter <= 0 )
            continue;

        EDA_COLOR_T color = aBoard->GetVisibleElementColor(VIAS_VISIBLE + Via->m_Shape);
        // Set plot color (change WHITE to LIGHTGRAY because
        // the white items are not seen on a white paper or screen
        aPlotter->SetColor( color != WHITE ? color : LIGHTGRAY);
        aPlotter->FlashPadCircle( Via->m_Start, diameter, plotMode );
    }

    // Plot tracks (not vias) :
    for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( (GetLayerMask( track->GetLayer() ) & aLayerMask) == 0 )
            continue;

        int width = track->m_Width + itemplotter.getFineWidthAdj();
        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );
        aPlotter->ThickSegment( track->m_Start, track->m_End, width, plotMode );
    }

    // Plot zones (outdated, for old boards compatibility):
    for( TRACK* track = aBoard->m_Zone; track; track = track->Next() )
    {
        if( (GetLayerMask( track->GetLayer() ) & aLayerMask) == 0 )
            continue;

        int width = track->m_Width + itemplotter.getFineWidthAdj();
        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );
        aPlotter->ThickSegment( track->m_Start, track->m_End, width, plotMode );
    }

    // Plot filled ares
    for( int ii = 0; ii < aBoard->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = aBoard->GetArea( ii );

        if( ( ( 1 << zone->GetLayer() ) & aLayerMask ) == 0 )
            continue;

        itemplotter.PlotFilledAreas( zone );
    }

    // Adding drill marks, if required and if the plotter is able to plot them:
    if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        itemplotter.PlotDrillMarks();
}

/** Helper function to plot a single drill mark. It compensate and clamp
 *   the drill mark size depending on the current plot options
 */
void BRDITEMS_PLOTTER::plotOneDrillMark( PAD_SHAPE_T aDrillShape,
                           const wxPoint &aDrillPos, wxSize aDrillSize,
                           const wxSize &aPadSize,
                           double aOrientation, int aSmallDrill )
{
    // Small drill marks have no significance when applied to slots
    if( aSmallDrill && aDrillShape == PAD_ROUND )
        aDrillSize.x = std::min( aSmallDrill, aDrillSize.x );

    // Round holes only have x diameter, slots have both
    aDrillSize.x -= getFineWidthAdj();
    aDrillSize.x = Clamp( 1, aDrillSize.x, aPadSize.x - 1 );
    if( aDrillShape == PAD_OVAL )
    {
        aDrillSize.y -= getFineWidthAdj();
        aDrillSize.y = Clamp( 1, aDrillSize.y, aPadSize.y - 1 );
        m_plotter->FlashPadOval( aDrillPos, aDrillSize, aOrientation, GetMode() );
    }
    else
        m_plotter->FlashPadCircle( aDrillPos, aDrillSize.x, GetMode() );
}

/* Function PlotDrillMarks
 * Draw a drill mark for pads and vias.
 * Must be called after all drawings, because it
 * redraw the drill mark on a pad or via, as a negative (i.e. white) shape in
 * FILLED plot mode (for PS and PDF outputs)
 */
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
        if( pts->Type() != PCB_VIA_T )
            continue;

        plotOneDrillMark(PAD_CIRCLE,
                       pts->m_Start, wxSize( pts->GetDrillValue(), 0 ),
                       wxSize( pts->m_Width, 0 ), 0, small_drill );
    }

    for( MODULE *Module = m_board->m_Modules; Module != NULL; Module = Module->Next() )
    {
        for( D_PAD *pad = Module->m_Pads; pad != NULL; pad = pad->Next() )
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

/** Set up most plot options for plotting a board (especially the viewport)
 * Important thing:
 *      page size is the 'drawing' page size,
 *      paper size is the physical page size
 */
static void PlotSetupPlotter( PLOTTER *aPlotter, PCB_PLOT_PARAMS *aPlotOpts,
                              const PAGE_INFO &aPageInfo,
                              const EDA_RECT &aBbbox,
                              const wxPoint &aOrigin,
                              const wxString &aFilename )
{
    PAGE_INFO pageA4( wxT( "A4" ) );
    const PAGE_INFO* sheet_info;
    double paperscale; // Page-to-paper ratio
    wxSize paperSizeIU;
    wxSize pageSizeIU( aPageInfo.GetSizeIU() );
    bool autocenter = false;

    /* Special options: to fit the sheet to an A4 sheet replace
       the paper size. However there is a difference between
       the autoscale and the a4paper option:
       - Autoscale fits the board to the paper size
       - A4paper fits the original paper size to an A4 sheet
       - Both of them fit the board to an A4 sheet
     */
    if( aPlotOpts->GetA4Output() )      // Fit paper to A4
    {
        sheet_info  = &pageA4;
        paperSizeIU = pageA4.GetSizeIU();
        paperscale  = (double) paperSizeIU.x / pageSizeIU.x;
        autocenter  = true;
    }
    else
    {
        sheet_info  = &aPageInfo;
        paperSizeIU = pageSizeIU;
        paperscale  = 1;

        // Need autocentering only if scale is not 1:1
        autocenter  = (aPlotOpts->GetScale() != 1.0);
    }

    wxPoint boardCenter = aBbbox.Centre();

    double compound_scale;
    wxSize boardSize = aBbbox.GetSize();

    /* Fit to 80% of the page if asked; it could be that the board is empty,
     * in this case regress to 1:1 scale */
    if( aPlotOpts->GetAutoScale() && boardSize.x > 0 && boardSize.y > 0 )
    {
        double xscale = (paperSizeIU.x * 0.8) / boardSize.x;
        double yscale = (paperSizeIU.y * 0.8) / boardSize.y;

        compound_scale = std::min( xscale, yscale ) * paperscale;
    }
    else
        compound_scale = aPlotOpts->GetScale() * paperscale;


    /* For the plot offset we have to keep in mind the auxiliary origin
       too: if autoscaling is off we check that plot option (i.e. autoscaling
       overrides auxiliary origin) */
    wxPoint offset( 0, 0);

    if( autocenter )
    {
        offset.x = KiROUND( boardCenter.x - ( paperSizeIU.x / 2.0 ) / compound_scale );
        offset.y = KiROUND( boardCenter.y - ( paperSizeIU.y / 2.0 ) / compound_scale );
    }
    else
    {
        if( aPlotOpts->GetUseAuxOrigin() )
            offset = aOrigin;
    }

    /* Configure the plotter object with all the stuff computed and
       most of that taken from the options */
    aPlotter->SetPageSettings( *sheet_info );

    aPlotter->SetViewport( offset, IU_PER_DECIMILS, compound_scale,
                           aPlotOpts->GetMirror() );
    aPlotter->SetDefaultLineWidth( aPlotOpts->GetLineWidth() );
    aPlotter->SetCreator( wxT( "PCBNEW" ) );
    aPlotter->SetColorMode( false );        // default is plot in Black and White.
    aPlotter->SetFilename( aFilename );
    aPlotter->SetTextMode( aPlotOpts->GetTextMode() );
}

/** Prefill in black an area a little bigger than the board to prepare for the
 *  negative plot */
static void FillNegativeKnockout( PLOTTER *aPlotter, const EDA_RECT &aBbbox )
{
    const int margin = 5 * IU_PER_MM;   // Add a 5 mm margin around the board
    aPlotter->SetNegative( true );
    aPlotter->SetColor( WHITE );       // Which will be plotted as black
    EDA_RECT area = aBbbox;
    area.Inflate( margin );
    aPlotter->Rect( area.GetOrigin(), area.GetEnd(), FILLED_SHAPE );
    aPlotter->SetColor( BLACK );
}

/** Calculate the effective size of HPGL pens and set them in the
 * plotter object */
static void ConfigureHPGLPenSizes( HPGL_PLOTTER *aPlotter,
                                   PCB_PLOT_PARAMS *aPlotOpts )
{
    /* Compute pen_dim (the value is given in mils) in pcb units,
       with plot scale (if Scale is 2, pen diameter value is always m_HPGLPenDiam
       so apparent pen diam is actually pen diam / Scale */
    int pen_diam = KiROUND( aPlotOpts->GetHPGLPenDiameter() * IU_PER_MILS /
                            aPlotOpts->GetScale() );

    // compute pen_overlay (value comes in mils) in pcb units with plot scale
    if( aPlotOpts->GetHPGLPenOverlay() < 0 )
        aPlotOpts->SetHPGLPenOverlay( 0 );

    if( aPlotOpts->GetHPGLPenOverlay() >= aPlotOpts->GetHPGLPenDiameter() )
        aPlotOpts->SetHPGLPenOverlay( aPlotOpts->GetHPGLPenDiameter() - 1 );

    int pen_overlay = KiROUND( aPlotOpts->GetHPGLPenOverlay() * IU_PER_MILS /
                               aPlotOpts->GetScale() );

    // Set HPGL-specific options and start
    aPlotter->SetPenSpeed( aPlotOpts->GetHPGLPenSpeed() );
    aPlotter->SetPenNumber( aPlotOpts->GetHPGLPenNum() );
    aPlotter->SetPenOverlap( pen_overlay );
    aPlotter->SetPenDiameter( pen_diam );
}

/** Open a new plotfile using the options (and especially the format)
 * specified in the options and prepare the page for plotting.
 * Return the plotter object if OK, NULL if the file is not created
 * (or has a problem)
 */
PLOTTER *StartPlotBoard( BOARD *aBoard, PCB_PLOT_PARAMS *aPlotOpts,
                         const wxString& aFullFileName,
                         const wxString& aSheetDesc )
{
    const PAGE_INFO& pageInfo = aBoard->GetPageSettings();
    FILE* output_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( output_file == NULL )
        return NULL;

    // Create the plotter driver and set the few plotter specific
    // options
    PLOTTER *the_plotter = NULL;
    switch( aPlotOpts->GetFormat() )
    {
    case PLOT_FORMAT_DXF:
        the_plotter = new DXF_PLOTTER();
        break;

    case PLOT_FORMAT_POST:
        PS_PLOTTER* PS_plotter;
        PS_plotter = new PS_PLOTTER();
        PS_plotter->SetScaleAdjust( aPlotOpts->GetFineScaleAdjustX(),
                                    aPlotOpts->GetFineScaleAdjustY() );
        the_plotter = PS_plotter;
        break;

    case PLOT_FORMAT_PDF:
        the_plotter = new PDF_PLOTTER();
        break;

    case PLOT_FORMAT_HPGL:
        HPGL_PLOTTER* HPGL_plotter;
        HPGL_plotter = new HPGL_PLOTTER();

        /* HPGL options are a little more convoluted to compute, so
           they're split in an other function */
        ConfigureHPGLPenSizes( HPGL_plotter, aPlotOpts );
        the_plotter = HPGL_plotter;
        break;

    case PLOT_FORMAT_GERBER:
        the_plotter = new GERBER_PLOTTER();
        break;

    case PLOT_FORMAT_SVG:
        the_plotter = new SVG_PLOTTER();
        break;

    default:
        wxASSERT( false );
    }

    if( the_plotter )
    {
        EDA_RECT bbbox = aBoard->ComputeBoundingBox();
        wxPoint auxOrigin( aBoard->GetOriginAxisPosition() );
        // Compute the viewport and set the other options
        PlotSetupPlotter( the_plotter, aPlotOpts, pageInfo, bbbox, auxOrigin,
                          aFullFileName );

        if( the_plotter->StartPlot( output_file ) )
        {
            /* When plotting a negative board: draw a black rectangle
             * (background for plot board in white) and switch the current
             * color to WHITE; note the color inversion is actually done
             * in the driver (if supported) */
            if( aPlotOpts->GetNegative() )
                FillNegativeKnockout( the_plotter, bbbox );

            // Plot the frame reference if requested
            if( aPlotOpts->GetPlotFrameRef() )
                PlotWorkSheet( the_plotter, aBoard->GetTitleBlock(),
                               aBoard->GetPageSettings(),
                               1, 1, // Only one page
                               aSheetDesc, aBoard->GetFileName() );

            return the_plotter;
        }
    }

    // error in start_plot( ) or before
    wxMessageBox(  _("Error creating plot file") );
    delete the_plotter;
    return NULL;
}

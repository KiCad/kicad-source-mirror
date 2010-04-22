/*************************/
/* Common plot routines. */
/*************************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"
#include "class_board_design_settings.h"


static void Plot_Edges_Modules( PLOTTER* plotter, BOARD* pcb, int masque_layer,
                                GRTraceMode trace_mode );
static void PlotTextModule( PLOTTER* plotter, TEXTE_MODULE* pt_texte,
                            GRTraceMode trace_mode );


/* Creates the plot for silkscreen layers
 */
void WinEDA_BasePcbFrame::Plot_Serigraphie( PLOTTER*    plotter,
                                            int         masque_layer,
                                            GRTraceMode trace_mode )
{
    wxPoint         pos, shape_pos;
    wxSize          size;
    bool            trace_val, trace_ref;
    D_PAD*          pt_pad;
    TEXTE_MODULE*   pt_texte;
    EDA_BaseStruct* PtStruct;

    /* Plot edge layer and graphic items */

    for( PtStruct = m_Pcb->m_Drawings;
        PtStruct != NULL;
        PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            PlotDrawSegment( plotter,
                             (DRAWSEGMENT*) PtStruct,
                             masque_layer,
                             trace_mode );
            break;

        case TYPE_TEXTE:
            PlotTextePcb( plotter,
                          (TEXTE_PCB*) PtStruct,
                          masque_layer,
                          trace_mode );
            break;

        case TYPE_COTATION:
            PlotCotation( plotter,
                          (COTATION*) PtStruct,
                          masque_layer,
                          trace_mode );
            break;

        case TYPE_MIRE:
            PlotMirePcb( plotter,
                         (MIREPCB*) PtStruct,
                         masque_layer,
                         trace_mode );
            break;

        case TYPE_MARKER_PCB:
            break;

        default:
            DisplayError( this,
                          wxT( "Plot_Serigraphie() error: unexpected Type()" ) );
            break;
        }
    }

    /* Plot footprint outlines : */
    Plot_Edges_Modules( plotter, m_Pcb, masque_layer, trace_mode );

    /* Plot pads (creates pads outlines, for pads on silkscreen layers) */
    if( g_pcb_plot_options.PlotPadsOnSilkLayer )
    {
        for( MODULE* Module = m_Pcb->m_Modules;
             Module;
             Module = Module->Next() )
        {
            for( pt_pad = (D_PAD*) Module->m_Pads;
                 pt_pad != NULL;
                 pt_pad = pt_pad->Next() )
            {
                /* See if the pad is on this layer */
                if( (pt_pad->m_Masque_Layer & masque_layer) == 0 )
                    continue;

                shape_pos = pt_pad->ReturnShapePos();
                pos  = shape_pos;
                size = pt_pad->m_Size;

                switch( pt_pad->m_PadShape & 0x7F )
                {
                case PAD_CIRCLE:
                    plotter->flash_pad_circle( pos, size.x, FILAIRE );
                    break;

                case PAD_OVAL:
                    plotter->flash_pad_oval( pos, size,
                                             pt_pad->m_Orient, FILAIRE );
                    break;

                case PAD_TRAPEZOID:
                {
                    wxSize delta;
                    delta = pt_pad->m_DeltaSize;
                    plotter->flash_pad_trapez( pos, size,
                                               delta, pt_pad->m_Orient,
                                               FILAIRE );
                    break;
                }

                case PAD_RECT:
                default:
                    plotter->flash_pad_rect( pos,
                                             size,
                                             pt_pad->m_Orient,
                                             FILAIRE );
                    break;
                }
            }
        }
    }

    /* Plot footprints fields (ref, value ...) */
    for( MODULE* Module = m_Pcb->m_Modules; Module; Module = Module->Next() )
    {
        /* see if we want to plot VALUE and REF fields */
        trace_val = g_pcb_plot_options.Sel_Texte_Valeur;
        trace_ref = g_pcb_plot_options.Sel_Texte_Reference;

        TEXTE_MODULE* text = Module->m_Reference;
        unsigned      textLayer = text->GetLayer();

        if( textLayer >= 32 )
        {
            wxString errMsg;

            errMsg.Printf( _( "Your BOARD has a bad layer number of %u for \
module\n %s's \"reference\" text." ),
                           textLayer, GetChars( Module->GetReference() ) );
            DisplayError( this, errMsg );
            return;
        }

        if( ( ( 1 << textLayer ) & masque_layer ) == 0 )
            trace_ref = FALSE;

        if( text->m_NoShow && !g_pcb_plot_options.Sel_Texte_Invisible )
            trace_ref = FALSE;

        text = Module->m_Value;
        textLayer = text->GetLayer();

        if( textLayer > 32 )
        {
            wxString errMsg;

            errMsg.Printf( _( "Your BOARD has a bad layer number of %u for \
module\n %s's \"value\" text." ),
                           textLayer, GetChars( Module->GetReference() ) );
            DisplayError( this, errMsg );
            return;
        }

        if( ( (1 << textLayer) & masque_layer ) == 0 )
            trace_val = FALSE;

        if( text->m_NoShow && !g_pcb_plot_options.Sel_Texte_Invisible )
            trace_val = FALSE;

        /* Plot text fields, if allowed */
        if( trace_ref )
            PlotTextModule( plotter, Module->m_Reference, trace_mode );

        if( trace_val )
            PlotTextModule( plotter, Module->m_Value, trace_mode );

        for( pt_texte = (TEXTE_MODULE*) Module->m_Drawings.GetFirst();
             pt_texte != NULL;
             pt_texte = pt_texte->Next() )
        {
            if( pt_texte->Type() != TYPE_TEXTE_MODULE )
                continue;

            if( !g_pcb_plot_options.Sel_Texte_Divers )
                continue;
            if( (pt_texte->m_NoShow)
               && !g_pcb_plot_options.Sel_Texte_Invisible )
                continue;

            textLayer = pt_texte->GetLayer();
            if( textLayer >= 32 )
            {
                wxString errMsg;

                errMsg.Printf( _( "Your BOARD has a bad layer number of %u \
for module\n %s's \"module text\" text of %s." ),
                               textLayer, GetChars( Module->GetReference() ),
                               GetChars( pt_texte->m_Text ) );
                DisplayError( this, errMsg );
                return;
            }

            if( !( ( 1 << textLayer ) & masque_layer ) )
                continue;

            PlotTextModule( plotter, pt_texte, trace_mode );
        }
    }

    /* Plot filled areas */
    for( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->GetArea( ii );
        if( ( ( 1 << edge_zone->GetLayer() ) & masque_layer ) == 0 )
            continue;
        PlotFilledAreas( plotter, edge_zone, trace_mode );
    }

    // Plot segments used to fill zone areas (outdated, but here for old boards
    // compatibility):
    for( SEGZONE* seg = m_Pcb->m_Zone; seg != NULL; seg = seg->Next() )
    {
        if( ( ( 1 << seg->GetLayer() ) & masque_layer ) == 0 )
            continue;
        plotter->thick_segment( seg->m_Start, seg->m_End, seg->m_Width,
                                trace_mode );
    }
}


static void PlotTextModule( PLOTTER* plotter, TEXTE_MODULE* pt_texte,
                            GRTraceMode trace_mode )
{
    wxSize  size;
    wxPoint pos;
    int     orient, thickness;

    /* calculate some text parameters :*/
    size = pt_texte->m_Size;
    pos  = pt_texte->m_Pos;

    orient = pt_texte->GetDrawRotation();

    thickness = pt_texte->m_Width;
    if( trace_mode == FILAIRE )
        thickness = -1;

    if( pt_texte->m_Mirror )
        NEGATE( size.x );  // Text is mirrored

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = pt_texte->m_Bold || thickness;

    plotter->text( pos, BLACK,
                   pt_texte->m_Text,
                   orient, size,
                   pt_texte->m_HJustify, pt_texte->m_VJustify,
                   thickness, pt_texte->m_Italic, allow_bold );
}


void PlotCotation( PLOTTER* plotter, COTATION* Cotation, int masque_layer,
                   GRTraceMode trace_mode )
{
    DRAWSEGMENT* DrawTmp;

    if( (g_TabOneLayerMask[Cotation->GetLayer()] & masque_layer) == 0 )
        return;

    DrawTmp = new DRAWSEGMENT( NULL );

    DrawTmp->m_Width = (trace_mode==FILAIRE) ? -1 : Cotation->m_Width;
    DrawTmp->SetLayer( Cotation->GetLayer() );

    PlotTextePcb( plotter, Cotation->m_Text, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->Barre_ox;
    DrawTmp->m_Start.y = Cotation->Barre_oy;
    DrawTmp->m_End.x = Cotation->Barre_fx;
    DrawTmp->m_End.y = Cotation->Barre_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->TraitG_ox;
    DrawTmp->m_Start.y = Cotation->TraitG_oy;
    DrawTmp->m_End.x = Cotation->TraitG_fx;
    DrawTmp->m_End.y = Cotation->TraitG_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->TraitD_ox;
    DrawTmp->m_Start.y = Cotation->TraitD_oy;
    DrawTmp->m_End.x = Cotation->TraitD_fx;
    DrawTmp->m_End.y = Cotation->TraitD_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->FlecheD1_ox;
    DrawTmp->m_Start.y = Cotation->FlecheD1_oy;
    DrawTmp->m_End.x = Cotation->FlecheD1_fx;
    DrawTmp->m_End.y = Cotation->FlecheD1_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->FlecheD2_ox;
    DrawTmp->m_Start.y = Cotation->FlecheD2_oy;
    DrawTmp->m_End.x = Cotation->FlecheD2_fx;
    DrawTmp->m_End.y = Cotation->FlecheD2_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->FlecheG1_ox;
    DrawTmp->m_Start.y = Cotation->FlecheG1_oy;
    DrawTmp->m_End.x = Cotation->FlecheG1_fx;
    DrawTmp->m_End.y = Cotation->FlecheG1_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Cotation->FlecheG2_ox;
    DrawTmp->m_Start.y = Cotation->FlecheG2_oy;
    DrawTmp->m_End.x = Cotation->FlecheG2_fx;
    DrawTmp->m_End.y = Cotation->FlecheG2_fy;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    delete DrawTmp;
}


void PlotMirePcb( PLOTTER* plotter, MIREPCB* Mire, int masque_layer,
                  GRTraceMode trace_mode )
{
    DRAWSEGMENT* DrawTmp;
    int          dx1, dx2, dy1, dy2, radius;

    if( (g_TabOneLayerMask[Mire->GetLayer()] & masque_layer) == 0 )
        return;

    DrawTmp = new DRAWSEGMENT( NULL );

    DrawTmp->m_Width = ( trace_mode == FILAIRE ) ? -1 : Mire->m_Width;
    DrawTmp->SetLayer( Mire->GetLayer() );

    DrawTmp->m_Start.x = Mire->m_Pos.x; DrawTmp->m_Start.y = Mire->m_Pos.y;
    DrawTmp->m_End.x   = DrawTmp->m_Start.x + ( Mire->m_Size / 4 );
    DrawTmp->m_End.y   = DrawTmp->m_Start.y;
    DrawTmp->m_Shape   = S_CIRCLE;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Shape = S_SEGMENT;

    radius = Mire->m_Size / 2;
    dx1    = radius;
    dy1    = 0;
    dx2    = 0;
    dy2    = radius;

    if( Mire->m_Shape ) /* Shape X */
    {
        dx1 = dy1 = ( radius * 7 ) / 5;
        dx2 = dx1;
        dy2 = -dy1;
    }

    DrawTmp->m_Start.x = Mire->m_Pos.x - dx1;
    DrawTmp->m_Start.y = Mire->m_Pos.y - dy1;
    DrawTmp->m_End.x = Mire->m_Pos.x + dx1;
    DrawTmp->m_End.y = Mire->m_Pos.y + dy1;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    DrawTmp->m_Start.x = Mire->m_Pos.x - dx2;
    DrawTmp->m_Start.y = Mire->m_Pos.y - dy2;
    DrawTmp->m_End.x = Mire->m_Pos.x + dx2;
    DrawTmp->m_End.y = Mire->m_Pos.y + dy2;
    PlotDrawSegment( plotter, DrawTmp, masque_layer, trace_mode );

    delete DrawTmp;
}


/* Plot footprints graphic items (outlines) */
void Plot_Edges_Modules( PLOTTER* plotter, BOARD* pcb, int masque_layer,
                         GRTraceMode trace_mode )
{
    for( MODULE* module = pcb->m_Modules; module; module = module->Next() )
    {
        for( EDGE_MODULE* edge = (EDGE_MODULE*) module->m_Drawings.GetFirst();
             edge;
             edge = edge->Next() )
        {
            if( edge->Type() != TYPE_EDGE_MODULE )
                continue;

            if( ( g_TabOneLayerMask[edge->GetLayer()] & masque_layer ) == 0 )
                continue;

            Plot_1_EdgeModule( plotter, edge, trace_mode );
        }
    }
}


/* Plot a graphic item (outline) relative to a footprint */
void Plot_1_EdgeModule( PLOTTER* plotter, EDGE_MODULE* PtEdge,
                        GRTraceMode trace_mode )
{
    int     type_trace;         /* Type of item to plot. */
    int     thickness;          /* Segment thickness. */
    int     radius;             /* Circle radius. */
    int     StAngle, EndAngle;
    wxPoint pos, end;

    if( PtEdge->Type() != TYPE_EDGE_MODULE )
        return;

    type_trace = PtEdge->m_Shape;
    thickness  = PtEdge->m_Width;

    pos = PtEdge->m_Start;
    end = PtEdge->m_End;

    switch( type_trace )
    {
    case S_SEGMENT:
        plotter->thick_segment( pos, end, thickness, trace_mode );
        break;

    case S_CIRCLE:
        radius = (int) hypot( (double) ( end.x - pos.x ),
                              (double) ( end.y - pos.y ) );
        plotter->thick_circle( pos, radius * 2, thickness, trace_mode );
        break;

    case S_ARC:
        radius = (int) hypot( (double) ( end.x - pos.x ),
                              (double) ( end.y - pos.y ) );
        StAngle  = ArcTangente( end.y - pos.y, end.x - pos.x );
        EndAngle = StAngle + PtEdge->m_Angle;
        plotter->thick_arc( pos,
                            -EndAngle,
                            -StAngle,
                            radius,
                            thickness,
                            trace_mode );
        break;

    case S_POLYGON:
    {
        // We must compute true coordinates from m_PolyList
        // which are relative to module position, orientation 0
        MODULE* Module = NULL;
        if( PtEdge->GetParent() && (PtEdge->GetParent()->Type() == TYPE_MODULE) )
            Module = (MODULE*) PtEdge->GetParent();

        int* ptr_base =
            (int*) MyMalloc( 2 * PtEdge->m_PolyPoints.size() * sizeof(int) );
        int* ptr = ptr_base;

        int* source = (int*) &PtEdge->m_PolyPoints[0];

        for( unsigned ii = 0; ii < PtEdge->m_PolyPoints.size(); ii++ )
        {
            int x = *source++;
            int y = *source++;

            if( Module )
            {
                RotatePoint( &x, &y, Module->m_Orient );
                x += Module->m_Pos.x;
                y += Module->m_Pos.y;
            }

            x += PtEdge->m_Start0.x;
            y += PtEdge->m_Start0.y;

            *ptr++ = x;
            *ptr++ = y;
        }

        plotter->poly( PtEdge->m_PolyPoints.size(), ptr_base, NO_FILL,
                       thickness );
        free( ptr_base );
    }
    break;
    }
}


/* Plot a PCB Text, i;e. a text found on a copper or technical layer */
void PlotTextePcb( PLOTTER* plotter, TEXTE_PCB* pt_texte, int masque_layer,
                   GRTraceMode trace_mode )
{
    int     orient, thickness;
    wxPoint pos;
    wxSize  size;

    if( pt_texte->m_Text.IsEmpty() )
        return;
    if( ( g_TabOneLayerMask[pt_texte->GetLayer()] & masque_layer ) == 0 )
        return;

    size = pt_texte->m_Size;
    pos  = pt_texte->m_Pos;
    orient    = pt_texte->m_Orient;
    thickness = ( trace_mode==FILAIRE ) ? -1 : pt_texte->m_Width;

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
            plotter->text( pos, BLACK,
                           txt,
                           orient, size,
                           pt_texte->m_HJustify, pt_texte->m_VJustify,
                           thickness, pt_texte->m_Italic, allow_bold );
            pos += offset;
        }

        delete (list);
    }
    else
        plotter->text( pos, BLACK,
                       pt_texte->m_Text,
                       orient, size,
                       pt_texte->m_HJustify, pt_texte->m_VJustify,
                       thickness, pt_texte->m_Italic, allow_bold );
}


/* Plot areas (given by .m_FilledPolysList member) in a zone
 */
void PlotFilledAreas( PLOTTER* plotter, ZONE_CONTAINER* aZone,
                      GRTraceMode trace_mode )
{
    static int*     CornersBuffer     = NULL;
    static unsigned CornersBufferSize = 0;
    unsigned        imax = aZone->m_FilledPolysList.size();

    if( imax == 0 )  // Nothing to draw
        return;

    // We need a buffer to store corners coordinates:

    imax++;   // provide room to sore an extra coordinate to close the polygon
    if( CornersBuffer == NULL )
    {
        CornersBufferSize = imax * 2;
        CornersBuffer     = (int*) MyMalloc( CornersBufferSize * sizeof(int) );
    }

    if( (imax * 4) > CornersBufferSize )
    {
        CornersBufferSize = imax * 2;
        CornersBuffer     = (int*) realloc( CornersBuffer,
                                            CornersBufferSize * sizeof(int) );
    }

    imax--;

    /* Plot all filled areas: filled areas have a filled area and a thick
     * outline we must plot the filled area itself ( as a filled polygon
     * OR a set of segments ) and plot the thick outline itself
     *
     * in non filled mode the outline is plotted, but not the filling items
     */
    int corners_count = 0;
    for( unsigned ic = 0, ii = 0; ic < imax; ic++ )
    {
        CPolyPt* corner = &aZone->m_FilledPolysList[ic];
        CornersBuffer[ii++] = corner->x;
        CornersBuffer[ii++] = corner->y;
        corners_count++;
        if( corner->end_contour )   // Plot the current filled area outline
        {
            // First, close the outline
            if( CornersBuffer[0] != CornersBuffer[ii - 2]
                || CornersBuffer[1] != CornersBuffer[ii - 1] )
            {
                CornersBuffer[ii++] = CornersBuffer[0];
                CornersBuffer[ii]   = CornersBuffer[1];
                corners_count++;
            }

            // Plot the current filled area and its outline
            if( trace_mode == FILLED )
            {
                // Plot the current filled area polygon
                if( aZone->m_FillMode == 0 )    // We are using solid polygons
                                                // (if != 0: using segments )
                    plotter->poly( corners_count, CornersBuffer, FILLED_SHAPE );
                else                            // We are using areas filled by
                                                // segments: plot hem )
                {
                    for( unsigned iseg = 0;
                         iseg < aZone->m_FillSegmList.size();
                         iseg++ )
                    {
                        wxPoint start = aZone->m_FillSegmList[iseg].m_Start;
                        wxPoint end   = aZone->m_FillSegmList[iseg].m_End;
                        plotter->thick_segment( start,
                                                end,
                                                aZone->m_ZoneMinThickness,
                                                trace_mode );
                    }
                }

                // Plot the current filled area outline
                if( aZone->m_ZoneMinThickness > 0 )
                    plotter->poly( corners_count, CornersBuffer, NO_FILL,
                                   aZone->m_ZoneMinThickness );
            }
            else
            {
                if( aZone->m_ZoneMinThickness > 0 )
                {
                    for( int ii = 1; ii<corners_count; ii++ )
                        plotter->thick_segment(
                            wxPoint( CornersBuffer[ii * 2 - 2],
                                     CornersBuffer[ii * 2 - 1] ),
                            wxPoint( CornersBuffer[ii * 2],
                                     CornersBuffer[ii * 2 + 1] ),
                            ( trace_mode == FILAIRE ) ? -1 : aZone->m_ZoneMinThickness,
                            trace_mode );
                }
                plotter->set_current_line_width( -1 );
            }
            corners_count = 0;
            ii = 0;
        }
    }
}


/* Plot items type DRAWSEGMENT on layers allowed by masque_layer
 */
void PlotDrawSegment( PLOTTER* plotter, DRAWSEGMENT* pt_segm, int masque_layer,
                      GRTraceMode trace_mode )
{
    wxPoint start, end;
    int     thickness;
    int     radius = 0, StAngle = 0, EndAngle = 0;

    if( (g_TabOneLayerMask[pt_segm->GetLayer()] & masque_layer) == 0 )
        return;

    if( trace_mode == FILAIRE )
        thickness = g_pcb_plot_options.PlotLine_Width;
    else
        thickness = pt_segm->m_Width;

    start = pt_segm->m_Start;
    end   = pt_segm->m_End;

    plotter->set_current_line_width( thickness );
    switch( pt_segm->m_Shape )
    {
    case S_CIRCLE:
        radius =
            (int) hypot( (double) ( end.x - start.x ),
                         (double) ( end.y - start.y ) );
        plotter->thick_circle( start, radius * 2, thickness, trace_mode );
        break;

    case S_ARC:
        radius =
            (int) hypot( (double) ( end.x - start.x ),
                         (double) ( end.y - start.y ) );
        StAngle  = ArcTangente( end.y - start.y, end.x - start.x );
        EndAngle = StAngle + pt_segm->m_Angle;
        plotter->thick_arc( start,
                            -EndAngle,
                            -StAngle,
                            radius,
                            thickness,
                            trace_mode );
        break;

    case S_CURVE:
        for( unsigned i = 1; i < pt_segm->m_BezierPoints.size(); i++ )
            plotter->thick_segment( pt_segm->m_BezierPoints[i - 1],
                                    pt_segm->m_BezierPoints[i],
                                    thickness,
                                    trace_mode );

        break;

    default:
        plotter->thick_segment( start, end, thickness, trace_mode );
    }
}


void WinEDA_BasePcbFrame::Plot_Layer( PLOTTER* plotter, int Layer,
                                      GRTraceMode trace_mode )
{
    // Specify that the contents of the "Edges Pcb" layer are to be plotted
    // in addition to the contents of the currently specified layer.
    int layer_mask = g_TabOneLayerMask[Layer];

    if( !g_pcb_plot_options.Exclude_Edges_Pcb )
        layer_mask |= EDGE_LAYER;

    switch( Layer )
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
        Plot_Standard_Layer( plotter, layer_mask, true, trace_mode );

        // Adding drill marks, if required and if the plotter is able to plot
        // them:
        if( g_pcb_plot_options.DrillShapeOpt !=
            PCB_Plot_Options::NO_DRILL_SHAPE )
        {
            if( plotter->GetPlotterType() == PLOT_FORMAT_POST )
                PlotDrillMark( plotter,
                               trace_mode,
                               g_pcb_plot_options.DrillShapeOpt ==
                               PCB_Plot_Options::SMALL_DRILL_SHAPE );
        }
        break;

    case SOLDERMASK_N_BACK:
    case SOLDERMASK_N_FRONT:
        Plot_Standard_Layer( plotter, layer_mask,
                             g_pcb_plot_options.DrawViaOnMaskLayer, trace_mode );
        break;

    case SOLDERPASTE_N_BACK:
    case SOLDERPASTE_N_FRONT:
        Plot_Standard_Layer( plotter, layer_mask, false, trace_mode );
        break;

    default:
        Plot_Serigraphie( plotter, layer_mask, trace_mode );
        break;
    }
}


/* Plot a copper layer or mask in HPGL format.
 * HPGL unit = 0.98 mils (1 mil = 1.02041 unit HPGL).
 */
void WinEDA_BasePcbFrame::Plot_Standard_Layer( PLOTTER*    aPlotter,
                                               int         aLayerMask,
                                               bool        aPlotVia,
                                               GRTraceMode aPlotMode )
{
    wxPoint  pos;
    wxSize   size;
    wxString msg;

    // Plot pcb draw items.
    for( BOARD_ITEM* item = m_Pcb->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_DRAWSEGMENT:
            PlotDrawSegment( aPlotter,
                             (DRAWSEGMENT*) item,
                             aLayerMask,
                             aPlotMode );
            break;

        case TYPE_TEXTE:
            PlotTextePcb( aPlotter, (TEXTE_PCB*) item, aLayerMask, aPlotMode );
            break;

        case TYPE_COTATION:
            PlotCotation( aPlotter, (COTATION*) item, aLayerMask, aPlotMode );
            break;

        case TYPE_MIRE:
            PlotMirePcb( aPlotter, (MIREPCB*) item, aLayerMask, aPlotMode );
            break;

        case TYPE_MARKER_PCB:
            break;

        default:
            DisplayError( this,
                          wxT( "Plot_Standard_Layer() error : Unexpected Draw Type" ) );
            break;
        }
    }

    /* Draw footprint shapes without pads (pads will plotted later) */
    for( MODULE* module = m_Pcb->m_Modules; module; module = module->Next() )
    {
        for( BOARD_ITEM* item = module->m_Drawings; item; item = item->Next() )
        {
            switch( item->Type() )
            {
            case TYPE_EDGE_MODULE:
                if( aLayerMask & g_TabOneLayerMask[ item->GetLayer() ] )
                    Plot_1_EdgeModule( aPlotter,
                                       (EDGE_MODULE*) item,
                                       aPlotMode );
                break;

            default:
                break;
            }
        }
    }

    /* Plot footprint pads */
    for( MODULE* module = m_Pcb->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad; pad = pad->Next() )
        {
            wxPoint shape_pos;
            if( (pad->m_Masque_Layer & aLayerMask) == 0 )
                continue;

            shape_pos = pad->ReturnShapePos();
            pos = shape_pos;
            wxSize margin;
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

            size.x = pad->m_Size.x + ( 2 * margin.x );
            size.y = pad->m_Size.y + ( 2 * margin.y );

            /* Don't draw a null size item : */
            if( size.x <= 0 || size.y <= 0 )
                continue;

            switch( pad->m_PadShape )
            {
            case PAD_CIRCLE:
                aPlotter->flash_pad_circle( pos, size.x, aPlotMode );
                break;

            case PAD_OVAL:
                aPlotter->flash_pad_oval( pos, size, pad->m_Orient, aPlotMode );
                break;

            case PAD_TRAPEZOID:
            {
                wxSize delta = pad->m_DeltaSize;
                aPlotter->flash_pad_trapez( pos,
                                            size,
                                            delta,
                                            pad->m_Orient,
                                            aPlotMode );
            }
            break;

            case PAD_RECT:
            default:
                aPlotter->flash_pad_rect( pos, size, pad->m_Orient, aPlotMode );
                break;
            }
        }
    }

    /* Plot vias : */
    if( aPlotVia )
    {
        for( TRACK* track = m_Pcb->m_Track; track; track = track->Next() )
        {
            if( track->Type() != TYPE_VIA )
                continue;

            SEGVIA* Via = (SEGVIA*) track;

            // vias are not plotted if not on selected layer, but if layer
            // is SOLDERMASK_LAYER_BACK or SOLDERMASK_LAYER_FRONT,vias are drawn,
            // if they are on an external copper layer
            int via_mask_layer = Via->ReturnMaskLayer();
            if( via_mask_layer & LAYER_BACK )
                via_mask_layer |= SOLDERMASK_LAYER_BACK;
            if( via_mask_layer & LAYER_FRONT )
                via_mask_layer |= SOLDERMASK_LAYER_FRONT;
            if( ( via_mask_layer & aLayerMask ) == 0 )
                continue;

            int via_margin = 0;

            // If the current layer is a solder mask, use the global mask
            // clearance for vias
            if( ( aLayerMask & ( SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT ) ) )
                via_margin = GetBoard()->GetBoardDesignSettings()->m_SolderMaskMargin;
            pos    = Via->m_Start;
            size.x = size.y = Via->m_Width + 2 * via_margin;

            /* Don't draw a null size item : */
            if( size.x <= 0 )
                continue;

            aPlotter->flash_pad_circle( pos, size.x, aPlotMode );
        }
    }

    /* Plot tracks (not vias) : */
    for( TRACK* track = m_Pcb->m_Track; track; track = track->Next() )
    {
        wxPoint end;

        if( track->Type() == TYPE_VIA )
            continue;

        if( (g_TabOneLayerMask[track->GetLayer()] & aLayerMask) == 0 )
            continue;

        size.x = size.y = track->m_Width;
        pos    = track->m_Start;
        end    = track->m_End;

        aPlotter->thick_segment( pos, end, size.x, aPlotMode );
    }

    /* Plot zones (outdated, for old boards compatibility): */
    for( TRACK* track = m_Pcb->m_Zone; track; track = track->Next() )
    {
        wxPoint end;

        if( (g_TabOneLayerMask[track->GetLayer()] & aLayerMask) == 0 )
            continue;

        size.x = size.y = track->m_Width;
        pos    = track->m_Start;
        end    = track->m_End;

        aPlotter->thick_segment( pos, end, size.x, aPlotMode );
    }

    /* Plot filled ares */
    for( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->GetArea( ii );
        if( ( ( 1 << edge_zone->GetLayer() ) & aLayerMask ) == 0 )
            continue;
        PlotFilledAreas( aPlotter, edge_zone, aPlotMode );
    }
}


/** function PlotDrillMark
 * Draw a drill mark for pads and vias.
 * Must be called after all drawings, because it
 * redraw the drill mark on a pad or via, as a negative (i.e. white) shape in
 * FILLED plot mode
 * @param aPlotter = the PLOTTER
 * @param aTraceMode = the mode of plot (FILLED, SKETCH)
 * @param aSmallDrillShape = true to plot a small drill shape, false to plot
 * the actual drill shape
 */
void WinEDA_BasePcbFrame::PlotDrillMark( PLOTTER*    aPlotter,
                                         GRTraceMode aTraceMode,
                                         bool        aSmallDrillShape )
{
    wxPoint   pos;
    wxSize    diam;
    MODULE*   Module;
    D_PAD*    PtPad;
    TRACK*    pts;

    if( aTraceMode == FILLED )
    {
        aPlotter->set_color( WHITE );
    }

    for( pts = m_Pcb->m_Track; pts != NULL; pts = pts->Next() )
    {
        if( pts->Type() != TYPE_VIA )
            continue;
        pos = pts->m_Start;
        if( g_pcb_plot_options.DrillShapeOpt ==
            PCB_Plot_Options::SMALL_DRILL_SHAPE )
            diam.x = diam.y = SMALL_DRILL;
        else
            diam.x = diam.y = pts->GetDrillValue();

        aPlotter->flash_pad_circle( pos, diam.x, aTraceMode );
    }

    for( Module = m_Pcb->m_Modules;
         Module != NULL;
         Module = Module->Next() )
    {
        for( PtPad = Module->m_Pads;
             PtPad != NULL;
             PtPad = PtPad->Next() )
        {
            if( PtPad->m_Drill.x == 0 )
                continue;

            // Output hole shapes:
            pos = PtPad->m_Pos;
            if( PtPad->m_DrillShape == PAD_OVAL )
            {
                diam = PtPad->m_Drill;
                aPlotter->flash_pad_oval( pos,
                                          diam,
                                          PtPad->m_Orient,
                                          aTraceMode );
            }
            else
            {
                diam.x = aSmallDrillShape ? SMALL_DRILL : PtPad->m_Drill.x;
                aPlotter->flash_pad_circle( pos, diam.x, aTraceMode );
            }
        }
    }

    if( aTraceMode == FILLED )
    {
        aPlotter->set_color( BLACK );
    }
}

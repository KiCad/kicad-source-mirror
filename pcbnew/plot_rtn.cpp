/*********************************************************/
/* Routines de trace: fonction communes aux diff formats */
/*********************************************************/

/* Fichier PLOT_RTN.CPP*/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"


/* Fonctions locales */
static void Plot_Edges_Modules( BOARD* pcb, int format_plot, int masque_layer );
static void PlotTextModule( TEXTE_MODULE* pt_texte, int format_plot );


/**********************************************************/
void WinEDA_BasePcbFrame::Plot_Serigraphie( int format_plot,
                                            FILE* File, int masque_layer )
/***********************************************************/

/* Genere le trace des couches type serigraphie, en format HPGL ou GERBER*/
{
    wxPoint         pos, shape_pos;
    wxSize          size;
    bool            trace_val, trace_ref;
    D_PAD*          pt_pad;
    TEXTE_MODULE*   pt_texte;
    EDA_BaseStruct* PtStruct;
    wxString        msg;

    /* Trace du contour du PCB et des Elements du  type Drawings Pcb */
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            PlotDrawSegment( (DRAWSEGMENT*) PtStruct, format_plot, masque_layer );
            break;

        case TYPE_TEXTE:
            PlotTextePcb( (TEXTE_PCB*) PtStruct, format_plot, masque_layer );
            break;

        case TYPE_COTATION:
            PlotCotation( (COTATION*) PtStruct, format_plot, masque_layer );
            break;

        case TYPE_MIRE:
            PlotMirePcb( (MIREPCB*) PtStruct, format_plot, masque_layer );
            break;

        case TYPE_MARKER:
            break;

        default:
            DisplayError( this, wxT( "Plot_Serigraphie() error: unexpected Type()" ) );
            break;
        }
    }

    /* trace des contours des MODULES : */
    Plot_Edges_Modules( m_Pcb, format_plot, masque_layer );

    /* Trace des MODULES : PADS */
    if( PlotPadsOnSilkLayer || Plot_Pads_All_Layers )
    {
        for( MODULE* Module = m_Pcb->m_Modules;  Module;  Module = Module->Next() )
        {
            pt_pad = (D_PAD*) Module->m_Pads;
            for( ; pt_pad != NULL; pt_pad = pt_pad->Next() )
            {
                /* Tst si layer OK */
                if( (pt_pad->m_Masque_Layer & masque_layer) == 0 )
                {
                    if( !Plot_Pads_All_Layers )
                        continue;
                }

                shape_pos = pt_pad->ReturnShapePos();
                pos  = shape_pos;
                size = pt_pad->m_Size;

                switch( pt_pad->m_PadShape & 0x7F )
                {
                case PAD_CIRCLE:

                    switch( format_plot )
                    {
                    case PLOT_FORMAT_GERBER:
                        PlotCircle( PLOT_FORMAT_GERBER,
                                    g_PlotLine_Width, pos, size.x / 2 );
                        break;

                    case PLOT_FORMAT_HPGL:
                        trace_1_pastille_RONDE_HPGL( pos, size.x, FILAIRE );
                        break;

                    case PLOT_FORMAT_POST:
                        trace_1_pastille_RONDE_POST( pos, size.x, FILAIRE );
                        break;
                    }

                    break;

                case PAD_OVAL:

                    switch( format_plot )
                    {
                    case PLOT_FORMAT_GERBER:
                        trace_1_contour_GERBER( pos, size, wxSize( 0, 0 ),
                                                g_PlotLine_Width,
                                                pt_pad->m_Orient );
                        break;

                    case PLOT_FORMAT_HPGL:
                        trace_1_pastille_OVALE_HPGL( pos, size,
                                                     pt_pad->m_Orient, FILAIRE );
                        break;

                    case PLOT_FORMAT_POST:
                        trace_1_pastille_OVALE_POST( pos, size,
                                                     pt_pad->m_Orient, FILAIRE );
                        break;
                    }

                    break;

                case PAD_TRAPEZOID:
                {
                    wxSize delta;
                    delta = pt_pad->m_DeltaSize;

                    switch( format_plot )
                    {
                    case PLOT_FORMAT_GERBER:
                        trace_1_contour_GERBER( pos, size, delta,
                                                g_PlotLine_Width,
                                                (int) pt_pad->m_Orient );
                        break;

                    case PLOT_FORMAT_HPGL:
                        trace_1_pad_TRAPEZE_HPGL( pos, size,
                                                  delta, (int) pt_pad->m_Orient,
                                                  FILAIRE );
                        break;

                    case PLOT_FORMAT_POST:
                        trace_1_pad_TRAPEZE_POST( pos, size,
                                                  delta, (int) pt_pad->m_Orient,
                                                  FILAIRE );
                        break;
                    }

                    break;
                }

                case PAD_RECT:
                default:

                    switch( format_plot )
                    {
                    case PLOT_FORMAT_GERBER:
                        trace_1_contour_GERBER( pos, size, wxSize( 0, 0 ),
                                                g_PlotLine_Width,
                                                (int) pt_pad->m_Orient );
                        break;

                    case PLOT_FORMAT_HPGL:
                        PlotRectangularPad_HPGL( pos, size, pt_pad->m_Orient, FILAIRE );
                        break;

                    case PLOT_FORMAT_POST:
                        trace_1_pad_rectangulaire_POST( pos, size,
                                                        (int) pt_pad->m_Orient, FILAIRE );
                        break;
                    }

                    break;
                }
            }
        }
    }     /* Fin Sequence de trace des Pads */

    /* Trace Textes MODULES */
    for( MODULE* Module = m_Pcb->m_Modules;  Module;  Module = Module->Next() )
    {
        /* Analyse des autorisations de trace pour les textes VALEUR et REF */
        trace_val = Sel_Texte_Valeur;
        trace_ref = Sel_Texte_Reference; // les 2 autorisations de tracer sont donnees

        TEXTE_MODULE* text = Module->m_Reference;
        unsigned      textLayer = text->GetLayer();

        if( textLayer >= 32 )
        {
            wxString errMsg;

            errMsg.Printf(
                _( "Your BOARD has a bad layer number of %u for module\n %s's \"reference\" text." ),
                textLayer, Module->GetReference().GetData() );
            DisplayError( this, errMsg );
            goto exit;
        }

        if( ( (1 << textLayer) & masque_layer ) == 0 )
            trace_ref = FALSE;

        if( text->m_NoShow && !Sel_Texte_Invisible )
            trace_ref = FALSE;

        text = Module->m_Value;
        textLayer = text->GetLayer();

        if( textLayer > 32 )
        {
            wxString errMsg;

            errMsg.Printf(
                _( "Your BOARD has a bad layer number of %u for module\n %s's \"value\" text." ),
                textLayer, Module->GetReference().GetData() );
            DisplayError( this, errMsg );
            goto exit;
        }

        if( ( (1 << textLayer) & masque_layer ) == 0 )
            trace_val = FALSE;

        if( text->m_NoShow && !Sel_Texte_Invisible )
            trace_val = FALSE;

        /* Trace effectif des textes */
        if( trace_ref )
        {
            PlotTextModule( Module->m_Reference, format_plot );
        }

        if( trace_val )
        {
            PlotTextModule( Module->m_Value, format_plot );
        }

        pt_texte = (TEXTE_MODULE*) Module->m_Drawings.GetFirst();
        for( ; pt_texte != NULL; pt_texte = pt_texte->Next() )
        {
            if( pt_texte->Type() != TYPE_TEXTE_MODULE )
                continue;

            if( !Sel_Texte_Divers )
                continue;
            if( (pt_texte->m_NoShow) && !Sel_Texte_Invisible )
                continue;

            textLayer = pt_texte->GetLayer();
            if( textLayer >= 32 )
            {
                wxString errMsg;

                errMsg.Printf(
                    _(
                        "Your BOARD has a bad layer number of %u for module\n %s's \"module text\" text of %s." ),
                    textLayer, Module->GetReference().GetData(), pt_texte->m_Text.GetData() );
                DisplayError( this, errMsg );
                goto exit;
            }

            if( !( (1 << textLayer) & masque_layer ) )
                continue;

            PlotTextModule( pt_texte, format_plot );
        }
    }

    /* Plot filled ares */
    for( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->GetArea( ii );
        if( ( ( 1 << edge_zone->GetLayer() ) & masque_layer ) == 0 )
            continue;
        PlotFilledAreas( edge_zone, format_plot );
    }

    // Plot segments used to fill zone areas:
    for( SEGZONE* seg = m_Pcb->m_Zone; seg != NULL; seg = seg->Next() )
    {
        if( ( ( 1 << seg->GetLayer() ) & masque_layer ) == 0 )
            continue;
        switch( format_plot )
        {
        case PLOT_FORMAT_GERBER:
            SelectD_CODE_For_LineDraw( seg->m_Width );
            PlotGERBERLine( seg->m_Start, seg->m_End, seg->m_Width );
            break;

        case PLOT_FORMAT_HPGL:
            Plot_Filled_Segment_HPGL( seg->m_Start, seg->m_End, seg->m_Width, FILLED );
            break;

        case PLOT_FORMAT_POST:
            PlotFilledSegmentPS( seg->m_Start, seg->m_End, seg->m_Width );
            break;
        }
    }

exit:
    ;
}


/********************************************************************/
static void PlotTextModule( TEXTE_MODULE* pt_texte, int format_plot )
/********************************************************************/
{
    wxSize  size;
    wxPoint pos;
    int     orient, thickness;

    /* calcul des parametres du texte :*/
    size = pt_texte->m_Size;
    pos  = pt_texte->m_Pos;

    orient = pt_texte->GetDrawRotation();

    thickness = pt_texte->m_Width;
    if( g_Plot_Mode == FILAIRE )
        thickness = g_PlotLine_Width;

    if( pt_texte->m_Mirror )
        size.x = -size.x; // Text is mirrored

    switch( format_plot )
    {
    case PLOT_FORMAT_GERBER:
        SelectD_CODE_For_LineDraw( thickness );
        break;

    case PLOT_FORMAT_HPGL:
        break;

    case PLOT_FORMAT_POST:
        SetCurrentLineWidthPS( thickness );
        break;
    }

    PlotGraphicText( format_plot, pos, BLACK,
                     pt_texte->m_Text,
                     orient, size,
                     pt_texte->m_HJustify, pt_texte->m_VJustify,
                     thickness, pt_texte->m_Italic, pt_texte->m_Bold );
}


/*******************************************************************************/
void PlotCotation( COTATION* Cotation, int format_plot, int masque_layer )
/*******************************************************************************/
{
    DRAWSEGMENT* DrawTmp;

    if( (g_TabOneLayerMask[Cotation->GetLayer()] & masque_layer) == 0 )
        return;

    DrawTmp = new DRAWSEGMENT( NULL );

    DrawTmp->m_Width = Cotation->m_Width;
    DrawTmp->SetLayer( Cotation->GetLayer() );

    PlotTextePcb( Cotation->m_Text, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->Barre_ox; DrawTmp->m_Start.y = Cotation->Barre_oy;
    DrawTmp->m_End.x   = Cotation->Barre_fx; DrawTmp->m_End.y = Cotation->Barre_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->TraitG_ox; DrawTmp->m_Start.y = Cotation->TraitG_oy;
    DrawTmp->m_End.x   = Cotation->TraitG_fx; DrawTmp->m_End.y = Cotation->TraitG_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->TraitD_ox; DrawTmp->m_Start.y = Cotation->TraitD_oy;
    DrawTmp->m_End.x   = Cotation->TraitD_fx; DrawTmp->m_End.y = Cotation->TraitD_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->FlecheD1_ox; DrawTmp->m_Start.y = Cotation->FlecheD1_oy;
    DrawTmp->m_End.x   = Cotation->FlecheD1_fx; DrawTmp->m_End.y = Cotation->FlecheD1_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->FlecheD2_ox; DrawTmp->m_Start.y = Cotation->FlecheD2_oy;
    DrawTmp->m_End.x   = Cotation->FlecheD2_fx; DrawTmp->m_End.y = Cotation->FlecheD2_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->FlecheG1_ox; DrawTmp->m_Start.y = Cotation->FlecheG1_oy;
    DrawTmp->m_End.x   = Cotation->FlecheG1_fx; DrawTmp->m_End.y = Cotation->FlecheG1_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Cotation->FlecheG2_ox; DrawTmp->m_Start.y = Cotation->FlecheG2_oy;
    DrawTmp->m_End.x   = Cotation->FlecheG2_fx; DrawTmp->m_End.y = Cotation->FlecheG2_fy;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    delete DrawTmp;
}


/*****************************************************************/
void PlotMirePcb( MIREPCB* Mire, int format_plot, int masque_layer )
/*****************************************************************/
{
    DRAWSEGMENT* DrawTmp;
    int          dx1, dx2, dy1, dy2, radius;

    if( (g_TabOneLayerMask[Mire->GetLayer()] & masque_layer) == 0 )
        return;

    DrawTmp = new DRAWSEGMENT( NULL );

    DrawTmp->m_Width = Mire->m_Width;
    DrawTmp->SetLayer( Mire->GetLayer() );

    DrawTmp->m_Start.x = Mire->m_Pos.x; DrawTmp->m_Start.y = Mire->m_Pos.y;
    DrawTmp->m_End.x   = DrawTmp->m_Start.x + (Mire->m_Size / 4);
    DrawTmp->m_End.y   = DrawTmp->m_Start.y;
    DrawTmp->m_Shape   = S_CIRCLE;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Shape = S_SEGMENT;
    /* Trace des 2 traits */
    radius = Mire->m_Size / 2;
    dx1    = radius, dy1 = 0; dx2 = 0, dy2 = radius;

    if( Mire->m_Shape ) /* Forme X */
    {
        dx1 = dy1 = (radius * 7) / 5;
        dx2 = dx1;
        dy2 = -dy1;
    }

    DrawTmp->m_Start.x = Mire->m_Pos.x - dx1; DrawTmp->m_Start.y = Mire->m_Pos.y - dy1;
    DrawTmp->m_End.x   = Mire->m_Pos.x + dx1; DrawTmp->m_End.y = Mire->m_Pos.y + dy1;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    DrawTmp->m_Start.x = Mire->m_Pos.x - dx2; DrawTmp->m_Start.y = Mire->m_Pos.y - dy2;
    DrawTmp->m_End.x   = Mire->m_Pos.x + dx2; DrawTmp->m_End.y = Mire->m_Pos.y + dy2;
    PlotDrawSegment( DrawTmp, format_plot, masque_layer );

    delete DrawTmp;
}


/**********************************************************************/
void Plot_Edges_Modules( BOARD* pcb, int format_plot, int masque_layer )
/**********************************************************************/
/* Trace les contours des modules */
{
    int      nb_items;      /* Pour affichage activite: nbr modules traites */
    wxString msg;

    nb_items = 0;
    for( MODULE* module = pcb->m_Modules;  module;  module = module->Next() )
    {
        EDGE_MODULE* edge = (EDGE_MODULE*) module->m_Drawings.GetFirst();
        for( ; edge; edge = edge->Next() )
        {
            if( edge->Type() != TYPE_EDGE_MODULE )
                continue;

            if( (g_TabOneLayerMask[edge->GetLayer()] & masque_layer) == 0 )
                continue;

            Plot_1_EdgeModule( format_plot, edge );
        }

        /* Affichage du nombre de modules traites */
        nb_items++;
        msg.Printf( wxT( "%d" ), nb_items );
    }
}


/**************************************************************/
void Plot_1_EdgeModule( int format_plot, EDGE_MODULE* PtEdge )
/**************************************************************/
/* Trace les contours des modules */
{
    int     type_trace;         /* forme a tracer (segment, cercle) */
    int     thickness;          /* thickness des segments */
    int     radius;             /* radius des cercles a tracer */
    int     StAngle, EndAngle;
    wxPoint pos, end;           /* Coord des segments a tracer */

    if( PtEdge->Type() != TYPE_EDGE_MODULE )
        return;

    type_trace = PtEdge->m_Shape;
    thickness  = PtEdge->m_Width;

    if( g_Plot_Mode == FILAIRE )
    {
        thickness = g_PlotLine_Width;
        wxLogDebug( wxT( "changing edgemodule thickness to g_PlotLine_Width" ) );
    }

    pos = PtEdge->m_Start;
    end = PtEdge->m_End;

    switch( type_trace )
    {
    case S_SEGMENT:

        /* segment simple */
        switch( format_plot )
        {
        case PLOT_FORMAT_GERBER:
            SelectD_CODE_For_LineDraw( thickness );
            PlotGERBERLine( pos, end, thickness );
            break;

        case PLOT_FORMAT_HPGL:
            Plot_Filled_Segment_HPGL( pos, end, thickness, (GRFillMode) g_Plot_Mode );
            break;

        case PLOT_FORMAT_POST:
            PlotFilledSegmentPS( pos, end, thickness );
            break;
        }

        break;      /* Fin trace segment simple */

    case S_CIRCLE:
        radius = (int) hypot( (double) ( end.x - pos.x ), (double) ( end.y - pos.y ) );
        PlotCircle( format_plot, thickness, pos, radius );
        break;

    case S_ARC:
        radius   = (int) hypot( (double) ( end.x - pos.x ), (double) ( end.y - pos.y ) );
        StAngle  = ArcTangente( end.y - pos.y, end.x - pos.x );
        EndAngle = StAngle + PtEdge->m_Angle;
        if( StAngle > EndAngle )
            EXCHG( StAngle, EndAngle );
        PlotArc( format_plot, pos, StAngle, EndAngle, radius, thickness );
        break;

    case S_POLYGON:
    {
        // We must compute true coordinates from m_PolyList
        // which are relative to module position, orientation 0
        MODULE* Module = NULL;
        if( PtEdge->GetParent() && (PtEdge->GetParent()->Type() == TYPE_MODULE) )
            Module = (MODULE*) PtEdge->GetParent();

        int* ptr_base = (int*) MyMalloc( 2 * PtEdge->m_PolyPoints.size() * sizeof(int) );
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

        PlotFilledPolygon( format_plot, PtEdge->m_PolyPoints.size(), ptr_base );
        free( ptr_base );
    }
    break;
    }
}


/****************************************************************************/
void PlotTextePcb( TEXTE_PCB* pt_texte, int format_plot, int masque_layer )
/****************************************************************************/
/* Trace 1 Texte type PCB , c.a.d autre que les textes sur modules */
{
    int     orient, thickness;
    wxPoint pos;
    wxSize  size;

    if( pt_texte->m_Text.IsEmpty() )
        return;
    if( (g_TabOneLayerMask[pt_texte->GetLayer()] & masque_layer) == 0 )
        return;

    /* calcul des parametres du texte :*/
    size      = pt_texte->m_Size;
    pos       = pt_texte->m_Pos;
    orient    = pt_texte->m_Orient;
    thickness = pt_texte->m_Width;

    if( pt_texte->m_Mirror )
        size.x = -size.x;

    switch( format_plot )
    {
    case PLOT_FORMAT_GERBER:
        SelectD_CODE_For_LineDraw( thickness );
        break;

    case PLOT_FORMAT_HPGL:
        break;

    case PLOT_FORMAT_POST:
        SetCurrentLineWidthPS( thickness );
        break;
    }

    if( pt_texte->m_MultilineAllowed )
    {
        wxArrayString* list = wxStringSplit( pt_texte->m_Text, '\n' );
        wxPoint        offset;

        offset.y = pt_texte->GetInterline();

        RotatePoint( &offset, orient );
        for( unsigned i = 0; i<list->Count(); i++ )
        {
            wxString txt = list->Item( i );
            PlotGraphicText( format_plot, pos, BLACK,
                     txt,
                     orient, size,
                     pt_texte->m_HJustify, pt_texte->m_VJustify,
                     thickness, pt_texte->m_Italic, pt_texte->m_Bold );
            pos += offset;
        }

        delete (list);
    }
    
    else
        PlotGraphicText( format_plot, pos, BLACK,
                     pt_texte->m_Text,
                     orient, size,
                     pt_texte->m_HJustify, pt_texte->m_VJustify,
                     thickness, pt_texte->m_Italic, pt_texte->m_Bold );
}


/*********************************************************/
void PlotFilledAreas( ZONE_CONTAINER* aZone, int aFormat )
/*********************************************************/

/* Plot areas (given by .m_FilledPolysList member) in a zone
 */
{
    static int*     CornersBuffer     = NULL;
    static unsigned CornersBufferSize = 0;
    unsigned        imax = aZone->m_FilledPolysList.size();

    if( imax == 0 )  // Nothing to draw
        return;

    // We need a buffer to store corners coordinates:

    imax++;   // provide room to sore an extra coordinte to close the ploygon
    if( CornersBuffer == NULL )
    {
        CornersBufferSize = imax * 4;
        CornersBuffer     = (int*) MyMalloc( CornersBufferSize * sizeof(int) );
    }

    if( (imax * 4) > CornersBufferSize )
    {
        CornersBufferSize = imax * 4;
        CornersBuffer     = (int*) realloc( CornersBuffer, CornersBufferSize * sizeof(int) );
    }

    imax--;

    // Plot all filled areas
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
            if( CornersBuffer[0] != CornersBuffer[ii - 2] || CornersBuffer[1] !=
                CornersBuffer[ii - 1] )
            {
                CornersBuffer[ii++] = CornersBuffer[0];
                CornersBuffer[ii]   = CornersBuffer[1];
                corners_count++;
            }

            // Plot the current filled area outline
            if( aZone->m_FillMode == 0 )  // We are using solid polygons (if != 0: using segments in m_Zone)
                PlotFilledPolygon( aFormat, corners_count, CornersBuffer );
            if( aZone->m_ZoneMinThickness > 0 )
                PlotPolygon( aFormat, corners_count, CornersBuffer, aZone->m_ZoneMinThickness );
            corners_count = 0;
            ii = 0;
        }
    }
}


/******************************************************************************/
void PlotDrawSegment( DRAWSEGMENT* pt_segm, int Format, int masque_layer )
/******************************************************************************/

/* Trace un element du type DRAWSEGMENT draw appartenant
 *   aux couches specifiees par masque_layer
 */
{
    wxPoint start, end;
    int     thickness;
    int     radius = 0, StAngle = 0, EndAngle = 0;

    if( (g_TabOneLayerMask[pt_segm->GetLayer()] & masque_layer) == 0 )
        return;

    thickness = pt_segm->m_Width;
    if( g_Plot_Mode == FILAIRE )
        thickness = g_PlotLine_Width;

    start = pt_segm->m_Start;
    end   = pt_segm->m_End;

    if( pt_segm->m_Shape == S_CIRCLE )
    {
        radius = (int) hypot( (double) ( end.x - start.x ), (double) ( end.y - start.y ) );
    }

    if( pt_segm->m_Shape == S_ARC )
    {
        radius   = (int) hypot( (double) ( end.x - start.x ), (double) ( end.y - start.y ) );
        StAngle  = ArcTangente( end.y - start.y, end.x - start.x );
        EndAngle = StAngle + pt_segm->m_Angle;
        if( StAngle > EndAngle )
            EXCHG( StAngle, EndAngle );
    }

    switch( Format )
    {
    case PLOT_FORMAT_GERBER:
        SelectD_CODE_For_LineDraw( thickness );

        switch(pt_segm->m_Shape)
        {
            case S_CIRCLE:
                PlotCircle( PLOT_FORMAT_GERBER, thickness, start, radius );
            break;

            case S_ARC:
                PlotArc( PLOT_FORMAT_GERBER, start,
                     StAngle, EndAngle, radius, thickness );
            break;
            case S_CURVE:
                for (unsigned int i=1; i < pt_segm->m_BezierPoints.size(); i++) {
                    PlotGERBERLine( pt_segm->m_BezierPoints[i-1], pt_segm->m_BezierPoints[i], thickness);
                }
            break; 
            default:
                 PlotGERBERLine( start, end, thickness );
        }
       break;

    case PLOT_FORMAT_HPGL:

        switch(pt_segm->m_Shape)
        {
            case S_CIRCLE:
                PlotCircle( PLOT_FORMAT_HPGL, thickness, start, radius );
            break;

            case S_ARC:
                PlotArc( PLOT_FORMAT_HPGL, start, StAngle, EndAngle, radius, thickness );
            break;

            case S_CURVE:
                for (unsigned int i=1; i < pt_segm->m_BezierPoints.size(); i++) {
                    Plot_Filled_Segment_HPGL( pt_segm->m_BezierPoints[i-1], pt_segm->m_BezierPoints[i], thickness,(GRFillMode) g_Plot_Mode);
                }
            break; 
            default:
                Plot_Filled_Segment_HPGL( start, end, thickness, (GRFillMode) g_Plot_Mode );
        }
        break;
    case PLOT_FORMAT_POST:
        switch(pt_segm->m_Shape)
        {
            case S_CIRCLE:
                PlotCircle( PLOT_FORMAT_POST, thickness, start, radius );
            break;

            case S_ARC:
                PlotArc( PLOT_FORMAT_POST, start,
                     StAngle, EndAngle, radius, thickness );
            break;

            case S_CURVE:
                for (unsigned int i=1; i < pt_segm->m_BezierPoints.size(); i++) {
                    PlotFilledSegmentPS( pt_segm->m_BezierPoints[i-1], pt_segm->m_BezierPoints[i], thickness);
                }
            break;

            default: 
                PlotFilledSegmentPS( start, end, thickness );
        }
        break;
    }
}


/*****************************************************************************/
void PlotCircle( int format_plot, int thickness, wxPoint centre, int radius )
/*****************************************************************************/
/* routine de trace de 1 cercle de centre cx, cy */
{
    switch( format_plot )
    {
    case PLOT_FORMAT_GERBER:
        SelectD_CODE_For_LineDraw( thickness );
        PlotCircle_GERBER( centre, radius, thickness );
        break;

    case PLOT_FORMAT_HPGL:
        trace_1_pastille_RONDE_HPGL( centre, radius * 2, FILAIRE );
        break;

    case PLOT_FORMAT_POST:
        PlotCirclePS( centre, radius * 2, false, thickness );
        break;
    }
}


/**********************************************************************/
void PlotFilledPolygon( int format_plot, int nbpoints, int* coord )
/**********************************************************************/
/* plot a polygon */
{
    switch( format_plot )
    {
    case PLOT_FORMAT_GERBER:
        PlotFilledPolygon_GERBER( nbpoints, coord );
        break;

    case PLOT_FORMAT_HPGL:
        PlotPolyHPGL( nbpoints, coord, true );
        break;

    case PLOT_FORMAT_POST:
        PlotPolyPS( nbpoints, coord, true );
        break;
    }
}


/**********************************************************************/
void PlotPolygon( int format_plot, int nbpoints, int* coord, int width )
/**********************************************************************/

/* plot a non filled polygon
 */
{
    switch( format_plot )
    {
    case PLOT_FORMAT_GERBER:
        SelectD_CODE_For_LineDraw( width );
        PlotPolygon_GERBER( nbpoints, coord, width );
        break;

    case PLOT_FORMAT_HPGL:
    {
        // Compute pen_dim (from g_HPGL_Pen_Diam in mils) in pcb units,
        // with plot scale (if Scale is 2, pen diametre is always g_HPGL_Pen_Diam
        // so apparent pen diam is real pen diam / Scale
        int      pen_diam = wxRound( (g_HPGL_Pen_Diam * U_PCB) / Scale_X ); // Assume Scale_X # Scale_Y
        wxString msg;
        if( pen_diam >= width )
            PlotPolyHPGL( nbpoints, coord, false, width ); // PlotPolyHPGL does not handle width
        else
        {
            wxPoint start, end;
            start.x = *coord++;
            start.y = *coord++;
            for( int ii = 1; ii < nbpoints; ii++ )
            {
                end.x = *coord++;
                end.y = *coord++;
                Plot_Filled_Segment_HPGL( start, end, width, (GRFillMode) g_Plot_Mode );
                start = end;
            }
        }
    }
    break;

    case PLOT_FORMAT_POST:
        PlotPolyPS( nbpoints, coord, false, width );
        break;
    }
}


/************************************************************************/
void PlotArc( int format_plot, wxPoint centre, int start_angle, int end_angle,
              int radius, int thickness )
/************************************************************************/

/* Polt 1 arc
 *  start, end = angles in 1/10 degree for start and stop point
 */
{
    int ii;
    int ox, oy, fx, fy;
    int delta;              /* increment (en 0.1 degres) angulaire pour trace de cercles */

    if( g_Plot_Mode == FILAIRE )
        thickness = g_PlotLine_Width;

    if( IsPostScript( format_plot ) )
    {
        PlotArcPS( centre, start_angle, end_angle, radius, false, thickness );
        return;
    }

    // change due to Y axis is negative orientation on screen
    start_angle = -start_angle;
    end_angle   = -end_angle;
    EXCHG( start_angle, end_angle );

    /* Correction pour petits cercles par rapport a l'thickness du trait */
    if( radius < (thickness * 10) )
        delta = 225;    /* 16 segm pour 360 deg */
    if( radius < (thickness * 5) )
        delta = 300;    /* 12 segm pour 360 deg */

    if( start_angle > end_angle )
        end_angle += 3600;

    ox = radius;
    oy = 0;

    RotatePoint( &ox, &oy, start_angle );

    if( format_plot == PLOT_FORMAT_GERBER )
        SelectD_CODE_For_LineDraw( thickness );

    delta = 120;    /* un cercle sera trace en 3600/delta = 30 segments / cercle*/
    for( ii = start_angle + delta; ii < end_angle; ii += delta )
    {
        fx = radius;
        fy = 0;

        RotatePoint( &fx, &fy, ii );

        switch( format_plot )
        {
        case PLOT_FORMAT_GERBER:
            PlotGERBERLine( wxPoint( centre.x + ox, centre.y + oy ),
                            wxPoint( centre.x + fx, centre.y + fy ), thickness );
            break;

        case PLOT_FORMAT_HPGL:
            Plot_Filled_Segment_HPGL( wxPoint( centre.x + ox, centre.y + oy ),
                                      wxPoint( centre.x + fx, centre.y + fy ),
                                      thickness, (GRFillMode) g_Plot_Mode );
            break;

        case PLOT_FORMAT_POST:
            break;
        }

        ox = fx;
        oy = fy;
    }

    fx = radius;
    fy = 0;

    RotatePoint( &fx, &fy, end_angle );

    switch( format_plot )
    {
    case PLOT_FORMAT_GERBER:
        PlotGERBERLine( wxPoint( centre.x + ox, centre.y + oy ),
                        wxPoint( centre.x + fx, centre.y + fy ), thickness );
        break;

    case PLOT_FORMAT_HPGL:
        Plot_Filled_Segment_HPGL( wxPoint( centre.x + ox, centre.y + oy ),
                                  wxPoint( centre.x + fx, centre.y + fy ),
                                  thickness, (GRFillMode) g_Plot_Mode );
        break;

    case PLOT_FORMAT_POST:
        break;
    }
}

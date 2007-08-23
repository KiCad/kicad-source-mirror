/*************************************/
/**** Pcbnew: Routine de trace PS ****/
/*************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

#include "protos.h"

#include "wx/defs.h"

// Routines Locales
static void PrintDrillMark( BOARD* Pcb );

static Ki_PageDescr* SheetPS;

// variables locales:
const int            DRILL_MARK = 1;


/****************************************************************************/
void WinEDA_BasePcbFrame::Genere_PS( const wxString& FullFileName, int Layer )
/****************************************************************************/

/* Genere un fichier POSTSCRIPT (*.ps) de trace du circuit, couche layer
 * if layer < 0: all layers
 */
{
    int           modetrace, tracevia;
    wxSize        PcbSheetSize;
    wxSize        PaperSize;
    wxSize        BoardSize;
    wxPoint       BoardCenter;
    bool          Center = FALSE;
    Ki_PageDescr* currentsheet = m_CurrentScreen->m_CurrentSheet;
    double        scale_format; // Facteur correctif pour conversion forlat Ax->A4
    double        scale_x, scale_y;
    int           PlotMarge_in_mils = 0;

    MsgPanel->EraseMsgBox();

    dest = wxFopen( FullFileName, wxT( "wt" ) );
    if( dest == NULL )
    {
        wxString msg = _( "Unable to create file " ) + FullFileName;
        DisplayError( this, msg );
        return;
    }

    setlocale( LC_NUMERIC, "C" );

    Affiche_1_Parametre( this, 0, _( "File" ), FullFileName, CYAN );

    if( g_PlotScaleOpt != 1 )
        Center = TRUE; // Echelle != 1 donc trace centree du PCB
    modetrace    = Plot_Mode;
    scale_format = 1.0;

    // Set default line width
    if( g_PlotLine_Width < 1 )
        g_PlotLine_Width = 1;

    // calcul en unites internes des dimensions des feuilles ( connues en 1/1000 pouce )
    PcbSheetSize.x = currentsheet->m_Size.x * U_PCB;
    PcbSheetSize.y = currentsheet->m_Size.y * U_PCB;
    if( g_ForcePlotPS_On_A4 )
    {
        SheetPS      = &g_Sheet_A4;
        PaperSize.x  = g_Sheet_A4.m_Size.x * U_PCB;
        PaperSize.y  = g_Sheet_A4.m_Size.y * U_PCB;
        scale_format = (float) PaperSize.x / PcbSheetSize.x;
    }
    else
    {
        SheetPS   = currentsheet;
        PaperSize = PcbSheetSize;
    }

    // calcul de l'offset de trace:
    // calcul du cadrage horizontal du mode paysage ( val algebr. plus grande = decalage a gauche )
    g_PlotOffset.x = PlotMarge_in_mils * U_PCB;

    // cadrage vertical du mode paysage ( val algebr. plus grande = decalage vers le haut )
    g_PlotOffset.y = PaperSize.y - PlotMarge_in_mils * U_PCB;

    int BBox[4];
    BBox[0] = BBox[1] = PlotMarge_in_mils;
    BBox[2] = SheetPS->m_Size.x - PlotMarge_in_mils;
    BBox[3] = SheetPS->m_Size.y - PlotMarge_in_mils;
    scale_x = scale_y = 1.0;
    InitPlotParametresPS( g_PlotOffset, SheetPS, 1.0 / m_InternalUnits, 1.0 / m_InternalUnits );
    SetDefaultLineWidthPS( g_PlotLine_Width );
    PrintHeaderPS( dest, wxT( "PCBNEW-PS" ), FullFileName, 1, BBox, wxLANDSCAPE );

    if( Plot_Sheet_Ref )
    {
        int tmp = g_PlotOrient;
        g_PlotOrient = 0;
        SetPlotScale( 1.0, 1.0 );
        PlotWorkSheet( PLOT_FORMAT_POST, m_CurrentScreen );
        g_PlotOrient = tmp;
    }

    // calcul des dimensions et centre du PCB
    m_Pcb->ComputeBoundaryBox();
    BoardSize   = m_Pcb->m_BoundaryBox.GetSize();
    BoardCenter = m_Pcb->m_BoundaryBox.Centre();

    scale_x = Scale_X;
    scale_y = Scale_Y;
    if( g_PlotScaleOpt == 0 )       // Optimum scale
    {
        float Xscale, Yscale;
        int   noprint_size = 2 * PlotMarge_in_mils * U_PCB;
        if( Plot_Sheet_Ref )
            noprint_size += 500 * U_PCB;
        Xscale  = (float) ( PaperSize.x - noprint_size ) / BoardSize.x;
        Yscale  = (float) ( PaperSize.y - noprint_size ) / BoardSize.y;
        scale_x = scale_y = MIN( Xscale, Yscale );
    }

    BoardCenter.x = (int) (BoardCenter.x * scale_x);
    BoardCenter.y = (int) (BoardCenter.y * scale_y);

    // Calcul du cadrage (echelle != 1 donc recadrage du trace)
    if( Center )
    {
        g_PlotOffset.x -= PaperSize.x / 2 - BoardCenter.x + PlotMarge_in_mils * U_PCB;
        g_PlotOffset.y  = PaperSize.y / 2 + BoardCenter.y;  // cadrage horizontal du mode paysage
    }

    if( g_PlotOrient == PLOT_MIROIR )
    {
        if( Center )
            g_PlotOffset.y = -PaperSize.y / 2 + BoardCenter.y;
        else
            g_PlotOffset.y = -PaperSize.y + m_Pcb->m_BoundaryBox.GetBottom()
                             + m_Pcb->m_BoundaryBox.GetY() + PlotMarge_in_mils * U_PCB;
    }

    InitPlotParametresPS( g_PlotOffset, SheetPS, scale_x, scale_y, g_PlotOrient );

    // If plot a negative board:
    // Draw a black rectangle (background for plot board in white)
    // and switch the current color to WHITE
    if( g_Plot_PS_Negative )
    {
        int Rectangle[10];  // Put here the board corners
        int margin = 500;   // Add a 0.1 inch margin around the board
        Rectangle[0] = m_Pcb->m_BoundaryBox.GetX() - margin;
        Rectangle[1] = m_Pcb->m_BoundaryBox.GetY() - margin;
        Rectangle[2] = m_Pcb->m_BoundaryBox.GetRight() + margin;
        Rectangle[3] = Rectangle[1];
        Rectangle[4] = Rectangle[2];
        Rectangle[5] = m_Pcb->m_BoundaryBox.GetBottom() + margin;
        Rectangle[6] = Rectangle[0];
        Rectangle[7] = Rectangle[5];
        Rectangle[8] = Rectangle[0];
        Rectangle[9] = Rectangle[1];
        SetColorMapPS( BLACK );
        PlotPolyPS( 5, Rectangle, TRUE );
        SetColorMapPS( WHITE );
    }

    switch( Layer )
    {
    case - 1:
        Plot_Layer_PS( dest, g_TabOneLayerMask[Layer], 0, 1, modetrace );
        break;

    case CUIVRE_N:
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
    case CMP_N:
        Plot_Layer_PS( dest, g_TabOneLayerMask[Layer], 0, 1, modetrace );
        break;

    case SILKSCREEN_N_CU:
    case SILKSCREEN_N_CMP:
        Plot_Serigraphie( PLOT_FORMAT_POST, dest, g_TabOneLayerMask[Layer] );
        break;

    case SOLDERMASK_N_CU:
    case SOLDERMASK_N_CMP:  // Trace du vernis epargne
        if( g_DrawViaOnMaskLayer )
            tracevia = 1;
        else
            tracevia = 0;
        Plot_Layer_PS( dest, g_TabOneLayerMask[Layer], g_DesignSettings.m_MaskMargin,
                       tracevia, modetrace );
        break;

    case SOLDERPASTE_N_CU:
    case SOLDERPASTE_N_CMP:  // Trace du masque de pate de soudure
        Plot_Layer_PS( dest, g_TabOneLayerMask[Layer], 0, 0, modetrace );
        break;

    default:
        Plot_Serigraphie( PLOT_FORMAT_POST, dest, g_TabOneLayerMask[Layer] );
        break;
    }

    // fin
    CloseFilePS( dest );
    setlocale( LC_NUMERIC, "" );
}


/********************************************************************/
void WinEDA_BasePcbFrame::Plot_Layer_PS( FILE* File, int masque_layer,
                                         int garde, int tracevia, int modetrace )
/********************************************************************/

/* Trace en format POSTSCRIPT d'une couche cuivre ou masque
 */
{
    wxPoint     pos, end;
    wxSize      size;
    MODULE*     Module;
    D_PAD*      PtPad;
    TRACK*      pts;
    BOARD_ITEM* PtStruct;
    wxString    msg;

    masque_layer |= EDGE_LAYER; // Les elements de la couche EDGE sont tj traces

    // trace des elements type Drawings Pcb :
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->m_StructType )
        {
        case TYPEDRAWSEGMENT:
            PlotDrawSegment( (DRAWSEGMENT*) PtStruct, PLOT_FORMAT_POST,
                            masque_layer );
            break;

        case TYPETEXTE:
            PlotTextePcb( (TEXTE_PCB*) PtStruct, PLOT_FORMAT_POST,
                         masque_layer );
            break;

        case TYPECOTATION:
            PlotCotation( (COTATION*) PtStruct, PLOT_FORMAT_POST,
                         masque_layer );
            break;

        case TYPEMIRE:
            PlotMirePcb( (MIREPCB*) PtStruct, PLOT_FORMAT_POST,
                        masque_layer );
            break;

        case TYPEMARQUEUR:
            break;

        default:
            DisplayError( this,
                         wxT( "WinEDA_BasePcbFrame::Plot_Layer_PS() : Unexpected Draw Type" ) );
            break;
        }
    }

    // Trace des Elements des modules autres que pads
    nb_items = 0;
    Affiche_1_Parametre( this, 48, wxT( "DrawMod" ), wxEmptyString, GREEN );
    Module = m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        PtStruct = Module->m_Drawings;
        for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
        {
            switch( PtStruct->m_StructType )
            {
            case TYPEEDGEMODULE:
                if( masque_layer & g_TabOneLayerMask[ PtStruct->GetLayer() ] )
                    Plot_1_EdgeModule( PLOT_FORMAT_POST, (EDGE_MODULE*) PtStruct );
                break;

            default:
                break;
            }
        }
    }

    // Trace des Elements des modules : Pastilles
    nb_items = 0;
    Affiche_1_Parametre( this, 48, wxT( "Pads   " ), wxEmptyString, GREEN );
    Module = m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        PtPad = (D_PAD*) Module->m_Pads;
        for( ; PtPad != NULL; PtPad = (D_PAD*) PtPad->Pnext )
        {
            if( (PtPad->m_Masque_Layer & masque_layer) == 0 )
                continue;
            wxPoint shape_pos = PtPad->ReturnShapePos();
            pos = shape_pos;

            size.x = PtPad->m_Size.x + garde * 2;
            size.y = PtPad->m_Size.y + garde * 2;

            nb_items++;

            switch( PtPad->m_PadShape )
            {
            case CIRCLE:
                trace_1_pastille_RONDE_POST( pos, size.x, modetrace );
                break;

            case OVALE:
                trace_1_pastille_OVALE_POST( pos, size, PtPad->m_Orient, modetrace );
                break;

            case TRAPEZE:
            {
                wxSize delta;
                delta = PtPad->m_DeltaSize;
                trace_1_pad_TRAPEZE_POST( pos, size, delta,
                                          PtPad->m_Orient, modetrace );
                break;
            }

            case RECT:
            default:
                trace_1_pad_rectangulaire_POST( pos, size, PtPad->m_Orient, modetrace );
                break;
            }

            msg.Printf( wxT( "%d" ), nb_items );
            Affiche_1_Parametre( this, 48, wxT( "Pads" ), msg, GREEN );
        }
    }

    // trace des VIAS :
    if( tracevia )
    {
        nb_items = 0;
        Affiche_1_Parametre( this, 56, _( "Vias" ), wxEmptyString, RED );
        for( pts = m_Pcb->m_Track; pts != NULL; pts = pts->Next() )
        {
            if( pts->m_StructType != TYPEVIA )
                continue;
            SEGVIA* Via = (SEGVIA*) pts;

            // vias not plotted if not on selected layer, but if layer
            // == SOLDERMASK_LAYER_CU or SOLDERMASK_LAYER_CMP, vias are drawn,
            // if they are on a external copper layer
            int via_mask_layer = Via->ReturnMaskLayer();
            if( via_mask_layer & CUIVRE_LAYER )
                via_mask_layer |= SOLDERMASK_LAYER_CU;
            if( via_mask_layer & CMP_LAYER )
                via_mask_layer |= SOLDERMASK_LAYER_CMP;
            if( (via_mask_layer & masque_layer) == 0 )
                continue;

            pos    = Via->m_Start;
            size.x = size.y = Via->m_Width + garde * 2;
            trace_1_pastille_RONDE_POST( pos, size.x, modetrace );
            nb_items++;
            msg.Printf( wxT( "%d" ), nb_items );
            Affiche_1_Parametre( this, 56, wxEmptyString, msg, RED );
        }
    }

    // trace des pistes et zones:
    nb_items = 0;
    Affiche_1_Parametre( this, 64, _( "Tracks" ), wxEmptyString, YELLOW );

    for( pts = m_Pcb->m_Track; pts != NULL; pts = (TRACK*) pts->Pnext )
    {
        if( pts->m_StructType == TYPEVIA )
            continue;

        if( (g_TabOneLayerMask[pts->GetLayer()] & masque_layer) == 0 )
            continue;
        size.x = size.y = pts->m_Width;
        pos    = pts->m_Start;
        end    = pts->m_End;

        PlotFilledSegmentPS( pos, end, size.x );

        nb_items++;
        msg.Printf( wxT( "%d" ), nb_items );
        Affiche_1_Parametre( this, 64, wxEmptyString, msg, YELLOW );
    }

    nb_items = 0;
    Affiche_1_Parametre( this, 64, wxT( "Zones  " ), wxEmptyString, YELLOW );

    for( pts = m_Pcb->m_Zone; pts != NULL; pts = (TRACK*) pts->Pnext )
    {
        if( (g_TabOneLayerMask[pts->GetLayer()] & masque_layer) == 0 )
            continue;
        size.x = size.y = pts->m_Width;
        pos    = pts->m_Start;
        end    = pts->m_End;
        PlotFilledSegmentPS( pos, end, size.x );
        nb_items++;
        msg.Printf( wxT( "%d" ), nb_items );
        Affiche_1_Parametre( this, 64, wxEmptyString, msg, YELLOW );
    }

    // Trace des trous de percage
    if( modetrace == FILLED )
        PrintDrillMark( m_Pcb );
}


/*************************************/
static void PrintDrillMark( BOARD* Pcb )
/*************************************/

/* Draw a drill mark for pads and vias.
 * Must be called after all drawings, because it
 * redraw the drill mark on a pad or via
 */
{
    const int SMALL_DRILL = 150;
    wxPoint   pos;
    wxSize    diam;
    MODULE*   Module;
    D_PAD*    PtPad;
    TRACK*    pts;

    if( g_DrillShapeOpt == 0 )
        return;

    if( g_Plot_PS_Negative )
        fprintf( dest, " 0 setgray\n" );
    else
        fprintf( dest, " 1 setgray\n" );

    diam.x = diam.y = (g_DrillShapeOpt == DRILL_MARK) ? SMALL_DRILL :
                      g_DesignSettings.m_ViaDrill;

    for( pts = Pcb->m_Track; pts != NULL; pts = (TRACK*) pts->Pnext )
    {
        if( pts->m_StructType != TYPEVIA )
            continue;
        pos = pts->m_Start;
        if( g_DrillShapeOpt == DRILL_MARK )
            diam.x = diam.y = SMALL_DRILL;
        else
        {
            if( pts->m_Drill < 0 )
                diam.x = diam.y = g_DesignSettings.m_ViaDrill;
            else
                diam.x = diam.y = pts->m_Drill;
        }
        trace_1_pastille_RONDE_POST( pos, diam.x, FILLED );
    }

    Module = Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        PtPad = (D_PAD*) Module->m_Pads;
        for( ; PtPad != NULL; PtPad = (D_PAD*) PtPad->Pnext )
        {
            if( PtPad->m_Drill.x == 0 )
                continue;

            // calcul de la position des trous, selon echelle
            pos    = PtPad->m_Pos;
            diam.x = diam.y = (g_DrillShapeOpt == DRILL_MARK) ? SMALL_DRILL :
                              PtPad->m_Drill.x;
            trace_1_pastille_RONDE_POST( pos, diam.x, FILLED );
        }
    }

    fprintf( dest, " 0 setgray\n" );
}


/***********************************************************************************/
void trace_1_pastille_OVALE_POST( wxPoint pos, wxSize size, int orient, int modetrace )
/************************************************************************************/

/* Trace 1 pastille OVALE en position pos_X,Y:
 * dimensions dx,dy,
 * orientation orient
 * La forme est tracee comme un segment
 */
{
    int x0, y0, x1, y1, delta;
    int w, rayon;

    // la pastille est ramenee a une pastille ovale avec dy > dx
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y );
        orient += 900;
        if( orient >= 3600 )
            orient -= 3600;
    }

    delta = size.y - size.x;
    x0    = 0;
    y0    = -delta / 2;
    x1    = 0;
    y1    = delta / 2;
    RotatePoint( &x0, &y0, orient );
    RotatePoint( &x1, &y1, orient );

    if( modetrace == FILLED )
    {
        PlotFilledSegmentPS( wxPoint( pos.x + x0, pos.y + y0 ),
                             wxPoint( pos.x + x1, pos.y + y1 ), size.x );
    }
    else
    {
        w     = g_PlotLine_Width;
        rayon = (size.x - w) / 2;
        if( rayon < 1 )
            rayon = 1;
        if( rayon < w )
            w = rayon;
        PlotArcPS( wxPoint( pos.x + x1, pos.y + y1 ), -orient, -orient + 1800, rayon, w );
        PlotArcPS( wxPoint( pos.x + x0, pos.y + y0 ), -orient + 1800, -orient, rayon, w );

        x0 = -rayon;
        y0 = -delta / 2;
        x1 = -rayon;
        y1 = delta / 2;
        RotatePoint( &x0, &y0, orient );
        RotatePoint( &x1, &y1, orient );
        PlotFilledSegmentPS( wxPoint( pos.x + x0, pos.y + y0 ),
                             wxPoint( pos.x + x1, pos.y + y1 ), w );

        x0 = rayon;
        y0 = -delta / 2;
        x1 = rayon;
        y1 = delta / 2;
        RotatePoint( &x0, &y0, orient );
        RotatePoint( &x1, &y1, orient );
        PlotFilledSegmentPS( wxPoint( pos.x + x0, pos.y + y0 ),
                             wxPoint( pos.x + x1, pos.y + y1 ), w );
    }
}


/*******************************************************************************/
void trace_1_pastille_RONDE_POST( wxPoint centre, int diametre, int modetrace )
/*******************************************************************************/

/* Trace 1 pastille RONDE (via,pad rond) en position pos_X,Y
 */
{
    int rayon, w;

    wxSize diam( diametre, diametre );

    UserToDeviceCoordinate( centre );
    UserToDeviceSize( diam );

    if( modetrace == FILLED )
    {
        fprintf( dest, "%d setlinewidth\n", g_PlotLine_Width );
        rayon = diam.x / 2;
        if( rayon < 1 )
            rayon = 1;
        fprintf( dest, "newpath %d %d %d 0 360 arc fill stroke\n",
                 centre.x, centre.y, rayon );
    }
    else
    {
        w     = g_PlotLine_Width;
        rayon = (diam.x - w) / 2;
        if( rayon < 1 )
            rayon = 1;
        if( rayon < w )
            w = rayon;
        fprintf( dest, "%d setlinewidth\n", w );
        fprintf( dest, "newpath %d %d %d 0 360 arc stroke\n",
                 centre.x, centre.y, rayon );
    }
}


/**************************************************************************/
void trace_1_pad_rectangulaire_POST( wxPoint centre,
                                     wxSize size, int orient, int modetrace )
/**************************************************************************/

/*
 * Trace 1 pad rectangulaire d'orientation quelconque
 * donne par son centre, ses dimensions,
 * et son orientation orient
 */
{
    int x0, y0, x1, y1, w;

    if( modetrace == FILLED )
    {
        x0 = centre.x - size.x / 2;
        x1 = centre.x + size.x / 2;
        y0 = y1 = centre.y;
        w  = size.y;

        RotatePoint( &x0, &y0, centre.x, centre.y, orient );
        RotatePoint( &x1, &y1, centre.x, centre.y, orient );

        fprintf( dest, "0 setlinewidth 0 setlinecap 0 setlinejoin\n" );
        ForcePenReinit();   // Force init line width for PlotFilledSegmentPS
        PlotFilledSegmentPS( wxPoint( x0, y0 ), wxPoint( x1, y1 ), w );
        ForcePenReinit();
        SetCurrentLineWidthPS( 0 );   // Force init line width to default
        fprintf( dest, "1 setlinecap 1 setlinejoin\n" );
    }
    else
    {
        w       = g_PlotLine_Width;
        size.x -= w;
        if( size.x < 1 )
            size.x = 1;
        size.y -= w;
        if( size.y < 1 )
            size.y = 1;
        trace_1_contour_POST( centre, size, wxSize( 0, 0 ), w, orient );
    }
}


/**************************************************************/
void trace_1_contour_POST( wxPoint centre, wxSize size, wxSize delta,
                           int dim_trait, int orient )
/**************************************************************/

/*
 * Trace 1 contour rectangulaire ou trapezoidal d'orientation quelconque
 * donne par son centre centre,
 * ses dimensions size,
 * ses variations delta
 * epaisseur de trait dim_trait
 * et son orientation orient (en 0.1 degres)
 */
{
    int     ii;
    int     dx, dy, lpen;
    int     ddx, ddy;
    wxPoint coord[4];

    lpen = dim_trait;

    dx  = size.x / 2;
    dy  = size.y / 2;
    ddx = delta.x >> 1;
    ddy = delta.y >> 1; // demi dim  dx et dy

    coord[0].x = centre.x - dx - ddy;
    coord[0].y = centre.y + dy + ddx;

    coord[1].x = centre.x - dx + ddy;
    coord[1].y = centre.y - dy - ddx;

    coord[2].x = centre.x + dx - ddy;
    coord[2].y = centre.y - dy + ddx;

    coord[3].x = centre.x + dx + ddy;
    coord[3].y = centre.y + dy - ddx;

    for( ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &coord[ii], centre, orient );
    }

    PlotFilledSegmentPS( coord[0], coord[1], lpen );
    PlotFilledSegmentPS( coord[1], coord[2], lpen );
    PlotFilledSegmentPS( coord[2], coord[3], lpen );
    PlotFilledSegmentPS( coord[3], coord[0], lpen );
}


/*******************************************************************/
void trace_1_pad_TRAPEZE_POST( wxPoint centre, wxSize size, wxSize delta,
                               int orient, int modetrace )
/*******************************************************************/

/*
 * Trace 1 pad trapezoidal donne par :
 * son centre centre
 * ses dimensions size
 * les variations delta ( 1 des deux au moins doit etre nulle)
 * son orientation orient en 0.1 degres
 * le mode de trace (FILLED, SKETCH, FILAIRE)
 *
 * Le trace n'est fait que pour un trapeze, c.a.d que deltaX ou deltaY
 * = 0.
 *
 * les notation des sommets sont ( vis a vis de la table tracante )
 *
 * "       0 ------------- 3   "
 * "        .             .    "
 * "         .     O     .     "
 * "          .         .      "
 * "           1 ---- 2        "
 *
 *
 * exemple de Disposition pour deltaY > 0, deltaX = 0
 * "           1 ---- 2        "
 * "          .         .      "
 * "         .     O     .     "
 * "        .             .    "
 * "       0 ------------- 3   "
 *
 *
 * exemple de Disposition pour deltaY = 0, deltaX > 0
 * "       0                  "
 * "       . .                "
 * "       .     .            "
 * "       .           3      "
 * "       .           .      "
 * "       .     O     .      "
 * "       .           .      "
 * "       .           2      "
 * "       .     .            "
 * "       . .                "
 * "       1                  "
 */
{
    int     ii;
    int     dx, dy;
    wxPoint polygone[4];    // coord des sommets / centre du pad
    int     ddx, ddy;
    int     l_pen;          // diam spot (plume)

    l_pen = 1;
    if( modetrace == FILAIRE || Plot_Mode == FILAIRE )
    {
        wxSize lsize( g_PlotLine_Width, g_PlotLine_Width );

        UserToDeviceSize( lsize );
        l_pen = lsize.x;
    }

    dx  = size.x / 2;
    dy  = size.y / 2;
    ddx = delta.x / 2;
    ddy = delta.y / 2;

    polygone[0].x = -dx - ddy;
    polygone[0].y = +dy + ddx;
    polygone[1].x = -dx + ddy;
    polygone[1].y = -dy - ddx;
    polygone[2].x = +dx - ddy;
    polygone[2].y = -dy + ddx;
    polygone[3].x = +dx + ddy;
    polygone[3].y = +dy - ddx;

    for( ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &polygone[ii].x, &polygone[ii].y, orient );
        polygone[ii].x += centre.x;
        polygone[ii].y += centre.y;
    }

    SetCurrentLineWidthPS( l_pen );

    UserToDeviceCoordinate( polygone[0] );
    fprintf( dest, "newpath %d %d moveto\n", polygone[0].x, polygone[0].y );

    for( ii = 1; ii < 4; ii++ )
    {
        UserToDeviceCoordinate( polygone[ii] );
        fprintf( dest, "%d %d lineto\n", polygone[ii].x, polygone[ii].y );
    }

    fprintf( dest, "%d %d lineto ", polygone[0].x, polygone[0].y );

    if( modetrace == FILLED )
        fprintf( dest, "fill " );
    fprintf( dest, "stroke\n" );
}

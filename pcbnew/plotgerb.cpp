/*********************************************************/
/****Function to plot a board in GERBER RS274X format ****/
/*********************************************************/

/* Creates the output files, one per board layer:
 * filenames are like xxxc.PHO and use the RS274X format
 * Units = inches
 * format 3.4, Leading zero omitted, Abs format
 * format 3.4 uses the native pcbnew units (1/10000 inch).
 */

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "confirm.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "trigo.h"

/* Class to handle a D_CODE when plotting a board : */
#define FIRST_DCODE_VALUE 10    // D_CODE < 10 is a command, D_CODE >= 10 is a tool

class D_CODE
{
public:
    D_CODE* m_Pnext, * m_Pback;     /* for  a linked list */
    wxSize  m_Size;                 /* horiz and Vert size*/
    int     m_Type;                 /* Type ( Line, rect , circulaire , ovale .. ); -1 = not used (free) descr */
    int     m_NumDcode;             /* code number ( >= 10 ); 0 = not in use */

    D_CODE()
    {
        m_Pnext    = m_Pback = NULL;
        m_Type     = -1;
        m_NumDcode = 0;
    }
};


/* Variables locales : */
static int     s_Last_D_code;
static float   Gerb_scale_plot;     // Coeff de conversion d'unites des traces
static D_CODE* s_DCodeList;         // Pointeur sur la zone de stockage des D_CODES
wxString       GerberFullFileName;
static double  scale_x, scale_y;    // echelles de convertion en X et Y (compte tenu
                                    // des unites relatives du PCB et des traceurs
static bool    ShowDcodeError = TRUE;
static void    CloseFileGERBER( void );
static int     Gen_D_CODE_File( FILE* penfile );

/* Routines Locales */

static void    Init_ApertureList();
static void    CloseFileGERBER();
static void    Plot_1_CIRCLE_pad_GERBER( wxPoint pos, int diametre );
static void    trace_1_pastille_OVALE_GERBER( wxPoint pos, wxSize size, int orient );
static void    PlotRectangularPad_GERBER( wxPoint pos, wxSize size, int orient );

static D_CODE* get_D_code( int dx, int dy, int type, int drill );
static void    trace_1_pad_TRAPEZE_GERBER( wxPoint pos, wxSize size, wxSize delta,
                                           int orient, int modetrace );


/********************************************************************************/
void WinEDA_BasePcbFrame::Genere_GERBER( const wxString& FullFileName, int Layer,
                                         bool PlotOriginIsAuxAxis )
/********************************************************************************/

/* Creates the output files, one per board layer:
 * filenames are like xxxc.PHO and use the RS274X format
 * Units = inches
 * format 3.4, Leading zero omitted, Abs format
 * format 3.4 uses the native pcbnew units (1/10000 inch).
 */
{
    int tracevia = 1;

    EraseMsgBox();
    GerberFullFileName = FullFileName;

    g_PlotOrient = 0;
    if( Plot_Set_MIROIR )
        g_PlotOrient |= PLOT_MIROIR;

    /* Calculate scaling from pcbnew units (in 0.1 mil or 0.0001 inch) to gerber units */
    Gerb_scale_plot = 1.0;  /* for format 3.4 (4 digits for decimal format means 0.1 mil per gerber unit */
    scale_x = Scale_X * Gerb_scale_plot;
    scale_y = Scale_Y * Gerb_scale_plot;
    g_PlotOffset.x = 0;
    g_PlotOffset.y = 0;
    if( PlotOriginIsAuxAxis )
        g_PlotOffset = m_Auxiliary_Axis_Position;

    g_Plot_PlotOutputFile = wxFopen( FullFileName, wxT( "wt" ) );
    if( g_Plot_PlotOutputFile == NULL )
    {
        wxString msg = _( "unable to create file " ) + FullFileName;
        DisplayError( this, msg );
        return;
    }

    SetLocaleTo_C_standard();

    InitPlotParametresGERBER( g_PlotOffset, scale_x, scale_y );

    /*	Clear the memory used for handle the D_CODE (aperture) list	 */
    Init_ApertureList();

    Affiche_1_Parametre( this, 0, _( "File" ), FullFileName, CYAN );

    s_Last_D_code = 0;

    Write_Header_GERBER( g_Main_Title, g_Plot_PlotOutputFile );

    int layer_mask = g_TabOneLayerMask[Layer];

    // Specify that the contents of the "Edges Pcb" layer are also to be
    // plotted, unless the option of excluding that layer has been selected.
    if( !g_Exclude_Edges_Pcb )
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
        Plot_Layer_GERBER( g_Plot_PlotOutputFile, layer_mask, 0, 1 );
        break;

    case SOLDERMASK_N_CU:
    case SOLDERMASK_N_CMP:  /* Trace du vernis epargne */
        if( g_DrawViaOnMaskLayer )
            tracevia = 1;
        else
            tracevia = 0;
        Plot_Layer_GERBER( g_Plot_PlotOutputFile, layer_mask, g_DesignSettings.m_MaskMargin, tracevia );
        break;

    case SOLDERPASTE_N_CU:
    case SOLDERPASTE_N_CMP:  /* Trace du masque de pate de soudure */
        Plot_Layer_GERBER( g_Plot_PlotOutputFile, layer_mask, 0, 0 );
        break;

    default:
        Plot_Serigraphie( PLOT_FORMAT_GERBER, g_Plot_PlotOutputFile, layer_mask );
        break;
    }

    CloseFileGERBER();
    SetLocaleTo_Default();
}


/***********************************************************************/
void WinEDA_BasePcbFrame::Plot_Layer_GERBER( FILE* File, int masque_layer,
                                             int garde, int tracevia )
/***********************************************************************/

/* Creates one GERBER file for a copper layer or a technical layer
 * the silkscreen layers are plotted by Plot_Serigraphie() because they have special features
 */
{
    wxPoint         pos;
    wxSize          size;
    MODULE*         Module;
    D_PAD*          PtPad;
    TRACK*          track;
    EDA_BaseStruct* PtStruct;
    wxString        msg;

    /* Draw items type Drawings Pcb matching with masque_layer: */
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            PlotDrawSegment( (DRAWSEGMENT*) PtStruct, PLOT_FORMAT_GERBER,
                masque_layer );
            break;

        case TYPE_TEXTE:
            PlotTextePcb( (TEXTE_PCB*) PtStruct, PLOT_FORMAT_GERBER,
                masque_layer );
            break;

        case TYPE_COTATION:
            PlotCotation( (COTATION*) PtStruct, PLOT_FORMAT_GERBER,
                masque_layer );
            break;

        case TYPE_MIRE:
            PlotMirePcb( (MIREPCB*) PtStruct, PLOT_FORMAT_GERBER,
                masque_layer );
            break;

        case TYPE_MARKER:
            break;

        default:
            DisplayError( this, wxT( "Type Draw non gere" ) );
            break;
        }
    }

    /* Draw footprints shapes without pads (pads will plotted later) */
    Module = m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
    {
        PtStruct = Module->m_Drawings;
        for( ; PtStruct != NULL; PtStruct = PtStruct->Next() )
        {
            switch( PtStruct->Type() )
            {
            case TYPE_EDGE_MODULE:
                if( masque_layer & g_TabOneLayerMask[( (EDGE_MODULE*) PtStruct )->GetLayer()] )
                    Plot_1_EdgeModule( PLOT_FORMAT_GERBER, (EDGE_MODULE*) PtStruct );
                break;

            default:
                break;
            }
        }
    }

    /* Plot footprint pads */
    Module = m_Pcb->m_Modules;
    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
    {
        PtPad = (D_PAD*) Module->m_Pads;
        for( ; PtPad != NULL; PtPad = (D_PAD*) PtPad->Next() )
        {
            wxPoint shape_pos;
            if( (PtPad->m_Masque_Layer & masque_layer) == 0 )
                continue;
            shape_pos = PtPad->ReturnShapePos();
            pos = shape_pos;

            size.x = PtPad->m_Size.x + 2 * garde;
            size.y = PtPad->m_Size.y + 2 * garde;

            /* Don't draw a null size item : */
            if( size.x <= 0 || size.y <= 0 )
                continue;

            switch( PtPad->m_PadShape )
            {
            case PAD_CIRCLE:
                Plot_1_CIRCLE_pad_GERBER( pos, size.x );
                break;

            case PAD_OVAL:

                // Check whether the pad really has a circular shape instead
                if( size.x == size.y )
                    Plot_1_CIRCLE_pad_GERBER( pos, size.x );
                else
                    trace_1_pastille_OVALE_GERBER( pos, size, PtPad->m_Orient );
                break;

            case PAD_TRAPEZOID:
            {
                wxSize delta = PtPad->m_DeltaSize;
                trace_1_pad_TRAPEZE_GERBER( pos, size,
                    delta, PtPad->m_Orient, FILLED );
            }
            break;

            case PAD_RECT:
            default:
                PlotRectangularPad_GERBER( pos, size, PtPad->m_Orient );
                break;
            }
        }
    }

    /* Plot vias : */
    if( tracevia )
    {
        for( track = m_Pcb->m_Track; track != NULL; track = (TRACK*) track->Next() )
        {
            if( track->Type() != TYPE_VIA )
                continue;
            SEGVIA* Via = (SEGVIA*) track;

            // vias not plotted if not on selected layer, but if layer
            // == SOLDERMASK_LAYER_CU or SOLDERMASK_LAYER_CMP, vias are drawn,
            // if they are on a external copper layer
            int via_mask_layer = Via->ReturnMaskLayer();
            if( via_mask_layer & CUIVRE_LAYER )
                via_mask_layer |= SOLDERMASK_LAYER_CU;
            if( via_mask_layer & CMP_LAYER )
                via_mask_layer |= SOLDERMASK_LAYER_CMP;
            if( ( via_mask_layer & masque_layer) == 0 )
                continue;

            pos    = Via->m_Start;
            size.x = size.y = Via->m_Width + 2 * garde;
            /* Don't draw a null size item : */
            if( size.x <= 0 )
                continue;
            Plot_1_CIRCLE_pad_GERBER( pos, size.x );
        }
    }

    /* Plot tracks (not vias) : */
    for( track = m_Pcb->m_Track; track != NULL; track = (TRACK*) track->Next() )
    {
        wxPoint end;

        if( track->Type() == TYPE_VIA )
            continue;
        if( (g_TabOneLayerMask[track->GetLayer()] & masque_layer) == 0 )
            continue;

        size.x = size.y = track->m_Width;
        pos    = track->m_Start;
        end    = track->m_End;

        SelectD_CODE_For_LineDraw( size.x );
        PlotGERBERLine( pos, end, size.x );
    }

    /* Plot zones: */
    for( track = m_Pcb->m_Zone; track != NULL; track = (TRACK*) track->Next() )
    {
        wxPoint end;

        if( (g_TabOneLayerMask[track->GetLayer()] & masque_layer) == 0 )
            continue;

        size.x = size.y = track->m_Width;
        pos    = track->m_Start;
        end    = track->m_End;

        SelectD_CODE_For_LineDraw( size.x );
        PlotGERBERLine( pos, end, size.x );
    }

    /* Plot filled ares */
    for( int ii = 0; ii < m_Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = m_Pcb->GetArea( ii );
        if( ( ( 1 << edge_zone->GetLayer() ) & masque_layer ) == 0 )
            continue;
        PlotFilledAreas( edge_zone, PLOT_FORMAT_GERBER );
    }
}


/**********************************************************************/
void trace_1_pastille_OVALE_GERBER( wxPoint pos, wxSize size, int orient )
/**********************************************************************/

/* Trace 1 pastille PAD_OVAL en position pos_X,Y:
 *     dimensions dx, dy,
 *     orientation orient
 * Pour une orientation verticale ou horizontale, la forme est flashee
 * Pour une orientation quelconque la forme est tracee comme un segment
 */
{
    D_CODE* dcode_ptr;
    char    cbuf[256];
    int     x0, y0, x1, y1, delta;

    if( orient == 900 || orient == 2700 )  /* orient tournee de 90 deg */
        EXCHG( size.x, size.y );

    /* Trace de la forme flashee */
    if( orient == 0 || orient == 900 || orient == 1800 || orient == 2700 )
    {
        UserToDeviceCoordinate( pos );
        UserToDeviceSize( size );

        dcode_ptr = get_D_code( size.x, size.y, GERB_OVALE, 0 );
        if( dcode_ptr->m_NumDcode != s_Last_D_code )
        {
            sprintf( cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode );
            fputs( cbuf, g_Plot_PlotOutputFile );
            s_Last_D_code = dcode_ptr->m_NumDcode;
        }
        sprintf( cbuf, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
        fputs( cbuf, g_Plot_PlotOutputFile );
    }
    else /* Forme tracee comme un segment */
    {
        if( size.x > size.y )
        {
            EXCHG( size.x, size.y );
            if( orient < 2700 )
                orient += 900;
            else
                orient -= 2700;
        }
        /* la pastille est ramenee a une pastille ovale avec dy > dx */
        delta = size.y - size.x;
        x0    = 0;
        y0    = -delta / 2;
        x1    = 0;
        y1    = delta / 2;
        RotatePoint( &x0, &y0, orient );
        RotatePoint( &x1, &y1, orient );
        SelectD_CODE_For_LineDraw( size.x );
        PlotGERBERLine( wxPoint( pos.x + x0, pos.y + y0 ),
            wxPoint( pos.x + x1, pos.y + y1 ), size.x );
    }
}


/******************************************************************/
void Plot_1_CIRCLE_pad_GERBER( wxPoint pos, int diametre )
/******************************************************************/

/* Plot a circular pad or via at the user position pos
 */
{
    D_CODE* dcode_ptr;
    char    cbuf[256];

    wxSize  size( diametre, diametre );

    UserToDeviceCoordinate( pos );
    UserToDeviceSize( size );

    dcode_ptr = get_D_code( size.x, size.x, GERB_CIRCLE, 0 );
    if( dcode_ptr->m_NumDcode != s_Last_D_code )
    {
        sprintf( cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode );
        fputs( cbuf, g_Plot_PlotOutputFile );
        s_Last_D_code = dcode_ptr->m_NumDcode;
    }

    sprintf( cbuf, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
    fputs( cbuf, g_Plot_PlotOutputFile );
}


/**************************************************************************/
void PlotRectangularPad_GERBER( wxPoint pos, wxSize size, int orient )
/**************************************************************************/

/* Plot 1 rectangular pad
 * donne par son centre, ses dimensions, et son orientation
 * For a vertical or horizontal shape, the shape is an aperture (Dcode) and it is flashed
 * For others orientations the shape is plotted as a polygon
 */
{
    D_CODE* dcode_ptr;
    char    cbuf[256];

    /* Trace de la forme flashee */
    switch( orient )
    {
    case 900:
    case 2700: /* la rotation de 90 ou 270 degres revient a permutter des dimensions */
        EXCHG( size.x, size.y );

    // Pass through

    case 0:
    case 1800:
        UserToDeviceCoordinate( pos );
        UserToDeviceSize( size );

        dcode_ptr = get_D_code( size.x, size.y, GERB_RECT, 0 );
        if( dcode_ptr->m_NumDcode != s_Last_D_code )
        {
            sprintf( cbuf, "G54D%d*\n", dcode_ptr->m_NumDcode );
            fputs( cbuf, g_Plot_PlotOutputFile );
            s_Last_D_code = dcode_ptr->m_NumDcode;
        }
        sprintf( cbuf, "X%5.5dY%5.5dD03*\n", pos.x, pos.y );
        fputs( cbuf, g_Plot_PlotOutputFile );
        break;

    default: /* plot pad shape as polygon */
        trace_1_pad_TRAPEZE_GERBER( pos, size, wxSize( 0, 0 ), orient, FILLED );
        break;
    }
}


/*****************************************************************/
void trace_1_contour_GERBER( wxPoint pos, wxSize size, wxSize delta,
                             int penwidth, int orient )
/*****************************************************************/

/* Trace 1 contour rectangulaire ou trapezoidal d'orientation quelconque
 * donne par son centre,
 * ses dimensions ,
 * ses variations ,
 * l'epaisseur du trait,
 * et son orientation orient
 */
{
    int     ii;
    wxPoint coord[4];

    size.x  /= 2;
    size.y  /= 2;
    delta.x /= 2;
    delta.y /= 2; /* demi dim  dx et dy */

    coord[0].x = pos.x - size.x - delta.y;
    coord[0].y = pos.y + size.y + delta.x;

    coord[1].x = pos.x - size.x + delta.y;
    coord[1].y = pos.y - size.y - delta.x;

    coord[2].x = pos.x + size.x - delta.y;
    coord[2].y = pos.y - size.y + delta.x;

    coord[3].x = pos.x + size.x + delta.y;
    coord[3].y = pos.y + size.y - delta.x;

    for( ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &coord[ii].x, &coord[ii].y, pos.x, pos.y, orient );
    }

    SelectD_CODE_For_LineDraw( penwidth );
    PlotGERBERLine( coord[0], coord[1], penwidth );
    PlotGERBERLine( coord[1], coord[2], penwidth );
    PlotGERBERLine( coord[2], coord[3], penwidth );
    PlotGERBERLine( coord[3], coord[0], penwidth );
}


/*******************************************************************/
void trace_1_pad_TRAPEZE_GERBER( wxPoint pos, wxSize size, wxSize delta,
                                 int orient, int modetrace )
/*******************************************************************/

/* Trace 1 pad trapezoidal donne par :
 *    son centre pos.x,pos.y
 *    ses dimensions size.x et size.y
 *    les variations delta.x et delta.y ( 1 des deux au moins doit etre nulle)
 *    son orientation orient en 0.1 degres
 *    le mode de trace (FILLED, SKETCH, FILAIRE)
 *
 * Le trace n'est fait que pour un trapeze, c.a.d que delta.x ou delta.y
 *    = 0.
 *
 *   les notation des sommets sont ( vis a vis de la table tracante )
 *
 * "       0 ------------- 3   "
 * "        .             .    "
 * "         .     O     .     "
 * "          .         .      "
 * "           1 ---- 2        "
 *
 *
 *   exemple de Disposition pour delta.y > 0, delta.x = 0
 * "           1 ---- 2        "
 * "          .         .      "
 * "         .     O     .     "
 * "        .             .    "
 * "       0 ------------- 3   "
 *
 *
 *   exemple de Disposition pour delta.y = 0, delta.x > 0
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
    int     ii, jj;
    int     dx, dy;
    wxPoint polygon[4]; /* polygon corners */
    int     coord[8];
    int     ddx, ddy;

    /* calcul des dimensions optimales du spot choisi = 1/4 plus petite dim */
    dx = size.x - abs( delta.y );
    dy = size.y - abs( delta.x );

    dx  = size.x / 2;
    dy  = size.y / 2;
    ddx = delta.x / 2;
    ddy = delta.y / 2;

    polygon[0].x = -dx - ddy;
    polygon[0].y = +dy + ddx;
    polygon[1].x = -dx + ddy;
    polygon[1].y = -dy - ddx;
    polygon[2].x = +dx - ddy;
    polygon[2].y = -dy + ddx;
    polygon[3].x = +dx + ddy;
    polygon[3].y = +dy - ddx;

    /* Dessin du polygone et Remplissage eventuel de l'interieur */

    for( ii = 0, jj = 0; ii < 4; ii++ )
    {
        RotatePoint( &polygon[ii].x, &polygon[ii].y, orient );
        coord[jj] = polygon[ii].x += pos.x;
        jj++;
        coord[jj] = polygon[ii].y += pos.y;
        jj++;
    }

    if( modetrace != FILLED )
    {
        int plotLine_width = (int) ( 10 * g_PlotLine_Width * Gerb_scale_plot );
        SelectD_CODE_For_LineDraw( plotLine_width );
        PlotGERBERLine( polygon[0], polygon[1], plotLine_width );
        PlotGERBERLine( polygon[1], polygon[2], plotLine_width );
        PlotGERBERLine( polygon[2], polygon[3], plotLine_width );
        PlotGERBERLine( polygon[3], polygon[0], plotLine_width );
    }
    else
        PlotFilledPolygon_GERBER( 4, coord );
}


/**********************************************************/
void SelectD_CODE_For_LineDraw( int aSize )
/**********************************************************/

/** Selects a D_Code nn to draw lines and writes G54Dnn to output file
 * @param aSize = D_CODE diameter
 */
{
    D_CODE* dcode_ptr;

    dcode_ptr = get_D_code( aSize, aSize, GERB_LINE, 0 );
    if( dcode_ptr->m_NumDcode != s_Last_D_code )
    {
        fprintf( g_Plot_PlotOutputFile, "G54D%d*\n", dcode_ptr->m_NumDcode );
        s_Last_D_code = dcode_ptr->m_NumDcode;
    }
}


/*******************************************************/
D_CODE* get_D_code( int dx, int dy, int type, int drill )
/*******************************************************/

/* Fonction Recherchant et Creant eventuellement la description
 *   du D_CODE du type et dimensions demandees
 */
{
    D_CODE* ptr_tool, * last_dcode_ptr;
    int     num_new_D_code = FIRST_DCODE_VALUE;


    ptr_tool = last_dcode_ptr = s_DCodeList;

    while( ptr_tool && ptr_tool->m_Type >= 0 )
    {
        if( ( ptr_tool->m_Size.x == dx )
           && ( ptr_tool->m_Size.y == dy )
           && ( ptr_tool->m_Type == type ) )
            return ptr_tool;            /* D_code deja existant */
        last_dcode_ptr = ptr_tool;
        ptr_tool = ptr_tool->m_Pnext;
        num_new_D_code++;
    }

    /* At this point, the requested D_CODE does not exist: It will be created */
    if( ptr_tool == NULL )  /* We must create a new data */
    {
        ptr_tool = new D_CODE();

        ptr_tool->m_NumDcode = num_new_D_code;
        if( last_dcode_ptr )
        {
            ptr_tool->m_Pback = last_dcode_ptr;
            last_dcode_ptr->m_Pnext = ptr_tool;
        }
        else
            s_DCodeList = ptr_tool;
    }
    ptr_tool->m_Size.x = dx;
    ptr_tool->m_Size.y = dy;
    ptr_tool->m_Type   = type;
    return ptr_tool;
}


/***********************************/
static void Init_ApertureList()
/***********************************/

/* Init the memory to handle the aperture list:
 *   the member .m_Type is used by get_D_code() to handle the end of list:
 *   .m_Type < 0 is the first free aperture descr
 */
{
    D_CODE* ptr_tool;

    ptr_tool = s_DCodeList;
    while( ptr_tool )
    {
        s_DCodeList->m_Type = -1;
        ptr_tool = ptr_tool->m_Pnext;
    }

    ShowDcodeError = TRUE;
}


/******************************************************/
int Gen_D_CODE_File( FILE* penfile )
/******************************************************/

/* Genere la liste courante des D_CODES
 * Retourne le nombre de D_Codes utilises
 * Genere une sequence RS274X
 */
{
    D_CODE* ptr_tool;
    char    cbuf[1024];
    int     nb_dcodes = 0;

    /* Init : */
    ptr_tool = s_DCodeList;

    while( ptr_tool && ( ptr_tool->m_Type >= 0 ) )
    {
        float fscale = 0.0001; // For 3.4 format
        char* text;
        sprintf( cbuf, "%%ADD%d", ptr_tool->m_NumDcode );
        text = cbuf + strlen( cbuf );

        switch( ptr_tool->m_Type )
        {
        case 1: // Circle (flash )
            sprintf( text, "C,%f*%%\n", ptr_tool->m_Size.x * fscale );
            break;

        case 2: // PAD_RECT
            sprintf( text, "R,%fX%f*%%\n", ptr_tool->m_Size.x * fscale,
                ptr_tool->m_Size.y * fscale );
            break;

        case 3: // Circle ( lines )
            sprintf( text, "C,%f*%%\n", ptr_tool->m_Size.x * fscale );
            break;

        case 4: // PAD_OVAL
            sprintf( text, "O,%fX%f*%%\n", ptr_tool->m_Size.x * fscale,
                ptr_tool->m_Size.y * fscale );
            break;

        default:
            DisplayError( NULL, wxT( "Gen_D_CODE_File(): Dcode Type err" ) );
            break;
        }

        fputs( cbuf, penfile );
        ptr_tool = ptr_tool->m_Pnext;
        nb_dcodes++;
    }

    return nb_dcodes;
}


/*****************************/
void CloseFileGERBER( void )
/****************************/

/** Function CloseFileGERBER
 */
{
    char     line[1024];
    wxString TmpFileName, msg;
    FILE*    tmpfile;

    fputs( "M02*\n", g_Plot_PlotOutputFile );
    fclose( g_Plot_PlotOutputFile );

    // Reouverture g_Plot_PlotOutputFile pour ajout des Apertures
    g_Plot_PlotOutputFile = wxFopen( GerberFullFileName, wxT( "rt" ) );
    if( g_Plot_PlotOutputFile == NULL )
    {
        msg.Printf( _( "unable to reopen file <%s>" ), GerberFullFileName.GetData() );
        DisplayError( NULL, msg );
        return;
    }

    // Ouverture tmpfile
    TmpFileName = GerberFullFileName + wxT( ".$$$" );
    tmpfile     = wxFopen( TmpFileName, wxT( "wt" ) );
    if( tmpfile == NULL )
    {
        fclose( g_Plot_PlotOutputFile );
        DisplayError( NULL, wxT( "CloseFileGERBER(): Can't Open tmp file" ) );
        return;
    }

    // Placement des Apertures en RS274X
    rewind( g_Plot_PlotOutputFile );
    while( fgets( line, 1024, g_Plot_PlotOutputFile ) )
    {
        fputs( line, tmpfile );
        if( strcmp( strtok( line, "\n\r" ), "G04 APERTURE LIST*" ) == 0 )
        {
            Gen_D_CODE_File( tmpfile );
            fputs( "G04 APERTURE END LIST*\n", tmpfile );
        }
    }

    fclose( tmpfile );
    fclose( g_Plot_PlotOutputFile );
    wxRemoveFile( GerberFullFileName );
    wxRenameFile( TmpFileName, GerberFullFileName );
}

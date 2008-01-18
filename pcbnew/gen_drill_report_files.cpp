/*************************************************************************/
/* Functions to create drill data used to create aFiles and report  aFiles */
/*************************************************************************/

#include "fctsys.h"

using namespace std;

#include <vector>

#include "common.h"
#include "plot_common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "macros.h"
#include "gendrill.h"

static void PlotDrillSymbol( const wxPoint& position, int diametre, int aShapeId, int format );
static void PlotOvalDrillSymbol( const wxPoint& position,
                                 const wxSize&  size,
                                 int            orient,
                                 int            format );


/**********************************************************************************/
void GenDrillMapFile( BOARD* aPcb, FILE* aFile, const wxString& aFullFileName, wxSize aSheetSize,
                      std::vector<HOLE_INFO> aHoleListBuffer,
                      std::vector<DRILL_TOOL> aToolListBuffer,
                      bool aUnit_Drill_is_Inch, int format )
/**********************************************************************************/

/* Genere le plan de percage (Drill map) format HPGL ou POSTSCRIPT
 */
{
    unsigned        ii;
    int             x, y;
    int             plotX, plotY, TextWidth;
    int             intervalle = 0, CharSize = 0;
    EDA_BaseStruct* PtStruct;
    int             old_g_PlotOrient = g_PlotOrient;
    char            line[1024];
    int             dX, dY;
    wxPoint         BoardCentre;
    int             PlotMarge_in_mils = 400; // Margin in 1/1000 inch
    int             marge = PlotMarge_in_mils * U_PCB;
    wxSize          SheetSize;
    float           fTextScale = 1.0;
    double          scale_x    = 1.0, scale_y = 1.0;
    Ki_PageDescr*   SheetPS    = NULL;
    wxString        msg;


    setlocale( LC_NUMERIC, "C" ); // Use the standard notation for float numbers
    g_PlotOrient = 0;
    /* calcul des dimensions et centre du PCB */
    aPcb->ComputeBoundaryBox();

    dX = aPcb->m_BoundaryBox.GetWidth();
    dY = aPcb->m_BoundaryBox.GetHeight();
    BoardCentre = aPcb->m_BoundaryBox.Centre();

    // Calcul de l'echelle du dessin du PCB,
    // Echelle 1 en HPGL, dessin sur feuille A4 en PS, + texte description des symboles
    switch( format )
    {
    case PLOT_FORMAT_HPGL:     /* Calcul des echelles de conversion format HPGL */
        Scale_X        = Scale_Y = 1.0;
        scale_x        = Scale_X * SCALE_HPGL;
        scale_y        = Scale_Y * SCALE_HPGL;
        fTextScale     = SCALE_HPGL;
        SheetSize      = aSheetSize;
        SheetSize.x   *= U_PCB;
        SheetSize.y   *= U_PCB;
        g_PlotOffset.x = 0;
        g_PlotOffset.y = (int) (SheetSize.y * scale_y);
        break;

    case PLOT_FORMAT_POST:
    {
        // calcul en unites internes des dimensions de la feuille ( connues en 1/1000 pouce )
        SheetPS     = &g_Sheet_A4;
        SheetSize.x = SheetPS->m_Size.x * U_PCB;
        SheetSize.y = SheetPS->m_Size.y * U_PCB;
        float Xscale = (float) ( SheetSize.x - (marge * 2) ) / dX;
        float Yscale = (float) ( SheetSize.y * 0.6 - (marge * 2) ) / dY;

        scale_x = scale_y = MIN( Xscale, Yscale );

        g_PlotOffset.x = -(SheetSize.x / 2) +
                         (int) (BoardCentre.x * scale_x) + marge;
        g_PlotOffset.y = SheetSize.y / 2 +
                         (int) (BoardCentre.y * scale_y) - marge;
        g_PlotOffset.y += SheetSize.y / 8;      /* decalage pour legende */
        break;
    }

    default:
        break;
    }

    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        InitPlotParametresHPGL( g_PlotOffset, scale_x, scale_y );
        PrintHeaderHPGL( aFile, g_HPGL_Pen_Speed, g_HPGL_Pen_Num );
        break;

    case PLOT_FORMAT_POST:
    {
        int BBox[4];
        BBox[0] = BBox[1] = PlotMarge_in_mils;
        BBox[2] = SheetPS->m_Size.x - PlotMarge_in_mils;
        BBox[3] = SheetPS->m_Size.y - PlotMarge_in_mils;
        InitPlotParametresPS( g_PlotOffset,
                              SheetPS,
                              (double) 1.0 / PCB_INTERNAL_UNIT,
                              (double) 1.0 / PCB_INTERNAL_UNIT );
        SetDefaultLineWidthPS( 10 );     // Set line with to 10/1000 inch
        PrintHeaderPS( aFile, wxT( "PCBNEW-PS" ), aFullFileName, 1, BBox, wxLANDSCAPE );
        InitPlotParametresPS( g_PlotOffset, SheetPS, scale_x, scale_y );
    }
        break;

    default:
        break;
    }

    /* Draw items on edge layer */
    PtStruct = aPcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        switch( PtStruct->Type() )
        {
        case TYPEDRAWSEGMENT:
            PlotDrawSegment( (DRAWSEGMENT*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPETEXTE:
            PlotTextePcb( (TEXTE_PCB*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPECOTATION:
            PlotCotation( (COTATION*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPEMIRE:
            PlotMirePcb( (MIREPCB*) PtStruct, format, EDGE_LAYER );
            break;

        case TYPEMARKER:     // do not draw
            break;

        default:
            DisplayError( NULL, wxT( "WinEDA_DrillFrame::GenDrillMap() : Unexpected Draw Type" ) );
            break;
        }
    }

    TextWidth = 50;     // Set Drill Symbols width in 1/10000 mils

    if( format == PLOT_FORMAT_POST )
    {
        sprintf( line, "%d setlinewidth\n", TextWidth );
        fputs( line, aFile );
    }

    Gen_Drill_PcbMap( aPcb, aFile, aHoleListBuffer, aToolListBuffer, format );

    /* Impression de la liste des symboles utilises */
    CharSize = 800;                             /* text size in 1/10000 mils */
    float CharScale = 1.0 / scale_x;        /* real scale will be CharScale * scale_x,
                                             *  because the global plot scale is scale_x */
    TextWidth  = (int) (50 * CharScale);        // Set text width
    intervalle = (int) (CharSize * CharScale) + TextWidth;

    switch( format )
    {
    case PLOT_FORMAT_HPGL:
    {
        /* generation des dim: commande SI x,y; x et y = dim en cm */
        char csize[256];
        sprintf( csize, "%2.3f", (float) CharSize * CharScale * 0.000254 );
        sprintf( line, "SI %s, %s;\n", csize, csize );
        break;
    }

    case PLOT_FORMAT_POST:
        /* Reglage de l'epaisseur de traits des textes */
        sprintf( line, "%d setlinewidth\n", TextWidth );
        break;

    default:
        *line = 0;
        break;
    }

    fputs( line, aFile );

    switch( format )
    {
    default:
    case PLOT_FORMAT_POST:
        g_PlotOffset.x = 0;
        g_PlotOffset.y = 0;
        InitPlotParametresPS( g_PlotOffset, SheetPS, scale_x, scale_x );
        break;

    case PLOT_FORMAT_HPGL:
        InitPlotParametresHPGL( g_PlotOffset, scale_x, scale_x );
        break;
    }

    /* Trace des informations */

    /* Trace de "Infos" */
    plotX = marge + 1000;
    plotY = CharSize + 1000;
    x = plotX; y = plotY;
    x = +g_PlotOffset.x + (int) (x * fTextScale);
    y = g_PlotOffset.y - (int) (y * fTextScale);

    plotY += (int) ( intervalle * 1.2) + 200;

    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        sprintf( line, "PU %d, %d; LBInfos\03;\n",
                x + (int) (intervalle * CharScale * fTextScale),
                y - (int) (CharSize / 2 * CharScale * fTextScale) );
        fputs( line, aFile );
        break;

    case PLOT_FORMAT_POST:
        wxString Text = wxT( "Infos" );
        Plot_1_texte( format, Text, 0, TextWidth,
                      x, y,
                      (int) (CharSize * CharScale), (int) (CharSize * CharScale),
                      FALSE );
        break;
    }


    for( ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        int plot_diam;
        if( aToolListBuffer[ii].m_TotalCount == 0 )
            continue;

        plot_diam = (int) (aToolListBuffer[ii].m_Diameter);
        x = plotX; y = plotY;
        x = -g_PlotOffset.x + (int) (x * fTextScale);
        y = g_PlotOffset.y - (int) (y * fTextScale);
        PlotDrillSymbol( wxPoint( x, y ), plot_diam, ii, format );

        intervalle = (int) (CharSize * CharScale) + TextWidth;
        intervalle = (int) ( intervalle * 1.2);

        if( intervalle < (plot_diam + 200 + TextWidth) )
            intervalle = plot_diam + 200 + TextWidth;

        int rayon = plot_diam / 2;
        x = plotX + rayon + (int) (CharSize * CharScale); y = plotY;
        x = -g_PlotOffset.x + (int) (x * fTextScale);
        y = g_PlotOffset.y - (int) (y * fTextScale);

        /* Trace de la legende associee */
        switch( format )
        {
        case PLOT_FORMAT_HPGL:

            // List the diameter of each drill in the selected Drill Unit,
            // and then its diameter in the other Drill Unit.
            if( aUnit_Drill_is_Inch )
                sprintf( line, "PU %d, %d; LB%2.3f\" / %2.2fmm ",
                         x + (int) (intervalle * CharScale * fTextScale),
                         y - (int) (CharSize / 2 * CharScale * fTextScale),
                         float (aToolListBuffer[ii].m_Diameter) * 0.0001,
                         float (aToolListBuffer[ii].m_Diameter) * 0.00254 );
            else
                sprintf( line, "PU %d, %d; LB%2.2fmm / %2.3f\" ",
                         x + (int) (intervalle * CharScale * fTextScale),
                         y - (int) (CharSize / 2 * CharScale * fTextScale),
                         float (aToolListBuffer[ii].m_Diameter) * 0.00254,
                         float (aToolListBuffer[ii].m_Diameter) * 0.0001 );
            fputs( line, aFile );

            // Now list how many holes and ovals are associated with each drill.
            if( ( aToolListBuffer[ii].m_TotalCount == 1 ) &&
               ( aToolListBuffer[ii].m_OvalCount == 0 ) )
                sprintf( line, "(1 hole)\n" );
            else if( aToolListBuffer[ii].m_TotalCount == 1 )      // && ( buffer[ii]m_OvalCount == 1 )
                sprintf( line, "(1 hole) (with 1 oblong)\n" );
            else if( aToolListBuffer[ii].m_OvalCount == 0 )
                sprintf( line, "(%d holes)\n",
                         aToolListBuffer[ii].m_TotalCount );
            else if( aToolListBuffer[ii].m_OvalCount == 1 )
                sprintf( line, "(%d holes) (with 1 oblong)\n",
                         aToolListBuffer[ii].m_TotalCount );
            else      //  if ( aToolListBuffer[ii]m_OvalCount > 1 )
                sprintf( line, "(%d holes) (with %d oblongs)\n",
                         aToolListBuffer[ii].m_TotalCount,
                         aToolListBuffer[ii].m_OvalCount );
            fputs( line, aFile );
            fputs( "\03;\n", aFile );
            break;

        case PLOT_FORMAT_POST:

            // List the diameter of each drill in the selected Drill Unit,
            // and then its diameter in the other Drill Unit.
            if( aUnit_Drill_is_Inch )
                sprintf( line, "%2.3f\" / %2.2fmm ",
                         float (aToolListBuffer[ii].m_Diameter) * 0.0001,
                         float (aToolListBuffer[ii].m_Diameter) * 0.00254 );
            else
                sprintf( line, "%2.2fmm / %2.3f\" ",
                         float (aToolListBuffer[ii].m_Diameter) * 0.00254,
                         float (aToolListBuffer[ii].m_Diameter) * 0.0001 );
            msg = CONV_FROM_UTF8( line );

            // Now list how many holes and ovals are associated with each drill.
            if( ( aToolListBuffer[ii].m_TotalCount == 1 ) &&
               ( aToolListBuffer[ii].m_OvalCount == 0 ) )
                sprintf( line, "(1 hole)" );
            else if( aToolListBuffer[ii].m_TotalCount == 1 )      // && ( aToolListBuffer[ii]m_OvalCount == 1 )
                sprintf( line, "(1 hole) (with 1 oblong)" );
            else if( aToolListBuffer[ii].m_OvalCount == 0 )
                sprintf( line, "(%d holes)",
                         aToolListBuffer[ii].m_TotalCount );
            else if( aToolListBuffer[ii].m_OvalCount == 1 )
                sprintf( line, "(%d holes) (with 1 oblong)",
                         aToolListBuffer[ii].m_TotalCount );
            else      // if ( aToolListBuffer[ii]m_OvalCount > 1 )
                sprintf( line, "(%d holes) (with %d oblongs)",
                         aToolListBuffer[ii].m_TotalCount,
                         aToolListBuffer[ii].m_OvalCount );
            msg += CONV_FROM_UTF8( line );
            Plot_1_texte( format, msg, 0, TextWidth,
                          x, y,
                          (int) (CharSize * CharScale),
                          (int) (CharSize * CharScale),
                          FALSE );
            break;
        }

        plotY += intervalle;
    }

    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        CloseFileHPGL( aFile );
        break;

    case PLOT_FORMAT_POST:
        CloseFilePS( aFile );
        break;
    }

    setlocale( LC_NUMERIC, "" );    // Revert to local notation for float numbers

    g_PlotOrient = old_g_PlotOrient;
}


/****************************************************************************************/
void Gen_Drill_PcbMap( BOARD* aPcb, FILE* aFile,
                       std::vector<HOLE_INFO>& aHoleListBuffer,
                       std::vector<DRILL_TOOL>& aToolListBuffer,
                       int format )
/****************************************************************************************/

/** Creates the drill map aFile in HPGL or POSTSCRIPT format
 * @param aPcb BOARD
 * @param aFile = output aFile
 * @param aHoleListBuffer = std::vector<HOLE_INFO> list of holes descriptors
 * @param aToolListBuffer = std::vector<DRILL_TOOL> drill list buffer
 * @param format = ouput format (hpgl / ps)
 */
{
    wxPoint pos;

    /* create the drill list */
    if( aToolListBuffer.size() > 13 )
    {
        DisplayInfo( NULL,
                     _(
                         " Drill map: Too many diameter values to draw to draw one symbol per drill value (max 13)\nPlot uses circle shape for some drill values" ),
                     10 );
    }

    // Plot the drill map:
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        pos.x = aHoleListBuffer[ii].m_Hole_Pos_X;
        pos.y = aHoleListBuffer[ii].m_Hole_Pos_Y;

        if( aHoleListBuffer[ii].m_Hole_Shape == 0 )
        {
            PlotDrillSymbol( pos, aHoleListBuffer[ii].m_Hole_Diameter,
                             aHoleListBuffer[ii].m_Tool_Reference - 1,
                             format );
        }
        else
        {
            wxSize oblong_size;
            oblong_size.x = aHoleListBuffer[ii].m_Hole_SizeX;
            oblong_size.y = aHoleListBuffer[ii].m_Hole_SizeY;
            PlotOvalDrillSymbol( pos, oblong_size, aHoleListBuffer[ii].m_Hole_Orient, format );
        }
    }
}


/************************************************************************************/
void PlotDrillSymbol( const wxPoint& position, int diametre, int aShapeId, int format )
/************************************************************************************/

/* Trace un motif de numero de forme aShapeId, aux coord x0, y0.
 *  x0, y0 = coordonnees tables
 *  diametre = diametre (coord table) du trou
 *  aShapeId = index ( permet de generer des formes caract )
 */
{
    int  rayon = diametre / 2;

    void (*FctPlume)( wxPoint pos, int state );
    int  x0, y0;

    x0 = position.x; y0 = position.y;
    FctPlume = Move_Plume_HPGL;
    if( format == PLOT_FORMAT_POST )
        FctPlume = LineTo_PS;

    switch( aShapeId )
    {
    case 0:     /* vias : forme en X */
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 + rayon ), 'D' );
        break;

    case 1:     /* Cercle */
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 2:     /* forme en + */
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 ), 'D' );
        break;

    case 3:     /* forme en X cercle */
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 4:     /* forme en cercle barre de - */
        FctPlume( wxPoint( x0 - rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 5:     /* forme en cercle barre de | */
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;

    case 6:     /* forme en carre */
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        break;

    case 7:     /* forme en losange */
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    case 8:     /* forme en carre barre par un X*/
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        break;

    case 9:     /* forme en losange barre par un +*/
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 - rayon, y0 ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    case 10:     /* forme en carre barre par un '/' */
        FctPlume( wxPoint( x0 - rayon, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 0,
                                      FILAIRE );
        break;

    case 11:     /* forme en losange barre par un |*/
        FctPlume( wxPoint( x0, y0 - rayon ), 'U' );
        FctPlume( wxPoint( x0, y0 + rayon ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    case 12:     /* forme en losange barre par un -*/
        FctPlume( wxPoint( x0 - rayon, y0 ), 'U' );
        FctPlume( wxPoint( x0 + rayon, y0 ), 'D' );
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pad_TRAPEZE_HPGL( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pad_TRAPEZE_POST( wxPoint( x0, y0 ), wxSize( rayon, rayon ), wxSize( 0,
                                                                                         0 ), 450,
                                      FILAIRE );
        break;

    default:
        if( format == PLOT_FORMAT_HPGL )
            trace_1_pastille_RONDE_HPGL( wxPoint( x0, y0 ), diametre, FILAIRE );
        if( format == PLOT_FORMAT_POST )
            trace_1_pastille_RONDE_POST( wxPoint( x0, y0 ), diametre, FILAIRE );
        break;
    }

    if( format == PLOT_FORMAT_HPGL )
        Plume_HPGL( 'U' );
}


/*********************************************************************************************/
void PlotOvalDrillSymbol( const wxPoint& position, const wxSize& size, int orient, int format )
/*********************************************************************************************/

/* Draws an oblong hole.
 * because functions to draw oblong shapes exist to draw oblong pads, Use they.
 */
{
    switch( format )
    {
    case PLOT_FORMAT_HPGL:
        trace_1_pastille_OVALE_HPGL( position, size, orient, FILAIRE );
        break;

    case PLOT_FORMAT_POST:
        trace_1_pastille_OVALE_POST( position, size, orient, FILAIRE );
        break;
    }
}


/**************************************************************************************************/
void GenDrillReportFile( FILE* aFile, const wxString& aBoardFilename,
                         std::vector<DRILL_TOOL>& aToolListBuffer, bool aUnit_Drill_is_Inch )
/*************************************************************************************************/

/*
 *  Create a list of drill values and drill count
 */
{
    unsigned TotalHoleCount;
    char     line[1024];

    fprintf( aFile, "Drill report for %s\n", CONV_TO_UTF8( aBoardFilename ) );
    fprintf( aFile, "Created on %s\n", DateAndTime( line ) );

    // List which Drill Unit option had been selected for the associated drill aFile.
    if( aUnit_Drill_is_Inch )
        fputs( "Selected Drill Unit: Imperial (\")\n\n", aFile );
    else
        fputs( "Selected Drill Unit: Metric (mm)\n\n", aFile );

    TotalHoleCount = 0;

    for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        // List the tool number assigned to each drill,
        // then its diameter in the selected Drill Unit,
        // and then its diameter in the other Drill Unit.
        if( aUnit_Drill_is_Inch )
            sprintf( line, "T%d  %2.3f\"  %2.2fmm  ",
                     ii + 1,
                     float (aToolListBuffer[ii].m_Diameter) * 0.0001,
                     float (aToolListBuffer[ii].m_Diameter) * 0.00254 );
        else
            sprintf( line, "T%d  %2.2fmm  %2.3f\"  ",
                     ii + 1,
                     float (aToolListBuffer[ii].m_Diameter) * 0.00254,
                     float (aToolListBuffer[ii].m_Diameter) * 0.0001 );
        fputs( line, aFile );

        // Now list how many holes and ovals are associated with each drill.
        if( ( aToolListBuffer[ii].m_TotalCount == 1 ) && ( aToolListBuffer[ii].m_OvalCount == 0 ) )
            sprintf( line, "(1 hole)\n" );
        else if( aToolListBuffer[ii].m_TotalCount == 1 )
            sprintf( line, "(1 hole)  (with 1 oblong)\n" );
        else if( aToolListBuffer[ii].m_OvalCount == 0 )
            sprintf( line, "(%d holes)\n",
                     aToolListBuffer[ii].m_TotalCount );
        else if( aToolListBuffer[ii].m_OvalCount == 1 )
            sprintf( line, "(%d holes)  (with 1 oblong)\n",
                     aToolListBuffer[ii].m_TotalCount );
        else  //  if ( buffer[ii]m_OvalCount > 1 )
            sprintf( line, "(%d holes)  (with %d oblongs)\n",
                     aToolListBuffer[ii].m_TotalCount,
                     aToolListBuffer[ii].m_OvalCount );
        fputs( line, aFile );

        TotalHoleCount += aToolListBuffer[ii].m_TotalCount;
    }

    sprintf( line, "\ntotal holes count %d\n", TotalHoleCount );
    fputs( line, aFile );

    fclose( aFile );
}

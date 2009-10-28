/*************************************************************************/
/* Functions to create drill data used to create aFiles and report  aFiles */
/*************************************************************************/

#include "fctsys.h"

using namespace std;

#include <vector>

#include "common.h"
#include "plot_common.h"
#include "base_struct.h"
#include "colors.h"
#include "drawtxt.h"
#include "confirm.h"
#include "kicad_string.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "macros.h"
#include "class_board_design_settings.h"
#include "gendrill.h"

/**********************************************************************************/
void GenDrillMapFile( BOARD* aPcb, FILE* aFile, const wxString& aFullFileName,
                      Ki_PageDescr* aSheet,
                      std::vector<HOLE_INFO> aHoleListBuffer,
                      std::vector<DRILL_TOOL> aToolListBuffer,
                      bool aUnit_Drill_is_Inch, int format,
                      const wxPoint& auxoffset )
/**********************************************************************************/

/* Genere le plan de percage (Drill map)
 */
{
    int             x, y;
    int             plotX, plotY, TextWidth;
    double          scale = 1.0;
    int             intervalle = 0, CharSize = 0;
    EDA_BaseStruct* PtStruct;
    char            line[1024];
    int             dX, dY;
    wxPoint         BoardCentre;
    wxPoint         offset;
    wxString        msg;
    PLOTTER*        plotter = NULL;


    SetLocaleTo_C_standard();  // Use the standard notation for float numbers
    /* calcul des dimensions et centre du PCB */
    aPcb->ComputeBoundaryBox();

    dX = aPcb->m_BoundaryBox.GetWidth();
    dY = aPcb->m_BoundaryBox.GetHeight();
    BoardCentre = aPcb->m_BoundaryBox.Centre();

    // Calcul de l'echelle du dessin du PCB,
    // Echelle 1 en HPGL, dessin sur feuille A4 en PS, + texte description des symboles
    switch( format )
    {
    case PLOT_FORMAT_GERBER:
        scale   = 1;
        offset  = auxoffset;
        plotter = new GERBER_PLOTTER();
        plotter->set_viewport( offset, scale, 0 );
        break;

    case PLOT_FORMAT_HPGL:     /* Calcul des echelles de conversion format HPGL */
    {
        offset.x = 0;
        offset.y = 0;
        scale    = 1;
        HPGL_PLOTTER* hpgl_plotter = new HPGL_PLOTTER;
        plotter = hpgl_plotter;
        hpgl_plotter->set_pen_number( g_pcb_plot_options.HPGL_Pen_Num );
        hpgl_plotter->set_pen_speed( g_pcb_plot_options.HPGL_Pen_Speed );
        hpgl_plotter->set_pen_overlap( 0 );
        plotter->set_paper_size( aSheet );
        plotter->set_viewport( offset, scale, 0 );
    }
    break;

    case PLOT_FORMAT_POST:
    {
        Ki_PageDescr* SheetPS = &g_Sheet_A4;
        wxSize        SheetSize;
        SheetSize.x = SheetPS->m_Size.x * U_PCB;
        SheetSize.y = SheetPS->m_Size.y * U_PCB;
        /* Keep size for drill legend */
        double Xscale = (double) ( SheetSize.x * 0.8 ) / dX;
        double Yscale = (double) ( SheetSize.y * 0.6 ) / dY;

        scale = MIN( Xscale, Yscale );

        offset.x  = BoardCentre.x - (SheetSize.x / 2) / scale;
        offset.y  = BoardCentre.y - (SheetSize.y / 2) / scale;
        offset.y += SheetSize.y / 8;      /* decalage pour legende */
        PS_PLOTTER* ps_plotter = new PS_PLOTTER;
        plotter = ps_plotter;
        ps_plotter->set_paper_size( SheetPS );
        plotter->set_viewport( offset, scale, 0 );
        break;
    }

    case PLOT_FORMAT_DXF:
    {
        offset.x = 0;
        offset.y = 0;
        scale    = 1;
        DXF_PLOTTER* dxf_plotter = new DXF_PLOTTER;
        plotter = dxf_plotter;
        plotter->set_paper_size( aSheet );
        plotter->set_viewport( offset, scale, 0 );
        break;
    }

    default:
        wxASSERT( false );
    }

    plotter->set_creator( wxT( "PCBNEW" ) );
    plotter->set_filename( aFullFileName );
    plotter->set_default_line_width( 10 );
    plotter->start_plot( aFile );

    /* Draw items on edge layer */

    for( PtStruct = aPcb->m_Drawings;
        PtStruct != NULL;
        PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case TYPE_DRAWSEGMENT:
            PlotDrawSegment( plotter, (DRAWSEGMENT*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case TYPE_TEXTE:
            PlotTextePcb( plotter, (TEXTE_PCB*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case TYPE_COTATION:
            PlotCotation( plotter, (COTATION*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case TYPE_MIRE:
            PlotMirePcb( plotter, (MIREPCB*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case TYPE_MARKER_PCB:     // do not draw
            break;

        default:
            DisplayError( NULL, wxT( "WinEDA_DrillFrame::GenDrillMap() : Unexpected Draw Type" ) );
            break;
        }
    }

    // Set Drill Symbols width in 1/10000 mils
    plotter->set_default_line_width( 10 );
    plotter->set_current_line_width( -1 );

    // Plot board outlines and drill map
    Gen_Drill_PcbMap( aPcb, plotter, aHoleListBuffer, aToolListBuffer );

    /* Impression de la liste des symboles utilises */
    CharSize = 800;                                             /* text size in 1/10000 mils */
    double CharScale = 1.0 / scale;        /* real scale will be CharScale * scale_x,
                                            *  because the global plot scale is scale_x */
    TextWidth  = (int) ( (CharSize * CharScale) / 10 );         // Set text width (thickness)
    intervalle = (int) ( CharSize * CharScale ) + TextWidth;

    /* Trace des informations */
    plotX = aPcb->m_BoundaryBox.GetX() + 200 * CharScale;
    plotY = aPcb->m_BoundaryBox.GetBottom() + intervalle;

    /* Plot title  "Info" */
    wxString Text = wxT( "Drill Map:" );
    plotter->text( wxPoint( plotX, plotY ), BLACK,
                   Text,
                   0, wxSize( (int) ( CharSize * CharScale ), (int) ( CharSize * CharScale ) ),
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                   TextWidth, false, false );

    for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        int plot_diam;
        if( aToolListBuffer[ii].m_TotalCount == 0 )
            continue;

        plotY += intervalle;

        plot_diam = (int) ( aToolListBuffer[ii].m_Diameter );
        x = plotX - 200 * CharScale - plot_diam / 2;
        y = plotY + CharSize * CharScale;
        plotter->marker( wxPoint( x, y ), plot_diam, ii );

        /* Trace de la legende associee */

        // List the diameter of each drill in the selected Drill Unit,
        // and then its diameter in the other Drill Unit.
        if( aUnit_Drill_is_Inch )
            sprintf( line, "%2.3f\" / %2.2fmm ",
                     double (aToolListBuffer[ii].m_Diameter) * 0.0001,
                     double (aToolListBuffer[ii].m_Diameter) * 0.00254 );
        else
            sprintf( line, "%2.2fmm / %2.3f\" ",
                     double (aToolListBuffer[ii].m_Diameter) * 0.00254,
                     double (aToolListBuffer[ii].m_Diameter) * 0.0001 );
        msg = CONV_FROM_UTF8( line );

        // Now list how many holes and ovals are associated with each drill.
        if( ( aToolListBuffer[ii].m_TotalCount == 1 )
           && ( aToolListBuffer[ii].m_OvalCount == 0 ) )
            sprintf( line, "(1 hole)" );
        else if( aToolListBuffer[ii].m_TotalCount == 1 )      // && ( aToolListBuffer[ii]m_OvalCount == 1 )
            sprintf( line, "(1 slot)" );
        else if( aToolListBuffer[ii].m_OvalCount == 0 )
            sprintf( line, "(%d holes)",
                     aToolListBuffer[ii].m_TotalCount );
        else if( aToolListBuffer[ii].m_OvalCount == 1 )
            sprintf( line, "(%d holes + 1 slot)",
                     aToolListBuffer[ii].m_TotalCount - 1 );
        else      // if ( aToolListBuffer[ii]m_OvalCount > 1 )
            sprintf( line, "(%d holes + %d slots)",
                     aToolListBuffer[ii].m_TotalCount -
                     aToolListBuffer[ii].m_OvalCount,
                     aToolListBuffer[ii].m_OvalCount );
        msg += CONV_FROM_UTF8( line );
        plotter->text( wxPoint( plotX, y ), BLACK,
                       msg,
                       0, wxSize( (int) ( CharSize * CharScale ), (int) ( CharSize * CharScale ) ),
                       GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                       TextWidth, false, false );

        intervalle = (int) ( CharSize * CharScale ) + TextWidth;
        intervalle = (int) ( intervalle * 1.2 );

        if( intervalle < (plot_diam + 200 + TextWidth) )
            intervalle = plot_diam + 200 + TextWidth;
    }

    plotter->end_plot();
    delete plotter;
    SetLocaleTo_Default();     // Revert to local notation for float numbers
}


/****************************************************************************************/
void Gen_Drill_PcbMap( BOARD* aPcb, PLOTTER* plotter,
                       std::vector<HOLE_INFO>& aHoleListBuffer,
                       std::vector<DRILL_TOOL>& aToolListBuffer )
/****************************************************************************************/

/** Creates the drill map aFile in HPGL or POSTSCRIPT format
 * @param aPcb BOARD
 * @param aHoleListBuffer = std::vector<HOLE_INFO> list of holes descriptors
 * @param aToolListBuffer = std::vector<DRILL_TOOL> drill list buffer
 */
{
    wxPoint pos;

    /* create the drill list */
    if( aToolListBuffer.size() > 13 )
    {
        DisplayInfoMessage( NULL,
                            _(
                                " Drill map: Too many diameter values to draw to draw one symbol per drill value (max 13)\nPlot uses circle shape for some drill values" ),
                            10 );
    }

    // Plot the drill map:
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        pos.x = aHoleListBuffer[ii].m_Hole_Pos_X;
        pos.y = aHoleListBuffer[ii].m_Hole_Pos_Y;

        /* Always plot the drill symbol (for slots identifies the needed
         * cutter!) */
        plotter->marker( pos, aHoleListBuffer[ii].m_Hole_Diameter,
                         aHoleListBuffer[ii].m_Tool_Reference - 1 );
        if( aHoleListBuffer[ii].m_Hole_Shape != 0 )
        {
            wxSize oblong_size;
            oblong_size.x = aHoleListBuffer[ii].m_Hole_SizeX;
            oblong_size.y = aHoleListBuffer[ii].m_Hole_SizeY;
            plotter->flash_pad_oval( pos, oblong_size,
                                     aHoleListBuffer[ii].m_Hole_Orient, FILAIRE );
        }
    }
}


/**************************************************************************************************/
void GenDrillReportFile( FILE* aFile, BOARD* aPcb, const wxString& aBoardFilename,
                         bool aUnit_Drill_is_Inch,
                         std::vector<HOLE_INFO>& aHoleListBuffer,
                         std::vector<DRILL_TOOL>& aToolListBuffer
                         )
/*************************************************************************************************/

/*
 *  Create a list of drill values and drill count
 */
{
    unsigned TotalHoleCount;
    char     line[1024];
    int      layer1 = COPPER_LAYER_N;
    int      layer2 = LAYER_CMP_N;
    bool     gen_through_holes = true;


    fprintf( aFile, "Drill report for %s\n", CONV_TO_UTF8( aBoardFilename ) );
    fprintf( aFile, "Created on %s\n", DateAndTime( line ) );

    // List which Drill Unit option had been selected for the associated drill aFile.
    if( aUnit_Drill_is_Inch )
        fputs( "Selected Drill Unit: Imperial (\")\n\n", aFile );
    else
        fputs( "Selected Drill Unit: Metric (mm)\n\n", aFile );

    /* build hole lists:
     * 1 - through holes
     * 2 - for partial holes only: by layer pair
     */

    for( ; ; )
    {
        Build_Holes_List( aPcb,
                          aHoleListBuffer,
                          aToolListBuffer,
                          layer1,
                          layer2,
                          gen_through_holes ? false : true );

        TotalHoleCount = 0;

        if( gen_through_holes )
        {
            sprintf( line, "Drill report for through holes :\n" );
        }
        else
        {
            if( layer1 == COPPER_LAYER_N )  // First partial hole list
            {
                sprintf( line, "Drill report for buried and blind vias :\n\n" );
                fputs( line, aFile );
            }

            sprintf( line, "Drill report for holes from layer %s to layer %s\n",
                    CONV_TO_UTF8( aPcb->GetLayerName( layer1 ) ),
                    CONV_TO_UTF8( aPcb->GetLayerName( layer2 ) ) );
        }

        fputs( line, aFile );

        for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
        {
            // List the tool number assigned to each drill,
            // then its diameter in the selected Drill Unit,
            // and then its diameter in the other Drill Unit.
            if( aUnit_Drill_is_Inch )
                sprintf( line, "T%d  %2.3f\"  %2.2fmm  ",
                         ii + 1,
                         double (aToolListBuffer[ii].m_Diameter) * 0.0001,
                         double (aToolListBuffer[ii].m_Diameter) * 0.00254 );
            else
                sprintf( line, "T%d  %2.2fmm  %2.3f\"  ",
                         ii + 1,
                         double (aToolListBuffer[ii].m_Diameter) * 0.00254,
                         double (aToolListBuffer[ii].m_Diameter) * 0.0001 );
            fputs( line, aFile );

            // Now list how many holes and ovals are associated with each drill.
            if( ( aToolListBuffer[ii].m_TotalCount == 1 )
               && ( aToolListBuffer[ii].m_OvalCount == 0 ) )
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

        sprintf( line, "\ntotal holes count %d\n\n\n", TotalHoleCount );
        fputs( line, aFile );

        if( g_DesignSettings.GetCopperLayerCount() <= 2 )
            break;

        if(  gen_through_holes )
            layer2 = layer1 + 1;
        else
        {
            if( layer2 >= LAYER_CMP_N )                                 // no more layer pair to consider
                break;
            layer1++; layer2++;                                         // use next layer pair
            if( layer2 == g_DesignSettings.GetCopperLayerCount() - 1 )     // The last layer is reached
                layer2 = LAYER_CMP_N;                                   // the last layer is always the component layer
        }
        gen_through_holes = false;
    }

    fclose( aFile );
}

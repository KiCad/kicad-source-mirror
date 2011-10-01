/************************************************************************/
/* Functions to create drill data used to create files and report files */
/************************************************************************/

#include "fctsys.h"
#include "common.h"
#include "plot_common.h"
#include "base_struct.h"
#include "colors.h"
#include "drawtxt.h"
#include "confirm.h"
#include "kicad_string.h"
#include "macros.h"

#include "class_board.h"

#include "pcbnew.h"
#include "pcbplot.h"
#include "gendrill.h"


void GenDrillMapFile( BOARD* aPcb, FILE* aFile, const wxString& aFullFileName,
                      Ki_PageDescr* aSheet,
                      std::vector<HOLE_INFO> aHoleListBuffer,
                      std::vector<DRILL_TOOL> aToolListBuffer,
                      bool aUnit_Drill_is_Inch, int format,
                      const wxPoint& auxoffset )
{
    int       x, y;
    int       plotX, plotY, TextWidth;
    double    scale = 1.0;
    int       intervalle = 0, CharSize = 0;
    EDA_ITEM* PtStruct;
    char      line[1024];
    int       dX, dY;
    wxPoint   BoardCentre;
    wxPoint   offset;
    wxString  msg;
    PLOTTER*  plotter = NULL;

    SetLocaleTo_C_standard();  // Use the standard notation for float numbers

    // Calculate dimensions and center of PCB
    aPcb->ComputeBoundingBox();

    dX = aPcb->m_BoundaryBox.GetWidth();
    dY = aPcb->m_BoundaryBox.GetHeight();
    BoardCentre = aPcb->m_BoundaryBox.Centre();

    // Calculate the scale for the format type, scale 1 in HPGL, drawing on
    // an A4 sheet in PS, + text description of symbols
    switch( format )
    {
    case PLOT_FORMAT_GERBER:
        scale   = 1;
        offset  = auxoffset;
        plotter = new GERBER_PLOTTER();
        plotter->set_viewport( offset, scale, 0 );
        break;

    case PLOT_FORMAT_HPGL:  /* Scale for HPGL format. */
    {
        offset.x = 0;
        offset.y = 0;
        scale    = 1;
        HPGL_PLOTTER* hpgl_plotter = new HPGL_PLOTTER;
        plotter = hpgl_plotter;
        hpgl_plotter->set_pen_number( g_PcbPlotOptions.m_HPGLPenNum );
        hpgl_plotter->set_pen_speed( g_PcbPlotOptions.m_HPGLPenSpeed );
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

        offset.x  = (int) ( (double) BoardCentre.x - ( (double) SheetSize.x / 2.0 ) / scale );
        offset.y  = (int) ( (double) BoardCentre.y - ( (double) SheetSize.y / 2.0 ) / scale );
        offset.y += SheetSize.y / 8;      /* offset to legend */
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

    for( PtStruct = aPcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_LINE_T:
            PlotDrawSegment( plotter, (DRAWSEGMENT*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case PCB_TEXT_T:
            PlotTextePcb( plotter, (TEXTE_PCB*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case PCB_DIMENSION_T:
            PlotDimension( plotter, (DIMENSION*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case PCB_TARGET_T:
            PlotPcbTarget( plotter, (PCB_TARGET*) PtStruct, EDGE_LAYER, FILLED );
            break;

        case PCB_MARKER_T:     // do not draw
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

    /* Print a list of symbols used. */
    CharSize = 800;                        /* text size in 1/10000 mils */
    double CharScale = 1.0 / scale;        /* real scale will be CharScale
                                            * scale_x, because the global
                                            * plot scale is scale_x */
    TextWidth  = (int) ( (CharSize * CharScale) / 10 );   // Set text width (thickness)
    intervalle = (int) ( CharSize * CharScale ) + TextWidth;

    /* Trace information. */
    plotX = (int) ( (double) aPcb->m_BoundaryBox.GetX() + 200.0 * CharScale );
    plotY = aPcb->m_BoundaryBox.GetBottom() + intervalle;

    /* Plot title  "Info" */
    wxString Text = wxT( "Drill Map:" );
    plotter->text( wxPoint( plotX, plotY ), BLACK, Text, 0,
                   wxSize( (int) ( CharSize * CharScale ),
                           (int) ( CharSize * CharScale ) ),
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                   TextWidth, false, false );

    for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        int plot_diam;

        if( aToolListBuffer[ii].m_TotalCount == 0 )
            continue;

        plotY += intervalle;

        plot_diam = (int) ( aToolListBuffer[ii].m_Diameter );
        x = (int) ( (double) plotX - 200.0 * CharScale - (double)plot_diam / 2.0 );
        y = (int) ( (double) plotY + (double) CharSize * CharScale );
        plotter->marker( wxPoint( x, y ), plot_diam, ii );

        /* Trace the legends. */

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

        msg = FROM_UTF8( line );

        // Now list how many holes and ovals are associated with each drill.
        if( ( aToolListBuffer[ii].m_TotalCount == 1 )
           && ( aToolListBuffer[ii].m_OvalCount == 0 ) )
            sprintf( line, "(1 hole)" );
        else if( aToolListBuffer[ii].m_TotalCount == 1 )      // && ( aToolListBuffer[ii]m_OvalCount == 1 )
            sprintf( line, "(1 slot)" );
        else if( aToolListBuffer[ii].m_OvalCount == 0 )
            sprintf( line, "(%d holes)", aToolListBuffer[ii].m_TotalCount );
        else if( aToolListBuffer[ii].m_OvalCount == 1 )
            sprintf( line, "(%d holes + 1 slot)", aToolListBuffer[ii].m_TotalCount - 1 );
        else      // if ( aToolListBuffer[ii]m_OvalCount > 1 )
            sprintf( line, "(%d holes + %d slots)",
                     aToolListBuffer[ii].m_TotalCount -
                     aToolListBuffer[ii].m_OvalCount,
                     aToolListBuffer[ii].m_OvalCount );

        msg += FROM_UTF8( line );
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


/** Creates the drill map aFile in HPGL or POSTSCRIPT format
 * @param aPcb = the BOARD
 * @param aPlotter = a PLOTTER instance (HPGL or POSTSCRIPT plotter).
 * @param aHoleListBuffer = std::vector<HOLE_INFO> list of holes descriptors
 * @param aToolListBuffer = std::vector<DRILL_TOOL> drill list buffer
 */
void Gen_Drill_PcbMap( BOARD* aPcb, PLOTTER* aPlotter,
                       std::vector<HOLE_INFO>& aHoleListBuffer,
                       std::vector<DRILL_TOOL>& aToolListBuffer )
{
    wxPoint pos;

    /* create the drill list */
    if( aToolListBuffer.size() > 13 )
    {
        DisplayInfoMessage( NULL,
                            _( " Drill map: Too many diameter values to draw to draw one symbol per drill value (max 13)\nPlot uses circle shape for some drill values" ),
                            10 );
    }

    // Plot the drill map:
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        pos = aHoleListBuffer[ii].m_Hole_Pos;

        /* Always plot the drill symbol (for slots identifies the needed
         * cutter!) */
        aPlotter->marker( pos, aHoleListBuffer[ii].m_Hole_Diameter,
                          aHoleListBuffer[ii].m_Tool_Reference - 1 );

        if( aHoleListBuffer[ii].m_Hole_Shape != 0 )
        {
            wxSize oblong_size;
            oblong_size = aHoleListBuffer[ii].m_Hole_Size;
            aPlotter->flash_pad_oval( pos, oblong_size,
                                      aHoleListBuffer[ii].m_Hole_Orient, FILAIRE );
        }
    }
}


/*
 *  Create a list of drill values and drill count
 */
void GenDrillReportFile( FILE* aFile, BOARD* aPcb,
                         const wxString& aBoardFilename,
                         bool aUnit_Drill_is_Inch,
                         std::vector<HOLE_INFO>& aHoleListBuffer,
                         std::vector<DRILL_TOOL>& aToolListBuffer )
{
    unsigned TotalHoleCount;
    char     line[1024];
    int      layer1 = LAYER_N_BACK;
    int      layer2 = LAYER_N_FRONT;
    bool     gen_through_holes = true;
    bool     gen_NPTH_holes = false;


    fprintf( aFile, "Drill report for %s\n", TO_UTF8( aBoardFilename ) );
    fprintf( aFile, "Created on %s\n", DateAndTime( line ) );

    // List which Drill Unit option had been selected for the associated
    // drill aFile.
    if( aUnit_Drill_is_Inch )
        fputs( "Selected Drill Unit: Imperial (\")\n\n", aFile );
    else
        fputs( "Selected Drill Unit: Metric (mm)\n\n", aFile );

    /* build hole lists:
     * 1 - through holes
     * 2 - for partial holes only: by layer pair
     * 3 - Not Plated through holes
     */

    for( ; ; )
    {
        Build_Holes_List( aPcb,
                          aHoleListBuffer,
                          aToolListBuffer,
                          layer1,
                          layer2,
                          gen_through_holes ? false : true, gen_NPTH_holes );

        TotalHoleCount = 0;

        if( gen_NPTH_holes )
        {
            sprintf( line, "Drill report for Not Plated through holes :\n" );
        }

        else if( gen_through_holes )
        {
            sprintf( line, "Drill report for through holes :\n" );
        }
        else
        {
            if( layer1 == LAYER_N_BACK )  // First partial hole list
            {
                sprintf( line, "Drill report for buried and blind vias :\n\n" );
                fputs( line, aFile );
            }

            sprintf( line, "Drill report for holes from layer %s to layer %s\n",
                     TO_UTF8( aPcb->GetLayerName( layer1 ) ),
                     TO_UTF8( aPcb->GetLayerName( layer2 ) ) );
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
                sprintf( line, "(%d holes)\n", aToolListBuffer[ii].m_TotalCount );
            else if( aToolListBuffer[ii].m_OvalCount == 1 )
                sprintf( line, "(%d holes)  (with 1 oblong)\n", aToolListBuffer[ii].m_TotalCount );
            else  //  if ( buffer[ii]m_OvalCount > 1 )
                sprintf( line, "(%d holes)  (with %d oblongs)\n",
                         aToolListBuffer[ii].m_TotalCount,
                         aToolListBuffer[ii].m_OvalCount );

            fputs( line, aFile );

            TotalHoleCount += aToolListBuffer[ii].m_TotalCount;
        }

        if( gen_NPTH_holes )
            sprintf( line, "\ntotal Not Plated holes count %d\n\n\n", TotalHoleCount );
        else
            sprintf( line, "\ntotal plated holes count %d\n\n\n", TotalHoleCount );

        fputs( line, aFile );

        if( gen_NPTH_holes )
        {
            break;
        }
        else
        {
            if( aPcb->GetCopperLayerCount() <= 2 )
            {
                gen_NPTH_holes = true;
                continue;
            }

            if(  gen_through_holes )
            {
                layer2 = layer1 + 1;
            }
            else
            {
                if( layer2 >= LAYER_N_FRONT )    // no more layer pair to consider
                {
                    gen_NPTH_holes = true;
                    continue;
                }

                layer1++; layer2++;           // use next layer pair

                if( layer2 == aPcb->GetCopperLayerCount() - 1 )
                    layer2 = LAYER_N_FRONT;  // the last layer is always the
                                           // component layer
            }
            gen_through_holes = false;
        }
    }

    fclose( aFile );
}

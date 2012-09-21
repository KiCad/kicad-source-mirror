/************************************************************************/
/* Functions to create drill data used to create files and report files */
/************************************************************************/

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <base_struct.h>
#include <colors.h>
#include <drawtxt.h>
#include <confirm.h>
#include <kicad_string.h>
#include <macros.h>

#include <class_board.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <gendrill.h>

/* Conversion utilities - these will be used often in there... */
static double diameter_in_inches(double ius)
{
    return ius * 0.001 / IU_PER_MILS;
}

static double diameter_in_mm(double ius)
{
    return ius / IU_PER_MM;
}


void GenDrillMapFile( BOARD* aPcb, FILE* aFile, const wxString& aFullFileName,
                      const PAGE_INFO& aSheet,
                      std::vector<HOLE_INFO> aHoleListBuffer,
                      std::vector<DRILL_TOOL> aToolListBuffer,
                      bool aUnit_Drill_is_Inch, int format,
                      const wxPoint& auxoffset )
{
    double      scale = 1.0;
    wxPoint     offset;
    PLOTTER*    plotter = NULL;

    PCB_PLOT_PARAMS plot_opts;    // starts plotting with default options

    LOCALE_IO   toggle;         // use standard C notation for float numbers

    // Calculate dimensions and center of PCB
    EDA_RECT bbbox = aPcb->ComputeBoundingBox(true);

    // Calculate the scale for the format type, scale 1 in HPGL, drawing on
    // an A4 sheet in PS, + text description of symbols
    switch( format )
    {
    case PLOT_FORMAT_GERBER:
        offset  = auxoffset;
        plotter = new GERBER_PLOTTER();
        plotter->SetViewport( offset, IU_PER_DECIMILS, scale, false );
        break;

    case PLOT_FORMAT_HPGL:  // Scale for HPGL format.
        {
            HPGL_PLOTTER* hpgl_plotter = new HPGL_PLOTTER;
            plotter = hpgl_plotter;
            hpgl_plotter->SetPenNumber( plot_opts.GetHPGLPenNum() );
            hpgl_plotter->SetPenSpeed( plot_opts.GetHPGLPenSpeed() );
            hpgl_plotter->SetPenOverlap( 0 );
            plotter->SetPageSettings( aSheet );
            plotter->SetViewport( offset, IU_PER_DECIMILS, scale, false );
        }
        break;

    case PLOT_FORMAT_POST:
        {
            PAGE_INFO   pageA4( wxT( "A4" ) );
            wxSize      pageSizeIU = pageA4.GetSizeIU();

            // Reserve a margin around the page.
            int margin = (int)(20 * IU_PER_MM );

            // Calculate a scaling factor to print the board on the sheet
            double Xscale = (double)( pageSizeIU.x - ( 2 * margin ) ) / bbbox.GetWidth();

            // We should print the list of drill sizes, so reserve room for it
            // 60% height for board 40% height for list
            int ypagesize_for_board = (int) (pageSizeIU.y * 0.6);
            double Yscale = (double)( ypagesize_for_board - margin ) / bbbox.GetHeight();

            scale = std::min( Xscale, Yscale );

            // Experience shows the scale should not to large, because texts
            // create problem (can be to big or too small).
            // So the scale is clipped at 3.0;
            scale = std::min( scale, 3.0 );

            offset.x  = (int) ( (double) bbbox.Centre().x - ( pageSizeIU.x / 2.0 ) / scale );
            offset.y  = (int) ( (double) bbbox.Centre().y -
                                ( ypagesize_for_board / 2.0 ) / scale );

            PS_PLOTTER* ps_plotter = new PS_PLOTTER;
            plotter = ps_plotter;
            ps_plotter->SetPageSettings( pageA4 );
            plotter->SetViewport( offset, IU_PER_DECIMILS, scale, false );
        }
        break;

    case PLOT_FORMAT_DXF:
        {
            DXF_PLOTTER* dxf_plotter = new DXF_PLOTTER;
            plotter = dxf_plotter;
            plotter->SetPageSettings( aSheet );
            plotter->SetViewport( offset, IU_PER_DECIMILS, scale, false );
        }
        break;

    case PLOT_FORMAT_SVG:
        {
            SVG_PLOTTER* svg_plotter = new SVG_PLOTTER;
            plotter = svg_plotter;
            plotter->SetPageSettings( aSheet );
            plotter->SetViewport( offset, IU_PER_DECIMILS, scale, false );
        }
        break;

    default:
        wxASSERT( false );
    }

    plotter->SetCreator( wxT( "PCBNEW" ) );
    plotter->SetFilename( aFullFileName );
    plotter->SetDefaultLineWidth( 10 * IU_PER_DECIMILS );
    plotter->StartPlot( aFile );

    // Draw items on edge layer (not all, only items useful for drill map
    BRDITEMS_PLOTTER itemplotter( plotter, aPcb, plot_opts );
    itemplotter.SetLayerMask( EDGE_LAYER );

    for( EDA_ITEM* PtStruct = aPcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_LINE_T:
            itemplotter.PlotDrawSegment( (DRAWSEGMENT*) PtStruct );
            break;

        case PCB_TEXT_T:
            itemplotter.PlotTextePcb( (TEXTE_PCB*) PtStruct );
            break;

        case PCB_DIMENSION_T:
        case PCB_TARGET_T:
        case PCB_MARKER_T:     // do not draw
        default:
            break;
        }
    }

    int         x, y;
    int         plotX, plotY, TextWidth;
    int         intervalle = 0;
    char        line[1024];
    wxString    msg;
    int textmarginaftersymbol = (int) (2 * IU_PER_MM);

    // Set Drill Symbols width
    plotter->SetDefaultLineWidth( 0.2 * IU_PER_MM / scale );
    plotter->SetCurrentLineWidth( -1 );

    // Plot board outlines and drill map
    Gen_Drill_PcbMap( aPcb, plotter, aHoleListBuffer, aToolListBuffer );

    // Print a list of symbols used.
    int charSize = 3 * IU_PER_MM;       // text size in IUs
    double charScale = 1.0/scale;       // real scale will be 1/scale,
                                        //because the global plot scale is scale
    TextWidth  = (int) ( (charSize * charScale) / 10 );   // Set text width (thickness)
    intervalle = (int) ( charSize * charScale ) + TextWidth;

    // Trace information.
    plotX = (int) ( (double) bbbox.GetX() + textmarginaftersymbol * charScale );
    plotY = bbbox.GetBottom() + intervalle;

    // Plot title  "Info"
    wxString Text = wxT( "Drill Map:" );
    plotter->Text( wxPoint( plotX, plotY ), UNSPECIFIED_COLOR, Text, 0,
                   wxSize( (int) ( charSize * charScale ),
                           (int) ( charSize * charScale ) ),
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                   TextWidth, false, false );

    for( unsigned ii = 0; ii < aToolListBuffer.size(); ii++ )
    {
        int plot_diam;

        if( aToolListBuffer[ii].m_TotalCount == 0 )
            continue;

        plotY += intervalle;

        plot_diam = (int) aToolListBuffer[ii].m_Diameter;
        x = (int) ( (double) plotX - textmarginaftersymbol * charScale
                - (double)plot_diam / 2.0 );
        y = (int) ( (double) plotY + (double) charSize * charScale );
        plotter->Marker( wxPoint( x, y ), plot_diam, ii );

        // Trace the legends.

        // List the diameter of each drill in the selected Drill Unit,
        // and then its diameter in the other Drill Unit.
        if( aUnit_Drill_is_Inch )
            sprintf( line, "%2.3f\" / %2.2fmm ",
                    diameter_in_inches( aToolListBuffer[ii].m_Diameter ),
                    diameter_in_mm( aToolListBuffer[ii].m_Diameter ) );
        else
            sprintf( line, "%2.2fmm / %2.3f\" ",
                    diameter_in_mm( aToolListBuffer[ii].m_Diameter ),
                    diameter_in_inches( aToolListBuffer[ii].m_Diameter ) );

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
                     aToolListBuffer[ii].m_TotalCount - aToolListBuffer[ii].m_OvalCount,
                     aToolListBuffer[ii].m_OvalCount );

        msg += FROM_UTF8( line );
        plotter->Text( wxPoint( plotX, y ), UNSPECIFIED_COLOR,
                       msg,
                       0, wxSize( (int) ( charSize * charScale ),
                       (int) ( charSize * charScale ) ),
                       GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                       TextWidth, false, false );

        intervalle = (int) ( charSize * charScale ) + TextWidth;
        intervalle = (int) ( intervalle * 1.2 );

        if( intervalle < (plot_diam + (1*IU_PER_MM/scale) + TextWidth) )
            intervalle = plot_diam + (1*IU_PER_MM/scale) + TextWidth;
    }

    plotter->EndPlot();
    delete plotter;
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

    // create the drill list
    if( aToolListBuffer.size() > PLOTTER::MARKER_COUNT )
    {
        DisplayInfoMessage( NULL,
                            _( " Drill map: Too many diameter values to draw one symbol per drill value\n"
"Plot will use circle shape for some drill values" ),
                            10 );
    }

    // Plot the drill map:
    for( unsigned ii = 0; ii < aHoleListBuffer.size(); ii++ )
    {
        pos = aHoleListBuffer[ii].m_Hole_Pos;

        /* Always plot the drill symbol (for slots identifies the needed
         * cutter!) */
        aPlotter->Marker( pos, aHoleListBuffer[ii].m_Hole_Diameter,
                          aHoleListBuffer[ii].m_Tool_Reference - 1 );

        if( aHoleListBuffer[ii].m_Hole_Shape != 0 )
        {
            wxSize oblong_size;
            oblong_size = aHoleListBuffer[ii].m_Hole_Size;
            aPlotter->FlashPadOval( pos, oblong_size,
                                    aHoleListBuffer[ii].m_Hole_Orient, LINE );
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
    fprintf( aFile, "Created on %s\n", TO_UTF8( DateAndTime() ) );

    // List which Drill Unit option had been selected for the associated
    // drill aFile.
    if( aUnit_Drill_is_Inch )
        fputs( "Selected Drill Unit: Imperial (inches)\n\n", aFile );
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
            sprintf( line, "Drill report for unplated through holes :\n" );
        }

        else if( gen_through_holes )
        {
            sprintf( line, "Drill report for plated through holes :\n" );
        }
        else
        {
            if( layer1 == LAYER_N_BACK )  // First partial hole list
            {
                sprintf( line, "Drill report for buried and blind vias :\n\n" );
                fputs( line, aFile );
            }

            sprintf( line, "Drill report for holes from layer %s to layer %s :\n",
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
            {
                sprintf( line, "T%d  %2.3f\"  %2.2fmm  ",
                         ii + 1,
                         diameter_in_inches( aToolListBuffer[ii].m_Diameter ),
                         diameter_in_mm( aToolListBuffer[ii].m_Diameter ) );
            }
            else
            {
                sprintf( line, "T%d  %2.2fmm  %2.3f\"  ",
                         ii + 1,
                         diameter_in_mm( aToolListBuffer[ii].m_Diameter ),
                         diameter_in_inches( aToolListBuffer[ii].m_Diameter ) );
            }

            fputs( line, aFile );

            // Now list how many holes and ovals are associated with each drill.
            if( ( aToolListBuffer[ii].m_TotalCount == 1 )
               && ( aToolListBuffer[ii].m_OvalCount == 0 ) )
                sprintf( line, "(1 hole)\n" );
            else if( aToolListBuffer[ii].m_TotalCount == 1 )
                sprintf( line, "(1 hole)  (with 1 slot)\n" );
            else if( aToolListBuffer[ii].m_OvalCount == 0 )
                sprintf( line, "(%d holes)\n", aToolListBuffer[ii].m_TotalCount );
            else if( aToolListBuffer[ii].m_OvalCount == 1 )
                sprintf( line, "(%d holes)  (with 1 slot)\n", aToolListBuffer[ii].m_TotalCount );
            else  //  if ( buffer[ii]m_OvalCount > 1 )
                sprintf( line, "(%d holes)  (with %d slots)\n",
                         aToolListBuffer[ii].m_TotalCount,
                         aToolListBuffer[ii].m_OvalCount );

            fputs( line, aFile );

            TotalHoleCount += aToolListBuffer[ii].m_TotalCount;
        }

        if( gen_NPTH_holes )
            sprintf( line, "\nTotal unplated holes count %d\n\n\n", TotalHoleCount );
        else
            sprintf( line, "\nTotal plated holes count %d\n\n\n", TotalHoleCount );

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

/**
 * @file gen_drill_report_files.cpp
 * @brief Functions to create report and map files for EXCELLON drill files.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <plotter.h>
#include <plotters_specific.h>
#include <eda_item.h>
#include <gr_text.h>
#include <confirm.h>
#include <kicad_string.h>
#include <locale_io.h>
#include <math/util.h>      // for KiROUND

#include <board.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <gendrill_file_writer_base.h>
#include <pcb_painter.h>

/* Conversion utilities - these will be used often in there... */
inline double diameter_in_inches( double ius )
{
    return ius * 0.001 / IU_PER_MILS;
}


inline double diameter_in_mm( double ius )
{
    return ius / IU_PER_MM;
}


// return a pen size to plot markers and having a readable shape
inline int getMarkerBestPenSize( int aMarkerDiameter )
{
    return aMarkerDiameter / 10;
}


bool GENDRILL_WRITER_BASE::genDrillMapFile( const wxString& aFullFileName, PLOT_FORMAT aFormat )
{
    // Remark:
    // Hole list must be created before calling this function, by buildHolesList(),
    // for the right holes set (PTH, NPTH, buried/blind vias ...)

    double    scale = 1.0;
    wxPoint   offset;
    PLOTTER*  plotter = NULL;
    PAGE_INFO dummy( PAGE_INFO::A4, false );
    int  bottom_limit = 0;        // Y coord limit of page. 0 mean do not use

    PCB_PLOT_PARAMS plot_opts; // starts plotting with default options

    LOCALE_IO toggle; // use standard C notation for float numbers

    const PAGE_INFO& page_info = m_pageInfo ? *m_pageInfo : dummy;

    // Calculate dimensions and center of PCB. The Edge_Cuts layer must be visible
    // to calculate the board edges bounding box
    LSET visibleLayers = m_pcb->GetVisibleLayers();
    m_pcb->SetVisibleLayers( visibleLayers | LSET( Edge_Cuts ) );
    EDA_RECT bbbox = m_pcb->GetBoardEdgesBoundingBox();
    m_pcb->SetVisibleLayers( visibleLayers );

    // Calculate the scale for the format type, scale 1 in HPGL, drawing on
    // an A4 sheet in PS, + text description of symbols
    switch( aFormat )
    {
    case PLOT_FORMAT::GERBER:
        offset  = GetOffset();
        plotter = new GERBER_PLOTTER();
        plotter->SetViewport( offset, IU_PER_MILS / 10, scale, false );
        plotter->SetGerberCoordinatesFormat( 5 ); // format x.5 unit = mm
        break;

    case PLOT_FORMAT::HPGL: // Scale for HPGL format.
    {
        HPGL_PLOTTER* hpgl_plotter = new HPGL_PLOTTER;
        plotter                    = hpgl_plotter;
        hpgl_plotter->SetPenNumber( plot_opts.GetHPGLPenNum() );
        hpgl_plotter->SetPenSpeed( plot_opts.GetHPGLPenSpeed() );
        plotter->SetPageSettings( page_info );
        plotter->SetViewport( offset, IU_PER_MILS / 10, scale, false );
    }
    break;


    default:
        wxASSERT( false );
        KI_FALLTHROUGH;

    case PLOT_FORMAT::PDF:
    case PLOT_FORMAT::POST:
    case PLOT_FORMAT::SVG:
    {
        PAGE_INFO pageA4( wxT( "A4" ) );
        wxSize    pageSizeIU = pageA4.GetSizeIU();

        // Reserve a 10 mm margin around the page.
        int margin = Millimeter2iu( 10 );

        // Calculate a scaling factor to print the board on the sheet
        double Xscale = double( pageSizeIU.x - ( 2 * margin ) ) / bbbox.GetWidth();

        // We should print the list of drill sizes, so reserve room for it
        // 60% height for board 40% height for list
        int    ypagesize_for_board = KiROUND( pageSizeIU.y * 0.6 );
        double Yscale              = double( ypagesize_for_board - margin ) / bbbox.GetHeight();

        scale = std::min( Xscale, Yscale );

        // Experience shows the scale should not to large, because texts
        // create problem (can be to big or too small).
        // So the scale is clipped at 3.0;
        scale = std::min( scale, 3.0 );

        offset.x = KiROUND( double( bbbox.Centre().x ) - ( pageSizeIU.x / 2.0 ) / scale );
        offset.y = KiROUND( double( bbbox.Centre().y ) - ( ypagesize_for_board / 2.0 ) / scale );

        // bottom_limit is used to plot the legend (drill diameters)
        // texts are scaled differently for scale > 1.0 and <= 1.0
        // so the limit is scaled differently.
        bottom_limit = ( pageSizeIU.y - margin ) / std::min( scale, 1.0 );

        if( aFormat == PLOT_FORMAT::SVG )
            plotter = new SVG_PLOTTER;
        else if( aFormat == PLOT_FORMAT::PDF )
            plotter = new PDF_PLOTTER;
        else
            plotter = new PS_PLOTTER;

        plotter->SetPageSettings( pageA4 );
        plotter->SetViewport( offset, IU_PER_MILS / 10, scale, false );
    }
    break;

    case PLOT_FORMAT::DXF:
    {
        DXF_PLOTTER* dxf_plotter = new DXF_PLOTTER;

        if( m_unitsMetric )
            dxf_plotter->SetUnits( DXF_UNITS::MILLIMETERS );
        else
            dxf_plotter->SetUnits( DXF_UNITS::INCHES );

        plotter = dxf_plotter;
        plotter->SetPageSettings( page_info );
        plotter->SetViewport( offset, IU_PER_MILS / 10, scale, false );
    }
    break;
    }

    plotter->SetCreator( wxT( "PCBNEW" ) );
    plotter->SetColorMode( false );

    KIGFX::PCB_RENDER_SETTINGS renderSettings;
    renderSettings.SetDefaultPenWidth( Millimeter2iu( 0.2 ) );

    plotter->SetRenderSettings( &renderSettings );

    if( !plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return false;
    }

    plotter->StartPlot();

    // Draw items on edge layer (not all, only items useful for drill map
    BRDITEMS_PLOTTER itemplotter( plotter, m_pcb, plot_opts );
    itemplotter.SetLayerSet( Edge_Cuts );

    for( auto PtStruct : m_pcb->Drawings() )
    {
        switch( PtStruct->Type() )
        {
        case PCB_SHAPE_T:
            itemplotter.PlotPcbShape( (PCB_SHAPE*) PtStruct );
            break;

        case PCB_TEXT_T:
            itemplotter.PlotPcbText( (PCB_TEXT*) PtStruct );
            break;

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
        case PCB_TARGET_T:
        case PCB_MARKER_T: // do not draw
        default:
            break;
        }
    }

    int      plotX, plotY, TextWidth;
    int      intervalle = 0;
    char     line[1024];
    wxString msg;
    int      textmarginaftersymbol = Millimeter2iu( 2 );

    // Set Drill Symbols width
    plotter->SetCurrentLineWidth( -1 );

    // Plot board outlines and drill map
    plotDrillMarks( plotter );

    // Print a list of symbols used.
    int    charSize = Millimeter2iu( 2 );  // text size in IUs
    // real char scale will be 1/scale, because the global plot scale is scale
    // for scale < 1.0 ( plot bigger actual size)
    // Therefore charScale = 1.0 / scale keep the initial charSize
    // (for scale < 1 we use the global scaling factor: the board must be plotted
    // smaller than the actual size)
    double charScale = std::min( 1.0, 1.0 / scale );

    TextWidth  = KiROUND( ( charSize * charScale ) / 10.0 ); // Set text width (thickness)
    intervalle = KiROUND( charSize * charScale ) + TextWidth;

    // Trace information.
    plotX = KiROUND( bbbox.GetX() + textmarginaftersymbol * charScale );
    plotY = bbbox.GetBottom() + intervalle;

    // Plot title  "Info"
    wxString Text = wxT( "Drill Map:" );
    plotter->Text( wxPoint( plotX, plotY ), COLOR4D::UNSPECIFIED, Text, 0,
            wxSize( KiROUND( charSize * charScale ), KiROUND( charSize * charScale ) ),
            GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, TextWidth, false, false );

    // For some formats (PS, PDF SVG) we plot the drill size list on more than one column
    // because the list must be contained inside the printed page
    // (others formats do not have a defined page size)
    int max_line_len = 0;   // The max line len in iu of the currently plotted column

    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        DRILL_TOOL& tool = m_toolListBuffer[ii];

        if( tool.m_TotalCount == 0 )
            continue;

        plotY += intervalle;

        // Ensure there are room to plot the line
        if( bottom_limit && ( plotY+intervalle > bottom_limit ) )
        {
            plotY = bbbox.GetBottom() + intervalle;
            plotX += max_line_len + Millimeter2iu( 10 );//column_width;
            max_line_len = 0;
        }

        int plot_diam = KiROUND( tool.m_Diameter );
        // For markers plotted with the comment, keep marker size <= text height
        plot_diam = std::min( plot_diam, KiROUND( charSize * charScale ) );
        int x = KiROUND( plotX - textmarginaftersymbol * charScale - plot_diam / 2.0 );
        int y = KiROUND( plotY + charSize * charScale );

        plotter->SetCurrentLineWidth( getMarkerBestPenSize( plot_diam ) );
        plotter->Marker( wxPoint( x, y ), plot_diam, ii );
        plotter->SetCurrentLineWidth( -1 );

        // List the diameter of each drill in mm and inches.
        sprintf( line, "%3.3fmm / %2.4f\" ", diameter_in_mm( tool.m_Diameter ),
                diameter_in_inches( tool.m_Diameter ) );

        msg = FROM_UTF8( line );

        // Now list how many holes and ovals are associated with each drill.
        if( ( tool.m_TotalCount == 1 ) && ( tool.m_OvalCount == 0 ) )
            sprintf( line, "(1 hole)" );
        else if( tool.m_TotalCount == 1 ) // && ( toolm_OvalCount == 1 )
            sprintf( line, "(1 slot)" );
        else if( tool.m_OvalCount == 0 )
            sprintf( line, "(%d holes)", tool.m_TotalCount );
        else if( tool.m_OvalCount == 1 )
            sprintf( line, "(%d holes + 1 slot)", tool.m_TotalCount - 1 );
        else // if ( toolm_OvalCount > 1 )
            sprintf( line, "(%d holes + %d slots)", tool.m_TotalCount - tool.m_OvalCount,
                    tool.m_OvalCount );

        msg += FROM_UTF8( line );

        if( tool.m_Hole_NotPlated )
            msg += wxT( " (not plated)" );

        plotter->Text( wxPoint( plotX, y ), COLOR4D::UNSPECIFIED, msg, 0,
                wxSize( KiROUND( charSize * charScale ), KiROUND( charSize * charScale ) ),
                GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER, TextWidth, false, false );

        intervalle = KiROUND( ( ( charSize * charScale ) + TextWidth ) * 1.2 );

        if( intervalle < ( plot_diam + ( 1 * IU_PER_MM / scale ) + TextWidth ) )
            intervalle = plot_diam + ( 1 * IU_PER_MM / scale ) + TextWidth;

        // Evaluate the text horizontal size, to know the maximal column size
        // This is a rough value, but ok to create a new column to plot next texts
        int text_len = msg.Len() * ( ( charSize * charScale ) + TextWidth );
        max_line_len = std::max( max_line_len, text_len + plot_diam );
    }

    plotter->EndPlot();
    delete plotter;

    return true;
}


bool GENDRILL_WRITER_BASE::GenDrillReportFile( const wxString& aFullFileName )
{
    FILE_OUTPUTFORMATTER    out( aFullFileName );

    static const char separator[] =
        "    =============================================================\n";

    wxASSERT( m_pcb );

    unsigned    totalHoleCount;
    wxFileName  brdFilename( m_pcb->GetFileName() );

    std::vector<DRILL_LAYER_PAIR> hole_sets = getUniqueLayerPairs();

    out.Print( 0, "Drill report for %s\n", TO_UTF8( brdFilename.GetFullName() ) );
    out.Print( 0, "Created on %s\n\n", TO_UTF8( DateAndTime() ) );

    // Output the cu layer stackup, so layer name references make sense.
    out.Print( 0, "Copper Layer Stackup:\n" );
    out.Print( 0, separator );

    LSET cu = m_pcb->GetEnabledLayers() & LSET::AllCuMask();

    int conventional_layer_num = 1;

    for( LSEQ seq = cu.Seq();  seq;  ++seq, ++conventional_layer_num )
    {
        out.Print( 0, "    L%-2d:  %-25s %s\n",
            conventional_layer_num,
            TO_UTF8( m_pcb->GetLayerName( *seq ) ),
            layerName( *seq ).c_str()       // generic layer name
            );
    }

    out.Print( 0, "\n\n" );

    /* output hole lists:
     * 1 - through holes
     * 2 - for partial holes only: by layer starting and ending pair
     * 3 - Non Plated through holes
     */

    bool buildNPTHlist = false;     // First pass: build PTH list only

    // in this loop are plated only:
    for( unsigned pair_ndx = 0; pair_ndx < hole_sets.size();  ++pair_ndx )
    {
        DRILL_LAYER_PAIR  pair = hole_sets[pair_ndx];

        buildHolesList( pair, buildNPTHlist );

        if( pair == DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
        {
            out.Print( 0, "Drill file '%s' contains\n",
                TO_UTF8( getDrillFileName( pair, false, m_merge_PTH_NPTH ) ) );

            out.Print( 0, "    plated through holes:\n" );
            out.Print( 0, separator );
            totalHoleCount = printToolSummary( out, false );
            out.Print( 0, "    Total plated holes count %u\n", totalHoleCount );
        }
        else    // blind/buried
        {
            out.Print( 0, "Drill file '%s' contains\n",
                TO_UTF8( getDrillFileName( pair, false, m_merge_PTH_NPTH ) ) );

            out.Print( 0, "    holes connecting layer pair: '%s and %s' (%s vias):\n",
                TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( pair.first ) ) ),
                TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( pair.second ) ) ),
                pair.first == F_Cu || pair.second == B_Cu ? "blind" : "buried"
                );

            out.Print( 0, separator );
            totalHoleCount = printToolSummary( out, false );
            out.Print( 0, "    Total plated holes count %u\n", totalHoleCount );
        }

        out.Print( 0, "\n\n" );
    }

    // NPTHoles. Generate the full list (pads+vias) if PTH and NPTH are merged,
    // or only the NPTH list (which never has vias)
    if( !m_merge_PTH_NPTH )
        buildNPTHlist = true;

    buildHolesList( DRILL_LAYER_PAIR( F_Cu, B_Cu ), buildNPTHlist );

    // nothing wrong with an empty NPTH file in report.
    if( m_merge_PTH_NPTH )
        out.Print( 0, "Not plated through holes are merged with plated holes\n" );
    else
        out.Print( 0, "Drill file '%s' contains\n",
                   TO_UTF8( getDrillFileName( DRILL_LAYER_PAIR( F_Cu, B_Cu ),
                   true, m_merge_PTH_NPTH ) ) );

    out.Print( 0, "    unplated through holes:\n" );
    out.Print( 0, separator );
    totalHoleCount = printToolSummary( out, true );
    out.Print( 0, "    Total unplated holes count %u\n", totalHoleCount );

    return true;
}


bool GENDRILL_WRITER_BASE::plotDrillMarks( PLOTTER* aPlotter )
{
    // Plot the drill map:
    wxPoint pos;

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        const HOLE_INFO& hole = m_holeListBuffer[ii];
        pos = hole.m_Hole_Pos;

        // Gives a good line thickness to have a good marker shape:
        aPlotter->SetCurrentLineWidth( getMarkerBestPenSize( hole.m_Hole_Diameter ) );

        // Always plot the drill symbol (for slots identifies the needed cutter!
        aPlotter->Marker( pos, hole.m_Hole_Diameter, hole.m_Tool_Reference - 1 );

        if( hole.m_Hole_Shape != 0 )
        {
            wxSize oblong_size = hole.m_Hole_Size;
            aPlotter->FlashPadOval( pos, oblong_size, hole.m_Hole_Orient, SKETCH, NULL );
        }
    }

    aPlotter->SetCurrentLineWidth( -1 );

    return true;
}


unsigned GENDRILL_WRITER_BASE::printToolSummary( OUTPUTFORMATTER& out, bool aSummaryNPTH ) const
{
    unsigned totalHoleCount = 0;

    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        const DRILL_TOOL& tool = m_toolListBuffer[ii];

        if( aSummaryNPTH && !tool.m_Hole_NotPlated )
            continue;

        if( !aSummaryNPTH && tool.m_Hole_NotPlated )
            continue;

        // List the tool number assigned to each drill,
        // in mm then in inches.
        int tool_number = ii+1;
        out.Print( 0, "    T%d  %2.2fmm  %2.3f\"  ", tool_number,
                 diameter_in_mm( tool.m_Diameter ),
                 diameter_in_inches( tool.m_Diameter ) );

        // Now list how many holes and ovals are associated with each drill.
        if( ( tool.m_TotalCount == 1 ) && ( tool.m_OvalCount == 0 ) )
            out.Print( 0, "(1 hole)\n" );
        else if( tool.m_TotalCount == 1 )
            out.Print( 0, "(1 hole)  (with 1 slot)\n" );
        else if( tool.m_OvalCount == 0 )
            out.Print( 0, "(%d holes)\n", tool.m_TotalCount );
        else if( tool.m_OvalCount == 1 )
            out.Print( 0, "(%d holes)  (with 1 slot)\n", tool.m_TotalCount );
        else // tool.m_OvalCount > 1
            out.Print( 0, "(%d holes)  (with %d slots)\n",
                     tool.m_TotalCount, tool.m_OvalCount );

        totalHoleCount += tool.m_TotalCount;
    }

    out.Print( 0, "\n" );

    return totalHoleCount;
}

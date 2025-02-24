/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <plotters/plotter_dxf.h>
#include <plotters/plotter_gerber.h>
#include <plotters/plotters_pslike.h>
#include <eda_item.h>
#include <font/font.h>
#include <confirm.h>
#include <richio.h>
#include <string_utils.h>
#include <locale_io.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND

#include <board.h>
#include <footprint.h>

#include <pcbplot.h>
#include <gendrill_file_writer_base.h>
#include <pcb_painter.h>
#include <pcb_shape.h>


/* Conversion utilities - these will be used often in there... */
inline double diameter_in_inches( double ius )
{
    return ius * 0.001 / pcbIUScale.IU_PER_MILS;
}


inline double diameter_in_mm( double ius )
{
    return ius / pcbIUScale.IU_PER_MM;
}


// return a pen size to plot markers and having a readable shape
// clamped to be >= MIN_SIZE_MM to avoid too small line width
static int getMarkerBestPenSize( int aMarkerDiameter )
{
    int bestsize = aMarkerDiameter / 10;

    const double MIN_SIZE_MM = 0.1;
    bestsize = std::max( bestsize, pcbIUScale.mmToIU( MIN_SIZE_MM ) );

    return bestsize;
}


// return a pen size to plot outlines for oval holes
inline int getSketchOvalBestPenSize()
{
    const double SKETCH_LINE_WIDTH_MM = 0.1;
    return pcbIUScale.mmToIU( SKETCH_LINE_WIDTH_MM );
}


// return a default pen size to plot items with no specific line thickness
inline int getDefaultPenSize()
{
    const double DEFAULT_LINE_WIDTH_MM = 0.2;
    return pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH_MM );
}


bool GENDRILL_WRITER_BASE::genDrillMapFile( const wxString& aFullFileName, PLOT_FORMAT aFormat )
{
    // Remark:
    // Hole list must be created before calling this function, by buildHolesList(),
    // for the right holes set (PTH, NPTH, buried/blind vias ...)

    double    scale = 1.0;
    VECTOR2I  offset = GetOffset();
    PLOTTER*  plotter = nullptr;
    PAGE_INFO dummy( PAGE_INFO::A4, false );
    int       bottom_limit = 0;        // Y coord limit of page. 0 mean do not use

    PCB_PLOT_PARAMS plot_opts; // starts plotting with default options

    LOCALE_IO toggle; // use standard C notation for float numbers

    const PAGE_INFO& page_info = m_pageInfo ? *m_pageInfo : dummy;

    // Calculate dimensions and center of PCB. The Edge_Cuts layer must be visible
    // to calculate the board edges bounding box
    LSET visibleLayers = m_pcb->GetVisibleLayers();
    m_pcb->SetVisibleLayers( visibleLayers | LSET( { Edge_Cuts } ) );
    BOX2I bbbox = m_pcb->GetBoardEdgesBoundingBox();
    m_pcb->SetVisibleLayers( visibleLayers );

    // Some formats cannot be used to generate a document like the map files
    // Currently HPGL (old format not very used)

    if( aFormat == PLOT_FORMAT::HPGL  )
        aFormat = PLOT_FORMAT::PDF;

    // Calculate the scale for the format type, scale 1 in HPGL, drawing on
    // an A4 sheet in PS, + text description of symbols
    switch( aFormat )
    {
    case PLOT_FORMAT::GERBER:
        plotter = new GERBER_PLOTTER();
        plotter->SetViewport( offset, pcbIUScale.IU_PER_MILS / 10, scale, false );
        plotter->SetGerberCoordinatesFormat( 5 ); // format x.5 unit = mm
        break;

    default:
        wxASSERT( false );
        KI_FALLTHROUGH;

    case PLOT_FORMAT::PDF:
    case PLOT_FORMAT::POST:
    case PLOT_FORMAT::SVG:
    {
        PAGE_INFO pageA4( wxT( "A4" ) );
        VECTOR2I  pageSizeIU = pageA4.GetSizeIU( pcbIUScale.IU_PER_MILS );

        // Reserve a 10 mm margin around the page.
        int margin = pcbIUScale.mmToIU( 10 );

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
        plotter->SetViewport( offset, pcbIUScale.IU_PER_MILS / 10, scale, false );
        break;
    }

    case PLOT_FORMAT::DXF:
    {
        DXF_PLOTTER* dxf_plotter = new DXF_PLOTTER;

        if( m_unitsMetric )
            dxf_plotter->SetUnits( DXF_UNITS::MILLIMETERS );
        else
            dxf_plotter->SetUnits( DXF_UNITS::INCHES );

        plotter = dxf_plotter;
        plotter->SetPageSettings( page_info );
        plotter->SetViewport( offset, pcbIUScale.IU_PER_MILS / 10, scale, false );
        break;
    }
    }

    plotter->SetCreator( wxT( "PCBNEW" ) );
    plotter->SetColorMode( false );

    KIGFX::PCB_RENDER_SETTINGS renderSettings;
    renderSettings.SetDefaultPenWidth( getDefaultPenSize() );

    plotter->SetRenderSettings( &renderSettings );

    if( !plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return false;
    }

    plotter->ClearHeaderLinesList();

    // For the Gerber X2 format we need to set the  "FileFunction" to Drillmap
    // and set a few other options.
    if( plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
    {
        GERBER_PLOTTER* gbrplotter = static_cast <GERBER_PLOTTER*> ( plotter );
        gbrplotter->DisableApertMacros( false );
        gbrplotter->UseX2format( true );            // Mandatory
        gbrplotter->UseX2NetAttributes( false );    // net attributes have no meaning here

        // Attributes are added using X2 format
        AddGerberX2Header( gbrplotter, m_pcb, false );

        wxString text;

        // Add the TF.FileFunction
        text = "%TF.FileFunction,Drillmap*%";
        gbrplotter->AddLineToHeader( text );

        // Add the TF.FilePolarity
        text = wxT( "%TF.FilePolarity,Positive*%" );
        gbrplotter->AddLineToHeader( text );
    }

    plotter->StartPlot( wxT( "1" ) );

    // Draw items on edge layer.
    // Not all, only items useful for drill map, i.e. board outlines.
    BRDITEMS_PLOTTER itemplotter( plotter, m_pcb, plot_opts );

    // Use attributes of a drawing layer (we are not really draw the Edge.Cuts layer)
    itemplotter.SetLayerSet( { Dwgs_User } );

    for( BOARD_ITEM* item : m_pcb->Drawings() )
    {
        if( item->GetLayer() != Edge_Cuts )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
            {
            PCB_SHAPE dummy_shape( *static_cast<PCB_SHAPE*>( item ) );
            dummy_shape.SetLayer( Dwgs_User );
            dummy_shape.SetParentGroup( nullptr );      // Remove group association, not needed for plotting
            itemplotter.PlotShape( &dummy_shape );
            }
            break;

        default:
            break;
        }
    }

    // Plot edge cuts in footprints
    for( const FOOTPRINT* footprint : m_pcb->Footprints() )
    {
        for( BOARD_ITEM* item : footprint->GraphicalItems() )
        {
            if( item-> GetLayer() != Edge_Cuts )
                continue;

            switch( item->Type() )
            {
            case PCB_SHAPE_T:
                {
                PCB_SHAPE dummy_shape( *static_cast<PCB_SHAPE*>( item ) );
                dummy_shape.SetLayer( Dwgs_User );
                dummy_shape.SetParentGroup( nullptr );      // Remove group association, not needed for plotting
                itemplotter.PlotShape( &dummy_shape );
                }
                break;

            default:
                break;
            }
        }
    }

    int      plotX, plotY, TextWidth;
    int      intervalle = 0;
    char     line[1024];
    wxString msg;
    int      textmarginaftersymbol = pcbIUScale.mmToIU( 2 );

    // Set Drill Symbols width
    plotter->SetCurrentLineWidth( -1 );

    // Plot board outlines and drill map
    plotDrillMarks( plotter );

    // Print a list of symbols used.
    int    charSize = pcbIUScale.mmToIU( 2 );  // text size in IUs

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

    TEXT_ATTRIBUTES attrs;
    attrs.m_StrokeWidth = TextWidth;
    attrs.m_Angle = ANGLE_HORIZONTAL;
    attrs.m_Size = VECTOR2I( KiROUND( charSize * charScale ), KiROUND( charSize * charScale ) );
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_CENTER;
    attrs.m_Multiline = false;

    plotter->PlotText( VECTOR2I( plotX, plotY ), COLOR4D::UNSPECIFIED, Text, attrs,
                       nullptr /* stroke font */, KIFONT::METRICS::Default() );

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
            plotX += max_line_len + pcbIUScale.mmToIU( 10 );//column_width;
            max_line_len = 0;
        }

        int plot_diam = KiROUND( tool.m_Diameter );

        // For markers plotted with the comment, keep marker size <= text height
        plot_diam = std::min( plot_diam, KiROUND( charSize * charScale ) );
        int x = KiROUND( plotX - textmarginaftersymbol * charScale - plot_diam / 2.0 );
        int y = KiROUND( plotY + charSize * charScale );

        plotter->SetCurrentLineWidth( getMarkerBestPenSize( plot_diam ) );
        plotter->Marker( VECTOR2I( x, y ), plot_diam, ii );
        plotter->SetCurrentLineWidth( -1 );

        // List the diameter of each drill in mm and inches.
        snprintf( line, sizeof(line), "%3.3fmm / %2.4f\" ", diameter_in_mm( tool.m_Diameter ),
                 diameter_in_inches( tool.m_Diameter ) );

        msg = From_UTF8( line );

        // Now list how many holes and ovals are associated with each drill.
        if( ( tool.m_TotalCount == 1 ) && ( tool.m_OvalCount == 0 ) )
            snprintf( line, sizeof(line), "(1 hole)" );
        else if( tool.m_TotalCount == 1 ) // && ( toolm_OvalCount == 1 )
            snprintf( line, sizeof(line), "(1 slot)" );
        else if( tool.m_OvalCount == 0 )
            snprintf( line, sizeof(line), "(%d holes)", tool.m_TotalCount );
        else if( tool.m_OvalCount == 1 )
            snprintf( line, sizeof(line), "(%d holes + 1 slot)", tool.m_TotalCount - 1 );
        else // if ( toolm_OvalCount > 1 )
            snprintf( line, sizeof(line), "(%d holes + %d slots)", tool.m_TotalCount - tool.m_OvalCount,
                     tool.m_OvalCount );

        msg += From_UTF8( line );

        if( tool.m_Hole_NotPlated )
            msg += wxT( " (not plated)" );

        plotter->PlotText( VECTOR2I( plotX, y ), COLOR4D::UNSPECIFIED, msg, attrs,
                           nullptr /* stroke font */, KIFONT::METRICS::Default() );

        intervalle = KiROUND( ( ( charSize * charScale ) + TextWidth ) * 1.2 );

        if( intervalle < ( plot_diam + ( 1 * pcbIUScale.IU_PER_MM / scale ) + TextWidth ) )
            intervalle = plot_diam + ( 1 * pcbIUScale.IU_PER_MM / scale ) + TextWidth;

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
    out.Print( 0, "Created on %s\n\n", TO_UTF8( GetISO8601CurrentDateTime() ) );

    // Output the cu layer stackup, so layer name references make sense.
    out.Print( 0, "Copper Layer Stackup:\n" );
    out.Print( 0, separator );

    LSET cu = m_pcb->GetEnabledLayers() & LSET::AllCuMask();

    int conventional_layer_num = 1;

    for( PCB_LAYER_ID layer : cu.Seq() )
    {
        out.Print( 0, "    L%-2d:  %-25s %s\n",
                   conventional_layer_num++,
                   TO_UTF8( m_pcb->GetLayerName( layer ) ),
                   layerName( layer ).c_str() );             // generic layer name
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
                       pair.first == F_Cu || pair.second == B_Cu ? "blind" : "buried" );

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
    VECTOR2I pos;

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
            aPlotter->SetCurrentLineWidth( getSketchOvalBestPenSize() );
            // FlashPadOval uses also the render settings default pen size, so we need
            // to initialize it:
            KIGFX::RENDER_SETTINGS* renderSettings = aPlotter->RenderSettings();
            int curr_default_pensize = renderSettings->GetDefaultPenWidth();
            renderSettings->SetDefaultPenWidth( getSketchOvalBestPenSize() );
            aPlotter->FlashPadOval( pos, hole.m_Hole_Size, hole.m_Hole_Orient, SKETCH, nullptr );
            renderSettings->SetDefaultPenWidth( curr_default_pensize );
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

        // List the tool number assigned to each drill in mm then in inches.
        int tool_number = ii+1;
        out.Print( 0, "    T%d  %2.3fmm  %2.4f\"  ", tool_number,
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
            out.Print( 0, "(%d holes)  (with %d slots)\n", tool.m_TotalCount, tool.m_OvalCount );

        totalHoleCount += tool.m_TotalCount;
    }

    out.Print( 0, "\n" );

    return totalHoleCount;
}

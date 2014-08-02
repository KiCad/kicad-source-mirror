/**
 * @file gen_drill_report_files.cpp
 * @brief Functions to create report and map files for EXCELLON drill files.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Jean_Pierre Charras <jp.charras at wanadoo.fr>
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <plot_common.h>
#include <base_struct.h>
#include <drawtxt.h>
#include <confirm.h>
#include <kicad_string.h>
#include <macros.h>

#include <class_board.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <gendrill_Excellon_writer.h>

/* Conversion utilities - these will be used often in there... */
inline double diameter_in_inches( double ius )
{
    return ius * 0.001 / IU_PER_MILS;
}


inline double diameter_in_mm( double ius )
{
    return ius / IU_PER_MM;
}


bool EXCELLON_WRITER::GenDrillMapFile( const wxString& aFullFileName,
                                       const PAGE_INFO& aSheet,
                                       PlotFormat aFormat )
{
    double          scale = 1.0;
    wxPoint         offset;
    PLOTTER*        plotter = NULL;

    PCB_PLOT_PARAMS plot_opts;  // starts plotting with default options

    LOCALE_IO       toggle;     // use standard C notation for float numbers

    // Calculate dimensions and center of PCB
    EDA_RECT        bbbox = m_pcb->ComputeBoundingBox( true );

    // Calculate the scale for the format type, scale 1 in HPGL, drawing on
    // an A4 sheet in PS, + text description of symbols
    switch( aFormat )
    {
    case PLOT_FORMAT_GERBER:
        offset  = GetOffset();
        plotter = new GERBER_PLOTTER();
        plotter->SetViewport( offset, IU_PER_DECIMILS, scale, false );
        plotter->SetGerberCoordinatesFormat( 5 );   // format x.5 unit = mm
        break;

    case PLOT_FORMAT_HPGL:    // Scale for HPGL format.
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


    default:
        wxASSERT( false );
        // fall through
    case PLOT_FORMAT_PDF:
    case PLOT_FORMAT_POST:
    {
        PAGE_INFO   pageA4( wxT( "A4" ) );
        wxSize      pageSizeIU = pageA4.GetSizeIU();

        // Reserve a margin around the page.
        int         margin = KiROUND( 20 * IU_PER_MM );

        // Calculate a scaling factor to print the board on the sheet
        double      Xscale = double( pageSizeIU.x - ( 2 * margin ) ) / bbbox.GetWidth();

        // We should print the list of drill sizes, so reserve room for it
        // 60% height for board 40% height for list
        int     ypagesize_for_board = KiROUND( pageSizeIU.y * 0.6 );
        double  Yscale = double( ypagesize_for_board - margin ) / bbbox.GetHeight();

        scale = std::min( Xscale, Yscale );

        // Experience shows the scale should not to large, because texts
        // create problem (can be to big or too small).
        // So the scale is clipped at 3.0;
        scale = std::min( scale, 3.0 );

        offset.x    = KiROUND( double( bbbox.Centre().x ) -
                               ( pageSizeIU.x / 2.0 ) / scale );
        offset.y    = KiROUND( double( bbbox.Centre().y ) -
                               ( ypagesize_for_board / 2.0 ) / scale );

        if( aFormat == PLOT_FORMAT_PDF )
            plotter = new PDF_PLOTTER;
        else
            plotter = new PS_PLOTTER;

        plotter->SetPageSettings( pageA4 );
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
    }

    plotter->SetCreator( wxT( "PCBNEW" ) );
    plotter->SetDefaultLineWidth( 5 * IU_PER_MILS );
    plotter->SetColorMode( false );

    if( ! plotter->OpenFile( aFullFileName ) )
    {
        delete plotter;
        return false;
    }

    plotter->StartPlot();

    // Draw items on edge layer (not all, only items useful for drill map
    BRDITEMS_PLOTTER itemplotter( plotter, m_pcb, plot_opts );
    itemplotter.SetLayerSet( Edge_Cuts );

    for( EDA_ITEM* PtStruct = m_pcb->m_Drawings; PtStruct != NULL; PtStruct = PtStruct->Next() )
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
    int         textmarginaftersymbol = KiROUND( 2 * IU_PER_MM );

    // Set Drill Symbols width
    plotter->SetDefaultLineWidth( 0.2 * IU_PER_MM / scale );
    plotter->SetCurrentLineWidth( -1 );

    // Plot board outlines and drill map
    PlotDrillMarks( plotter );

    // Print a list of symbols used.
    int     charSize    = 3 * IU_PER_MM;                    // text size in IUs
    double  charScale   = 1.0 / scale;                      // real scale will be 1/scale,
                                                            // because the global plot scale is scale
    TextWidth   = KiROUND( (charSize * charScale) / 10.0 );    // Set text width (thickness)
    intervalle  = KiROUND( charSize * charScale ) + TextWidth;

    // Trace information.
    plotX   = KiROUND( bbbox.GetX() + textmarginaftersymbol * charScale );
    plotY   = bbbox.GetBottom() + intervalle;

    // Plot title  "Info"
    wxString Text = wxT( "Drill Map:" );
    plotter->Text( wxPoint( plotX, plotY ), UNSPECIFIED_COLOR, Text, 0,
                   wxSize( KiROUND( charSize * charScale ),
                           KiROUND( charSize * charScale ) ),
                   GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                   TextWidth, false, false );

    for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
    {
        int plot_diam;

        if( m_toolListBuffer[ii].m_TotalCount == 0 )
            continue;

        plotY += intervalle;

        plot_diam = KiROUND( m_toolListBuffer[ii].m_Diameter );
        x = KiROUND( plotX - textmarginaftersymbol * charScale - plot_diam / 2.0 );
        y = KiROUND( plotY + charSize * charScale );
        plotter->Marker( wxPoint( x, y ), plot_diam, ii );

        // List the diameter of each drill in mm and inches.
        sprintf( line, "%2.2fmm / %2.3f\" ",
                 diameter_in_mm( m_toolListBuffer[ii].m_Diameter ),
                 diameter_in_inches( m_toolListBuffer[ii].m_Diameter ) );

        msg = FROM_UTF8( line );

        // Now list how many holes and ovals are associated with each drill.
        if( ( m_toolListBuffer[ii].m_TotalCount == 1 )
            && ( m_toolListBuffer[ii].m_OvalCount == 0 ) )
            sprintf( line, "(1 hole)" );
        else if( m_toolListBuffer[ii].m_TotalCount == 1 ) // && ( m_toolListBuffer[ii]m_OvalCount == 1 )
            sprintf( line, "(1 slot)" );
        else if( m_toolListBuffer[ii].m_OvalCount == 0 )
            sprintf( line, "(%d holes)", m_toolListBuffer[ii].m_TotalCount );
        else if( m_toolListBuffer[ii].m_OvalCount == 1 )
            sprintf( line, "(%d holes + 1 slot)", m_toolListBuffer[ii].m_TotalCount - 1 );
        else // if ( m_toolListBuffer[ii]m_OvalCount > 1 )
            sprintf( line, "(%d holes + %d slots)",
                     m_toolListBuffer[ii].m_TotalCount - m_toolListBuffer[ii].m_OvalCount,
                     m_toolListBuffer[ii].m_OvalCount );

        msg += FROM_UTF8( line );
        plotter->Text( wxPoint( plotX, y ), UNSPECIFIED_COLOR, msg, 0,
                       wxSize( KiROUND( charSize * charScale ),
                               KiROUND( charSize * charScale ) ),
                       GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                       TextWidth, false, false );

        intervalle  = KiROUND( (( charSize * charScale ) + TextWidth) * 1.2);

        if( intervalle < (plot_diam + (1 * IU_PER_MM / scale) + TextWidth) )
            intervalle = plot_diam + (1 * IU_PER_MM / scale) + TextWidth;
    }

    plotter->EndPlot();
    delete plotter;

    return true;
}


bool EXCELLON_WRITER::GenDrillReportFile( const wxString& aFullFileName )
{
    unsigned    totalHoleCount;
    char        line[1024];
    LAYER_NUM   layer1 = B_Cu;
    LAYER_NUM   layer2 = F_Cu;
    bool        gen_through_holes   = true;
    bool        gen_NPTH_holes      = false;

    m_file = wxFopen( aFullFileName, wxT( "w" ) );

    if( m_file == NULL )
        return false;

    wxString brdFilename = m_pcb->GetFileName();
    fprintf( m_file, "Drill report for %s\n", TO_UTF8( brdFilename ) );
    fprintf( m_file, "Created on %s\n", TO_UTF8( DateAndTime() ) );

    /* build hole lists:
     * 1 - through holes
     * 2 - for partial holes only: by layer pair
     * 3 - Not Plated through holes
     */

    for( ; ; )
    {
        BuildHolesList( layer1, layer2,
                        gen_through_holes ? false : true, gen_NPTH_holes, false);

        totalHoleCount = 0;

        if( gen_NPTH_holes )
            sprintf( line, "Drill report for unplated through holes :\n" );
        else if( gen_through_holes )
            sprintf( line, "Drill report for plated through holes :\n" );
        else
        {
            // If this is the first partial hole list: print a title
            if( layer1 == B_Cu )
                fputs( "Drill report for buried and blind vias :\n\n", m_file );

            sprintf( line, "Drill report for holes from layer %s to layer %s :\n",
                     TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( layer1 ) ) ),
                     TO_UTF8( m_pcb->GetLayerName( ToLAYER_ID( layer2 ) ) ) );
        }

        fputs( line, m_file );

        for( unsigned ii = 0; ii < m_toolListBuffer.size(); ii++ )
        {
            // List the tool number assigned to each drill,
            // in mm then in inches.
            sprintf( line, "T%d  %2.2fmm  %2.3f\"  ",
                     ii + 1,
                     diameter_in_mm( m_toolListBuffer[ii].m_Diameter ),
                     diameter_in_inches( m_toolListBuffer[ii].m_Diameter ) );

            fputs( line, m_file );

            // Now list how many holes and ovals are associated with each drill.
            if( ( m_toolListBuffer[ii].m_TotalCount == 1 )
                && ( m_toolListBuffer[ii].m_OvalCount == 0 ) )
                sprintf( line, "(1 hole)\n" );
            else if( m_toolListBuffer[ii].m_TotalCount == 1 )
                sprintf( line, "(1 hole)  (with 1 slot)\n" );
            else if( m_toolListBuffer[ii].m_OvalCount == 0 )
                sprintf( line, "(%d holes)\n", m_toolListBuffer[ii].m_TotalCount );
            else if( m_toolListBuffer[ii].m_OvalCount == 1 )
                sprintf( line, "(%d holes)  (with 1 slot)\n",
                         m_toolListBuffer[ii].m_TotalCount );
            else // if ( buffer[ii]m_OvalCount > 1 )
                sprintf( line, "(%d holes)  (with %d slots)\n",
                         m_toolListBuffer[ii].m_TotalCount,
                         m_toolListBuffer[ii].m_OvalCount );

            fputs( line, m_file );

            totalHoleCount += m_toolListBuffer[ii].m_TotalCount;
        }

        if( gen_NPTH_holes )
            sprintf( line, "\nTotal unplated holes count %d\n\n\n", totalHoleCount );
        else
            sprintf( line, "\nTotal plated holes count %d\n\n\n", totalHoleCount );

        fputs( line, m_file );

        if( gen_NPTH_holes )
        {
            break;
        }
        else
        {
            if( m_pcb->GetCopperLayerCount() <= 2 )
            {
                gen_NPTH_holes = true;
                continue;
            }

            if( gen_through_holes )
            {
                layer2 = layer1 + 1;
            }
            else
            {
                if( layer2 >= F_Cu )    // no more layer pair to consider
                {
                    gen_NPTH_holes = true;
                    continue;
                }

                ++layer1;
                ++layer2;           // use next layer pair

                if( layer2 == m_pcb->GetCopperLayerCount() - 1 )
                    layer2 = F_Cu; // the last layer is always the

                // component layer
            }

            gen_through_holes = false;
        }
    }

    fclose( m_file );

    return true;
}


bool EXCELLON_WRITER::PlotDrillMarks( PLOTTER* aPlotter )
{
    // Plot the drill map:
    wxPoint pos;

    for( unsigned ii = 0; ii < m_holeListBuffer.size(); ii++ )
    {
        pos = m_holeListBuffer[ii].m_Hole_Pos;

        // Always plot the drill symbol (for slots identifies the needed cutter!
        aPlotter->Marker( pos, m_holeListBuffer[ii].m_Hole_Diameter,
                          m_holeListBuffer[ii].m_Tool_Reference - 1 );

        if( m_holeListBuffer[ii].m_Hole_Shape != 0 )
        {
            wxSize oblong_size;
            oblong_size = m_holeListBuffer[ii].m_Hole_Size;
            aPlotter->FlashPadOval( pos, oblong_size,
                                    m_holeListBuffer[ii].m_Hole_Orient, LINE );
        }
    }

    return true;
}

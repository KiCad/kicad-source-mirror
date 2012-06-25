/**
 * @file plothpgl.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
 * Copyright (C) 2012 Dick Hollenbeck, dick@softplc.com
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
#include <wxBasePcbFrame.h>
#include <class_board.h>
#include <pcbnew.h>
#include <pcbplot.h>
#include <convert_to_biu.h>


bool PCB_BASE_FRAME::ExportToHpglFile( const wxString& aFullFileName, int aLayer,
                                       EDA_DRAW_MODE_T aTraceMode )
{
    wxSize      boardSize;
    wxPoint     boardCenter;
    bool        center = false;
    double      scale;
    wxPoint     offset;
    LOCALE_IO   toggle;
    FILE*       output_file = wxFopen( aFullFileName, wxT( "wt" ) );

    if( output_file == NULL )
    {
        return false;
    }

    PCB_PLOT_PARAMS plot_opts = GetPlotSettings();

    // Compute pen_dim (from m_HPGLPenDiam in mils) in pcb units,
    // with plot scale (if Scale is 2, pen diameter value is always m_HPGLPenDiam
    // so apparent pen diam is real pen diam / Scale
    int pen_diam = KiROUND( plot_opts.m_HPGLPenDiam * IU_PER_MILS /
                            plot_opts.m_PlotScale );

    // compute pen_overlay (from m_HPGLPenOvr in mils) in pcb units
    // with plot scale
    if( plot_opts.m_HPGLPenOvr < 0 )
        plot_opts.m_HPGLPenOvr = 0;

    if( plot_opts.m_HPGLPenOvr >= plot_opts.m_HPGLPenDiam )
        plot_opts.m_HPGLPenOvr = plot_opts.m_HPGLPenDiam - 1;

    int   pen_overlay = KiROUND( plot_opts.m_HPGLPenOvr * IU_PER_MILS /
                                 plot_opts.m_PlotScale );


    if( plot_opts.m_PlotScale != 1.0 || plot_opts.m_AutoScale )
    {
        // when scale != 1.0 we must calculate the position in page
        // because actual position has no meaning
        center = true;
    }

    wxSize pageSizeIU = GetPageSizeIU();

    // Calculate the center of the PCB
    EDA_RECT bbbox = GetBoardBoundingBox();

    boardSize   = bbbox.GetSize();
    boardCenter = bbbox.Centre();

    if( plot_opts.m_AutoScale )       // Optimum scale
    {
        // Fit to 80% of the page
        double Xscale = ( ( pageSizeIU.x * 0.8 ) / boardSize.x );
        double Yscale = ( ( pageSizeIU.y * 0.8 ) / boardSize.y );
        scale  = std::min( Xscale, Yscale );
    }
    else
    {
        scale = plot_opts.m_PlotScale;
    }

    // Calculate the page size offset.
    if( center )
    {
        offset.x = KiROUND( (double) boardCenter.x -
                            ( (double) pageSizeIU.x / 2.0 ) / scale );
        offset.y = KiROUND( (double) boardCenter.y -
                            ( (double) pageSizeIU.y / 2.0 ) / scale );
    }
    else
    {
        offset.x = 0;
        offset.y = 0;
    }

    HPGL_PLOTTER* plotter = new HPGL_PLOTTER();

    plotter->SetPageSettings( GetPageSettings() );

    // why did we have to change these settings above?
    SetPlotSettings( plot_opts );

    plotter->SetViewport( offset, IU_PER_DECIMILS, scale,
	                  plot_opts.m_PlotMirror );
    plotter->SetDefaultLineWidth( plot_opts.m_PlotLineWidth );
    plotter->SetCreator( wxT( "PCBNEW-HPGL" ) );
    plotter->SetFilename( aFullFileName );
    plotter->SetPenSpeed( plot_opts.m_HPGLPenSpeed );
    plotter->SetPenNumber( plot_opts.m_HPGLPenNum );
    plotter->SetPenOverlap( pen_overlay );
    plotter->SetPenDiameter( pen_diam );
    plotter->StartPlot( output_file );

    // The worksheet is not significant with scale!=1... It is with paperscale!=1, anyway
    if( plot_opts.m_PlotFrameRef && !center )
        PlotWorkSheet( plotter, GetScreen(), plot_opts.GetPlotLineWidth() );

    Plot_Layer( plotter, aLayer, aTraceMode );
    plotter->EndPlot();
    delete plotter;

    return true;
}

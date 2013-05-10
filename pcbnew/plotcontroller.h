/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Lorenzo Marcantonio, <l.marcantonio@logossrl.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcbnew/pcbplot.h
 */

#ifndef PLOTCONTROLLER_H_
#define PLOTCONTROLLER_H_

#include <pcb_plot_params.h>
#include <layers_id_colors_and_visibility.h>

class PLOTTER;
class BOARD;
class REPORTER;


/**
 * Batch plotter state object. Keeps the plot options and handles multiple
 * plot requests
 */
class PLOT_CONTROLLER
{
public:
    /** Batch plotter constructor, nothing interesting here */
    PLOT_CONTROLLER( BOARD *aBoard );

    /** Batch plotter destructor, ensures that the last plot is closed */
    ~PLOT_CONTROLLER();

    PCB_PLOT_PARAMS *AccessPlotOpts() { return &m_plotOpts; }
    bool IsPlotOpen() const { return m_plotter != NULL; }

    /** Close the current plot, nothing happens if it isn't open */
    void ClosePlot();

    /** Open a new plotfile; works as a factory for plotter objects
     */
    bool OpenPlotfile( const wxString &aSuffix, PlotFormat aFormat,
                       const wxString &aSheetDesc );

    /** Plot a single layer on the current plotfile */
    bool PlotLayer( LAYER_NUM layer );

    void SetColorMode( bool aColorMode );
    bool GetColorMode();

private:
    /// Option bank
    PCB_PLOT_PARAMS m_plotOpts;

    /// This is the plotter object; it starts NULL and become instantiated
    /// when a plotfile is requested
    PLOTTER* m_plotter;

    /// The board we're plotting
    BOARD* m_board;
};

#endif


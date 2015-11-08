/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Lorenzo Marcantonio, <l.marcantonio@logossrl.com>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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


/**
 * Batch plotter state object. Keeps the plot options and handles multiple
 * plot requests
 * Especially useful in Python scripts
 */
class PLOT_CONTROLLER
{
public:
    /** Batch plotter constructor, nothing interesting here */
    PLOT_CONTROLLER( BOARD *aBoard );

    /** Batch plotter destructor, ensures that the last plot is closed
     */
    ~PLOT_CONTROLLER();

    /**
     * Accessor to the plot parameters and options
     */
    PCB_PLOT_PARAMS& GetPlotOptions() { return m_plotOptions; }

    void SetLayer( LAYER_NUM aLayer ) { m_plotLayer = aLayer; }
    LAYER_NUM GetLayer() { return m_plotLayer; }


    /**
     * @return true if a plotter is initialized and can be used
     */
    bool IsPlotOpen() const { return m_plotter != NULL; }

    /** Close the current plot, nothing happens if it isn't open
     */
    void ClosePlot();

    /** Open a new plotfile; works as a factory for plotter objects
     * @param aSuffix is a string added to the base filename (derived from
     * the board filename) to identify the plot file
     * @param aFormat is the plot file format identifier
     * @param aSheetDesc
     */
    bool OpenPlotfile( const wxString &aSuffix, PlotFormat aFormat,
                       const wxString &aSheetDesc );

    /** Plot a single layer on the current plotfile
     * m_plotLayer is the layer to plot
     */
    bool PlotLayer();

    /**
     * @return the current plot full filename, set by OpenPlotfile
     */
    const wxString GetPlotFileName() { return m_plotFile.GetFullPath(); }

    /**
     * @return the current plot full filename, set by OpenPlotfile
     */
    const wxString GetPlotDirName() { return m_plotFile.GetPathWithSep(); }

    /**
     * Plotters can plot in Black and White mode or Color mode
     * SetColorMode activate/de-actiavte the Color mode.
     * @param aColorMode = true to activate the plot color mode
     */
    void SetColorMode( bool  );

    /**
     * @return  true if the current plot color mode is Color,
     *   false if the current plot color mode is Black and White
     */
    bool GetColorMode();

private:
    /// the layer to plot
    LAYER_NUM m_plotLayer;

    /// Option bank
    PCB_PLOT_PARAMS m_plotOptions;

    /// This is the plotter object; it starts NULL and become instantiated
    /// when a plotfile is requested
    PLOTTER* m_plotter;

    /// The board we're plotting
    BOARD* m_board;

    /// The current plot filename, set by OpenPlotfile
    wxFileName m_plotFile;
};

#endif


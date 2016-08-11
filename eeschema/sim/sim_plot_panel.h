/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __SIM_PLOT_PANEL_H
#define __SIM_PLOT_PANEL_H

#include "mgl2/wx.h"

class SIM_PLOT_PANEL;

class SIM_PLOT_PAINTER : public mglDraw
{
public:
    SIM_PLOT_PAINTER( const SIM_PLOT_PANEL* aParent )
        : m_parent( aParent ), m_lightColorIdx( 0 ), m_darkColorIdx( 0 )
    {
    }

    ~SIM_PLOT_PAINTER()
    {
    }

    //void Click() override;

    int Draw( mglGraph* aGraph ) override;

    enum COLOR_TYPE { LIGHT, DARK };

    ///> Generates a new, unique color for plotting curves
    wxString GenerateColor( COLOR_TYPE aType );

private:
    const SIM_PLOT_PANEL* m_parent;
    int m_lightColorIdx, m_darkColorIdx;
};


class SIM_PLOT_PANEL : public wxMathGL
{
public:
    SIM_PLOT_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr );

    ~SIM_PLOT_PANEL();

    struct TRACE
    {
        wxString spiceName, title, style;
        mglData x, y;
    };

    void AddTrace( const wxString& aSpiceName, const wxString& aTitle, int aPoints,
                    double* aT, double* aY, int aFlags );
    void DeleteTraces();

    const std::vector<TRACE>& GetTraces() const
    {
        return m_traces;
    }

    void ResetAxisRanges();

private:
    SIM_PLOT_PAINTER m_painter;

    // Traces to be plotted
    std::vector<TRACE> m_traces;

    // Axis ranges determined by the added traces data
    std::pair<double, double> m_axisRangeX, m_axisRangeY;

    friend class SIM_PLOT_PAINTER;
};

#endif

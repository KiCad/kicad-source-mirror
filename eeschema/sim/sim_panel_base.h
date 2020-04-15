/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Sylwester Kocjan <s.kocjan@o2.pl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SIM_PLOT_PANEL_BASE_H
#define __SIM_PLOT_PANEL_BASE_H

#include "sim_types.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>


class SIM_PANEL_BASE : public wxWindow
{
public:
    SIM_PANEL_BASE();
    SIM_PANEL_BASE( SIM_TYPE );
    SIM_PANEL_BASE( SIM_TYPE aType, wxWindow* parent, wxWindowID id,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = 0, const wxString& name = wxPanelNameStr );
    virtual ~SIM_PANEL_BASE();

    static bool IsPlottable( SIM_TYPE aSimType );

    SIM_TYPE GetType() const
    {
        return m_type;
    }

private:
    const SIM_TYPE m_type;
};


class SIM_NOPLOT_PANEL : public SIM_PANEL_BASE
{
public:
    SIM_NOPLOT_PANEL( SIM_TYPE aType, wxWindow* parent, wxWindowID id,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = 0, const wxString& name = wxPanelNameStr );

    virtual ~SIM_NOPLOT_PANEL();

private:
    wxSizer*      m_sizer;
    wxStaticText* m_textInfo;
};


#endif

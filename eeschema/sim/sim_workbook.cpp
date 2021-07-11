/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sim/sim_workbook.h>


SIM_WORKBOOK::SIM_WORKBOOK() : wxAuiNotebook()
{
    m_modified = false;
}


SIM_WORKBOOK::SIM_WORKBOOK( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize&
        aSize, long aStyle ) : wxAuiNotebook( aParent, aId, aPos, aSize, aStyle )
{
    m_modified = false;
}


bool SIM_WORKBOOK::AddPage( wxWindow* page, const wxString& caption, bool select, const wxBitmap& bitmap )
{
    bool res = wxAuiNotebook::AddPage( page, caption, select, bitmap );
    setModified( res );
    return res;
}


bool SIM_WORKBOOK::AddPage( wxWindow* page, const wxString& text, bool select, int imageId )
{
    bool res = wxAuiNotebook::AddPage( page, text, select, imageId );
    setModified( res );
    return res;
}


bool SIM_WORKBOOK::DeleteAllPages()
{
    bool res = wxAuiNotebook::DeleteAllPages();
    setModified( res );
    return res;
}


bool SIM_WORKBOOK::DeletePage( size_t page )
{
    bool res = wxAuiNotebook::DeletePage( page );
    setModified( res );
    return res;
}


bool SIM_WORKBOOK::AddTrace( SIM_PLOT_PANEL* aPlotPanel, const wxString& aName, int aPoints, const
        double* aX, const double* aY, SIM_PLOT_TYPE aType, const wxString& aParam )
{
    bool res = aPlotPanel->addTrace( aName, aPoints, aX, aY, aType, aParam );
    setModified( res );
    return res;
}


bool SIM_WORKBOOK::DeleteTrace( SIM_PLOT_PANEL* aPlotPanel, const wxString& aName )
{
    bool res = aPlotPanel->deleteTrace( aName );
    setModified( res );
    return res;
}

void SIM_WORKBOOK::ClrModified()
{
    m_modified = false;
    wxPostEvent( GetParent(), wxCommandEvent( EVT_WORKBOOK_CLR_MODIFIED ) );
}

void SIM_WORKBOOK::setModified( bool value )
{
    m_modified = value;
    wxPostEvent( GetParent(), wxCommandEvent( EVT_WORKBOOK_MODIFIED ) );
}

wxDEFINE_EVENT( EVT_WORKBOOK_MODIFIED, wxCommandEvent );
wxDEFINE_EVENT( EVT_WORKBOOK_CLR_MODIFIED, wxCommandEvent );

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Mikołaj Wielgus <wielgusmikolaj@gmail.com>
 * Copyright (C) 2021-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "widgets/sim_notebook.h"


SIM_NOTEBOOK::SIM_NOTEBOOK() :
        wxAuiNotebook()
{ }


SIM_NOTEBOOK::SIM_NOTEBOOK( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos, const wxSize&
                            aSize, long aStyle ) :
        wxAuiNotebook( aParent, aId, aPos, aSize, aStyle )
{ }


bool SIM_NOTEBOOK::AddPage( wxWindow* page, const wxString& caption, bool select, const wxBitmap& bitmap )
{
    if( wxAuiNotebook::AddPage( page, caption, select, bitmap ) )
    {
        wxPostEvent( GetParent(), wxCommandEvent( EVT_WORKBOOK_MODIFIED ) );
        return true;
    }

    return false;
}


bool SIM_NOTEBOOK::AddPage( wxWindow* page, const wxString& text, bool select, int imageId )
{
    if(  wxAuiNotebook::AddPage( page, text, select, imageId ) )
    {
        wxPostEvent( GetParent(), wxCommandEvent( EVT_WORKBOOK_MODIFIED ) );
        return true;
    }

    return false;
}


bool SIM_NOTEBOOK::DeleteAllPages()
{
    if( wxAuiNotebook::DeleteAllPages() )
    {
        wxPostEvent( GetParent(), wxCommandEvent( EVT_WORKBOOK_MODIFIED ) );
        return true;
    }

    return false;
}


bool SIM_NOTEBOOK::DeletePage( size_t page )
{
    if( wxAuiNotebook::DeletePage( page ) )
    {
        wxPostEvent( GetParent(), wxCommandEvent( EVT_WORKBOOK_MODIFIED ) );
        return true;
    }

    return false;
}


wxDEFINE_EVENT( EVT_WORKBOOK_MODIFIED, wxCommandEvent );

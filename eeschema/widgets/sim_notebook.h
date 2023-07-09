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

#ifndef SIM_NOTEBOOK_H
#define SIM_NOTEBOOK_H

#include "dialogs/dialog_sim_command.h"
#include "sim/sim_tab.h"
#include "sim/sim_plot_tab.h"


class SIM_NOTEBOOK : public wxAuiNotebook
{
public:
    SIM_NOTEBOOK();
    SIM_NOTEBOOK( wxWindow* aParent, wxWindowID aId = wxID_ANY,
                  const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                  long aStyle = wxAUI_NB_DEFAULT_STYLE );

    // Methods from wxAuiNotebook
    
    bool AddPage( wxWindow* aPage, const wxString& aCaption, bool aSelect=false, 
                  const wxBitmap& aBitmap = wxNullBitmap );
    bool AddPage( wxWindow* aPage, const wxString& aText, bool aSelect, int aImageId ) override;

    bool DeleteAllPages() override; 
    bool DeletePage( size_t aPage ) override;
};

wxDECLARE_EVENT( EVT_WORKBOOK_MODIFIED, wxCommandEvent );

#endif // SIM_NOTEBOOK_H

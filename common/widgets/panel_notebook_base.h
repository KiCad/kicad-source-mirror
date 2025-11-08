/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/dialog.h>
#include <wx/panel.h>
#include <widgets/wx_grid.h>


class PANEL_NOTEBOOK_BASE : public wxPanel
{
public:
    PANEL_NOTEBOOK_BASE( wxWindow* parent, wxWindowID id = wxID_ANY,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxSize( -1, -1 ), long style = wxTAB_TRAVERSAL,
                         const wxString& name = wxEmptyString ) :
            wxPanel( parent, id, pos, size, style, name )
    { }

    void SetProjectTied( bool aYes ) { m_projectTied = aYes; }
    bool GetProjectTied() { return m_projectTied; }

    void SetClosable( bool aYes ) { m_closable = aYes; }
    bool GetClosable() const { return m_closable; }

    virtual bool GetCanClose() { return true; }

private:
    bool m_closable = false;
    bool m_projectTied = false;
};
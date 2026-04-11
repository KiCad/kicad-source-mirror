/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GRID_READONLY_TEXT_HELPERS_H
#define GRID_READONLY_TEXT_HELPERS_H

#include <wx/textctrl.h>
#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>


class GRID_CELL_READONLY_TEXT_EDITOR : public wxGridCellTextEditor
{
public:
    GRID_CELL_READONLY_TEXT_EDITOR() {};

    void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler ) override
    {
        DoCreate( parent, id, evtHandler );
        Text()->SetEditable( false );
    }

wxDECLARE_NO_COPY_CLASS( GRID_CELL_READONLY_TEXT_EDITOR );
};

#endif  // GRID_READONLY_TEXT_HELPERS_H

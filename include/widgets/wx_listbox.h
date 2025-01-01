/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_WX_LISTBOX_H
#define KICAD_WX_LISTBOX_H

#include <wx/listbox.h>


class WX_LISTBOX : public wxListBox
{
public:
    WX_LISTBOX( wxWindow *parent, wxWindowID winid, const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize, int n = 0, const wxString choices[] = NULL,
                long style = 0 ) :
            wxListBox( parent, winid, pos, size, n, choices, style )
    { }

    wxString GetStringSelection() const override;
    bool SetStringSelection( const wxString& s ) override;
    bool SetStringSelection( const wxString& s, bool select ) override;

    wxString GetBaseString( int n ) const;
    int FindString( const wxString& s, bool bCase = false ) const override;
};

#endif //KICAD_WX_LISTBOX_H

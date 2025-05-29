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

#include <widgets/wx_bitmap_combobox.h>
#include <wx/textctrl.h>

WX_BITMAP_COMBOBOX::WX_BITMAP_COMBOBOX( wxWindow* parent, wxWindowID id, const wxString& value,
                                        const wxPoint& pos, const wxSize& size, int n,
                                        const wxString choices[], long style,
                                        const wxValidator& validator, const wxString& name ) :
        wxBitmapComboBox( parent, id, value, pos, size, n, choices, style, validator, name )
{
}


wxSize WX_BITMAP_COMBOBOX::DoGetBestSize() const
{
    wxSize size = wxBitmapComboBox::DoGetBestSize();

#ifdef __WXGTK__
    // wxWidgets has a bug on GTK where the wxBitmapComboBox doesn't scale correctly with fontsize.
    // The following hack is incomplete, but gets around the worst of it.  The hack can be removed
    // once https://github.com/wxWidgets/wxWidgets/issues/25468 is fixed.
    wxTextCtrl dummyCtrl( m_parent, wxID_ANY );
    int        dummyWidth = 100;

    size.y = std::max( size.y, dummyCtrl.GetBestHeight( dummyWidth ) + 4 );
#endif

    return size;
}



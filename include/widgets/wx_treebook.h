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

#ifndef WX_TREEBOOK_H
#define WX_TREEBOOK_H

#include <functional>
#include <wx/treebook.h>

class WX_TREEBOOK : public wxTreebook
{
public:
    WX_TREEBOOK( wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize, long style = wxBK_DEFAULT,
                 const wxString& name = wxEmptyString );

    bool AddLazyPage( std::function<wxWindow*( wxWindow* aParent )> aLazyCtor,
                      const wxString& text, bool bSelect = false, int imageId = NO_IMAGE );

    bool AddLazySubPage( std::function<wxWindow*( wxWindow* aParent )> aLazyCtor,
                         const wxString& text, bool bSelect = false, int imageId = NO_IMAGE );

    wxWindow* ResolvePage( size_t aPage );
};


#endif // WX_TREEBOOK_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
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

#ifndef DIALOG_ITEMS_LIST_H
#define DIALOG_ITEMS_LIST_H

#include <dialog_shim.h>
#include <wx/collpane.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#include <functional>
#include <vector>

class DIALOG_ITEMS_LIST : public DIALOG_SHIM
{
public:
    DIALOG_ITEMS_LIST( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage,
                       const wxString& aDetailsLabel );
    ~DIALOG_ITEMS_LIST();

    void AddItems( const std::vector<wxString>& aItems );
    void SetSelectionCallback( std::function<void(int)> aCallback );

private:
    void onSelectionChanged( wxListEvent& aEvent );
    void onCollapse( wxCollapsiblePaneEvent& aEvent );

    wxStaticText* m_message;
    wxCollapsiblePane* m_collapsiblePane;
    wxListCtrl* m_listCtrl;
    std::function<void(int)> m_callback;
};

#endif // DIALOG_ITEMS_LIST_H

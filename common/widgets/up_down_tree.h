/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <map>
#include <kicommon.h>
#include <wx/treectrl.h>


class KICOMMON_API UP_DOWN_TREE : public wxTreeCtrl
{
public:
    UP_DOWN_TREE( wxWindow *parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize, long style = wxTR_DEFAULT_STYLE ) :
            wxTreeCtrl( parent, id, pos, size, style )
    {
    }

    int OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 ) override;

    void MoveItemUp( const wxTreeItemId& aItem );
    void MoveItemDown( const wxTreeItemId& aItem );

private:
    void prepareForSort( const wxTreeItemId& aItem );

private:
    std::map<wxTreeItemId, int> m_sortMap;

    // Need to use wxRTTI macros in order for OnCompareItems to work properly
    // See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
    wxDECLARE_ABSTRACT_CLASS( UP_DOWN_TREE );
};



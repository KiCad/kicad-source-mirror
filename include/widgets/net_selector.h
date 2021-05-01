/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __WIDGET_NET_SELECTOR_H
#define __WIDGET_NET_SELECTOR_H

#include <wx/combo.h>


class BOARD;
class NETINFO_LIST;
class NET_SELECTOR_COMBOPOPUP;


wxDECLARE_EVENT( NET_SELECTED, wxCommandEvent );


class NET_SELECTOR : public wxComboCtrl
{
public:
    // Note: this list of arguments is here because it keeps us from having to customize
    // the constructor calls in wxFormBuilder.
    NET_SELECTOR( wxWindow *parent, wxWindowID id,
                  const wxPoint &pos = wxDefaultPosition, const wxSize &size = wxDefaultSize,
                  long style = 0 );

    ~NET_SELECTOR() override;

    void SetNetInfo( NETINFO_LIST* aNetInfoList );

    // Set to wxEmptyString to disallow indeterminate settings
    void SetIndeterminateString( const wxString& aString );

    void SetBoard( BOARD* aBoard );

    void SetSelectedNetcode( int aNetcode );
    void SetSelectedNet( const wxString& aNetname );
    void SetIndeterminate();

    bool IsIndeterminate();
    int GetSelectedNetcode();
    wxString GetSelectedNetname();

protected:
    void onKeyDown( wxKeyEvent& aEvt );

    NET_SELECTOR_COMBOPOPUP* m_netSelectorPopup;
    wxString                 m_indeterminateString;
};


#endif

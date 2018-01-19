/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <wx/wx.h>
#include <vector>

class BOARD;

class WIDGET_NET_SELECTOR : public wxComboBox
{
public:
    // Note: this list of arguments is here because WIDGET_NET_SELECTOR must
    // have the same arguments as wxComboBox to be used inside wxFormaBuilder
    WIDGET_NET_SELECTOR( wxWindow *parent, wxWindowID id,
                         const wxString &value=wxEmptyString,
                         const wxPoint &pos=wxDefaultPosition,
                         const wxSize &size=wxDefaultSize,
                         int n=0, const wxString choices[]=NULL,
                         long style=0, const wxValidator &validator=wxDefaultValidator,
                         const wxString &name=wxComboBoxNameStr);

    ~WIDGET_NET_SELECTOR();

    void SetMultiple( bool aMultiple = true );
    void SetSelectedNet ( int aNetcode );
    int GetSelectedNet();

    bool IsUniqueNetSelected() const;

    // Build the list of netnames and populate the wxComboBox
    void SetBoard( BOARD* aBoard );

private:
    struct NET {
        int m_Code;
        int m_Pos;
        wxString m_Name;

        bool operator <( const NET& aOther ) const
        {
            return m_Name < aOther.m_Name;
        }
    };

    bool m_multiple;
    std::vector<NET> m_nets;
};

#endif

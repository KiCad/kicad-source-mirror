/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __dialog_design_rules_aux_helper_class_h_
#define __dialog_design_rules_aux_helper_class_h_

#include <wx/listctrl.h>

/**
 * Class NETS_LIST_CTRL
 * is a helper to display lists of nets and associated netclasses
 * used in dialog design rules.
 * It's needed because the 2 "wxListCtl"s used to display lists of nets
 * uses the wxLC_VIRTUAL option. The method:
 *
 *   virtual wxString OnGetItemText( long item, long column ) const
 *
 * must be overloaded.
 */
class NETS_LIST_CTRL: public wxListCtrl
{
public:
    NETS_LIST_CTRL( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize, long style = wxLC_ICON ):
        wxListCtrl( parent, id, pos, size, style )
    {
    };

    void ClearList()
    {
        SetItemCount( 0 );
        m_Netnames.Clear();
        m_Classnames.Clear();
    }

    /**
     * Function OnGetItemText
     * is an overloaded method needed by wxListCtrl with wxLC_VIRTUAL options
     */
    virtual wxString OnGetItemText( long item, long column ) const;

    /**
     * Function SetRowItems
     * sets the net name and the net class name at @a aRow.
     * @param aRow = row index (if aRow > number of stored row, empty rows will be created)
     * @param aNetname = the string to display in row aRow, column 0
     * @param aNetclassName = the string to display in row aRow, column 1
     */
    void SetRowItems( unsigned aRow, const wxString& aNetname, const wxString& aNetclassName );

private:
    wxArrayString   m_Netnames;     ///< column 0: nets
    wxArrayString   m_Classnames;   ///< column 1: netclasses
};


#endif //__dialog_design_rules_aux_helper_class_h_

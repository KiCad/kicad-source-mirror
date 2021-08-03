/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef  EDA_LIST_DIALOG_H
#define  EDA_LIST_DIALOG_H


#include <../common/dialogs/eda_list_dialog_base.h>
#include <eda_units.h>


class EDA_DRAW_FRAME;

/**
 * A dialog which shows:
 *  - a list of elements for selection,
 *  - a text control to display help or info about the selected item.
 *  - 2 buttons (OK and Cancel)
 *
 */
class EDA_LIST_DIALOG : public EDA_LIST_DIALOG_BASE
{
public:

    /**
     * @param aParent Pointer to the parent window.
     * @param aTitle The title shown on top.
     * @param aItemHeaders an optional array containing the column header names for the dialog.
     * @param aItemList A wxArrayString of the list of elements.
     * @param aPreselectText An item name if an item must be preselected.
     */
    EDA_LIST_DIALOG( wxWindow* aParent, const wxString& aTitle, const wxArrayString& aItemHeaders,
                     const std::vector<wxArrayString>& aItemList,
                     const wxString& aPreselectText = wxEmptyString );

    void SetListLabel( const wxString& aLabel );
    void SetOKLabel( const wxString& aLabel );
    void HideFilter();

    void Append( const wxArrayString& aItemStr );
    void InsertItems( const std::vector<wxArrayString>& aItemList, int aPosition = 0 );

    /**
     * Return the selected text from \a aColumn in the wxListCtrl in the dialog.
     *
     * @param aColumn is the column to return the text from.
     * @return the selected text from \a aColumn.
     */
    wxString GetTextSelection( int aColumn = 0 );

    long GetSelection();

private:
    void onListItemActivated( wxListEvent& event ) override;
    void textChangeInFilterBox(wxCommandEvent& event) override;

    void initDialog( const wxArrayString& aItemHeaders, const wxString& aSelection);
    void sortList();

private:
    const std::vector<wxArrayString>* m_itemsList;
};


#endif    // EDA_LIST_DIALOG_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
#include <dialogs/eda_list_dialog_base.h>


class EDA_DRAW_FRAME;
class wxCheckBox;

/**
 * A dialog which shows:
 *  - a list of elements for selection,
 *  - a text control to display help or info about the selected item.
 *  - 2 buttons (OK and Cancel)
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
                     const std::vector<wxArrayString>& aItemList, const wxString& aPreselectText = wxEmptyString,
                     bool aSortList = true );

    EDA_LIST_DIALOG( wxWindow* aParent, const wxString& aTitle, bool aSortList = true );

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

    /**
     * Add a checkbox value to the dialog.
     */
    void AddExtraCheckbox( const wxString& aLabel, bool* aValuePtr );

    /**
     * Fills in the value pointers from the checkboxes after the dialog has run.
     */
    void GetExtraCheckboxValues();

    long GetSelection();

    bool Show( bool show ) override;

protected:
    void initDialog( const wxArrayString& aItemHeaders, const std::vector<wxArrayString>& aItemList,
                     const wxString& aPreselectText);

    void textChangeInFilterBox(wxCommandEvent& event) override;

private:
    void onSize( wxSizeEvent& event ) override;
    void onListItemActivated( wxListEvent& event ) override;

    void sortList();

private:
    // The list of items, locally stored
    std::vector<wxArrayString>   m_itemsList;
    bool                         m_sortList;
    std::map<wxCheckBox*, bool*> m_extraCheckboxMap;
};


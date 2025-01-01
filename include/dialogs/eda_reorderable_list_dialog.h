/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EDA_REORDERABLE_LIST_DIALOG_H
#define EDA_REORDERABLE_LIST_DIALOG_H

#include <eda_reorderable_list_dialog_base.h>

/**
 * A dialog which allows selecting a list of items from a list of available items, and reordering
 * those items.
 */
class EDA_REORDERABLE_LIST_DIALOG : public EDA_REORDERABLE_LIST_DIALOG_BASE
{
public:

    /**
     * @param aParent Pointer to the parent window.
     * @param aTitle The title shown on top.
     * @param aAllItems A list of elements.
     * @param aEnabledItems A list of elements that are already in the "enabled" category.
     */
    EDA_REORDERABLE_LIST_DIALOG( wxWindow* aParent, const wxString& aTitle,
                                 const std::vector<wxString>& aAllItems,
                                 const std::vector<wxString>& aEnabledItems );

    const std::vector<wxString>& EnabledList() { return m_enabledItems; }

protected:
    void onAddItem( wxCommandEvent& aEvent ) override;
    void onRemoveItem( wxCommandEvent& aEvent ) override;
    void onMoveUp( wxCommandEvent& aEvent ) override;
    void onMoveDown( wxCommandEvent& aEvent ) override;
    void onAvailableListItemSelected( wxListEvent& event ) override;
    void onEnabledListItemSelected( wxListEvent& event ) override;

private:
    void updateItems();

    bool getSelectedItem( wxListCtrl* aList, wxListItem& aInfo );

    std::vector<wxString> m_availableItems;
    std::vector<wxString> m_enabledItems;

    long m_selectedAvailable;
    long m_selectedEnabled;
};

#endif

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIALOG_UPDATE_FIELDS_H
#define DIALOG_UPDATE_FIELDS_H

#include "dialog_update_fields_base.h"

#include <set>
#include <list>

class SCH_COMPONENT;
class SCH_SCREEN;
class SCH_EDIT_FRAME;

using std::set;
using std::list;

/**
 * Dialog to update component fields (i.e. restore them from the original library symbols).
 */
class DIALOG_UPDATE_FIELDS : public DIALOG_UPDATE_FIELDS_BASE
{
public:
    DIALOG_UPDATE_FIELDS( SCH_EDIT_FRAME* aParent, const list<SCH_COMPONENT*>& aComponents,
            bool aCreateUndoEntry = true );

private:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    ///> Update fields for a single component
    void updateFields( SCH_COMPONENT* aComponent );

    ///> Selects or deselects all fields in the listbox widget
    void checkAll( bool aCheck );

    void onSelectAll( wxCommandEvent& event ) override
    {
        checkAll( true );
    }

    void onSelectNone( wxCommandEvent& event ) override
    {
        checkAll( false );
    }

    ///> Parent frame
    SCH_EDIT_FRAME* m_frame;

    ///> Set of field names that should have values updated
    set<wxString> m_fields;

    ///> Components to update
    list<SCH_COMPONENT*> m_components;

    ///> Flag indicating whether an undo buffer entry should be created
    bool m_createUndo;
};

#endif /* DIALOG_UPDATE_FIELDS_H */

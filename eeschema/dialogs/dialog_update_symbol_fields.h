/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_UPDATE_SYMBOL_FIELDS_H
#define DIALOG_UPDATE_SYMBOL_FIELDS_H

#include <dialog_update_symbol_fields_base.h>

class LIB_ID;
class LIB_PART;
class SYMBOL_EDIT_FRAME;

/**
 * Dialog to update or change schematic library symbols.
 */
class DIALOG_UPDATE_SYMBOL_FIELDS : public DIALOG_UPDATE_SYMBOL_FIELDS_BASE
{
public:
    DIALOG_UPDATE_SYMBOL_FIELDS( SYMBOL_EDIT_FRAME* aParent, LIB_PART* aPart );
    ~DIALOG_UPDATE_SYMBOL_FIELDS() override;

protected:
    void onOkButtonClicked( wxCommandEvent& aEvent ) override;

    void onSelectAll( wxCommandEvent& event ) override
    {
        checkAll( true );
    }

    void onSelectNone( wxCommandEvent& event ) override
    {
        checkAll( false );
    }

    ///< Select or deselect all fields in the listbox widget
    void checkAll( bool aCheck );

private:
    void updateFieldsList();

    SYMBOL_EDIT_FRAME* m_editFrame;
    LIB_PART*          m_symbol;

    ///< Set of field names that should have values updated
    std::set<wxString> m_updateFields;
};

#endif // DIALOG_UPDATE_SYMBOL_FIELDS_H

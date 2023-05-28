/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "grid_tricks.h"

class LIB_TABLE_GRID_TRICKS : public GRID_TRICKS
{
    enum
    {
        LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED = GRIDTRICKS_FIRST_CLIENT_ID,
        LIB_TABLE_GRID_TRICKS_DEACTIVATE_SELECTED,
        LIB_TABLE_GRID_TRICKS_LIBRARY_SETTINGS,
        LIB_TABLE_GRID_TRICKS_OPTIONS_EDITOR
    };

public:
    explicit LIB_TABLE_GRID_TRICKS( WX_GRID* aGrid );

    virtual ~LIB_TABLE_GRID_TRICKS(){};

    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override;
    void doPopupSelection( wxCommandEvent& event ) override;

protected:
    virtual void optionsEditor( int aRow ) = 0;
    bool handleDoubleClick( wxGridEvent& aEvent ) override;
};

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

#pragma once

class EDA_ITEM;
class SCH_SCREEN;
class SCH_SELECTION_TOOL;
class SCH_GLOBALLABEL;

/**
 * This is a bridge class to help the schematic be able to affect SCH_EDIT_FRAME
 * without doing anything too wild in terms of passing callbacks constantly in numerous files
 *
 * The long term goal would be to fix the internal structure and make the relationship between
 * frame and schematic less intertwined
 */
class SCHEMATIC_HOLDER
{
public:
    /**
     * Add an item to the screen (and view)
     * aScreen is the screen the item is located on, if not the current screen
     */
    virtual void AddToScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen = nullptr ) = 0;

    virtual SCH_SELECTION_TOOL* GetSelectionTool() { return nullptr; }

    virtual void RemoveFromScreen( EDA_ITEM* aItem, SCH_SCREEN* aScreen ) = 0;

    virtual void IntersheetRefUpdate( SCH_GLOBALLABEL* aItem ) {}
};
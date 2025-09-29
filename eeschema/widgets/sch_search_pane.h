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

#include <schematic.h>
#include <widgets/search_pane.h>

class SCH_EDIT_FRAME;

class SCH_SEARCH_PANE : public SEARCH_PANE, public SCHEMATIC_LISTENER
{
public:
    SCH_SEARCH_PANE( SCH_EDIT_FRAME* aFrame );
    virtual ~SCH_SEARCH_PANE();

    virtual void OnSchItemsAdded( SCHEMATIC& aBoard, std::vector<SCH_ITEM*>& aBoardItems ) override;
    virtual void OnSchItemsRemoved( SCHEMATIC& aBoard, std::vector<SCH_ITEM*>& aBoardItems ) override;
    virtual void OnSchItemsChanged( SCHEMATIC&  aBoard, std::vector<SCH_ITEM*>& aBoardItems ) override;

private:
    void onUnitsChanged( wxCommandEvent& event );
    void onSchChanging( wxCommandEvent& event );
    void onSchChanged( wxCommandEvent& event );

private:
    SCH_EDIT_FRAME* m_schFrame;
    SCHEMATIC*      m_sch;
};

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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


#ifndef SHEET_SYNCHRONIZATION_AGENT_H
#define SHEET_SYNCHRONIZATION_AGENT_H

#include <functional>
#include <sch_sheet_path.h>

class EDA_ITEM;
class SCH_HIERLABEL;
class SCH_SHEET_PIN;
class TOOL_MANAGER;
class SHEET_SYNCHRONIZATION_ITEM;
class SCH_SHEET;
class SCH_EDIT_FRAME;
class SCH_SHEET_PATH;
class SCH_ITEM;
enum class SHEET_SYNCHRONIZATION_ITEM_KIND;

/**
 * Agent for all the modifications while syncing the sheet pin and hierarchical label.
 */
class SHEET_SYNCHRONIZATION_AGENT
{
public:
    enum SHEET_SYNCHRONIZATION_PLACEMENT
    {
        PLACE_SHEET_PIN,
        PLACE_HIERLABEL
    };

    using DO_DELETE_ITEM = std::function<void( EDA_ITEM*, SCH_SHEET_PATH )>;

    using MODIFICATION = std::function<void()>;

    using DO_MODIFY_ITEM = std::function<void( EDA_ITEM*, SCH_SHEET_PATH, MODIFICATION const& )>;

    using DO_PLACE_ITEM =
            std::function<void( SCH_SHEET*, SCH_SHEET_PATH, SHEET_SYNCHRONIZATION_PLACEMENT,
                                std::set<EDA_ITEM*> const& )>;


    SHEET_SYNCHRONIZATION_AGENT( DO_MODIFY_ITEM aDoModify, DO_DELETE_ITEM aDoDelete,
                                 DO_PLACE_ITEM aPlaceItem, TOOL_MANAGER* aToolManager,
                                 SCH_EDIT_FRAME* a_frame );
    ~SHEET_SYNCHRONIZATION_AGENT();

    void ModifyItem( SHEET_SYNCHRONIZATION_ITEM& aItem, std::function<void()> const& aDoModify,
                     const SCH_SHEET_PATH& aPath );

    void ModifyItem( SCH_ITEM* aItem, std::function<void()> const& aDoModify,
                     const SCH_SHEET_PATH& aPath, SHEET_SYNCHRONIZATION_ITEM_KIND aKind );

    void RemoveItem( SHEET_SYNCHRONIZATION_ITEM& aItem, SCH_SHEET* aSheet,
                     SCH_SHEET_PATH const& aPath );

    void PlaceSheetPin( SCH_SHEET* aSheet, SCH_SHEET_PATH const& aPath,
                        std::set<EDA_ITEM*> const& aLabels );

    void PlaceHieraLable( SCH_SHEET* aSheet, SCH_SHEET_PATH const& aPath,
                          std::set<EDA_ITEM*> const& aPins );


private:
    DO_MODIFY_ITEM  m_doModify;
    DO_DELETE_ITEM  m_doDelete;
    DO_PLACE_ITEM   m_doPlaceItem;
};

#endif

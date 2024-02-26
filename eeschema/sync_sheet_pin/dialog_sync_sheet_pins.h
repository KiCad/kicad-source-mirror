/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_SYNC_SHEET_PINS_H
#define DIALOG_SYNC_SHEET_PINS_H

#include "dialog_sync_sheet_pins_base.h"

#include <sch_sheet_path.h>
#include <list>
#include <memory>
#include <unordered_map>

class SCH_SHEET;
class EDA_ITEM;
class SHEET_SYNCHRONIZATION_AGENT;
class PANEL_SYNC_SHEET_PINS;
class SCH_HIERLABEL;

class DIALOG_SYNC_SHEET_PINS : public DIALOG_SYNC_SHEET_PINS_BASE
{
public:
    enum class PlaceItemKind
    {
        UNDEFINED,
        SHEET_PIN,
        HIERLABEL
    };

    DIALOG_SYNC_SHEET_PINS( wxWindow* aParent, std::list<SCH_SHEET_PATH> aSheetPath,
                            std::shared_ptr<SHEET_SYNCHRONIZATION_AGENT> aAgent );

    ~DIALOG_SYNC_SHEET_PINS() override;

    void OnCloseBtnClick( wxCommandEvent& event ) override;

    void OnClose( wxCloseEvent& aEvent );

    void EndPlaceItem( EDA_ITEM* aNewItem );

    /**
     * Start place a new #SHEET_PIN/#HIERLABEL.
     *
     * @param aSheet The sheet instance
     * @param aKind   SHEET_PIN / HIERLABEL
     * @param aTemplate The template used for the new SHEET_PIN / HIERLABEL
     */
    void BeginPlaceItem( SCH_SHEET* aSheet, PlaceItemKind aKind, EDA_ITEM* aTemplate );

    /**
     * Get the Placement Template SHEET_PIN / HIERLABEL used for place a new #HIERLABEL/#SHEET_PIN.
     *
     * @return SCH_HIERLABEL*
     */
    SCH_HIERLABEL* GetPlacementTemplate() const;


private:
    //It's the agent that performs modification and placement
    std::shared_ptr<SHEET_SYNCHRONIZATION_AGENT>           m_agent;
    SCH_SHEET*                                             m_lastEditSheet;

    //The same sheet may have mutiple instances
    std::unordered_map<SCH_SHEET*, PANEL_SYNC_SHEET_PINS*> m_panels;
    PlaceItemKind                                          m_placeItemKind;
    EDA_ITEM*                                              m_placementTemplate;
};

#endif

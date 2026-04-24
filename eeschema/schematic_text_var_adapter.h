/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 */

#ifndef SCHEMATIC_TEXT_VAR_ADAPTER_H_
#define SCHEMATIC_TEXT_VAR_ADAPTER_H_

#include <schematic.h>
#include <text_var_dependency.h>

class SCH_ITEM;
class SCH_SYMBOL;


/**
 * Bridges SCHEMATIC's listener stream into the generic TEXT_VAR_TRACKER.
 *
 * Mirrors BOARD_TEXT_VAR_ADAPTER but for schematic items. A SCH_SYMBOL can
 * carry multiple instance-specific references (different refdes on different
 * SCH_SHEET_PATH instances); the adapter keys cross-ref source emission on
 * the symbol's reference on the schematic's *current* sheet path. This is a
 * known simplification — repeat-sheet schematics where U1 has different
 * refdes in each sheet instance will over-invalidate but not miss updates.
 * The proper fix is to extend TEXT_VAR_REF_KEY with an optional KIID_PATH
 * scope so cross-refs can be per-instance (codex review finding 2).
 */
class SCHEMATIC_TEXT_VAR_ADAPTER : public SCHEMATIC_LISTENER
{
public:
    explicit SCHEMATIC_TEXT_VAR_ADAPTER( SCHEMATIC& aSchematic );
    ~SCHEMATIC_TEXT_VAR_ADAPTER() override = default;

    TEXT_VAR_TRACKER&       Tracker()       { return m_tracker; }
    const TEXT_VAR_TRACKER& Tracker() const { return m_tracker; }

    void OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aItems ) override;
    void OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aItems ) override;
    void OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aItems ) override;

    /**
     * Walk every sheet in the hierarchy and register text-bearing items.
     * Invoked after bulk operations (file load, sheet rename) that may have
     * bypassed per-item notifications.
     */
    void RebuildIndex();

    /**
     * Return the keys @p aItem could source as a cross-reference target. For
     * a SCH_SYMBOL, these are `${REFDES:FIELD}` keys — one per field, using
     * the symbol's reference on the schematic's current sheet path.
     */
    std::vector<TEXT_VAR_REF_KEY> ExtractSourceKeys( EDA_ITEM* aItem ) const;

private:
    void registerItem( SCH_ITEM* aItem );
    void unregisterItem( SCH_ITEM* aItem );
    void handleItemChanged( SCH_ITEM* aItem );

    SCHEMATIC&       m_schematic;
    TEXT_VAR_TRACKER m_tracker;
};


#endif // SCHEMATIC_TEXT_VAR_ADAPTER_H_

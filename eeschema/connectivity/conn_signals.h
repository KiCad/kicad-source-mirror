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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "conn_bindings.h"
#include "conn_summary.h"

namespace SCH_CONNECTIVITY
{
struct SIGNAL_RESULT
{
    SUMMARY summary;
    // Driverless components have no base name; publication assigns the empty name and code zero.
    NAME_ID                 baseName = INVALID_ID;
    std::vector<ITEM_KEY>   items;
    std::vector<SLOT_KEY>   slots;
    std::optional<SLOT_KEY> nameSlot;
};

// Fold current signal inputs and retain canonical membership for publication and succession.
SIGNAL_RESULT DeriveSignal( const PARTITION& aPartition, const RECORD_STORE::RECORD_CACHE& aRecords,
                            const SLOT_STORE::SLOT_CACHE& aSlots, const SESSION_KEYS& aKeys );
} // namespace SCH_CONNECTIVITY

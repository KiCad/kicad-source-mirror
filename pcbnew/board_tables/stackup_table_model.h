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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef STACKUP_TABLE_MODEL_H
#define STACKUP_TABLE_MODEL_H

#include <vector>

#include <board_tables/generated_table_schema.h>
#include <pcb_generated_table.h>

class BOARD;


enum class STACKUP_TABLE_COLUMN_ID
{
    LAYER_NAME,
    TYPE,
    MATERIAL,
    THICKNESS,
    COLOR,
    EPSILON_R,
    LOSS_TANGENT
};


/**
 * Every column a stackup table can show, all of them by default. Lengths default to mm with
 * three decimals.
 */
const GENERATED_TABLE_SCHEMA& StackupTableSchema();


/**
 * One row per stackup layer, and per sublayer of a split dielectric, top to bottom.
 *
 * Reads a synchronized copy of the stackup so a stale stored stackup is reported as the board
 * now is without being rewritten. Keys are "L:<canonical layer name>" for board layers and
 * "D:<canonical name of the copper layer above>:<sublayer index>" for dielectrics. A layer
 * rename does not change the key of a row that still reports the same layer, so its
 * formatting stays with it.
 */
std::vector<GENERATED_TABLE_ROW> BuildStackupTableRows( const BOARD& aBoard,
                                                        const std::vector<GENERATED_TABLE_COLUMN>& aColumns,
                                                        GENERATED_TABLE_UNITS aUnits, int aPrecision );

#endif // STACKUP_TABLE_MODEL_H

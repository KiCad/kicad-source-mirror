/*                                                                                                                    
 * This program source code file is part of KiCad, a free EDA CAD application.                                      
 *                                                                                                                    
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.                                             
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

#ifndef DRC_RE_NUMERIC_CONSTRAINT_TYPES_H_
#define DRC_RE_NUMERIC_CONSTRAINT_TYPES_H_

#include "drc_re_numeric_input_constraint_data.h"


class DRC_RE_BASIC_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_basic_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 28, 48, 20, 1 } }; }
};


class DRC_RE_BOARD_OUTLINE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_board_outline_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 87, 107, 104, 1 } }; }
};


class DRC_RE_COPPER_TO_EDGE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_copper_to_edge_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 99, 119, 78, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_COPPER_TO_HOLE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_copper_to_hole_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};


class DRC_RE_COURTYARD_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 80, 100, 102, 1 } }; }
};


class DRC_RE_PHYSICAL_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 28, 48, 16, 1 } }; }
};


class DRC_RE_CREEPAGE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_creepage_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 80, 100, 58, 1 } }; }
};


class DRC_RE_HOLE_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 72, 92, 22, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_HOLE_TO_HOLE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_to_hole_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 24, 1 } }; }
};


class DRC_RE_HOLE_TO_HOLE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_to_hole_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 54, 74, 38, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MAXIMUM_ALLOWED_DEVIATION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_maximum_allowed_deviation; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};


class DRC_RE_MAXIMUM_VIA_COUNT_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_maximum_via_count; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 83, 103, 67, 1 } }; }
};


class DRC_RE_MINIMUM_ANGULAR_RING_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_angular_ring; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 45, 65, 35, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_ANNULAR_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_annular_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_CONNECTION_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_connection_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_ITEM_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_item_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};


class DRC_RE_MINIMUM_SOLDERMASK_SILVER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_soldermask_silver; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 80, 100, 47, 1 } }; }
};


class DRC_RE_MINIMUM_THERMAL_SPOKE_COUNT_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_thermal_relief_spoke_count; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 86, 106, 14, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_THROUGH_HOLE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_through_hole; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_TRACK_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_track_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_UVIA_DIAMETER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_uvia_diameter; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_UVIA_HOLE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_uvia_hole; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};

// TODO: Image of wrong size
class DRC_RE_MINIMUM_VIA_DIAMETER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_via_diameter; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};


class DRC_RE_SILK_TO_SILK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_silk_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 85, 105, 58, 1 } }; }
};


class DRC_RE_SILK_TO_SOLDERMASK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_soldermask_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 20, 40, 20, 1 } }; }
};


class DRC_RE_SOLDERMASK_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_soldermask_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 160, 180, 50, 1 } }; }
};


class DRC_RE_SOLDERPASTE_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_solderpaste_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 55, 75, 30, 1 } }; }
};


#endif

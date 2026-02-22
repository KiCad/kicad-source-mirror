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
#include "drc_re_overlay_types.h"


class DRC_RE_COPPER_TO_EDGE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_copper_to_edge_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 205, 235, 157, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_COPPER_TO_HOLE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_copper_to_hole_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 200, 230, 107, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_COURTYARD_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 160, 190, 202, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_PHYSICAL_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 160, 190, 202, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_CREEPAGE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_creepage_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 150, 180, 117, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_HOLE_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 72, 92, 22, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_HOLE_TO_HOLE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_to_hole_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 185, 215, 107, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_HOLE_TO_HOLE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_to_hole_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 102, 132, 77, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MAXIMUM_ALLOWED_DEVIATION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_maximum_allowed_deviation; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 45, 75, 47, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MAXIMUM_VIA_COUNT_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_maximum_via_count; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override { return { { 165, 205, 57, 1 } }; }
};


class DRC_RE_MINIMUM_ANGULAR_RING_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_angular_ring; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 95, 125, 77, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_ANNULAR_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_annular_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 207, 237, 67, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 205, 235, 107, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_CONNECTION_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_connection_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 195, 225, 75, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_SOLDERMASK_SILVER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_soldermask_silver; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 170, 210, 94, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_THERMAL_SPOKE_COUNT_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_thermal_relief_spoke_count; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    { return { { 189, 210, 32, 1 } }; }
};

class DRC_RE_MINIMUM_DRILL_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_drill_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 205, 235, 107, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_UVIA_DIAMETER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_uvia_diameter; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 185, 215, 107, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_UVIA_HOLE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_uvia_hole; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 183, 213, 107, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_VIA_DIAMETER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_via_diameter; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 186, 206, 105, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SILK_TO_SILK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_silk_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 232, 272, 92, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SILK_TO_SOLDERMASK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_soldermask_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 20, 40, 20, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SOLDERMASK_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_soldermask_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 345, 375, 47, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SOLDERPASTE_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_solderpaste_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 245, 275, 67, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


#endif

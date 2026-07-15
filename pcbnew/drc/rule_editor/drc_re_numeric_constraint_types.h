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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
        return { { 208, 263, 177, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_COPPER_TO_HOLE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_copper_to_hole_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 200, 255, 119, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_COURTYARD_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_courtyard_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 260, 315, 129, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_PHYSICAL_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 160, 215, 214, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_CREEPAGE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_creepage_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 120, 175, 131, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_HOLE_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 72, 127, 34, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_HOLE_TO_HOLE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_to_hole_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 102, 157, 47, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MAXIMUM_VIA_COUNT_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_maximum_via_count; }
    bool                               IsIntegerOnly() const override { return true; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 165, 220, 69, 1 } };
    }
};


class DRC_RE_MINIMUM_ANNULAR_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_annular_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 209, 264, 79, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 205, 260, 119, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_CONNECTION_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_connection_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 230, 285, 116, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_SOLDERMASK_SLIVER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_soldermask_sliver; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 200, 255, 302, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_THERMAL_SPOKE_COUNT_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_thermal_relief_spoke_count; }
    bool    IsIntegerOnly() const override { return true; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 150, 205, 44, 1 } };
    }
};

class DRC_RE_MINIMUM_DRILL_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_drill_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 209, 264, 119, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_VIA_DIAMETER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_via_diameter; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 186, 241, 117, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SILK_TO_SILK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_silk_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 170, 225, 37, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SILK_TO_SOLDERMASK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_soldermask_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 235, 290, 72, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SOLDERMASK_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_soldermask_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 348, 403, 59, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SOLDERPASTE_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_solderpaste_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 245, 300, 79, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


#endif

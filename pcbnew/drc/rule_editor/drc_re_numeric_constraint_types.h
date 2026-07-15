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
        return { { 208 + DRC_RE_OVERLAY_XO, 248 + DRC_RE_OVERLAY_XO, 177 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_COPPER_TO_HOLE_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_copper_to_hole_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 200 + DRC_RE_OVERLAY_XO, 240 + DRC_RE_OVERLAY_XO, 119 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_COURTYARD_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_courtyard_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 260 + DRC_RE_OVERLAY_XO, 300 + DRC_RE_OVERLAY_XO, 129 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_PHYSICAL_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 160 + DRC_RE_OVERLAY_XO, 200 + DRC_RE_OVERLAY_XO, 214 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_CREEPAGE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_creepage_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 120 + DRC_RE_OVERLAY_XO, 160 + DRC_RE_OVERLAY_XO, 131 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_HOLE_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 72 + DRC_RE_OVERLAY_XO, 112 + DRC_RE_OVERLAY_XO, 34 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_HOLE_TO_HOLE_DISTANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_hole_to_hole_distance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 102 + DRC_RE_OVERLAY_XO, 142 + DRC_RE_OVERLAY_XO, 47 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
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
        return { { 165 + DRC_RE_OVERLAY_XO, 205 + DRC_RE_OVERLAY_XO, 69 + DRC_RE_OVERLAY_YO, 1 } };
    }
};


class DRC_RE_MINIMUM_ANNULAR_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_annular_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 209 + DRC_RE_OVERLAY_XO, 249 + DRC_RE_OVERLAY_XO, 79 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 205 + DRC_RE_OVERLAY_XO, 245 + DRC_RE_OVERLAY_XO, 119 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};

class DRC_RE_MINIMUM_CONNECTION_WIDTH_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_connection_width; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 230 + DRC_RE_OVERLAY_XO, 270 + DRC_RE_OVERLAY_XO, 116 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_SOLDERMASK_SLIVER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_soldermask_sliver; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 200 + DRC_RE_OVERLAY_XO, 240 + DRC_RE_OVERLAY_XO, 302 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
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
        return { { 150 + DRC_RE_OVERLAY_XO, 190 + DRC_RE_OVERLAY_XO, 44 + DRC_RE_OVERLAY_YO, 1 } };
    }
};

class DRC_RE_MINIMUM_DRILL_SIZE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_drill_size; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 209 + DRC_RE_OVERLAY_XO, 249 + DRC_RE_OVERLAY_XO, 119 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_MINIMUM_VIA_DIAMETER_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_minimum_via_diameter; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 186 + DRC_RE_OVERLAY_XO, 226 + DRC_RE_OVERLAY_XO, 117 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SILK_TO_SILK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_silk_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 170 + DRC_RE_OVERLAY_XO, 210 + DRC_RE_OVERLAY_XO, 37 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SILK_TO_SOLDERMASK_CLEARANCE_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_silk_to_soldermask_clearance; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 235 + DRC_RE_OVERLAY_XO, 275 + DRC_RE_OVERLAY_XO, 72 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SOLDERMASK_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_soldermask_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 348 + DRC_RE_OVERLAY_XO, 388 + DRC_RE_OVERLAY_XO, 59 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


class DRC_RE_SOLDERPASTE_EXPANSION_CONSTRAINT_DATA : public DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA
{
public:
    using DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA::DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA;
    BITMAPS GetOverlayBitmap() const override { return BITMAPS::constraint_solderpaste_expansion; }
    std::vector<DRC_RE_FIELD_POSITION> GetFieldPositions() const override
    {
        return { { 245 + DRC_RE_OVERLAY_XO, 285 + DRC_RE_OVERLAY_XO, 79 + DRC_RE_OVERLAY_YO, 1, wxS( "mm" ), LABEL_POSITION::RIGHT } };
    }
};


#endif

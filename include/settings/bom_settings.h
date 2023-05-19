/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _BOM_SETTINGS_H
#define _BOM_SETTINGS_H


#include <settings/json_settings.h>
#include <settings/parameters.h>
#include <i18n_utility.h>

// A single field within a BOM, e.g. Reference, Value, Footprint
struct BOM_FIELD
{
    wxString name;
    wxString label;
    bool     show = false;
    bool     groupBy = false;

    bool operator==( const BOM_FIELD& rhs ) const;
};

bool operator!=( const BOM_FIELD& lhs, const BOM_FIELD& rhs );
bool operator<( const BOM_FIELD& lhs, const BOM_FIELD& rhs );

void to_json( nlohmann::json& j, const BOM_FIELD& f );
void from_json( const nlohmann::json& j, BOM_FIELD& f );


// A complete preset defining a BOM "View" with a list of all the fields to show,
// group by, order, filtering settings, etc.
struct BOM_PRESET
{
    wxString               name;
    bool                   readOnly = false;
    std::vector<BOM_FIELD> fieldsOrdered;
    wxString               sortField;
    bool                   sortAsc = true;
    wxString               filterString;
    bool                   groupSymbols = false;
    bool                   excludeDNP = false;

    bool operator==( const BOM_PRESET& rhs ) const;

    static BOM_PRESET GroupedByValue();
    static BOM_PRESET GroupedByValueFootprint();
};

bool operator!=( const BOM_PRESET& lhs, const BOM_PRESET& rhs );
bool operator<( const BOM_PRESET& lhs, const BOM_PRESET& rhs );

void to_json( nlohmann::json& j, const BOM_PRESET& f );
void from_json( const nlohmann::json& j, BOM_PRESET& f );


// A formatting preset, like CSV (Comma Separated Values)
struct BOM_FMT_PRESET
{
    wxString name;
    bool     readOnly = false;
    wxString fieldDelimiter;
    wxString stringDelimiter;
    wxString refDelimiter;
    wxString refRangeDelimiter;
    bool     keepTabs = false;
    bool     keepLineBreaks = false;

    bool operator==( const BOM_FMT_PRESET& rhs ) const;

    static BOM_FMT_PRESET CSV();
    static BOM_FMT_PRESET TSV();
    static BOM_FMT_PRESET Semicolons();
};

bool operator!=( const BOM_FMT_PRESET& lhs, const BOM_FMT_PRESET& rhs );
bool operator<( const BOM_FMT_PRESET& lhs, const BOM_FMT_PRESET& rhs );

void to_json( nlohmann::json& j, const BOM_FMT_PRESET& f );
void from_json( const nlohmann::json& j, BOM_FMT_PRESET& f );


#endif

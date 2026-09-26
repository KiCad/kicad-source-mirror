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

#ifndef DRILL_CHART_TEMPLATE_H
#define DRILL_CHART_TEMPLATE_H

#include <optional>
#include <vector>

#include <board_tables/generated_table_schema.h>
#include <math/vector2d.h>
#include <wx/string.h>


enum class DRILL_CHART_COLUMN_ID
{
    SYMBOL,
    DRILL_DIAMETER,
    SLOT_SIZE,
    PLATING,
    OPERATION_COUNT,
    SITE_COUNT,
    LAYER_SPAN,
    OPERATION,
    PROTECTION,
    BACKDRILL_STUB,
    ASPECT_RATIO,
    DESCRIPTION
};


/**
 * Every column the drill chart schema knows how to build, in the order the schema lists them.
 */
const GENERATED_TABLE_SCHEMA& DrillChartSchema();


/**
 * Which holes a chart reports on.
 */
struct DRILL_CHART_FILTER
{
    bool m_Plated = true;
    bool m_NonPlated = true;
    bool m_Vias = true;
    bool m_Slots = true;
    bool m_Backdrills = true;
    bool m_Castellated = true;

    bool operator==( const DRILL_CHART_FILTER& aOther ) const;
};


/**
 * The column set and formatting a new chart starts from.
 *
 * A chart copies a template rather than linking to it, so editing or importing a template
 * cannot silently rewrite a fabrication drawing that has already been approved.
 */
class DRILL_CHART_TEMPLATE
{
public:
    DRILL_CHART_TEMPLATE();

    static DRILL_CHART_TEMPLATE MakeDefault();

    void SetName( const wxString& aName ) { m_name = aName; }

    std::vector<GENERATED_TABLE_COLUMN>& Columns() { return m_columns; }
    const std::vector<GENERATED_TABLE_COLUMN>& Columns() const { return m_columns; }

    GENERATED_TABLE_UNITS GetUnits() const { return m_units; }
    void SetUnits( GENERATED_TABLE_UNITS aUnits ) { m_units = aUnits; }

    int GetPrecision() const { return m_precision; }
    void SetPrecision( int aPrecision ) { m_precision = aPrecision; }

    bool GetShowTotals() const { return m_showTotals; }
    void SetShowTotals( bool aShow ) { m_showTotals = aShow; }

    /**
     * Read and write a template as JSON.
     *
     * A template is shared between projects and people, so it is a standalone file rather
     * than something only a board can carry. Import validates. An unknown column token is
     * skipped rather than becoming an out-of-range enum.
     */
    bool SaveToFile( const wxString& aPath, wxString* aError ) const;
    bool LoadFromFile( const wxString& aPath, wxString* aError );

private:
    /**
     * Identify the template file to whoever opens it. A chart copies formatting without
     * recording where it came from, so nothing in the board reads these back.
     */
    wxString m_name;
    int      m_version;

    std::vector<GENERATED_TABLE_COLUMN> m_columns;
    GENERATED_TABLE_UNITS                m_units;
    int                                   m_precision;
    bool                                  m_showTotals;
};

#endif // DRILL_CHART_TEMPLATE_H

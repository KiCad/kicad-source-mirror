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

#include <lset.h>
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


enum class DRILL_CHART_ALIGN
{
    LEFT,
    CENTER,
    RIGHT
};



enum class DRILL_CHART_UNITS
{
    MM,
    INCH
};


/**
 * Layers a chart or map may live on.
 *
 * Excludes copper, silkscreen, mask, paste, adhesive, Edge.Cuts, Margin, courtyard and rescue
 * layers. Chart artwork and hole symbols must not alter fabrication outputs or DRC constraints.
 */
LSET DrillDocumentationLayers();


/**
 * A chart column can be no wider than this, and no chart wider than that in total. Bounds
 * hostile input from any decoder. No real chart column is a metre across, and the totals are
 * summed in int64 before being checked so the check itself cannot overflow.
 */
constexpr int     DRILL_CHART_MAX_COLUMN_WIDTH = 1000 * 1000000;
constexpr int64_t DRILL_CHART_MAX_TOTAL_WIDTH = 10LL * 1000 * 1000000;



struct DRILL_CHART_COLUMN
{
    DRILL_CHART_COLUMN_ID m_Id = DRILL_CHART_COLUMN_ID::SYMBOL;
    wxString              m_Heading;
    DRILL_CHART_ALIGN     m_Align = DRILL_CHART_ALIGN::LEFT;
    int                   m_Width = 0;

    bool operator==( const DRILL_CHART_COLUMN& aOther ) const;
};


/**
 * Reject a column set that repeats an id or is implausibly wide. Shared so the s-expression
 * parser, the protobuf decoder and template import cannot disagree about what is acceptable.
 */
bool ValidateDrillChartColumns( std::vector<DRILL_CHART_COLUMN>& aColumns );


/**
 * The heading and alignment a column starts with, so the writer can leave them out of the
 * file and the parser can put the same values back. False for a column with no default.
 */
bool DrillChartDefaultColumn( DRILL_CHART_COLUMN_ID aId, DRILL_CHART_COLUMN& aColumn );


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

    std::vector<DRILL_CHART_COLUMN>& Columns() { return m_columns; }
    const std::vector<DRILL_CHART_COLUMN>& Columns() const { return m_columns; }

    DRILL_CHART_UNITS GetUnits() const { return m_units; }
    void SetUnits( DRILL_CHART_UNITS aUnits ) { m_units = aUnits; }

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

    std::vector<DRILL_CHART_COLUMN> m_columns;
    DRILL_CHART_UNITS               m_units;
    int                             m_precision;
    bool                            m_showTotals;
};

/**
 * Stable file tokens, independent of enum ordering so inserting a value later cannot change
 * what an existing board file means.
 */
const char* DrillChartUnitsToken( DRILL_CHART_UNITS aUnits );
const char* DrillChartColumnToken( DRILL_CHART_COLUMN_ID aId );
const char* DrillChartAlignToken( DRILL_CHART_ALIGN aAlign );

bool DrillChartUnitsFromToken( const wxString& aToken, DRILL_CHART_UNITS& aUnits );
bool DrillChartColumnFromToken( const wxString& aToken, DRILL_CHART_COLUMN_ID& aId );
bool DrillChartAlignFromToken( const wxString& aToken, DRILL_CHART_ALIGN& aAlign );

#endif // DRILL_CHART_TEMPLATE_H

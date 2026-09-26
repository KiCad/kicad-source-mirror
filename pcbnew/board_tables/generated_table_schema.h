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

#ifndef GENERATED_TABLE_SCHEMA_H
#define GENERATED_TABLE_SCHEMA_H

#include <cstdint>
#include <vector>

#include <lset.h>
#include <wx/string.h>


enum class GENERATED_TABLE_ALIGN
{
    LEFT,
    CENTER,
    RIGHT
};


enum class GENERATED_TABLE_UNITS
{
    MM,
    INCH,
    MILS
};


/**
 * A column can be no wider than this. Bounds hostile input from any decoder. No real table
 * column is a metre across.
 */
constexpr int GENERATED_TABLE_MAX_COLUMN_WIDTH = 1000 * 1000000;


/**
 * A column set can be no wider than this in total, summed after each column is clamped to
 * GENERATED_TABLE_MAX_COLUMN_WIDTH. Every column can pass the per-column bound on its own and
 * still push table geometry past int range once enough of them are laid out side by side, so
 * the total is checked separately, in int64 so the check itself cannot overflow.
 */
constexpr int64_t GENERATED_TABLE_MAX_TOTAL_WIDTH = 10LL * 1000 * 1000000;


/**
 * A column as a generated table actually carries it: what a row's cell in that column holds
 * and how it is presented. Every generated table (drill chart, stackup table, board
 * characteristics) shares this shape rather than inventing its own.
 */
struct GENERATED_TABLE_COLUMN
{
    int                   m_Id = 0;
    wxString              m_Heading;
    GENERATED_TABLE_ALIGN m_Align = GENERATED_TABLE_ALIGN::LEFT;
    int                   m_Width = 0;    // minimum width, 0 = autosize

    bool operator==( const GENERATED_TABLE_COLUMN& aOther ) const = default;
};


/**
 * The column a schema knows how to build, independent of whether any particular table shows
 * it. A schema's id space, not a table's.
 */
struct GENERATED_TABLE_COLUMN_DEF
{
    int                   m_Id;
    const char*           m_Token;           // file and JSON token, never translated
    const wxChar*         m_Heading;         // _HKI() string, translated when used
    GENERATED_TABLE_ALIGN m_Align;
    bool                  m_DefaultShown;    // part of Defaults()
    const wxChar*         m_Label = nullptr; // _HKI() UI name, the heading when null
};


/**
 * The columns one generator (the drill chart, a stackup table, ...) can produce, and the
 * units and precision it starts from.
 *
 * A schema is immutable once built, so every consumer reading the same id sees the same
 * heading, alignment and default. Stable ids and tokens let a saved file outlive a KiCad
 * release that reorders or extends the column list.
 */
class GENERATED_TABLE_SCHEMA
{
public:
    GENERATED_TABLE_SCHEMA( std::vector<GENERATED_TABLE_COLUMN_DEF> aDefs, GENERATED_TABLE_UNITS aDefaultUnits,
                            int aDefaultPrecision );

    const std::vector<GENERATED_TABLE_COLUMN_DEF>& Defs() const { return m_defs; }

    const GENERATED_TABLE_COLUMN_DEF* Find( int aId ) const;
    const GENERATED_TABLE_COLUMN_DEF* FindToken( const wxString& aToken ) const;

    /**
     * The file token for a known id, or a stable placeholder for one the schema does not
     * recognize, so a writer always has something parseable to put in "(id ...)".
     */
    const char* Token( int aId ) const;

    GENERATED_TABLE_COLUMN              DefaultColumn( int aId ) const;
    std::vector<GENERATED_TABLE_COLUMN> Defaults() const;

    /**
     * Drop a column whose id the schema does not know and a column that repeats an id
     * already kept, then clamp every surviving width. False when nothing usable is left or
     * the clamped total is implausibly wide, so a hostile or truncated file cannot hand a
     * caller an empty table to divide by or push its geometry past int range.
     */
    bool Validate( std::vector<GENERATED_TABLE_COLUMN>& aColumns ) const;

    GENERATED_TABLE_UNITS DefaultUnits() const { return m_defaultUnits; }
    int                   DefaultPrecision() const { return m_defaultPrecision; }

private:
    std::vector<GENERATED_TABLE_COLUMN_DEF> m_defs;
    GENERATED_TABLE_UNITS                   m_defaultUnits;
    int                                      m_defaultPrecision;
};


/**
 * Render a length the way a generated table's cell shows it. INCH and MILS both derive their
 * decimal count from aPrecision, so a table can switch units without needing a second
 * precision setting.
 */
wxString FormatGeneratedTableLength( int aValue, GENERATED_TABLE_UNITS aUnits, int aPrecision );


/**
 * Stable file tokens, independent of enum ordering so inserting a value later cannot change
 * what an existing board file means.
 */
const char* GeneratedTableUnitsToken( GENERATED_TABLE_UNITS aUnits );
bool        GeneratedTableUnitsFromToken( const wxString& aToken, GENERATED_TABLE_UNITS& aUnits );
const char* GeneratedTableAlignToken( GENERATED_TABLE_ALIGN aAlign );
bool        GeneratedTableAlignFromToken( const wxString& aToken, GENERATED_TABLE_ALIGN& aAlign );


/**
 * Layers a drill chart or drill map may live on. Manufacturing layers would be corrupted.
 *
 * Excludes copper, silkscreen, mask, paste, adhesive, Edge.Cuts, Margin, courtyard and rescue
 * layers. Chart artwork and hole symbols must not alter fabrication outputs or DRC constraints.
 */
LSET DocumentationLayers();

#endif // GENERATED_TABLE_SCHEMA_H

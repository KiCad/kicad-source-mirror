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

#ifndef DRILL_SYMBOL_PROFILE_H
#define DRILL_SYMBOL_PROFILE_H

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <kiid.h>
#include <math/vector2d.h>
#include <wx/string.h>

struct DRILL_OPERATION;


/**
 * Which properties split holes into separate chart rows and symbols.
 *
 * A disabled key does not merely go unprinted, it stops distinguishing holes, so two
 * operations differing only in a disabled property share one row and one symbol.
 */
enum class DRILL_GROUP_KEY
{
    SIZE,
    SLOT,
    PLATING,
    SPAN,
    OPERATION,
    HOLE_FUNCTION,
    PROTECTION,
    POST_MACHINING
};


enum class DRILL_MARK_MODE
{
    SHAPE,
    LETTER,
    SIZE_TEXT
};


enum class DRILL_MARK_POLICY
{
    SHAPES,
    LETTERS,
    SHAPES_THEN_LETTERS,
    SIZE_TEXT
};


struct DRILL_SYMBOL_ASSIGNMENT
{
    DRILL_MARK_MODE m_MarkMode = DRILL_MARK_MODE::SHAPE;
    int             m_ShapeIndex = 0;
    wxString        m_Letter;
    wxString        m_Description;
};


/**
 * Grouping rules and symbol assignments, shared by reference so a chart and its map can
 * never disagree about which symbol means which hole.
 */
class DRILL_SYMBOL_PROFILE
{
public:
    DRILL_SYMBOL_PROFILE();

    const KIID& Uuid() const { return m_uuid; }
    void SetUuid( const KIID& aUuid ) { m_uuid = aUuid; }

    const wxString& GetName() const { return m_name; }
    void SetName( const wxString& aName ) { m_name = aName; }

    bool IsGroupedBy( DRILL_GROUP_KEY aKey ) const { return m_groupBy.count( aKey ) > 0; }
    void SetGroupedBy( DRILL_GROUP_KEY aKey, bool aOn );
    const std::set<DRILL_GROUP_KEY>& GroupKeys() const { return m_groupBy; }

    DRILL_MARK_POLICY GetMarkPolicy() const { return m_markPolicy; }
    void SetMarkPolicy( DRILL_MARK_POLICY aPolicy ) { m_markPolicy = aPolicy; }

    int GetSymbolSize() const { return m_symbolSize; }
    void SetSymbolSize( int aSize ) { m_symbolSize = aSize; }

    int GetSymbolWidth() const { return m_symbolWidth; }
    void SetSymbolWidth( int aWidth ) { m_symbolWidth = aWidth; }

    bool GetFreezeAssignments() const { return m_freezeAssignments; }
    void SetFreezeAssignments( bool aFreeze ) { m_freezeAssignments = aFreeze; }

    /**
     * Stable identity for a group under the currently enabled keys.
     *
     * Built from nanometre integers and copper ordinals rather than layer names, so renaming
     * a layer or switching display units cannot silently reassign a symbol. The leading
     * version lets a later key-format change migrate instead of resetting.
     */
    std::string GroupKeyString( const DRILL_OPERATION& aOperation ) const;

    const std::map<std::string, DRILL_SYMBOL_ASSIGNMENT>& Assignments() const { return m_assignments; }

    const DRILL_SYMBOL_ASSIGNMENT* GetAssignment( const std::string& aKey ) const;
    void SetAssignment( const std::string& aKey, const DRILL_SYMBOL_ASSIGNMENT& aAssignment );

    /**
     * Assignments whose key no longer matches any hole are kept, so deleting and restoring
     * a hole size does not shuffle the chart.
     */
    void ClearAssignments() { m_assignments.clear(); }

    bool operator==( const DRILL_SYMBOL_PROFILE& aOther ) const;

    /**
     * Cheap value used to notice an edit. Not stored, so it need not be stable across builds.
     */
    uint64_t Fingerprint() const;

private:
    KIID                      m_uuid;
    wxString                  m_name;
    std::set<DRILL_GROUP_KEY> m_groupBy;
    DRILL_MARK_POLICY         m_markPolicy;
    int                       m_symbolSize;
    int                       m_symbolWidth;
    bool                      m_freezeAssignments;

    std::map<std::string, DRILL_SYMBOL_ASSIGNMENT> m_assignments;
};

/**
 * Stable file tokens, deliberately independent of enum ordering so a later insertion into
 * the enum cannot silently change what a board file means.
 */
const char* DrillGroupKeyToken( DRILL_GROUP_KEY aKey );
const char* DrillMarkPolicyToken( DRILL_MARK_POLICY aPolicy );
const char* DrillMarkModeToken( DRILL_MARK_MODE aMode );

bool DrillGroupKeyFromToken( const wxString& aToken, DRILL_GROUP_KEY& aKey );
bool DrillMarkPolicyFromToken( const wxString& aToken, DRILL_MARK_POLICY& aPolicy );
bool DrillMarkModeFromToken( const wxString& aToken, DRILL_MARK_MODE& aMode );

#endif // DRILL_SYMBOL_PROFILE_H

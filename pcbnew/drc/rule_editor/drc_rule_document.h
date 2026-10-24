/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DRC_RULE_DOCUMENT_H
#define DRC_RULE_DOCUMENT_H

#include <vector>
#include <wx/string.h>

#include "drc_re_loaded_rule.h"
#include "drc_rule_editor_enums.h"

class BOARD;


/**
 * The custom rules of one board, held in memory while Board Setup is open.
 *
 * Every Design Rules page edits this one object. The rules file is written once, when the
 * dialog is accepted, and never touched before that.
 */
class DRC_RULE_DOCUMENT
{
public:
    DRC_RULE_DOCUMENT( BOARD* aBoard );

    /**
     * Read the board's own rules file. Nothing is marked modified.
     */
    void Load();

    /**
     * Replace the rules with those of another board's file, as an import. A board without a
     * rules file changes nothing.
     */
    void LoadFrom( const wxString& aPath );

    /**
     * Write the board's rules file. Returns false and fills aError when the write fails.
     */
    bool Save( wxString* aError );

    /**
     * The rules as text, for the Custom Rules page. Until a page changes a rule, this is the
     * text as it was read or typed, so comments and layout outside the rules survive.
     */
    wxString GetText() const;

    /**
     * Take the text of the Custom Rules page. Text that parses replaces the rules. Text that
     * does not is kept verbatim so it is written back unchanged.
     */
    void SetText( const wxString& aText );

    std::vector<DRC_RE_LOADED_PANEL_ENTRY>& Entries() { return m_entries; }

    std::vector<int> IndicesFor( DRC_RULE_EDITOR_CONSTRAINT_NAME aType ) const;

    int Add( const DRC_RE_LOADED_PANEL_ENTRY& aEntry );

    /**
     * Write an edited rule back. Constraints the loader split out of the same rule share the
     * rule's name, condition, layers and severity, so those are carried over to them too.
     */
    void UpdateEntry( int aIndex, const DRC_RE_LOADED_PANEL_ENTRY& aEdited );

    void Remove( int aIndex );

    int Duplicate( int aIndex );

    /**
     * Move a rule one step among the rules listed for aType. Returns false when it is already
     * first or last.
     */
    bool Move( int aIndex, int aDelta, DRC_RULE_EDITOR_CONSTRAINT_NAME aType );

    bool IsRuleNameUnique( const wxString& aName, DRC_RULE_EDITOR_CONSTRAINT_NAME aType, int aExceptIndex ) const;

    wxString UniqueRuleName( const wxString& aBase ) const;

    int NextId() { return m_nextId++; }

    bool IsModified() const { return m_modified; }

    /**
     * True while the Custom Rules text could not be parsed. The pages must not edit entries
     * then, because the text is what gets written.
     */
    bool HasUnparsedText() const { return m_hasRawText; }

    /**
     * Position of the constraint clause for aCode in rule text, or npos. The code must end at
     * a delimiter so a code that starts like another is not matched.
     */
    static size_t FindConstraintClause( const wxString& aText, const wxString& aCode );

private:
    void load( const wxString& aPath );

    bool parseInto( const wxString& aText );

    void prepareEntries();

    void touch();

    BOARD*                                 m_board;
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> m_entries;
    DRC_RE_FILE_TRIVIA                     m_trivia;
    wxString                               m_rawText;
    bool                                   m_hasRawText;
    bool                                   m_textCurrent;
    bool                                   m_modified;
    int                                    m_nextId;
    int                                    m_nextGroupId;
};

#endif // DRC_RULE_DOCUMENT_H

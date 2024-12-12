/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef RULE_EDITOR_DATA_BASE_H_
#define RULE_EDITOR_DATA_BASE_H_

#include <lset.h>
#include <lseq.h>
#include <optional>

/**
 * Abstract interface class to enable polymorphic copying between objects.
 */
class ICopyable
{
public:
    virtual ~ICopyable() = default;

    virtual void CopyFrom( const ICopyable& source ) = 0;
};

/**
 * Concrete class representing the base data structure for a rule editor.
 */
class RULE_EDITOR_DATA_BASE : public ICopyable
{
public:
    RULE_EDITOR_DATA_BASE() = default;

    explicit RULE_EDITOR_DATA_BASE( int aId, int aParentId, wxString aRuleName ) :
            m_id( aId ), m_parentId( aParentId ), m_ruleName( aRuleName ), m_isNew( false )
    {
    }

    virtual ~RULE_EDITOR_DATA_BASE() = default;

    /**
     * Get the unique ID of the rule.
     * 
     * @return The unique ID of the rule.
     */
    int GetId() { return m_id; }

    /**
     * Set the unique ID of the rule.
     * 
     * @param aId The unique ID to set.
     */
    void SetId( int aId ) { m_id = aId; }

    /**
     * Get the parent ID of the rule.
     * 
     * @return The parent ID of the rule, or -1 if no parent is set.
     */
    int GetParentId() { return m_parentId.value_or( -1 ); }

    /**
     * Set the parent ID of the rule.
     * 
     * @param aParentId The parent ID to set.
     */
    void SetParentId( int aParentId ) { m_parentId = aParentId; }

    /**
     * Get the name of the rule.
     * 
     * @return The name of the rule.
     */
    wxString GetRuleName() { return m_ruleName; }

    /**
     * Set the name of the rule.
     * 
     * @param aRuleName The name of the rule to set.
     */
    void SetRuleName( wxString aRuleName ) { m_ruleName = aRuleName; }

    /**
     * Get the comment associated with the rule.
     * 
     * @return The comment of the rule.
     */
    wxString GetComment() { return m_comment; }

    /**
     * Set the comment for the rule.
     * 
     * @param aComment The comment to set.
     */
    void SetComment( wxString aComment ) { m_comment = aComment; }

    /**
     * Check if the rule is marked as new.
     * 
     * @return True if the rule is new (being created), false if it is being edited.
     */
    bool IsNew() { return m_isNew; }

    /**
     * Mark the rule as new or not.
     * 
     * @param aIsNew True to mark the rule as new, false to mark it as existing.
     */
    void SetIsNew( bool aIsNew ) { m_isNew = aIsNew; }

    /**
     * Implementation of the polymorphic `CopyFrom` method.
     * 
     * @param source The source object to copy from.
     */
    void CopyFrom( const ICopyable& aSource ) override
    {
        const auto& baseSource = dynamic_cast<const RULE_EDITOR_DATA_BASE&>( aSource );

        m_comment = baseSource.m_comment;
    }

private:
    int                m_id;       // Unique ID of the rule.
    std::optional<int> m_parentId; // Optional parent ID of the rule.
    wxString           m_ruleName;
    wxString           m_comment;
    bool               m_isNew; /**< Flag indicating if the user is creating a new rule (true) 
                                              or editing an existing rule (false). */
};

#endif // RULE_EDITOR_DATA_BASE_H_
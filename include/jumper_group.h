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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>

#include <optional>
#include <set>
#include <vector>

#include <wx/string.h>


/**
 * @brief Represents a group of jumper pins or pads, keyed by name.
 *
 * A jumper group always has at least one non-blank name.
 *
 * Jumper groups are agnostic to whether they are associated with a symbol or footprint.
 */
class KICOMMON_API JUMPER_GROUP
{
public:
    /**
     * Factory method to create a #JUMPER_GROUP from a set of names.  Blank names are discarded.
     *
     * @return std::nullopt if no usable names remain.
     */
    static std::optional<JUMPER_GROUP> Make( std::set<wxString> aNames );

    bool                      Contains( const wxString& aName ) const;
    const std::set<wxString>& GetNames() const { return m_names; }

    bool operator==( const JUMPER_GROUP& aOther ) const = default;

private:
    explicit JUMPER_GROUP( std::set<wxString> aNames );

    // This should never be empty
    std::set<wxString> m_names;
};


/**
 * @brief Represents an (ordered) list of #JUMPER_GROUP objects.
 *
 * This class is used to store the jumper groups associated with a symbol or footprint.
 * It can be empty (no defined jumper groups).
 */
class KICOMMON_API JUMPER_GROUP_SET
{
public:
    /**
     * Add a #JUMPER_GROUP to the set.
     */
    void Add( JUMPER_GROUP aGroup );

    /**
     * Convenience method to create and add a #JUMPER_GROUP from a set of names.
     * Blank names are discarded.  If no usable names remain, nothing is added.
     */
    void Add( std::set<wxString> aNames );

    void Clear() { m_groups.clear(); }

    const std::vector<JUMPER_GROUP>& GetAll() const { return m_groups; }
    bool                             IsEmpty() const { return m_groups.empty(); }

    /**
     * @return the first group containing \a aName, or nullptr if no group contains it.
     */
    const JUMPER_GROUP* FindContaining( const wxString& aName ) const;

    bool operator==( const JUMPER_GROUP_SET& aOther ) const = default;

private:
    std::vector<JUMPER_GROUP> m_groups;
};
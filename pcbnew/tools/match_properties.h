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

#ifndef MATCH_PROPERTIES_H
#define MATCH_PROPERTIES_H

#include <set>
#include <vector>

#include <wx/string.h>

class EDA_ITEM;


struct MATCH_PROPERTIES_RESULT
{
    explicit operator bool() const { return m_Error.IsEmpty(); }

    int      m_Changed = 0;
    wxString m_Error;
};


namespace MATCH_PROPERTIES_CATALOG
{

wxString Family( const EDA_ITEM& aItem );
wxString DisplayLabel( const wxString& aKey );

/// The translated name of a family, for the heading of its group.
wxString FamilyLabel( const wxString& aFamily );

/// The translated property name alone, without the family it belongs to.
wxString PropertyLabel( const wxString& aKey );

/// The families a key reaches.  One for a family key, several for a common one.
std::set<wxString> FamiliesFor( const wxString& aKey );

/// Whether aFamily registers a property of that name.  Used to prune the common table.
bool PropertyIsRegistered( const wxString& aFamily, const wxString& aName );
bool Compatible( const EDA_ITEM& aSource, const EDA_ITEM& aTarget );
std::vector<EDA_ITEM*> CompatibleTargets( const EDA_ITEM& aSource, const std::vector<EDA_ITEM*>& aCandidates );

/// True if any enabled key names a property of this item's family.
bool AnyEnabledFor( const EDA_ITEM& aItem, const std::set<wxString>& aEnabledKeys );

/**
 * Every property Match Properties may copy, keyed as "family/Property Name".
 *
 * Derived from PROPERTY_BASE::IsCopyable(), so a property becomes copyable by being flagged
 * where it is registered rather than by being named here.
 */
const std::set<wxString>& AllSafeKeys();

/// The subset enabled until the user says otherwise.
const std::set<wxString>& DefaultKeys();

MATCH_PROPERTIES_RESULT Copy( const EDA_ITEM& aSource, EDA_ITEM& aTarget,
                              const std::set<wxString>& aEnabledKeys );

}

#endif

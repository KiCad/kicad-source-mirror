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

#ifndef COMPONENT_CLASS_ASSIGNMENT_RULE_H
#define COMPONENT_CLASS_ASSIGNMENT_RULE_H

#include <memory>
#include <vector>
#include <wx/string.h>

#include <drc/drc_rule_condition.h>

class FOOTPRINT;

/**
 * Class which represents a component class assignment rule. These are used to dynamically assign component classes
 * based on FOOTPRINT parameters which are exposed through the DRC language.
 */
class COMPONENT_CLASS_ASSIGNMENT_RULE
{
public:
    /// Construct a component class assignment rule
    explicit COMPONENT_CLASS_ASSIGNMENT_RULE( const wxString&                       aComponentClass,
                                              std::shared_ptr<DRC_RULE_CONDITION>&& aCondition );

    /// The component class to assign to matching footprints
    wxString GetComponentClass() const { return m_componentClass; }
    void     SetComponentClass( const wxString& aComponentClass )
    {
        m_componentClass = aComponentClass;
    }

    /// Tests whether this rules matches the given footprint
    bool Matches( const FOOTPRINT* aFootprint ) const;

protected:
    /// The component class to assign to matching footprints
    wxString m_componentClass;

    /// The DRC condition which specifies footprint matches for this component class
    std::shared_ptr<DRC_RULE_CONDITION> m_condition;
};

#endif // COMPONENT_CLASS_ASSIGNMENT_RULE_H

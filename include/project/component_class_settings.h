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

#ifndef PROJECT_COMPONENT_CLASS_SETTINGS_H
#define PROJECT_COMPONENT_CLASS_SETTINGS_H

#include <vector>
#include <unordered_map>

#include <settings/nested_settings.h>


class KICOMMON_API COMPONENT_CLASS_ASSIGNMENT_DATA
{
public:
    /// A condition match type
    enum class CONDITION_TYPE
    {
        REFERENCE,
        FOOTPRINT,
        SIDE,
        ROTATION,
        FOOTPRINT_FIELD,
        CUSTOM,
        SHEET_NAME
    };

    /// Whether conditions are applied with OR or AND logic
    enum class CONDITIONS_OPERATOR
    {
        ALL,
        ANY
    };

    /// Sets the given condition type with the assocated match data
    void SetCondition( const CONDITION_TYPE aCondition, const wxString& aPrimaryData,
                       const wxString& aSecondaryData )
    {
        m_conditions[aCondition] = { aPrimaryData, aSecondaryData };
    }

    /// Gets all conditions
    const std::unordered_map<CONDITION_TYPE, std::pair<wxString, wxString>>& GetConditions() const
    {
        return m_conditions;
    }

    /// Sets the resulting component class for matching footprints
    void SetComponentClass( const wxString& aComponentClass )
    {
        m_componentClass = aComponentClass;
    }

    /// Gets the resulting component class for matching footprints
    const wxString& GetComponentClass() const { return m_componentClass; }

    /// Sets the boolean operation in use for all conditions
    void SetConditionsOperation( const CONDITIONS_OPERATOR aOperator )
    {
        m_conditionsOperator = aOperator;
    }

    /// Gets the boolean operation in use for all conditions
    CONDITIONS_OPERATOR GetConditionsOperator() const { return m_conditionsOperator; }

    /// Maps a CONDITION_TYPE to a descriptive string
    static wxString GetConditionName( const CONDITION_TYPE aCondition );

    /// Maps a descriptive string to a CONDITION_TYPE
    static CONDITION_TYPE GetConditionType( const wxString& aCondition );

    /// Returns the DRC rules language for this component class assignment
    wxString GetAssignmentInDRCLanguage() const;

protected:
    /// The name of the component class for this assignment rule
    wxString m_componentClass;

    /// Map of condition types to primary and secondary data fields for the condition
    std::unordered_map<CONDITION_TYPE, std::pair<wxString, wxString>> m_conditions;

    /// Whether conditions are applied with AND or OR logic
    /// Defaults to ALL
    CONDITIONS_OPERATOR m_conditionsOperator{ CONDITIONS_OPERATOR::ALL };
};


/**
 * COMPONENT_CLASS_SETTINGS stores data for component classes, including rules for automatic
 * generation of component classes.
 */
class KICOMMON_API COMPONENT_CLASS_SETTINGS : public NESTED_SETTINGS
{
public:
    COMPONENT_CLASS_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~COMPONENT_CLASS_SETTINGS();

    /// Sets whether component classes should be generated for components in hierarchical sheets
    void SetEnableSheetComponentClasses( bool aEnabled )
    {
        m_enableSheetComponentClasses = aEnabled;
    }

    /// Gets whether component classes should be generated for components in hierarchical sheets
    bool GetEnableSheetComponentClasses() const { return m_enableSheetComponentClasses; }

    /// Clear all dynamic component class assignments
    void ClearComponentClassAssignments() { m_componentClassAssignments.clear(); }

    /// Gets all dynamic component class assignments
    const std::vector<COMPONENT_CLASS_ASSIGNMENT_DATA>& GetComponentClassAssignments() const
    {
        return m_componentClassAssignments;
    }

    // Adds a dynamic component class assignment
    void AddComponentClassAssignment( const COMPONENT_CLASS_ASSIGNMENT_DATA& aAssignment )
    {
        m_componentClassAssignments.push_back( aAssignment );
    }

    bool operator==( const COMPONENT_CLASS_SETTINGS& aOther ) const;

    bool operator!=( const COMPONENT_CLASS_SETTINGS& aOther ) const
    {
        return !operator==( aOther );
    }

private:
    /// Toggle generation of component classes for hierarchical sheets
    bool m_enableSheetComponentClasses;

    /// All dynamic component class assignment rules
    std::vector<COMPONENT_CLASS_ASSIGNMENT_DATA> m_componentClassAssignments;

    /// Saves a dynamic component class assignment to JSON
    static nlohmann::json saveAssignment( const COMPONENT_CLASS_ASSIGNMENT_DATA& aAssignment );

    /// Loads a dynamic component class assignment from JSON
    static COMPONENT_CLASS_ASSIGNMENT_DATA loadAssignment( const nlohmann::json& aJson );
};

#endif // PROJECT_COMPONENT_CLASS_SETTINGS_H

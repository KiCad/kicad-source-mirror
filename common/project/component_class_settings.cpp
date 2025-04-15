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

#include <json_common.h>

#include <project/component_class_settings.h>
#include <settings/parameters.h>

constexpr int componentClassSettingsSchemaVersion = 0;


COMPONENT_CLASS_SETTINGS::COMPONENT_CLASS_SETTINGS( JSON_SETTINGS*     aParent,
                                                    const std::string& aPath ) :
        NESTED_SETTINGS( "component_class_settings", componentClassSettingsSchemaVersion, aParent,
                         aPath, false ),
        m_enableSheetComponentClasses( false )
{
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "sheet_component_classes",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                ret["enabled"] = m_enableSheetComponentClasses;

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                if( !aJson.contains( "enabled" ) )
                    return;

                m_enableSheetComponentClasses = aJson["enabled"].get<bool>();
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "assignments",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( const COMPONENT_CLASS_ASSIGNMENT_DATA& assignment :
                     m_componentClassAssignments )
                {
                    ret.push_back( saveAssignment( assignment ) );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                ClearComponentClassAssignments();

                for( const auto& assignmentJson : aJson )
                {
                    COMPONENT_CLASS_ASSIGNMENT_DATA assignment = loadAssignment( assignmentJson );
                    m_componentClassAssignments.push_back( assignment );
                }
            },
            {} ) );
}


COMPONENT_CLASS_SETTINGS::~COMPONENT_CLASS_SETTINGS()
{
    // Release early before destroying members
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


nlohmann::json
COMPONENT_CLASS_SETTINGS::saveAssignment( const COMPONENT_CLASS_ASSIGNMENT_DATA& aAssignment )
{
    nlohmann::json ret;

    const wxString matchOperator =
            aAssignment.GetConditionsOperator()
                            == COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR::ALL
                    ? wxT( "ALL" )
                    : wxT( "ANY" );

    ret["component_class"] = aAssignment.GetComponentClass().ToUTF8();
    ret["conditions_operator"] = matchOperator.ToUTF8();

    nlohmann::json conditionsJson;

    for( const auto& [conditionType, conditionData] : aAssignment.GetConditions() )
    {
        nlohmann::json conditionJson;

        if( !conditionData.first.empty() )
            conditionJson["primary"] = conditionData.first;

        if( !conditionData.second.empty() )
            conditionJson["secondary"] = conditionData.second;

        const wxString conditionName =
                COMPONENT_CLASS_ASSIGNMENT_DATA::GetConditionName( conditionType );
        conditionsJson[conditionName] = conditionJson;
    }

    ret["conditions"] = conditionsJson;

    return ret;
}


COMPONENT_CLASS_ASSIGNMENT_DATA
COMPONENT_CLASS_SETTINGS::loadAssignment( const nlohmann::json& aJson )
{
    COMPONENT_CLASS_ASSIGNMENT_DATA assignment;

    assignment.SetComponentClass(
            wxString( aJson["component_class"].get<std::string>().c_str(), wxConvUTF8 ) );

    const wxString matchOperator( aJson["conditions_operator"].get<std::string>().c_str(),
                                  wxConvUTF8 );

    if( matchOperator == wxT( "ALL" ) )
    {
        assignment.SetConditionsOperation(
                COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR::ALL );
    }
    else
    {
        assignment.SetConditionsOperation(
                COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR::ANY );
    }

    for( const auto& [conditionTypeStr, conditionData] : aJson["conditions"].items() )
    {
        wxString                                        primary, secondary;
        COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE conditionType =
                COMPONENT_CLASS_ASSIGNMENT_DATA::GetConditionType( conditionTypeStr );

        if( conditionData.contains( "primary" ) )
            primary = wxString( conditionData["primary"].get<std::string>().c_str(), wxConvUTF8 );

        if( conditionData.contains( "secondary" ) )
            secondary =
                    wxString( conditionData["secondary"].get<std::string>().c_str(), wxConvUTF8 );

        assignment.SetCondition( conditionType, primary, secondary );
    }

    return assignment;
}


bool COMPONENT_CLASS_SETTINGS::operator==( const COMPONENT_CLASS_SETTINGS& aOther ) const
{
    // TODO: Implement this
    throw;
    //return true;
}


/**************************************************************************************************
 *
 * COMPONENT_CLASS_ASSIGNMENT_DATA implementation
 *
 *************************************************************************************************/

wxString COMPONENT_CLASS_ASSIGNMENT_DATA::GetConditionName( const CONDITION_TYPE aCondition )
{
    switch( aCondition )
    {
    case CONDITION_TYPE::REFERENCE: return wxT( "REFERENCE" );
    case CONDITION_TYPE::FOOTPRINT: return wxT( "FOOTPRINT" );
    case CONDITION_TYPE::SIDE: return wxT( "SIDE" );
    case CONDITION_TYPE::ROTATION: return wxT( "ROTATION" );
    case CONDITION_TYPE::FOOTPRINT_FIELD: return wxT( "FOOTPRINT_FIELD" );
    case CONDITION_TYPE::CUSTOM: return wxT( "CUSTOM" );
    case CONDITION_TYPE::SHEET_NAME: return wxT( "SHEET_NAME" );
    }

    wxASSERT_MSG( false, "Invalid condition type" );

    return wxEmptyString;
}


COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE
COMPONENT_CLASS_ASSIGNMENT_DATA::GetConditionType( const wxString& aCondition )
{
    if( aCondition == wxT( "REFERENCE" ) )
        return CONDITION_TYPE::REFERENCE;
    if( aCondition == wxT( "FOOTPRINT" ) )
        return CONDITION_TYPE::FOOTPRINT;
    if( aCondition == wxT( "SIDE" ) )
        return CONDITION_TYPE::SIDE;
    if( aCondition == wxT( "ROTATION" ) )
        return CONDITION_TYPE::ROTATION;
    if( aCondition == wxT( "FOOTPRINT_FIELD" ) )
        return CONDITION_TYPE::FOOTPRINT_FIELD;
    if( aCondition == wxT( "CUSTOM" ) )
        return CONDITION_TYPE::CUSTOM;
    if( aCondition == wxT( "SHEET_NAME" ) )
        return CONDITION_TYPE::SHEET_NAME;

    wxASSERT_MSG( false, "Invalid condition type" );

    return CONDITION_TYPE::REFERENCE;
}


wxString COMPONENT_CLASS_ASSIGNMENT_DATA::GetAssignmentInDRCLanguage() const
{
    if( m_componentClass.empty() )
        return wxEmptyString;

    if( m_conditions.empty() )
    {
        // A condition which always applies the netclass
        return wxString::Format( wxT( "(version 1) (assign_component_class \"%s\")" ),
                                 m_componentClass );
    }

    // Lambda to format a comma-separated list of references in to a DRC expression
    auto getRefExpr = []( wxString aRefs ) -> wxString
    {
        aRefs.Trim( true ).Trim( false );

        wxArrayString refs = wxSplit( aRefs, ',' );

        if( refs.empty() )
            return wxEmptyString;

        std::ranges::transform( refs, refs.begin(),
                                []( const wxString& aRef )
                                {
                                    return wxString::Format( wxT( "A.Reference == '%s'" ), aRef );
                                } );

        wxString refsExpr = refs[0];

        if( refs.size() > 1 )
        {
            for( auto itr = refs.begin() + 1; itr != refs.end(); ++itr )
                refsExpr = refsExpr + wxT( " || " ) + *itr;
        }

        return wxString::Format( wxT( "( %s )" ), refsExpr );
    };

    // Lambda to format a footprint match DRC expression
    auto getFootprintExpr = []( wxString aFootprint ) -> wxString
    {
        aFootprint.Trim( true ).Trim( false );

        if( aFootprint.empty() )
            return wxEmptyString;

        return wxString::Format( wxT( "( A.Library_Link == '%s' )" ), aFootprint );
    };

    // Lambda to format a layer side DRC expression
    auto getSideExpr = []( const wxString& aSide ) -> wxString
    {
        if( aSide == wxT( "Any" ) )
            return wxEmptyString;

        return wxString::Format( wxT( "( A.Layer == '%s' )" ),
                                 aSide == wxT( "Front" ) ? wxT( "F.Cu" ) : wxT( "B.Cu" ) );
    };

    // Lambda to format a rotation DRC expression
    auto getRotationExpr = []( wxString aRotation ) -> wxString
    {
        aRotation.Trim( true ).Trim( false );

        int dummy;

        if( aRotation.empty() || aRotation == wxT( "Any" ) || !aRotation.ToInt( &dummy ) )
            return wxEmptyString;

        return wxString::Format( wxT( "( A.Orientation == %s deg )" ), aRotation );
    };


    // Lambda to format a footprint field DRC expression
    auto getFootprintFieldExpr = []( wxString aFieldName, wxString aFieldMatch ) -> wxString
    {
        aFieldName.Trim( true ).Trim( false );
        aFieldMatch.Trim( true ).Trim( false );

        if( aFieldName.empty() || aFieldMatch.empty() )
            return wxEmptyString;

        return wxString::Format( wxT( "( A.getField('%s') == '%s' )" ), aFieldName, aFieldMatch );
    };

    // Lambda to format a custom DRC expression
    auto getCustomFieldExpr = []( wxString aExpr ) -> wxString
    {
        aExpr.Trim( true ).Trim( false );

        if( aExpr.empty() )
            return wxEmptyString;

        return wxString::Format( wxT( "( %s )" ), aExpr );
    };

    // Lambda to format a sheet name expression
    auto getSheetNameExpr = []( wxString aSheetName ) -> wxString
    {
        aSheetName.Trim( true ).Trim( false );

        if( aSheetName.empty() )
            return wxEmptyString;

        return wxString::Format( wxT( "( A.memberOfSheet('%s') )" ), aSheetName );
    };

    std::vector<wxString> conditionsExprs;

    for( auto& [conditionType, conditionData] : m_conditions )
    {
        wxString conditionExpr;

        switch( conditionType )
        {
        case CONDITION_TYPE::REFERENCE: conditionExpr = getRefExpr( conditionData.first ); break;
        case CONDITION_TYPE::FOOTPRINT:
            conditionExpr = getFootprintExpr( conditionData.first );
            break;
        case CONDITION_TYPE::SIDE: conditionExpr = getSideExpr( conditionData.first ); break;
        case CONDITION_TYPE::ROTATION:
            conditionExpr = getRotationExpr( conditionData.first );
            break;
        case CONDITION_TYPE::FOOTPRINT_FIELD:
            conditionExpr = getFootprintFieldExpr( conditionData.first, conditionData.second );
            break;
        case CONDITION_TYPE::CUSTOM:
            conditionExpr = getCustomFieldExpr( conditionData.first );
            break;
        case CONDITION_TYPE::SHEET_NAME:
            conditionExpr = getSheetNameExpr( conditionData.first );
            break;
        }

        if( !conditionExpr.empty() )
            conditionsExprs.push_back( conditionExpr );
    }

    if( conditionsExprs.empty() )
        return wxString::Format( wxT( "(version 1) (assign_component_class \"%s\")" ),
                                 m_componentClass );

    wxString allConditionsExpr = conditionsExprs[0];

    if( conditionsExprs.size() > 1 )
    {
        wxString operatorExpr =
                m_conditionsOperator == CONDITIONS_OPERATOR::ALL ? wxT( " && " ) : wxT( " || " );

        for( auto itr = conditionsExprs.begin() + 1; itr != conditionsExprs.end(); ++itr )
        {
            allConditionsExpr = allConditionsExpr + operatorExpr + *itr;
        }
    }

    return wxString::Format(
            wxT( "(version 1) (assign_component_class \"%s\" (condition \"%s\" ) )" ),
            m_componentClass, allConditionsExpr );
}

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


#include <component_classes/component_class_manager.h>

#include <board.h>
#include <component_classes/component_class.h>
#include <component_classes/component_class_assignment_rule.h>
#include <drc/drc_cache_generator.h>
#include <drc/drc_rule_parser.h>
#include <footprint.h>
#include <project/component_class_settings.h>
#include <tools/drc_tool.h>


COMPONENT_CLASS_MANAGER::COMPONENT_CLASS_MANAGER( BOARD* board ) :
        m_board( board ), m_hasCustomAssignmentConditions( false )
{
    m_noneClass =
            std::make_shared<COMPONENT_CLASS>( wxEmptyString, COMPONENT_CLASS::USAGE::STATIC );
}


/**
 * Computes and returns an effective component class for a (possibly empty) set of constituent
 * class names. This is called by the netlist updater to set static component classes on footprints.
 *
 * Where constituent or effective component classes already exist, they are re-used. This allows
 * efficient comparison of (effective) component classes by pointer in DRC checks.
 *
 * Preconditions: InitNetlistUpdate() must be called before invoking this method.
 * @param classNames The constitent component class names
 * @return A pointer to an effective COMPONENT_CLASS representing all constituent component classes
 */
COMPONENT_CLASS* COMPONENT_CLASS_MANAGER::GetEffectiveStaticComponentClass(
        const std::unordered_set<wxString>& classNames )
{
    // Handle no component class condition
    if( classNames.size() == 0 )
        return m_noneClass.get();

    // Handle single-assignment component classes
    if( classNames.size() == 1 )
    {
        const wxString& className = *classNames.begin();
        m_staticClassNamesCache.erase( className );
        return getOrCreateConstituentClass( className, COMPONENT_CLASS::USAGE::STATIC );
    }

    // Handle composite component classes
    const std::vector<wxString> sortedClassNames = sortClassNames( classNames );
    wxString fullName = GetFullClassNameForConstituents( sortedClassNames );

    COMPONENT_CLASS* effectiveClass =
            getOrCreateEffectiveClass( sortedClassNames, COMPONENT_CLASS::USAGE::STATIC );

    for( const COMPONENT_CLASS* constituentClass : effectiveClass->GetConstituentClasses() )
        m_staticClassNamesCache.erase( constituentClass->GetName() );

    return effectiveClass;
}


void COMPONENT_CLASS_MANAGER::InitNetlistUpdate()
{
    for( const auto& [className, classPtr] : m_constituentClasses )
    {
        if( classPtr->GetUsageContext() == COMPONENT_CLASS::USAGE::STATIC
            || classPtr->GetUsageContext() == COMPONENT_CLASS::USAGE::STATIC_AND_DYNAMIC )
        {
            m_staticClassNamesCache.insert( className );
        }
    }

    ++m_ticker;
}


void COMPONENT_CLASS_MANAGER::FinishNetlistUpdate()
{
    // m_staticClassesCache now contains any static component classes that are unused from the
    // netlist update. Delete any effective component classes which refer to them in a static-only
    // context, or update their usage context.

    // First, collect all component classes that will be deleted so we can clear footprint pointers
    // before deletion. This prevents use-after-free when auto-save serializes footprints.
    std::unordered_set<const COMPONENT_CLASS*> classesToDelete;

    for( const wxString& className : m_staticClassNamesCache )
    {
        COMPONENT_CLASS* staticClass = m_constituentClasses[className].get();

        if( staticClass->GetUsageContext() == COMPONENT_CLASS::USAGE::STATIC )
        {
            classesToDelete.insert( staticClass );

            for( const auto& [combinedFullName, combinedCompClass] : m_effectiveClasses )
            {
                if( combinedCompClass->ContainsClassName( className ) )
                    classesToDelete.insert( combinedCompClass.get() );
            }
        }
    }

    // Clear footprint static component class pointers that reference classes being deleted
    if( !classesToDelete.empty() )
    {
        for( FOOTPRINT* footprint : m_board->Footprints() )
        {
            if( classesToDelete.count( footprint->GetStaticComponentClass() ) )
                footprint->SetStaticComponentClass( nullptr );
        }
    }

    // Now perform the actual deletions
    for( const wxString& className : m_staticClassNamesCache )
    {
        COMPONENT_CLASS* staticClass = m_constituentClasses[className].get();

        if( staticClass->GetUsageContext() == COMPONENT_CLASS::USAGE::STATIC )
        {
            // Any static-only classes can be deleted, along with effective classes which refer to them
            std::unordered_set<wxString> effectiveClassesToDelete;

            for( const auto& [combinedFullName, combinedCompClass] : m_effectiveClasses )
            {
                if( combinedCompClass->ContainsClassName( className ) )
                    effectiveClassesToDelete.insert( combinedFullName );
            }

            for( const wxString& classNameToDelete : effectiveClassesToDelete )
                m_effectiveClasses.erase( classNameToDelete );

            m_constituentClasses.erase( className );
        }
        else
        {
            // Set the component class to dynamic-only scope
            wxASSERT( staticClass->GetUsageContext()
                      == COMPONENT_CLASS::USAGE::STATIC_AND_DYNAMIC );
            staticClass->SetUsageContext( COMPONENT_CLASS::USAGE::DYNAMIC );
        }
    }

    // Clear the caches
    m_staticClassNamesCache.clear();
}


wxString COMPONENT_CLASS_MANAGER::GetFullClassNameForConstituents(
        const std::unordered_set<wxString>& classNames )
{
    const std::vector<wxString> sortedClassNames = sortClassNames( classNames );

    return GetFullClassNameForConstituents( sortedClassNames );
}


wxString
COMPONENT_CLASS_MANAGER::GetFullClassNameForConstituents( const std::vector<wxString>& classNames )
{
    if( classNames.size() == 0 )
        return wxEmptyString;

    wxString fullName = classNames[0];

    for( std::size_t i = 1; i < classNames.size(); ++i )
    {
        fullName += ",";
        fullName += classNames[i];
    }

    return fullName;
}


bool COMPONENT_CLASS_MANAGER::SyncDynamicComponentClassAssignments(
        const std::vector<COMPONENT_CLASS_ASSIGNMENT_DATA>& aAssignments,
        bool aGenerateSheetClasses, const std::unordered_set<wxString>& aNewSheetPaths )
{
    m_hasCustomAssignmentConditions = false;
    bool success = true;

    // Invalidate component class cache entries
    ++m_ticker;

    // Save previous dynamically assigned component class names
    std::unordered_set<wxString> prevClassNames;

    for( const auto& rule : m_assignmentRules )
        prevClassNames.insert( rule->GetComponentClass() );

    m_assignmentRules.clear();

    // Parse all assignment rules
    std::vector<std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>> rules;

    for( const COMPONENT_CLASS_ASSIGNMENT_DATA& assignment : aAssignments )
    {
        if( assignment.GetConditions().contains(
                    COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM ) )
        {
            m_hasCustomAssignmentConditions = true;
        }

        std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE> rule = CompileAssignmentRule( assignment );

        if( rule )
            rules.emplace_back( std::move( rule ) );
        else
            success = false;
    }

    // Generate sheet classes if required
    if( aGenerateSheetClasses )
    {
        std::unordered_set<wxString> sheetNames = aNewSheetPaths;

        for( const FOOTPRINT* footprint : m_board->Footprints() )
            sheetNames.insert( footprint->GetSheetname() );

        for( wxString sheetName : sheetNames )
        {
            // Don't generate a class for empty sheets (e.g. manually placed footprints) or the root
            // sheet
            if( sheetName.empty() || sheetName == wxT( "/" ) )
                continue;

            sheetName.Replace( wxT( "\"" ), wxT( "" ) );
            sheetName.Replace( wxT( "'" ), wxT( "" ) );

            COMPONENT_CLASS_ASSIGNMENT_DATA assignment;
            assignment.SetComponentClass( sheetName );
            assignment.SetCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SHEET_NAME,
                                     sheetName, wxEmptyString );

            std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE> rule =
                    CompileAssignmentRule( assignment );

            if( rule )
                rules.emplace_back( std::move( rule ) );
            else
                success = false;
        }
    }

    // Set the assignment rules
    if( success )
        m_assignmentRules = std::move( rules );

    // Re-use or create component classes which may be output by assignment rules
    for( const auto& rule : m_assignmentRules )
    {
        wxString className = rule->GetComponentClass();
        prevClassNames.erase( className );
        getOrCreateConstituentClass( className, COMPONENT_CLASS::USAGE::DYNAMIC );
    }

    // prevClassNames now contains all dynamic component class names no longer in use. Remove any
    // effective component classes no longer in use.
    for( const wxString& className : prevClassNames )
    {
        COMPONENT_CLASS* dynamicClass = m_constituentClasses[className].get();

        if( dynamicClass->GetUsageContext() == COMPONENT_CLASS::USAGE::DYNAMIC )
        {
            // Any dynamic-only classes can be deleted, along with effective classes which refer to them
            std::unordered_set<wxString> classesToDelete;

            for( const auto& [combinedFullName, combinedCompClass] : m_effectiveClasses )
            {
                if( combinedCompClass->ContainsClassName( className ) )
                    classesToDelete.insert( combinedFullName );
            }

            for( const wxString& classNameToDelete : classesToDelete )
                m_effectiveClasses.erase( classNameToDelete );

            m_constituentClasses.erase( className );
        }
        else
        {
            // Set the component class to dynamic-only scope
            wxASSERT( dynamicClass->GetUsageContext()
                      == COMPONENT_CLASS::USAGE::STATIC_AND_DYNAMIC );
            dynamicClass->SetUsageContext( COMPONENT_CLASS::USAGE::STATIC );
        }
    }

    return success;
}


std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>
COMPONENT_CLASS_MANAGER::CompileAssignmentRule( const COMPONENT_CLASS_ASSIGNMENT_DATA& aAssignment )
{
    const wxString ruleSource = aAssignment.GetAssignmentInDRCLanguage();

    // Ignore incomplete rules (e.g. no component class name specified)
    if( ruleSource.empty() )
        return nullptr;

    DRC_RULES_PARSER parser( ruleSource, wxT( "Component class assignment rule" ) );

    try
    {
        std::vector<std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>> parsed;

        WX_STRING_REPORTER reporter;
        parser.ParseComponentClassAssignmentRules( parsed, &reporter );

        if( reporter.HasMessageOfSeverity( RPT_SEVERITY_ERROR ) )
            return nullptr;

        if( parsed.size() != 1 )
            return nullptr;

        return parsed[0];
    }
    catch( PARSE_ERROR& )
    {
        return nullptr;
    }
}


const COMPONENT_CLASS*
COMPONENT_CLASS_MANAGER::GetCombinedComponentClass( const COMPONENT_CLASS* staticClass,
                                                    const COMPONENT_CLASS* dynamicClass )
{
    std::unordered_set<wxString> classNames;

    if( staticClass )
    {
        for( const COMPONENT_CLASS* compClass : staticClass->GetConstituentClasses() )
            classNames.insert( compClass->GetName() );
    }

    if( dynamicClass )
    {
        for( const COMPONENT_CLASS* compClass : dynamicClass->GetConstituentClasses() )
            classNames.insert( compClass->GetName() );
    }

    if( classNames.empty() )
        return GetNoneComponentClass();

    if( classNames.size() == 1 )
    {
        wxASSERT( m_constituentClasses.contains( *classNames.begin() ) );
        return m_constituentClasses[*classNames.begin()].get();
    }

    const std::vector<wxString> sortedClassNames = sortClassNames( classNames );

    wxString fullCombinedName = GetFullClassNameForConstituents( sortedClassNames );

    if( !m_effectiveClasses.contains( fullCombinedName ) )
    {
        std::unique_ptr<COMPONENT_CLASS> combinedClass = std::make_unique<COMPONENT_CLASS>(
                fullCombinedName, COMPONENT_CLASS::USAGE::EFFECTIVE );

        for( const wxString& className : sortedClassNames )
        {
            wxASSERT( m_constituentClasses.contains( className ) );
            combinedClass->AddConstituentClass( m_constituentClasses[className].get() );
        }

        m_effectiveClasses[fullCombinedName] = std::move( combinedClass );
    }

    return m_effectiveClasses[fullCombinedName].get();
}


void COMPONENT_CLASS_MANAGER::InvalidateComponentClasses()
{
    ++m_ticker;
}


void COMPONENT_CLASS_MANAGER::ForceComponentClassRecalculation() const
{
    for( const auto& footprint : m_board->Footprints() )
    {
        footprint->RecomputeComponentClass();
    }
}


const COMPONENT_CLASS*
COMPONENT_CLASS_MANAGER::GetDynamicComponentClassesForFootprint( const FOOTPRINT* footprint )
{
    std::unordered_set<wxString> classNames;

    // Assemble matching component class names
    for( const auto& rule : m_assignmentRules )
    {
        if( rule->Matches( footprint ) )
            classNames.insert( rule->GetComponentClass() );
    }

    // Handle composite component classes
    const std::vector<wxString> sortedClassNames = sortClassNames( classNames );
    const wxString fullName = GetFullClassNameForConstituents( sortedClassNames );

    // No matching component classes
    if( classNames.empty() )
        return nullptr;

    // One matching component class
    if( classNames.size() == 1 )
        return getOrCreateConstituentClass( *classNames.begin(), COMPONENT_CLASS::USAGE::DYNAMIC );

    // Multiple matching component classes
    return getOrCreateEffectiveClass( sortedClassNames, COMPONENT_CLASS::USAGE::DYNAMIC );
}


std::vector<wxString>
COMPONENT_CLASS_MANAGER::sortClassNames( const std::unordered_set<wxString>& classNames )
{
    std::vector<wxString> sortedClassNames( classNames.begin(), classNames.end() );

    std::ranges::sort( sortedClassNames,
                       []( const wxString& str1, const wxString& str2 )
                       {
                           return str1.Cmp( str2 ) < 0;
                       } );

    return sortedClassNames;
}


COMPONENT_CLASS*
COMPONENT_CLASS_MANAGER::getOrCreateConstituentClass( const wxString&        aClassName,
                                                      COMPONENT_CLASS::USAGE aContext )
{
    if( aContext == COMPONENT_CLASS::USAGE::STATIC_AND_DYNAMIC
        || aContext == COMPONENT_CLASS::USAGE::EFFECTIVE )
    {
        wxFAIL_MSG( "Can't create a STATIC_AND_DYNAMIC or EFFECTIVE constituent component class" );
        return m_noneClass.get();
    }

    if( m_constituentClasses.contains( aClassName ) )
    {
        COMPONENT_CLASS* compClass = m_constituentClasses[aClassName].get();

        if( aContext != compClass->GetUsageContext() )
            compClass->SetUsageContext( COMPONENT_CLASS::USAGE::STATIC_AND_DYNAMIC );

        return compClass;
    }

    std::unique_ptr<COMPONENT_CLASS> newClass =
            std::make_unique<COMPONENT_CLASS>( aClassName, aContext );
    newClass->AddConstituentClass( newClass.get() );

    m_constituentClasses[aClassName] = std::move( newClass );

    return m_constituentClasses[aClassName].get();
}


COMPONENT_CLASS*
COMPONENT_CLASS_MANAGER::getOrCreateEffectiveClass( const std::vector<wxString>& aClassNames,
                                                    COMPONENT_CLASS::USAGE       aContext )
{
    wxString fullClassName = GetFullClassNameForConstituents( aClassNames );

    if( m_effectiveClasses.contains( fullClassName ) )
    {
        COMPONENT_CLASS* compClass = m_effectiveClasses[fullClassName].get();

        for( COMPONENT_CLASS* constituentClass : compClass->GetConstituentClasses() )
        {
            if( constituentClass->GetUsageContext() != aContext )
                constituentClass->SetUsageContext( COMPONENT_CLASS::USAGE::STATIC_AND_DYNAMIC );
        }
    }
    else
    {
        std::unique_ptr<COMPONENT_CLASS> effectiveClass = std::make_unique<COMPONENT_CLASS>(
                fullClassName, COMPONENT_CLASS::USAGE::EFFECTIVE );

        for( const wxString& className : aClassNames )
        {
            COMPONENT_CLASS* constituentClass = getOrCreateConstituentClass( className, aContext );
            effectiveClass->AddConstituentClass( constituentClass );
        }

        m_effectiveClasses[fullClassName] = std::move( effectiveClass );
    }

    return m_effectiveClasses[fullClassName].get();
}


std::unordered_set<wxString> COMPONENT_CLASS_MANAGER::GetClassNames() const
{
    std::unordered_set<wxString> classNames;

    for( const auto& className : m_constituentClasses | std::views::keys )
        classNames.insert( className );

    return classNames;
}


void COMPONENT_CLASS_MANAGER::RebuildRequiredCaches( FOOTPRINT* aFootprint ) const
{
    if( aFootprint )
    {
        aFootprint->BuildCourtyardCaches();
    }
    else
    {
        for( FOOTPRINT* fp : m_board->Footprints() )
            fp->BuildCourtyardCaches();
    }
}

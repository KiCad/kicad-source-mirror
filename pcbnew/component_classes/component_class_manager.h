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

#ifndef PCBNEW_COMPONENT_CLASS_MANAGER_H
#define PCBNEW_COMPONENT_CLASS_MANAGER_H

#include <unordered_map>
#include <unordered_set>
#include <wx/string.h>

#include <component_classes/component_class.h>
#include <project/component_class_settings.h>


class BOARD;
class COMPONENT_CLASS_ASSIGNMENT_RULE;
class DRC_TOOL;
class FOOTPRINT;

/**
 * A class to manage Component Classes in a board context
 *
 * This manager owns generated COMPONENT_CLASS objects, and guarantees that pointers to managed
 * objects are valid for the duration of the board lifetime. Note that, in order to maintain this
 * guarantee, there are two methods that must be called when updating the board from the netlist
 * (InitNetlistUpdate and FinishNetlistUpdate).
 */
class COMPONENT_CLASS_MANAGER
{
public:
    explicit COMPONENT_CLASS_MANAGER( BOARD* board );

    /// @brief Gets the full effective class name for the given set of constituent classes
    static wxString
    GetFullClassNameForConstituents( const std::unordered_set<wxString>& classNames );

    /// @brief Gets the full effective class name for the given set of constituent classes
    /// @param classNames a sorted vector of consituent class names
    static wxString GetFullClassNameForConstituents( const std::vector<wxString>& classNames );

    /// @brief Gets an effective component class for the given constituent class names
    /// @param classNames The names of the constituent component classes
    /// @return Effective COMPONENT_CLASS object
    COMPONENT_CLASS*
    GetEffectiveStaticComponentClass( const std::unordered_set<wxString>& classNames );

    /// Returns the unassigned component class
    const COMPONENT_CLASS* GetNoneComponentClass() const { return m_noneClass.get(); }

    /// Prepare the manager for a board update
    /// Must be called prior to updating the PCB from the netlist
    void InitNetlistUpdate();

    /// Cleans up the manager after a board update
    /// Must be called after updating the PCB from the netlist
    void FinishNetlistUpdate();

    /// Fetches a read-only map of the fundamental component classes
    std::unordered_set<wxString> GetClassNames() const;

    /// Synchronises all dynamic component class assignment rules
    /// @returns false if rules fail to parse, true if successful
    bool SyncDynamicComponentClassAssignments(
            const std::vector<COMPONENT_CLASS_ASSIGNMENT_DATA>& aAssignments,
            bool aGenerateSheetClasses, const std::unordered_set<wxString>& aNewSheetPaths );

    /// Gets the dynamic component classes which match the given footprint
    const COMPONENT_CLASS* GetDynamicComponentClassesForFootprint( const FOOTPRINT* footprint );

    /// Gets the combined component class with the given static and dynamic constituent component classes
    const COMPONENT_CLASS* GetCombinedComponentClass( const COMPONENT_CLASS* staticClass,
                                                      const COMPONENT_CLASS* dynamicClass );

    /// Forces the component class for all footprints to be recalculated. This should be called before running DRC as
    /// checking for valid component class cache entries is threadsafe, but computing them is not. Blocking during this
    /// check would be a negative performance impact for DRC computation, so we force recalculation instead.
    void ForceComponentClassRecalculation() const;

    /// Gets the component class validity ticker
    /// Used to check validity of cached component classes
    long long int GetTicker() const { return m_ticker; }

    /// Invalidates any caches component classes and recomputes caches if required. This will force
    /// recomputation of component classes on next access
    void InvalidateComponentClasses();

    /// Rebuilds any caches that may be required by custom assignment rules
    /// @param fp the footprint to rebuild. If null, rebuilds all footprint caches
    void RebuildRequiredCaches( FOOTPRINT* aFootprint = nullptr ) const;

    /// Determines whether any custom dynamic rules have a custom assignment condition
    bool HasCustomAssignmentConditions() const { return m_hasCustomAssignmentConditions; }

    static std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>
    CompileAssignmentRule( const COMPONENT_CLASS_ASSIGNMENT_DATA& aAssignment );

protected:
    /// Sorts the given class names in to canonical order
    static std::vector<wxString> sortClassNames( const std::unordered_set<wxString>& classNames );

    /// Returns a constituent component class, re-using an existing instantiation where possible
    COMPONENT_CLASS* getOrCreateConstituentClass( const wxString&        aClassName,
                                                  COMPONENT_CLASS::USAGE aContext );

    /// Returns an effective component class for the given set of constituent class names
    /// Precondition: aClassNames is sorted by sortClassNames
    COMPONENT_CLASS* getOrCreateEffectiveClass( const std::vector<wxString>& aClassNames,
                                                COMPONENT_CLASS::USAGE       aContext );

    /// The board these component classes are assigned to / from
    BOARD* m_board;

    /// The class to represent an unassigned component class
    std::shared_ptr<COMPONENT_CLASS> m_noneClass;

    /// All individual component classes from static assignments
    std::unordered_map<wxString, std::unique_ptr<COMPONENT_CLASS>> m_constituentClasses;

    /// Generated effective (composite) static component classes
    std::unordered_map<wxString, std::unique_ptr<COMPONENT_CLASS>> m_effectiveClasses;

    /// Cache of in-use static component class names
    /// Used for cleanup following netlist updates
    std::unordered_set<wxString> m_staticClassNamesCache;


    /// Active component class assignment rules
    std::vector<std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE>> m_assignmentRules;

    /// Quick lookup of presence of custom dynamic assignment conditions
    bool m_hasCustomAssignmentConditions;

    /// Monotonically increasing ticker to test cached component class validity
    long long int m_ticker{ 0 };
};

#endif

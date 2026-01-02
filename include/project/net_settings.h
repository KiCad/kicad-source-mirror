/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_NET_SETTINGS_H
#define KICAD_NET_SETTINGS_H

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <netclass.h>
#include <settings/nested_settings.h>
#include <eda_pattern_match.h>

/**
 * NET_SETTINGS stores various net-related settings in a project context.  These settings are
 * accessible and editable from both the schematic and PCB editors.
 */
class KICOMMON_API NET_SETTINGS : public NESTED_SETTINGS
{
public:
    NET_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~NET_SETTINGS();

    bool operator==( const NET_SETTINGS& aOther ) const;

    bool operator!=( const NET_SETTINGS& aOther ) const { return !operator==( aOther ); }

    /// @brief Sets the default netclass for the project
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void SetDefaultNetclass( std::shared_ptr<NETCLASS> netclass );

    /// @brief Gets the default netclass for the project
    std::shared_ptr<NETCLASS> GetDefaultNetclass();

    /// @brief Determines if the given netclass exists
    bool HasNetclass( const wxString& netclassName ) const;

    /// @brief Sets the given netclass
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void SetNetclass( const wxString& netclassName, std::shared_ptr<NETCLASS>& netclass );

    /// @brief Sets all netclass
    /// Calling this method will reset the effective netclass calculation caches
    void SetNetclasses( const std::map<wxString, std::shared_ptr<NETCLASS>>& netclasses );

    /// @brief Gets all netclasses
    const std::map<wxString, std::shared_ptr<NETCLASS>>& GetNetclasses() const;

    /// @brief Gets all composite (multiple assignment / missing defaults) netclasses
    // Note the full connectivity or board net synchronisation must be run before calling
    // this, otherwise resolved netclasses may be missing
    const std::map<wxString, std::shared_ptr<NETCLASS>>& GetCompositeNetclasses() const;

    /// @brief Clears all netclasses
    /// Calling this method will reset the effective netclass calculation caches
    void ClearNetclasses();

    /// @brief Gets all current net name to netclasses assignments
    const std::map<wxString, std::set<wxString>>& GetNetclassLabelAssignments() const;

    /// @brief Clears all net name to netclasses assignments
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void ClearNetclassLabelAssignments();

    /// @brief Clears a specific net name to netclass assignment
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void ClearNetclassLabelAssignment( const wxString& netName );

    /// @brief Sets a net name to netclasses assignment
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void SetNetclassLabelAssignment( const wxString&           netName,
                                     const std::set<wxString>& netclasses );

    /// @brief Apppends to a net name to netclasses assignment
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void AppendNetclassLabelAssignment( const wxString&           netName,
                                        const std::set<wxString>& netclasses );

    /// @brief Determines if a given net name has netclasses assigned
    bool HasNetclassLabelAssignment( const wxString& netName ) const;

    /// @brief Sets a netclass pattern assignment
    /// Calling this method will reset the effective netclass calculation caches
    void SetNetclassPatternAssignment( const wxString& pattern, const wxString& netclass );

    /// @brief Sets all netclass pattern assignments
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void SetNetclassPatternAssignments(
            std::vector<std::pair<std::unique_ptr<EDA_COMBINED_MATCHER>, wxString>>&&
                    netclassPatterns );

    /// @brief Gets the netclass pattern assignments
    std::vector<std::pair<std::unique_ptr<EDA_COMBINED_MATCHER>, wxString>>&
    GetNetclassPatternAssignments();

    /// @brief Clears all netclass pattern assignments
    void ClearNetclassPatternAssignments();

    /// @brief Clears effective netclass cache for the given net
    void ClearCacheForNet( const wxString& netName );

    /// @brief Clears the effective netclass cache for all nets
    void ClearAllCaches();

    /// @brief Sets a net to color assignment
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void SetNetColorAssignment( const wxString& netName, const KIGFX::COLOR4D& color );

    /// @brief Gets all net name to color assignments
    const std::map<wxString, KIGFX::COLOR4D>& GetNetColorAssignments() const;

    /// @brief Clears all net name to color assignments
    /// Calling user is responsible for resetting the effective netclass calculation caches
    void ClearNetColorAssignments();

    /// @brief Determines if an effective netclass for the given net name has been cached
    bool HasEffectiveNetClass( const wxString& aNetName ) const;

    /// @brief Returns an already cached effective netclass for the given net name
    /// @return The netclass, or default netclass if not found
    std::shared_ptr<NETCLASS> GetCachedEffectiveNetClass( const wxString& aNetName ) const;

    /// @brief Fetches the effective (may be aggregate) netclass for the given net name
    // If the effective netclass has not been computed, it will be created and cached.
    std::shared_ptr<NETCLASS> GetEffectiveNetClass( const wxString& aNetName );

    /// @brief Recomputes the internal values of all aggregate effective netclasses
    /// Called when a value of a user-defined netclass changes, but the whole netclass list is not
    /// being recomputed.
    void RecomputeEffectiveNetclasses();

    /**
     * Get a NETCLASS object from a given Netclass name string
     *
     * @param aNetClassName the Netclass name to resolve
     * @return shared pointer to the requested NETCLASS object, or the default NETCLASS
    */
    std::shared_ptr<NETCLASS> GetNetClassByName( const wxString& aNetName ) const;

    /**
     * Parse a bus vector (e.g. A[7..0]) into name, begin, and end.
     *
     * Ensure that begin and end are positive and that end > begin.
     *
     * @param aBus is a bus vector label string
     * @param aName out is the bus name, e.g. "A"
     * @param aMemberList is a list of member strings, e.g. "A7", "A6", and so on
     * @return true if aBus was successfully parsed
     */
    static bool ParseBusVector( const wxString& aBus, wxString* aName,
                                std::vector<wxString>* aMemberList );

    /**
     * Parse a bus group label into the name and a list of components.
     *
     * @param aGroup is the input label, e.g. "USB{DP DM}"
     * @param name is the output group name, e.g. "USB"
     * @param aMemberList is a list of member strings, e.g. "DP", "DM"
     * @return true if aGroup was successfully parsed
     */
    static bool ParseBusGroup( const wxString& aGroup, wxString* name,
                               std::vector<wxString>* aMemberList );

    /**
     * Call a function for each member of an expanded bus pattern.
     *
     * Handles both vector buses (e.g., "IN[0..7]" -> "IN0", "IN1", ..., "IN7") and
     * bus groups (e.g., "PCI{A[0..1] B}" -> "A0", "A1", "B"). For nested buses,
     * recursively expands all levels.
     *
     * If the pattern is not a bus, calls the function once with the original pattern.
     *
     * @param aBusPattern the bus pattern to expand (e.g., "DATA[0..7]" or "BUS{A B[0..1]}")
     * @param aFunction function to call for each expanded member net name
     */
    static void ForEachBusMember( const wxString&                        aBusPattern,
                                  const std::function<void( const wxString& )>& aFunction );

private:
    bool migrateSchema0to1();
    bool migrateSchema1to2();
    bool migrateSchema2to3();
    bool migrateSchema3to4();
    bool migrateSchema4to5();

    /**
     * @brief Creates an effective aggregate netclass from the given constituent netclasses
     *
     * Takes the aggregate parameters from the constituent netclasses in priority order. If any
     * parameters are missing from the overall union, then they are filled from the default
     * netclass. Note that the netclasses vector will have the default netclass added if it is used
     * to provide missing defaults. The netclasses vector will be sorted by priority 1st and then
     * name alphabetically
     */
    void makeEffectiveNetclass( std::shared_ptr<NETCLASS>& effectiveNetclass,
                                std::vector<NETCLASS*>&    netclasses ) const;

    /// @brief Adds any missing fields to the given netclass from the default netclass
    /// @returns true if any fields were added from the default netclass
    bool addMissingDefaults( NETCLASS* nc ) const;

    /// @brief Adds a single pattern assignment without bus expansion (internal helper)
    void addSinglePatternAssignment( const wxString& pattern, const wxString& netclass );

    /// @brief The default netclass
    std::shared_ptr<NETCLASS> m_defaultNetClass;

    /// @brief Map of netclass names to netclass definitions
    std::map<wxString, std::shared_ptr<NETCLASS>> m_netClasses;

    /// @brief Map of net names to resolved netclasses
    std::map<wxString, std::set<wxString>> m_netClassLabelAssignments;

    /// @brief List of net class pattern assignments
    std::vector<std::pair<std::unique_ptr<EDA_COMBINED_MATCHER>, wxString>>
            m_netClassPatternAssignments;

    /// @brief Map of netclass names to netclass definitions for
    // composite (multiple netclass assignment / missing defaults) netclasses
    std::map<wxString, std::shared_ptr<NETCLASS>> m_compositeNetClasses;

    /// @brief Map of netclass names to netclass definitions for implicit netclasses
    ///
    /// Implicit netclasses are those which are in a netclass label, but which do not have a
    /// netclass definition in the netclass setup panel. They contribute as a constituent
    /// netclass to enable DRC rules and name resolution, but do not contribute parameters
    // to the effective netclasses which contain them.
    std::map<wxString, std::shared_ptr<NETCLASS>> m_impicitNetClasses;

    /// @brief Cache of nets to pattern-matched netclasses
    std::map<wxString, std::shared_ptr<NETCLASS>> m_effectiveNetclassCache;

    /**
     * A map of fully-qualified net names to colors used in the board context.
     * Since these color overrides are for the board, buses are not included here.
     * Only nets that the user has assigned custom colors to will be in this list.
     * Nets that no longer exist will be deleted during a netlist read in Pcbnew.
     */
    std::map<wxString, KIGFX::COLOR4D> m_netColorAssignments;

    // TODO: Add diff pairs, bus information, etc.
};

#endif // KICAD_NET_SETTINGS_H

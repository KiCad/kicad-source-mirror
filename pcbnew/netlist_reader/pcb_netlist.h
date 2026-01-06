/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras.
 * Copyright (C) 2013-2016 Wayne Stambaugh <stambaughw@gmail.com>.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_NETLIST_H
#define PCB_NETLIST_H

#include <boost/ptr_container/ptr_vector.hpp>
#include <wx/arrstr.h>
#include <json_common.h>
#include <unordered_set>

#include <lib_id.h>
#include <footprint.h>
#include <ctl_flags.h>
#include <case_insensitive_map.h>


class REPORTER;


/**
 * Used to store the component pin name to net name (and pin function)
 * associations stored in a netlist.
 */
class COMPONENT_NET
{
public:
    COMPONENT_NET() {}

    COMPONENT_NET( const wxString& aPinName, const wxString& aNetName,
                   const wxString& aPinFunction, const wxString& aPinType ) :
        m_pinName( aPinName ),
        m_netName( aNetName ),
        m_pinFunction( aPinFunction ),
        m_pinType( aPinType )
    {
    }

    const wxString& GetPinName() const { return m_pinName; }
    const wxString& GetNetName() const { return m_netName; }
    const wxString& GetPinFunction() const { return m_pinFunction; }
    const wxString& GetPinType() const { return m_pinType; }

    bool IsValid() const { return !m_pinName.IsEmpty(); }

    bool operator <( const COMPONENT_NET& aNet ) const
    {
        return m_pinName < aNet.m_pinName;
    }

    int Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl );

private:
    wxString m_pinName;
    wxString m_netName;
    wxString m_pinFunction;
    wxString m_pinType;
};


typedef std::vector< COMPONENT_NET > COMPONENT_NETS;


struct NETLIST_GROUP
{
    wxString name;
    KIID uuid;
    LIB_ID            libId;
    std::vector<KIID> members;
};

typedef boost::ptr_vector< NETLIST_GROUP > NETLIST_GROUPS;


struct COMPONENT_VARIANT
{
    COMPONENT_VARIANT( const wxString& aName = wxEmptyString ) : m_name( aName ) {}

    wxString m_name;
    bool     m_dnp = false;
    bool     m_excludedFromBOM = false;
    bool     m_excludedFromSim = false;
    bool     m_excludedFromPosFiles = false;

    bool     m_hasDnp = false;
    bool     m_hasExcludedFromBOM = false;
    bool     m_hasExcludedFromSim = false;
    bool     m_hasExcludedFromPosFiles = false;

    nlohmann::ordered_map<wxString, wxString> m_fields;
};


/**
 * Store all of the related footprint information found in a netlist.
 */
class COMPONENT
{
public:
    COMPONENT( const LIB_ID&            aFPID,
               const wxString&          aReference,
               const wxString&          aValue,
               const KIID_PATH&         aPath,
               const std::vector<KIID>& aKiids )
    {
        m_fpid             = aFPID;
        m_reference        = aReference;
        m_value            = aValue;
        m_pinCount         = 0;
        m_path             = aPath;
        m_kiids            = aKiids;
        m_duplicatePadNumbersAreJumpers = false;
        m_group            = nullptr;
    }

    virtual ~COMPONENT() { };

    void AddNet( const wxString& aPinName, const wxString& aNetName, const wxString& aPinFunction,
                 const wxString& aPinType )
    {
        m_nets.emplace_back( aPinName, aNetName, aPinFunction, aPinType );
    }

    unsigned GetNetCount() const { return m_nets.size(); }

    const COMPONENT_NET& GetNet( unsigned aIndex ) const { return m_nets[aIndex]; }

    const COMPONENT_NET& GetNet( const wxString& aPinName ) const;

    void ClearNets() { m_nets.clear(); }

    void SortPins() { sort( m_nets.begin(), m_nets.end() ); }

    void SetName( const wxString& aName ) { m_name = aName;}
    const wxString& GetName() const { return m_name; }

    void SetLibrary( const wxString& aLibrary ) { m_library = aLibrary; }
    const wxString& GetLibrary() const { return m_library; }

    void SetReference( const wxString& aReference ) { m_reference = aReference; }
    const wxString& GetReference() const { return m_reference; }

    void SetValue( const wxString& aValue ) { m_value = aValue; }
    const wxString& GetValue() const { return m_value; }

    void SetFields( nlohmann::ordered_map<wxString, wxString> aFields )
    {
        m_fields = std::move( aFields );
    }
    const nlohmann::ordered_map<wxString, wxString>& GetFields() const { return m_fields; }

    void SetProperties( std::map<wxString, wxString> aProps )
    {
        m_properties = std::move( aProps );
    }
    const std::map<wxString, wxString>& GetProperties() const { return m_properties; }

    void SetFPID( const LIB_ID& aFPID ) { m_fpid = aFPID;  }
    const LIB_ID& GetFPID() const { return m_fpid; }

    void SetAltFPID( const LIB_ID& aFPID ) { m_altFpid = aFPID; }
    const LIB_ID& GetAltFPID() const { return m_altFpid; }

    const KIID_PATH& GetPath() const { return m_path; }

    const std::vector<KIID>& GetKIIDs() const { return m_kiids; }

    void SetFootprintFilters( const wxArrayString& aFilters ) { m_footprintFilters = aFilters; }
    const wxArrayString& GetFootprintFilters() const { return m_footprintFilters; }

    void SetPinCount( int aPinCount ) { m_pinCount = aPinCount; }
    int GetPinCount() const { return m_pinCount; }

    FOOTPRINT* GetFootprint( bool aRelease = false )
    {
        return ( aRelease ) ? m_footprint.release() : m_footprint.get();
    }

    void SetFootprint( FOOTPRINT* aFootprint );

    bool IsLibSource( const wxString& aLibrary, const wxString& aName ) const
    {
        return aLibrary == m_library && aName == m_name;
    }

    void Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl );

    void SetHumanReadablePath( const wxString& aPath ) { m_humanReadablePath = aPath; }
    const wxString& GetHumanReadablePath() const { return m_humanReadablePath; }

    void SetComponentClassNames( const std::unordered_set<wxString>& aClassNames )
    {
        m_componentClassNames = aClassNames;
    }

    std::unordered_set<wxString>& GetComponentClassNames() { return m_componentClassNames; }

    bool GetDuplicatePadNumbersAreJumpers() const { return m_duplicatePadNumbersAreJumpers; }
    void SetDuplicatePadNumbersAreJumpers( bool aEnabled ) { m_duplicatePadNumbersAreJumpers = aEnabled; }

    std::vector<std::set<wxString>>& JumperPadGroups() { return m_jumperPadGroups; }
    const std::vector<std::set<wxString>>& JumperPadGroups() const { return m_jumperPadGroups; }

    NETLIST_GROUP* GetGroup() const { return m_group; }
    void SetGroup( NETLIST_GROUP* aGroup ) { m_group = aGroup; }

    // Unit info for multi-unit symbols
    struct UNIT_INFO
    {
        wxString              m_unitName; // e.g. A
        std::vector<wxString> m_pins;     // pin numbers in this unit
    };

    void                          SetUnitInfo( const std::vector<UNIT_INFO>& aUnits ) { m_units = aUnits; }
    const std::vector<UNIT_INFO>& GetUnitInfo() const { return m_units; }

    const CASE_INSENSITIVE_MAP<COMPONENT_VARIANT>& GetVariants() const { return m_variants; }
    const COMPONENT_VARIANT* GetVariant( const wxString& aVariantName ) const;
    COMPONENT_VARIANT* GetVariant( const wxString& aVariantName );
    void AddVariant( const COMPONENT_VARIANT& aVariant );

private:
    std::vector<COMPONENT_NET>   m_nets;  ///< list of nets shared by the component pins

    wxArrayString                m_footprintFilters;
    int                          m_pinCount;
    wxString                     m_reference;
    wxString                     m_value;

    // human-readable hierarchical sheet path (e.g. /root/block0/sheet1)
    wxString                     m_humanReadablePath;

    /// A fully specified path to the component (but not the component: [ sheetUUID, sheetUUID, .. ]
    KIID_PATH                    m_path;

    /// A vector of possible KIIDs corresponding to all units in a symbol
    std::vector<KIID>            m_kiids;

    /// The name of the component in #m_library used when it was placed on the schematic.
    wxString                     m_name;

    /// The name of the component library where #m_name was found.
    wxString                     m_library;

    /// The #LIB_ID of the footprint assigned to the component.
    LIB_ID                       m_fpid;

    /// The alt LIB_ID of the footprint, when there are 2 different assigned footprints,
    /// One from the netlist, the other from the .cmp file.
    /// this one is a copy of the netlist footprint assignment
    LIB_ID                       m_altFpid;

    /// The #FOOTPRINT loaded for #m_FPID.
    std::unique_ptr<FOOTPRINT>   m_footprint;

    /// Component-specific properties found in the netlist.
    std::map<wxString, wxString> m_properties;

    /// Component-specific user fields found in the netlist.
    nlohmann::ordered_map<wxString, wxString> m_fields;

    /// Component classes for this footprint
    std::unordered_set<wxString> m_componentClassNames;

    /// Jumper pad groups for this footprint
    std::vector<std::set<wxString>> m_jumperPadGroups;

    /// Flag that this footprint should automatically treat sets of two or more pads with the same
    /// number as jumpered pin groups
    bool m_duplicatePadNumbersAreJumpers;

    /// Group membership for this footprint. Nullptr if none.
    NETLIST_GROUP*               m_group;

    static COMPONENT_NET         m_emptyNet;

    // Unit information parsed from the netlist (optional)
    std::vector<UNIT_INFO> m_units;

    CASE_INSENSITIVE_MAP<COMPONENT_VARIANT> m_variants;
};


typedef boost::ptr_vector< COMPONENT > COMPONENTS;


/**
 * Store information read from a netlist along with the flags used to update the NETLIST in the
 * #BOARD.
 */
class NETLIST
{
public:
    NETLIST() :
        m_findByTimeStamp( false ),
        m_replaceFootprints( false )
    {
    }

    /**
     * @return true if there are no components in the netlist.
     */
    bool IsEmpty() const { return m_components.empty(); }

    /**
     * Remove all components from the netlist.
     */
    void Clear() { m_components.clear(); }

    /**
     * @return the number of components in the netlist.
     */
    unsigned GetCount() const { return m_components.size(); }

    /**
     * Return the #COMPONENT at \a aIndex.
     *
     * @param aIndex the index in #m_components to fetch.
     * @return a pointer to the #COMPONENT at \a Index.
     */
    COMPONENT* GetComponent( unsigned aIndex ) { return &m_components[ aIndex ]; }

    /**
     * Add \a aComponent to the NETLIST.
     *
     * @note If \a aComponent already exists in the NETLIST, \a aComponent is deleted
     *       to prevent memory leaks.  An assertion is raised in debug builds.
     *
     * @param aComponent is the COMPONENT to save to the NETLIST.
     */
    void AddComponent( COMPONENT* aComponent );

    /**
     * @note If \a aGroup already exists in the NETLIST, \a aGroup is deleted
     *       to prevent memory leaks.  An assertion is raised in debug builds.
     *
     * @param aGroup is the NETLIST_GROUP to save to the NETLIST.
     */
    void AddGroup( NETLIST_GROUP* aGroup );

    /**
     * @brief Return a #NETLIST_GROUP by \a aUuid.
     *
     * @param aUuid is the KIID of the #NETLIST_GROUP.
     *
     * @return a pointer to the #NETLIST_GROUP that matches \a aUuid if found.  Otherwise NULL.
     */
    NETLIST_GROUP* GetGroupByUuid( const KIID& aUuid );

    /**
     * After groups and components are parsed, apply the group memberships to the
     * internal components based on the group member UUIDs.
     */
    void ApplyGroupMembership();

    /**
     * Return a #COMPONENT by \a aReference.
     *
     * @param aReference is the reference designator the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aReference if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByReference( const wxString& aReference );

    /**
     * Return a #COMPONENT by \a aPath.
     *
     * @param aPath is the KIID_PATH [ sheetUUID, .., compUUID ] of the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aPath if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByPath( const KIID_PATH& aPath );

    /**
     * Return a #COMPONENT by \a aUuid.
     *
     * @param aUuid is the KIID of the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aUuid if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByUuid( const KIID& aUuid );

    void SortByFPID();
    void SortByReference();

    void SetFindByTimeStamp( bool aFindByTimeStamp ) { m_findByTimeStamp = aFindByTimeStamp; }
    bool IsFindByTimeStamp() const { return m_findByTimeStamp; }

    void SetReplaceFootprints( bool aReplace ) { m_replaceFootprints = aReplace; }
    bool GetReplaceFootprints() const { return m_replaceFootprints; }

    /**
     * @return true if any component with a footprint link is found.
     */
    bool AnyFootprintsLinked() const;

    void Format( const char* aDocName, OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl = 0 );

#define CTL_FOR_CVPCB    (CTL_OMIT_NETS | CTL_OMIT_FILTERS | CTL_OMIT_EXTRA)

    void FormatCvpcbNetlist( OUTPUTFORMATTER* aOut )
    {
        Format( "cvpcb_netlist", aOut, 0, CTL_FOR_CVPCB );
    }

    /**
     * @return a list of variant names found in the netlist.
     */
    const std::vector<wxString>& GetVariantNames() const { return m_variantNames; }

    /**
     * Add a variant name to the netlist.
     */
    void AddVariant( const wxString& aName, const wxString& aDescription = wxEmptyString )
    {
        if( aName.IsEmpty() )
            return;

        for( const wxString& existingName : m_variantNames )
        {
            if( existingName.CmpNoCase( aName ) == 0 )
            {
                if( aDescription.IsEmpty() )
                    m_variantDescriptions.erase( existingName );
                else
                    m_variantDescriptions[existingName] = aDescription;

                return;
            }
        }

        m_variantNames.push_back( aName );

        if( aDescription.IsEmpty() )
            return;

        m_variantDescriptions[aName] = aDescription;
    }

    /**
     * @return the description for a given variant, or empty string if not found.
     */
    wxString GetVariantDescription( const wxString& aVariantName ) const
    {
        wxString actualName;

        for( const wxString& existingName : m_variantNames )
        {
            if( existingName.CmpNoCase( aVariantName ) == 0 )
            {
                actualName = existingName;
                break;
            }
        }

        if( !actualName.IsEmpty() )
        {
            auto it = m_variantDescriptions.find( actualName );

            if( it != m_variantDescriptions.end() )
                return it->second;
        }

        return wxEmptyString;
    }

    /**
     * @return a map of all variant descriptions.
     */
    const std::map<wxString, wxString>& GetVariantDescriptions() const
    {
        return m_variantDescriptions;
    }

private:
    COMPONENTS m_components;          // Components found in the netlist.
    NETLIST_GROUPS m_groups;          // Groups found in the netlist.

    std::vector<wxString>            m_variantNames;         // Variant names in order.
    std::map<wxString, wxString>     m_variantDescriptions;  // Variant descriptions.

    bool       m_findByTimeStamp;     // Associate components by KIID (or refdes if false)
    bool       m_replaceFootprints;   // Update footprints to match footprints defined in netlist
};


#endif   // PCB_NETLIST_H

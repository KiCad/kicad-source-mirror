#ifndef PCB_NETLIST_H
#define PCB_NETLIST_H

/**
 * @file pcb_netlist.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras.
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>.
 * Copyright (C) 2012 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <boost/ptr_container/ptr_vector.hpp>
#include <wx/arrstr.h>

#include <fpid.h>


class MODULE;
class REPORTER;


/**
 * Class COMPONENT_NET
 * is used to store the component pin name to net name associations stored in a netlist.
 */
class COMPONENT_NET
{
    wxString m_pinName;
    wxString m_netName;

public:
    COMPONENT_NET() {}

    COMPONENT_NET( const wxString& aPinName, const wxString& aNetName ) :
        m_pinName( aPinName ), m_netName( aNetName )
    {
    }

    const wxString& GetPinName() const { return m_pinName; }

    const wxString& GetNetName() const { return m_netName; }

    bool IsValid() const { return !m_pinName.IsEmpty(); }

    bool operator <( const COMPONENT_NET& aNet ) const
    {
        return m_pinName < aNet.m_pinName;
    }

    int Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl );
};


typedef std::vector< COMPONENT_NET > COMPONENT_NETS;


/**
 * Class COMPONENT
 * is used to store components and all of their related information found in a netlist.
 */
class COMPONENT
{
    COMPONENT_NETS m_nets;
    wxArrayString  m_footprintFilters; ///< Footprint filters found in netlist.
    wxString       m_reference;        ///< The component reference designator found in netlist.
    wxString       m_value;            ///< The component value found in netlist.

    // ZZZ This timestamp is string, not time_t
    wxString       m_timeStamp;        ///< The component full time stamp found in netlist.

    /// The name of the component in #m_library used when it was placed on the schematic..
    wxString       m_name;

    /// The name of the component library where #m_name was found.
    wxString       m_library;

    /// The #FPID of the footprint assigned to the component.
    FPID           m_fpid;

    /// The #MODULE loaded for #m_fpid.
    std::auto_ptr< MODULE > m_footprint;

    /// Set to true if #m_fpid was changed when the footprint link file was read.
    bool           m_footprintChanged;

    static COMPONENT_NET    m_emptyNet;

public:
    COMPONENT( const FPID&     aFPID,
               const wxString& aReference,
               const wxString& aValue,
               const wxString& aTimeStamp )
    {
        m_fpid             = aFPID;
        m_reference        = aReference;
        m_value            = aValue;
        m_timeStamp        = aTimeStamp;
        m_footprintChanged = false;
    }

    virtual ~COMPONENT() { };

    void AddNet( const wxString& aPinName, const wxString& aNetName )
    {
        m_nets.push_back( COMPONENT_NET( aPinName, aNetName ) );
    }

    unsigned GetNetCount() const { return m_nets.size(); }

    const COMPONENT_NET& GetNet( unsigned aIndex ) const { return m_nets[aIndex]; }

    const COMPONENT_NET& GetNet( const wxString& aPinName );

    void SortPins() { sort( m_nets.begin(), m_nets.end() ); }

    void SetName( const wxString& aName ) { m_name = aName;}
    const wxString& GetName() const { return m_name; }

    void SetLibrary( const wxString& aLibrary ) { m_library = aLibrary; }
    const wxString& GetLibrary() const { return m_library; }

    const wxString& GetReference() const { return m_reference; }

    const wxString& GetValue() const { return m_value; }

    void SetFPID( const FPID& aFPID )
    {
        m_footprintChanged = !m_fpid.empty() && (m_fpid != aFPID);
        m_fpid = aFPID;
    }

    const FPID& GetFPID() const { return m_fpid; }

    const wxString& GetTimeStamp() const { return m_timeStamp; }

    void SetFootprintFilters( const wxArrayString& aFilterList )
    {
        m_footprintFilters = aFilterList;
    }

    const wxArrayString& GetFootprintFilters() const { return m_footprintFilters; }

    /**
     * Function MatchesFootprintFilters
     *
     * @return true if \a aFootprintName matches any of the footprint filters or no footprint
     *         filters are defined.
     */
    bool MatchesFootprintFilters( const wxString& aFootprintName ) const;

    MODULE* GetModule( bool aRelease = false )
    {
        return ( aRelease ) ? m_footprint.release() : m_footprint.get();
    }

    void SetModule( MODULE* aModule );

    bool IsLibSource( const wxString& aLibrary, const wxString& aName ) const
    {
        return aLibrary == m_library && aName == m_name;
    }

    bool FootprintChanged() const { return m_footprintChanged; }

    void Format( OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl );
};


typedef boost::ptr_vector< COMPONENT > COMPONENTS;
typedef COMPONENTS::iterator           COMPONENTS_ITER;
typedef COMPONENTS::const_iterator     COMPONENTS_CITER;


/**
 * Class NETLIST
 * stores all of information read from a netlist along with the flags used to update
 * the NETLIST in the #BOARD.
 */
class NETLIST
{
    COMPONENTS         m_components;           ///< Components found in the netlist.

    /// Remove footprints from #BOARD not found in netlist when true.
    bool               m_deleteExtraFootprints;

    /// Do not actually make any changes.  Only report changes to #BOARD from netlist
    /// when true.
    bool               m_isDryRun;

    /// Find component by time stamp if true or reference designator if false.
    bool               m_findByTimeStamp;

    /// Replace component footprints when they differ from the netlist if true.
    bool               m_replaceFootprints;

public:
    NETLIST() :
        m_deleteExtraFootprints( false ),
        m_isDryRun( false ),
        m_findByTimeStamp( false ),
        m_replaceFootprints( false )
    {
    }

    /**
     * Function IsEmpty()
     * @return true if there are no components in the netlist.
     */
    bool IsEmpty() const { return m_components.empty(); }

    /**
     * Function Clear
     * removes all components from the netlist.
     */
    void Clear() { m_components.clear(); }

    /**
     * Function GetCount
     * @return the number of components in the netlist.
     */
    unsigned GetCount() const { return m_components.size(); }

    /**
     * Function GetComponent
     * returns the #COMPONENT at \a aIndex.
     *
     * @param aIndex the index in #m_components to fetch.
     * @return a pointer to the #COMPONENT at \a Index.
     */
    COMPONENT* GetComponent( unsigned aIndex ) { return &m_components[ aIndex ]; }

    /**
     * Function AddComponent
     * adds \a aComponent to the NETLIST.
     *
     * @note If \a aComponent already exists in the NETLIST, \a aComponent is deleted
     *       to prevent memory leaks.  An assertion is raised in debug builds.
     *
     * @param aComponent is the COMPONENT to save to the NETLIST.
     */
    void AddComponent( COMPONENT* aComponent );

    /*
     * Function GetComponentByReference
     * returns a #COMPONENT by \a aReference.
     *
     * @param aReference is the reference designator the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aReference if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByReference( const wxString& aReference );

    /*
     * Function GetComponentByTimeStamp
     * returns a #COMPONENT by \a aTimeStamp.
     *
     * @param aTimeStamp is the time stamp the #COMPONENT.
     * @return a pointer to the #COMPONENT that matches \a aTimeStamp if found.  Otherwise NULL.
     */
    COMPONENT* GetComponentByTimeStamp( const wxString& aTimeStamp );

    void SortByFPID();

    void SortByReference();

    void SetDeleteExtraFootprints( bool aDeleteExtraFootprints )
    {
        m_deleteExtraFootprints = aDeleteExtraFootprints;
    }

    bool GetDeleteExtraFootprints() const { return m_deleteExtraFootprints; }

    void SetIsDryRun( bool aIsDryRun ) { m_isDryRun = aIsDryRun; }

    bool IsDryRun() const { return m_isDryRun; }

    void SetFindByTimeStamp( bool aFindByTimeStamp ) { m_findByTimeStamp = aFindByTimeStamp; }

    bool IsFindByTimeStamp() const { return m_findByTimeStamp; }

    void SetReplaceFootprints( bool aReplaceFootprints )
    {
        m_replaceFootprints = aReplaceFootprints;
    }

    bool GetReplaceFootprints() const { return m_replaceFootprints; }

    /**
     * Function AnyFootprintsLinked
     * @return true if any component with a footprint link is found.
     */
    bool AnyFootprintsLinked() const;

    /**
     * Function AllFootprintsLinked
     * @return true if all components have a footprint link.
     */
    bool AllFootprintsLinked() const;

    /**
     * Function NoFootprintsLinked
     * @return true if none of the components have a footprint link.
     */
    bool NoFootprintsLinked() const { return !AnyFootprintsLinked(); }

    /**
     * Function AnyFootprintsChanged
     * @return true if any components footprints were changed when the footprint link file
     *         (*.cmp)  was loaded.
     */
    bool AnyFootprintsChanged() const;

    void Format( const char* aDocName, OUTPUTFORMATTER* aOut, int aNestLevel, int aCtl = 0 );

#define CTL_OMIT_EXTRA      (1<<0)
#define CTL_OMIT_NETS       (1<<1)
#define CTL_OMIT_FILTERS    (1<<2)

#define CTL_FOR_BACKANNO    (CTL_OMIT_NETS | CTL_OMIT_FILTERS | CTL_OMIT_EXTRA)

    void FormatBackAnnotation( OUTPUTFORMATTER* aOut )
    {
        Format( "back_annotation", aOut, 0, CTL_FOR_BACKANNO );
    }
};


#endif   // PCB_NETLIST_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _SCH_CONNECTION_H
#define _SCH_CONNECTION_H

#include <memory>
#include <unordered_set>

#include <wx/regex.h>
#include <bus_alias.h>
#include <sch_sheet_path.h>
#include <widgets/msgpanel.h>


class CONNECTION_GRAPH;
class SCH_ITEM;
class SCH_SHEET_PATH;


enum class CONNECTION_TYPE
{
    NONE,      ///< No connection to this item
    NET,       ///< This item represents a net
    BUS,       ///< This item represents a bus vector
    BUS_GROUP, ///< This item represents a bus group
};

/**
 * Each graphical item can have a SCH_CONNECTION describing its logical
 * connection (to a bus or net).  These are generated when netlisting, or when
 * editing operations that can change the netlist are performed.
 *
 * In hierarchical schematics, a single SCH_ITEM object can refer to multiple
 * distinct parts of a design (in the case of a sub-sheet that is instanced
 * more than once in a higher level sheet).  Because of this, a single item may
 * contain more than one SCH_CONNECTION -- each is specific to a sheet.
 *
 * Symbols contain connections for each of their pins (and for each sheet they
 * exist on) but don't use their own connection object.
 */
class SCH_CONNECTION
{
public:
    SCH_CONNECTION( SCH_ITEM* aParent = nullptr, const SCH_SHEET_PATH& aPath = SCH_SHEET_PATH() );

    SCH_CONNECTION( SCH_CONNECTION& aOther );

    SCH_CONNECTION( CONNECTION_GRAPH* aGraph );

    ~SCH_CONNECTION()
    {}

    /**
     * Note: the equality operator for SCH_CONNECTION only tests the net
     * properties, not the ownership / sheet location!
     */
    bool operator==( const SCH_CONNECTION& aOther ) const;

    bool operator!=( const SCH_CONNECTION& aOther ) const;

    void SetGraph( CONNECTION_GRAPH* aGraph )
    {
        m_graph = aGraph;
    }

    /**
     * Configures the connection given a label.
     * For CONNECTION_NET, this just sets the name.
     * For CONNECTION_BUS, this will deduce the correct BUS_TYPE and also
     * generate a correct list of members.
     */
    void ConfigureFromLabel( const wxString& aLabel );

    /**
     * Clears connectivity information
     */
    void Reset();

    /**
     * Copies connectivity information (but not parent) from another connection
     *
     * @param aOther is the connection to clone
     */
    void Clone( const SCH_CONNECTION& aOther );

    SCH_ITEM* Parent() const { return m_parent; }

    SCH_ITEM* Driver() const { return m_driver; }
    void SetDriver( SCH_ITEM* aItem );

    SCH_SHEET_PATH Sheet() const { return m_sheet; }
    void SetSheet( const SCH_SHEET_PATH& aSheet );

    SCH_SHEET_PATH LocalSheet() const { return m_local_sheet; }

    /**
     * Checks if the SCH_ITEM this connection is attached to can drive connections
     * Drivers can be labels, sheet pins, or symbol pins.
     *
     * @return true if the attached items is a driver
     */
    bool IsDriver() const;

    bool IsBus() const
    {
        return ( m_type == CONNECTION_TYPE::BUS || m_type == CONNECTION_TYPE::BUS_GROUP );
    }

    bool IsNet() const
    {
        return ( m_type == CONNECTION_TYPE::NET );
    }

    bool IsUnconnected() const { return ( m_type == CONNECTION_TYPE::NONE ); }

    bool IsDirty() const { return m_dirty; }
    void SetDirty() { m_dirty = true; }
    void ClearDirty() { m_dirty = false; }

    bool HasDriverChanged() const;
    void ClearDriverChanged();
    void* GetLastDriver() const { return m_lastDriver; }

    wxString Name( bool aIgnoreSheet = false ) const;

    wxString LocalName() const { return m_local_name; }

    wxString FullLocalName() const
    {
        return m_local_prefix + m_local_name + m_suffix;
    }

    void SetName( const wxString& aName )
    {
        m_name = aName;
        recacheName();
    }

    wxString GetNetName() const;

    wxString Prefix() const { return m_prefix; }
    void SetPrefix( const wxString& aPrefix );

    wxString BusPrefix() const { return m_bus_prefix; }

    wxString Suffix() const { return m_suffix; }
    void SetSuffix( const wxString& aSuffix );

    CONNECTION_TYPE Type() const { return m_type; }

    void SetType( CONNECTION_TYPE aType )
    {
        m_type = aType;
        recacheName();
    }

    int NetCode() const { return m_net_code; }
    void SetNetCode( int aCode ) { m_net_code = aCode; }

    int BusCode() const { return m_bus_code; }
    void SetBusCode( int aCode ) { m_bus_code = aCode; }

    int SubgraphCode() const { return m_subgraph_code; }
    void SetSubgraphCode( int aCode ) { m_subgraph_code = aCode; }

    long VectorStart() const { return m_vector_start; }
    long VectorEnd() const { return m_vector_end; }

    long VectorIndex() const { return m_vector_index; }

    wxString VectorPrefix() const { return m_vector_prefix; }

    const std::vector< std::shared_ptr< SCH_CONNECTION > >& Members() const
    {
        return m_members;
    }

    const std::vector< std::shared_ptr< SCH_CONNECTION > > AllMembers() const;

    static wxString PrintBusForUI( const wxString& aString );

    /**
     * Returns true if this connection is contained within aOther (but not the same as aOther)
     * @return true if this connection is a member of aOther
     */
    bool IsSubsetOf( SCH_CONNECTION* aOther ) const;

    /**
     * Returns true if this connection is a member of bus connection aOther
     *
     * Will always return false if aOther is not a bus connection
     */
    bool IsMemberOfBus( SCH_CONNECTION* aOther ) const;

    /**
     * Adds information about the connection object to aList
     */
    void AppendInfoToMsgPanel( std::vector<MSG_PANEL_ITEM>& aList ) const;

    /**
     * Test if \a aLabel has a bus notation.
     *
     * @param aLabel A wxString object containing the label to test.
     * @return true if text is a bus notation format otherwise false is returned.
     */
    static bool IsBusLabel( const wxString& aLabel );

    /**
     * Test if \a aLabel looks like a bus notation.
     * This check is much less expensive than IsBusLabel.
     *
     * @param aLabel A wxString object containing the label to test.
     * @return true if text might be a bus label
     */
    static bool MightBeBusLabel( const wxString& aLabel );

private:
    void recacheName();

    bool m_dirty;

    SCH_SHEET_PATH m_sheet; ///< The hierarchical sheet this connection is on

    /**
     * When a connection is overridden by one on a different hierarchical sheet, it will be cloned
     * and m_sheet will point to the parent sheet.  This member stores the original sheet so that it
     * can be used for applications such as net highlighting to retrieve the sheet of the parent
     * item from the highlighted connection.
     */
    SCH_SHEET_PATH m_local_sheet;

    SCH_ITEM* m_parent;     ///< The SCH_ITEM this connection is owned by

    void*     m_lastDriver; ///< WEAK POINTER (there is no guarantee it is still allocated)
                            ///< Equality comparisons are OK, but that's pretty much it

    SCH_ITEM* m_driver;     ///< The SCH_ITEM that drives this connection's net

    CONNECTION_TYPE m_type; ///< @see enum CONNECTION_TYPE

    wxString m_name;        ///< Name of the connection.

    wxString m_cached_name; ///< Full name, including prefix and suffix

    wxString m_cached_name_with_path; ///< Full name including sheet path (if not global)

    /**
     * For bus members, we want to keep track of the "local" name of a member, that is,
     * the name it takes on from its parent bus name.  This is because we always want to use
     * the local name for bus unfolding, matching within buses, etc.  The actual resolved name
     * of this bus member might change, for example if it's connected elsewhere to some other
     * item with higher priority.
     */
    wxString m_local_name;

    /// Prefix if connection is member of a labeled bus group (or "" if not)
    wxString m_prefix;

    /// Local prefix for group bus members (used with m_local_name)
    wxString m_local_prefix;

    /// Optional prefix of a bus group (always empty for nets and vector buses)
    wxString m_bus_prefix;

    wxString m_suffix;      ///< Name suffix (used only for disambiguation)

    int m_net_code;         // TODO(JE) remove if unused

    int m_bus_code;         // TODO(JE) remove if unused

    int m_subgraph_code;    ///< Groups directly-connected items

    long m_vector_index;    ///< Index of bus vector member nets

    long m_vector_start;    ///< Highest member of a vector bus

    long m_vector_end;      ///< Lowest member of a vector bus

    /// Prefix name of the vector, if m_type == CONNECTION_BUS (or "" if not).
    wxString m_vector_prefix;

    /**
     * For bus connections, store a list of member connections
     *
     * NOTE: All connections that Clone() others share the list of member
     * pointers.  This seems fine at the moment.
     */
    std::vector< std::shared_ptr< SCH_CONNECTION > > m_members;

    /**
     * Pointer to the connection graph for the schematic this connection exists on.
     * Needed for bus alias lookups.
     */
    CONNECTION_GRAPH* m_graph;

};

#endif


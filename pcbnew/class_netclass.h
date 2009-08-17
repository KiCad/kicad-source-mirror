
/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2009 Kicad Developers, see change_log.txt for contributors.
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


#ifndef CLASS_NETCLASS_H
#define CLASS_NETCLASS_H

#include <set>
#include <map>


/**
 * Class NETCLASS
 * handles a collection of nets and the parameters used to route or
 * test these nets.
 */
class NETCLASS
{
protected:

    BOARD*      m_Parent;
    wxString    m_Name;                 ///< Name of the net class
    wxString    m_Description;          ///< what this NETCLASS is for.

    typedef std::set<wxString>       STRINGSET;

    STRINGSET   m_Members;              ///< names of NET members of this class

    /// The units on these parameters is 1/10000 of an inch.

    int         m_TrackWidth;           ///< value for tracks thickness used to route this net
    int         m_TrackMinWidth;        ///< minimum value for tracks thickness (used in DRC)
    int         m_ViaSize;              ///< default via size used to route this net
    int         m_ViaDrillSize;         ///< default via drill size used to create vias in this net
    int         m_ViaMinSize;           ///< minimum size for vias (used in DRC)
    int         m_Clearance;            ///< clearance when routing

public:

    static const wxString Default;      ///< the name of the default NETCLASS

    NETCLASS( BOARD* aParent, const wxString& aName );
    ~NETCLASS();

    wxString GetClass() const
    {
        return wxT( "NETCLASS" );
    }

    const wxString& GetName() const
    {
        return m_Name;
    }

    /**
     * Function GetCount
     * returns the number of nets in this NETCLASS, i.e. using these rules.
     */
    unsigned GetCount() const
    {
        return m_Members.size();
    }


    /**
     * Function Clear
     * empties the collection of members.
     */
    void Clear()
    {
        m_Members.clear();
    }


    /**
     * Function AddMember
     * adds \a aNetname to this NETCLASS if it is not already in this NETCLASS.
     * It is harmless to try and add a second identical name.
     */
    void Add( const wxString& aNetname )
    {
        m_Members.insert( aNetname );
    }

    typedef STRINGSET::iterator iterator;
    iterator begin() { return m_Members.begin(); }
    iterator end()   { return m_Members.end();   }

    typedef STRINGSET::const_iterator const_iterator;
    const_iterator begin() const { return m_Members.begin(); }
    const_iterator end()   const { return m_Members.end();   }

    /**
     * Function Remove
     * will remove NET name \a aName from the collection of members.
     */
    void Remove( iterator aName )
    {
        m_Members.erase( aName );
    }

    /**
     * Function Remove
     * will remove NET name \a aName from the collection of members.
     */
    void Remove( const wxString& aName )
    {
        m_Members.erase( aName );
    }

    const wxString& GetDescription() const  { return m_Description; }
    void    SetDescription( const wxString& aDesc ) { m_Description = aDesc; }

    int     GetTrackWidth() const           { return m_TrackWidth; }
    void    SetTrackWidth( int aWidth )     { m_TrackWidth = aWidth; }

    int     GetTrackMinWidth() const        { return m_TrackMinWidth; }
    void    SetTrackMinWidth( int aWidth )  { m_TrackMinWidth = aWidth; }

    int     GetViaSize() const              { return m_ViaSize; }
    void    SetViaSize( int aSize )         { m_ViaSize = aSize; }

    int     GetViaDrillSize() const         { return m_ViaDrillSize; }
    void    SetViaDrillSize( int aSize )    { m_ViaDrillSize = aSize; }

    int     GetViaMinSize() const           { return m_ViaMinSize; }
    void    SetViaMinSize( int aSize )      { m_ViaMinSize = aSize; }

    int     GetClearance() const            { return m_Clearance; }
    void    SetClearance( int aClearance )  { m_Clearance = aClearance; }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function ReadDescr
     * reads the data structures for this object from a FILE in "*.brd" format.
     * @param aFile The FILE to read to.
     * @return bool - true if success reading else false.
     */
    bool ReadDescr( FILE* aFile, int* aLineNum );

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif
};


/**
 * Class NETCLASSES
 * is a containter for NETCLASS instances.  It owns all its NETCLASSes
 * (=> it will delete them at time of destruction).  This container will always have
 * a default NETCLASS with the name given by const NETCLASS::Default.
 */
class NETCLASSES
{
private:
    BOARD*                  m_Parent;

    typedef std::map<wxString, NETCLASS*>   NETCLASSMAP;

    /// all the NETCLASSes except the default one.
    NETCLASSMAP             m_NetClasses;

    /// the default NETCLASS.
    NETCLASS                m_Default;

public:
    NETCLASSES( BOARD* aParent = NULL );
    ~NETCLASSES();

    /**
     * Function Clear
     * destroys any constained NETCLASS instances except the Default one.
     */
    void Clear();

    typedef NETCLASSMAP::iterator       iterator;
    iterator begin() { return m_NetClasses.begin(); }
    iterator end()   { return m_NetClasses.end(); }

    typedef NETCLASSMAP::const_iterator const_iterator;
    const_iterator begin() const { return m_NetClasses.begin(); }
    const_iterator end()   const { return m_NetClasses.end(); }


    /**
     * Function GetCount
     * @return the number of netclasses, excluding the default one.
     */
    unsigned GetCount() const
    {
        return m_NetClasses.size();
    }

    NETCLASS* GetDefault() const
    {
        return (NETCLASS*) &m_Default;
    }


    /**
     * Function Add
     * @param aNetclass is netclass to add
     * @return true if Ok, false if cannot be added (mainly because a
     *  netclass with the same name exists)
     */
    bool Add( const NETCLASS& aNetclass );

    /**
     * Function Remove
     * removes a NETCLASS from this container but does not destroy/delete it.
     * @param aNetName is the name of the net to delete, and it may not be NETCLASS::Default.
     * @return NETCLASS* - the NETCLASS associated with aNetName if found and removed, else NULL.
     * You have to delete the returned value if you intend to destroy the NETCLASS.
     */
    NETCLASS* Remove( const wxString& aNetName );

    /**
     * Function Find
     * searches this container for a NETCLASS given by \a aName.
     * @param aName is the name of the NETCLASS to search for.
     * @return NETCLASS* - if found, else NULL.
     */
    NETCLASS* Find( const wxString& aName ) const;

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;
};

#endif  // CLASS_NETCLASS_H


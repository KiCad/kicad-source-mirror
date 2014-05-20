/**
 * @file class_netclass.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2009 KiCad Developers, see change_log.txt for contributors.
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
#include <boost/shared_ptr.hpp>

#include <wx/string.h>

#include <richio.h>


class LINE_READER;
class BOARD;
class BOARD_DESIGN_SETTINGS;


/**
 * Class NETCLASS
 * handles a collection of nets and the parameters used to route or
 * test these nets.
 */

class NETCLASS
{
private:
    // Default values used to init a NETCLASS
    static const int DEFAULT_CLEARANCE;
    static const int DEFAULT_VIA_DRILL;
    static const int DEFAULT_UVIA_DRILL;

protected:
    wxString    m_Name;                 ///< Name of the net class
    wxString    m_Description;          ///< what this NETCLASS is for.

    typedef std::set<wxString> STRINGSET;

    STRINGSET   m_Members;              ///< names of NET members of this class

    /// The units on these parameters is Internal Units (1 decimil or 1 nm)

    int         m_Clearance;            ///< clearance when routing

    int         m_TrackWidth;           ///< track width used to route NETs in this NETCLASS
    int         m_ViaDia;               ///< via diameter
    int         m_ViaDrill;             ///< via drill hole diameter

    int         m_uViaDia;              ///< microvia diameter
    int         m_uViaDrill;            ///< microvia drill hole diameter

public:

    static const wxChar Default[];      ///< the name of the default NETCLASS

    /**
     * Constructor
     * stuffs a NETCLASS instance with aParent, aName, and optionally the initialParameters
     * @param aName = the name of this new netclass
     */
    NETCLASS( const wxString& aName );

    ~NETCLASS();

    wxString GetClass() const
    {
        return wxT( "NETCLASS" );
    }

    const wxString& GetName() const
    {
        return m_Name;
    }

    void SetName( const wxString& aName ) { m_Name = aName; }

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

    int     GetClearance() const            { return m_Clearance; }
    void    SetClearance( int aClearance )  { m_Clearance = aClearance; }

    int     GetTrackWidth() const           { return m_TrackWidth; }
    void    SetTrackWidth( int aWidth )     { m_TrackWidth = aWidth; }

    int     GetViaDiameter() const          { return m_ViaDia; }
    void    SetViaDiameter( int aDia )      { m_ViaDia = aDia; }

    int     GetViaDrill() const             { return m_ViaDrill; }
    void    SetViaDrill( int aSize )        { m_ViaDrill = aSize; }

    int     GetuViaDiameter() const         { return m_uViaDia; }
    void    SetuViaDiameter( int aSize )    { m_uViaDia = aSize; }

    int     GetuViaDrill() const            { return m_uViaDrill; }
    void    SetuViaDrill( int aSize )       { m_uViaDrill = aSize; }

    /**
     * Function SetParams
     * will set all the parameters by copying them from \a defaults.
     * Parameters are the values like m_ViaSize, etc, but do not include m_Description.
     * @param aDefaults is another NETCLASS object to copy from.
     */
    void SetParams( const NETCLASS& aDefaults );

    /**
     * Function SetParams
     * will set all the parameters by copying them from board design settings.
     * @param aSettings is a BOARD_DESIGN_SETTINGS object to copy from. Clearance, via drill and
     * microvia drill values are taken from the defaults.
     */
    void SetParams( const BOARD_DESIGN_SETTINGS& aSettings );

    /**
     * Function Format
     * outputs the net class to \a aFormatter in s-expression form.
     *
     * @param aFormatter The #OUTPUTFORMATTER object to write to.
     * @param aNestLevel The indentation next level.
     * @param aControlBits The control bit definition for object specific formatting.
     * @throw IO_ERROR on write error.
     */
    void Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
        throw( IO_ERROR );

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // overload
#endif
};

typedef boost::shared_ptr<NETCLASS> NETCLASSPTR;

/**
 * Class NETCLASSES
 * is a container for NETCLASS instances.  It owns all its NETCLASSes
 * (=> it will delete them at time of destruction).  This container will always have
 * a default NETCLASS with the name given by const NETCLASS::Default.
 */
class NETCLASSES
{
private:
    typedef std::map<wxString, NETCLASSPTR> NETCLASSMAP;

    /// all the NETCLASSes except the default one.
    NETCLASSMAP             m_NetClasses;

    /// the default NETCLASS.
    NETCLASSPTR             m_Default;

public:
    NETCLASSES();
    ~NETCLASSES();

    /**
     * Function Clear
     * destroys any contained NETCLASS instances except the Default one.
     */
    void Clear()
    {
        m_NetClasses.clear();
    }

    typedef NETCLASSMAP::iterator iterator;
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

    /**
     * Function GetDefault
     * @return the default net class.
     */
    NETCLASSPTR GetDefault() const
    {
        return m_Default;
    }

    /**
     * Function Add
     * takes ownership of \a aNetclass and puts it into this NETCLASSES container.
     * @param aNetclass is netclass to add
     * @return true if the name within aNetclass is unique and it could be inserted OK,
     *  else false because the name was not unique and caller still owns aNetclass.
     */
    bool Add( NETCLASSPTR aNetclass );

    /**
     * Function Remove
     * removes a NETCLASS from this container but does not destroy/delete it.
     * @param aNetName is the name of the net to delete, and it may not be NETCLASS::Default.
     * @return NETCLASSPTR - the NETCLASS associated with aNetName if found and removed, else NULL.
     */
    NETCLASSPTR Remove( const wxString& aNetName );

    /**
     * Function Find
     * searches this container for a NETCLASS given by \a aName.
     * @param aName is the name of the NETCLASS to search for.
     * @return NETCLASSPTR - if found, else NULL.
     */
    NETCLASSPTR Find( const wxString& aName ) const;

};

#endif  // CLASS_NETCLASS_H

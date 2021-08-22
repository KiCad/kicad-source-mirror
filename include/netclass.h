/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@inpg.fr
 * Copyright (C) 2009-2020 KiCad Developers, see change_log.txt for contributors.
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


#include <gal/color4d.h>
#include <core/optional.h>
#include <macros_swig.h>

class LINE_READER;
class BOARD;
class BOARD_DESIGN_SETTINGS;
using KIGFX::COLOR4D;


DECL_SET_FOR_SWIG( STRINGSET, wxString )


/**
 * A collection of nets and the parameters used to route or test these nets.
 */
class NETCLASS
{
public:
    static const char Default[];        ///< the name of the default NETCLASS

    /**
     * Create a NETCLASS instance with \a aName.
     *
     * @param aName is the name of this new netclass.
     */
    NETCLASS( const wxString& aName );

    ~NETCLASS();

    wxString GetClass() const
    {
        return wxT( "NETCLASS" );
    }

    const wxString GetName() const { return m_Name; }
    void SetName( const wxString& aName ) { m_Name = aName; }

    /**
     * Return the number of nets in this NETCLASS, i.e. using these rules.
     */
    unsigned GetCount() const
    {
        return m_Members.size();
    }

    /**
     * Empties the collection of members.
     */
    void Clear()
    {
        m_Members.clear();
    }

    /**
     * Adds \a aNetname to this NETCLASS if it is not already in this NETCLASS.
     *
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
     * Remove NET \a aName from the collection of members.
     */
    void Remove( iterator aName )
    {
        m_Members.erase( aName );
    }

    /**
     * Remove NET \a aName from the collection of members.
     */
    void Remove( const wxString& aName )
    {
        m_Members.erase( aName );
    }

    STRINGSET& NetNames()                   { return m_Members; }       ///< for SWIG

    const wxString& GetDescription() const  { return m_Description; }
    void  SetDescription( const wxString& aDesc ) { m_Description = aDesc; }

    bool    HasClearance() const { return (bool) m_Clearance; }
    int     GetClearance() const { return m_Clearance.value_or(-1); }
    void    SetClearance( int aClearance )  { m_Clearance = aClearance; }

    bool    HasTrackWidth() const { return (bool) m_TrackWidth; }
    int     GetTrackWidth() const           { return m_TrackWidth.value_or( -1 ); }
    void    SetTrackWidth( int aWidth )     { m_TrackWidth = aWidth; }

    bool    HasViaDiameter() const          { return (bool) m_ViaDia; }
    int     GetViaDiameter() const          { return m_ViaDia.value_or( -1 ); }
    void    SetViaDiameter( int aDia )      { m_ViaDia = aDia; }

    int     HasViaDrill() const             { return (bool) m_ViaDrill; }
    int     GetViaDrill() const             { return m_ViaDrill.value_or( -1 ); }
    void    SetViaDrill( int aSize )        { m_ViaDrill = aSize; }

    bool    HasuViaDiameter() const         { return (bool) m_uViaDia; }
    int     GetuViaDiameter() const         { return m_uViaDia.value_or( -1 ); }
    void    SetuViaDiameter( int aSize )    { m_uViaDia = aSize; }

    bool    HasuViaDrill() const            { return (bool) m_uViaDrill; }
    int     GetuViaDrill() const            { return m_uViaDrill.value_or( -1 ); }
    void    SetuViaDrill( int aSize )       { m_uViaDrill = aSize; }

    bool    HasDiffPairWidth() const        { return (bool) m_diffPairWidth; }
    int     GetDiffPairWidth() const        { return m_diffPairWidth.value_or( -1 ); }
    void    SetDiffPairWidth( int aSize )   { m_diffPairWidth = aSize; }

    bool    HasDiffPairGap() const          { return (bool) m_diffPairGap; }
    int     GetDiffPairGap() const          { return m_diffPairGap.value_or( -1 ); }
    void    SetDiffPairGap( int aSize )     { m_diffPairGap = aSize; }

    bool    HasDiffPairViaGap() const       { return (bool) m_diffPairViaGap; }
    int     GetDiffPairViaGap() const       { return m_diffPairViaGap.value_or( -1 ); }
    void    SetDiffPairViaGap( int aSize )  { m_diffPairViaGap = aSize; }

    COLOR4D GetPcbColor() const             { return m_PcbColor; }
    void    SetPcbColor( const COLOR4D& aColor ) { m_PcbColor = aColor; }

    int     GetWireWidth() const            { return m_wireWidth; }
    void    SetWireWidth( int aWidth )      { m_wireWidth = aWidth; }

    int     GetBusWidth() const             { return m_busWidth; }
    void    SetBusWidth( int aWidth )       { m_busWidth = aWidth; }

    COLOR4D GetSchematicColor() const       { return m_schematicColor; }
    void    SetSchematicColor( COLOR4D aColor ) { m_schematicColor = aColor; }

    int     GetLineStyle() const            { return m_lineStyle; }
    void    SetLineStyle( int aStyle )      { m_lineStyle = aStyle; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;
#endif

protected:
    wxString    m_Name;                 ///< Name of the net class
    wxString    m_Description;          ///< what this NETCLASS is for.

    STRINGSET   m_Members;              ///< names of NET members of this class

    /// The units on these parameters is Internal Units (1 nm)

    OPT<int>         m_Clearance;            ///< clearance when routing

    OPT<int>         m_TrackWidth;           ///< track width used to route NETs in this NETCLASS
    OPT<int>         m_ViaDia;               ///< via diameter
    OPT<int>         m_ViaDrill;             ///< via drill hole diameter

    OPT<int>         m_uViaDia;              ///< microvia diameter
    OPT<int>         m_uViaDrill;            ///< microvia drill hole diameter

    OPT<int>         m_diffPairWidth;
    OPT<int>         m_diffPairGap;
    OPT<int>         m_diffPairViaGap;

    int         m_wireWidth;
    int         m_busWidth;
    COLOR4D     m_schematicColor;
    int         m_lineStyle;

    COLOR4D     m_PcbColor;          ///< Optional color override for this netclass (PCB context)
};


DECL_SPTR_FOR_SWIG( NETCLASSPTR, NETCLASS )
DECL_MAP_FOR_SWIG( NETCLASS_MAP, wxString, NETCLASSPTR )


/**
 * A container for NETCLASS instances.
 *
 * It owns all its NETCLASSes.  This container will always have a default NETCLASS with the
 * name given by const NETCLASS::Default.
 */
class NETCLASSES
{
public:
    NETCLASSES();
    ~NETCLASSES();

    /**
     * Destroy any contained NETCLASS instances except the default one, and clears any
     * members from the default one.
     */
    void Clear()
    {
        m_NetClasses.clear();
        m_default->Clear();
    }

    typedef NETCLASS_MAP::iterator iterator;
    iterator begin() { return m_NetClasses.begin(); }
    iterator end()   { return m_NetClasses.end(); }

    typedef NETCLASS_MAP::const_iterator const_iterator;
    const_iterator begin() const { return m_NetClasses.begin(); }
    const_iterator end()   const { return m_NetClasses.end(); }

    /**
     * @return the number of netclasses excluding the default one.
     */
    unsigned GetCount() const
    {
        return m_NetClasses.size();
    }

    /**
     * @return the default net class.
     */
    NETCLASSPTR GetDefault() const
    {
        return m_default;
    }

    NETCLASS* GetDefaultPtr() const
    {
        return m_default.get();
    }

    /**
     * Add \a aNetclass and puts it into this NETCLASSES container.
     *
     * @param aNetclass is netclass to add
     * @return true if the name within aNetclass is unique and it could be inserted OK,
     *         else false because the name was not unique.
     */
    bool Add( const NETCLASSPTR& aNetclass );

    /**
     * Remove a #NETCLASS from this container but does not destroy/delete it.
     *
     * @param aNetName is the name of the net to delete, and it may not be NETCLASS::Default.
     * @return a pointer to the #NETCLASS associated with \a aNetName if found and removed,
     *         else NULL.
     */
    NETCLASSPTR Remove( const wxString& aNetName );

    /**
     * Search this container for a NETCLASS given by \a aName.
     *
     * @param aName is the name of the #NETCLASS to search for.
     * @return a pointer to the #NETCLASS if found, else NULL.
     */
    NETCLASSPTR Find( const wxString& aName ) const;

    /// Provide public access to m_NetClasses so it gets swigged.
    NETCLASS_MAP&   NetClasses()       { return m_NetClasses; }

private:
    NETCLASS_MAP    m_NetClasses;   // All the netclasses EXCEPT the default one
    NETCLASSPTR     m_default;
};

#endif  // CLASS_NETCLASS_H

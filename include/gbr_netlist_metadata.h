/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GBR_NETLIST_METADATA_H
#define GBR_NETLIST_METADATA_H


// this class handle info which can be added in a gerber file as attribute
// of an obtect
// the GBR_INFO_TYPE types can be OR'ed to add 2 (or more) attributes
// There are only 3 net attributes defined attached to an object by the %TO command
// %TO.P
// %TO.N
// %TO.C
// the .P attribute can be used only for flashed pads (using the D03 command)
// and only for external copper layers, if the component is on a external copper layer
// for other copper layer items (pads on internal layers, tracks ... ), only .N and .C
// can be used
class GBR_NETLIST_METADATA
{
public:
    // This enum enables the different net attributes attache to the object
    // the values can be ORed for items which can have more than one attribute
    // (A flashed pad has all allowed attributes)
    enum GBR_NETINFO_TYPE
    {
        GBR_NETINFO_UNSPECIFIED,    ///< idle command (no command)
        GBR_NETINFO_PAD = 1,        ///< print info associated to a flashed pad (TO.P attribute)
        GBR_NETINFO_NET = 2,        ///< print info associated to a net (TO.N attribute)
        GBR_NETINFO_CMP = 4         ///< print info associated to a component (TO.C attribute)
    };

    // these members are used in the %TO object attributes command.
    int      m_NetAttribType;   ///< the type of net info
                                ///< (used to define the gerber string to create)
    bool     m_NotInNet;        ///< true if a pad of a footprint cannot be connected
                                ///< (for instance a mechanical NPTH, ot a not named pad)
                                ///< in this case the pad net name is empty in gerber file
    wxString m_Padname;         ///< for a flashed pad: the pad name ((TO.P attribute)
    wxString m_Cmpref;          ///< the component reference parent of the data
    wxString m_Netname;         ///< for items associated to a net: the netname

    GBR_NETLIST_METADATA(): m_NetAttribType( GBR_NETINFO_UNSPECIFIED ), m_NotInNet( false )
    {
    }

    /**
     * remove the net attribute specified by aName
     * If aName == NULL or empty, remove all attributes
     * @param aName is the name (.CN, .P .N or .C) of the attribute to remove
     */
    void ClearAttribute( const wxString* aName )
    {
        if( m_NetAttribType == GBR_NETINFO_UNSPECIFIED )
        {
            m_Padname.clear();
            m_Cmpref.clear();
            m_Netname.clear();
            return;
        }

        if( !aName || aName->IsEmpty() || *aName == ".CN" )
        {
            m_NetAttribType = GBR_NETINFO_UNSPECIFIED;
            m_Padname.clear();
            m_Cmpref.clear();
            m_Netname.clear();
            return;
        }

        if( *aName == ".C" )
        {
            m_NetAttribType &= ~GBR_NETINFO_CMP;
            m_Cmpref.clear();
            return;
        }

        if( *aName == ".N" )
        {
            m_NetAttribType &= ~GBR_NETINFO_NET;
            m_Netname.clear();
            return;
        }

        if( *aName == ".P" )
        {
            m_NetAttribType &= ~GBR_NETINFO_PAD;
            m_Cmpref.clear();
            return;
        }
    }
};

// Flashed pads use the full attribute set: this is a helper for flashed pads
#define GBR_NETINFO_ALL (GBR_NETLIST_METADATA::GBR_NETINFO_PAD\
                        | GBR_NETLIST_METADATA::GBR_NETINFO_NET\
                        | GBR_NETLIST_METADATA::GBR_NETINFO_CMP )

#endif      // GBR_NETLIST_METADATA_H

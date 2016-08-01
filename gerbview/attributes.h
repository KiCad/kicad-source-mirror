/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H


// this class handle info which can be added in a gerber file as attribute
// of an obtect
// the GBR_INFO_TYPE types can be OR'ed to add 2 (or more) attributes
// There are only 3 net attributes attached to an object by the %TO command
// %TO.CN
// %TO.N
// %TO.C
// the .CN attribute can be used only for flashed pads (using the D03 command)
// and only for external copper layers, if the component is on a external copper layer
// for other copper layer items (pads on internal layers, tracks ... ), only .N and .C
// can be used
class GBR_NETLIST_METADATA
{
public:
    enum GBR_NETINFO_TYPE
    {
        GBR_NETINFO_UNSPECIFIED,    ///< idle command (no command)
        GBR_NETINFO_FLASHED_PAD,    ///< print info associated to a flashed pad (TO.CN attribute)
        GBR_NETINFO_NET,            ///< print info associated to a net (TO.N attribute)
        GBR_NETINFO_COMPONENT,      ///< print info associated to a footprint (TO.C attribute)
        GBR_NETINFO_NET_AND_CMP     ///< combine GBR_INFO_NET and GBR_INFO_NET_AND_COMPONENT
                                    ///< to add both TO.N and TO.C attributes
    };

    // these members are used in the %TO object attributes command.
    GBR_NETINFO_TYPE m_NetAttribType;   ///< the type of net info
                                        ///< (used to define the gerber string to create)
    wxString m_Padname;         ///< for a flashed pad: the pad name ((TO.CN attribute)
    wxString m_ComponentRef;    ///< the footprint reference parent of the data
    wxString m_Netname;         ///< for items associated to a net: the netname

    GBR_NETLIST_METADATA(): m_NetAttribType( GBR_NETINFO_UNSPECIFIED )
    {
    }

    /*
     * remove the net attribute specified by aName
     * If aName == NULL or empty, remove all attributes
     */
    void ClearAttribute( const wxString* aName )
    {
        if( m_NetAttribType == GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED )
            return;

        if( !aName || aName->IsEmpty() )
        {
            m_NetAttribType = GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED;
            m_Padname.clear();
            m_ComponentRef.clear();
            m_Netname.clear();
        }

        if( aName && *aName == ".CN" )
        {
            m_NetAttribType = GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED;
            m_Padname.clear();
            m_ComponentRef.clear();
            m_Netname.clear();
        }

        if( aName && *aName == ".C" )
        {
            if( m_NetAttribType == GBR_NETINFO_COMPONENT )
                m_NetAttribType = GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED;
            else
                m_NetAttribType = GBR_NETLIST_METADATA::GBR_NETINFO_NET;

            m_ComponentRef.clear();
        }

        if( aName && *aName == ".N" )
        {
            if( m_NetAttribType == GBR_NETINFO_NET )
                m_NetAttribType = GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED;
            else
                m_NetAttribType = GBR_NETLIST_METADATA::GBR_NETINFO_COMPONENT;

            m_Netname.clear();
        }
     }
};


#endif  // #define ATTRIBUTES_H

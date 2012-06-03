/**
 * @file cvpcb/readschematicnetlist.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras.
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

/* Read a nelist type Eeschema (New and Old format)
 * or OrcadPCB2 and build the component list
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <confirm.h>
#include <kicad_string.h>
#include <macros.h>

#include <cvpcb_mainframe.h>
#include <richio.h>


#include <netlist_reader.h>

// COMPONENT_INFO object list sort function:
bool operator < ( const COMPONENT_INFO& item1, const COMPONENT_INFO& item2 )
{
    return StrNumCmp( item1.m_Reference, item2.m_Reference, INT_MAX, true ) < 0;
}


int CVPCB_MAINFRAME::ReadSchematicNetlist()
{
    FILE* netfile = wxFopen( m_NetlistFileName.GetFullPath(), wxT( "rt" ) );

    if( netfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "Could not open file <%>" ),
                    GetChars( m_NetlistFileName.GetFullPath() ) );
        wxMessageBox( msg );
        return -1;
    }

    NETLIST_READER netList_Reader( NULL, NULL );
    netList_Reader.m_UseTimeStamp     = false;
    netList_Reader.m_ChangeFootprints = false;
    netList_Reader.m_UseCmpFile = false;
    netList_Reader.SetFilesnames( m_NetlistFileName.GetFullPath(), wxEmptyString );

    // True to read footprint filters section: true for CvPcb, false for Pcbnew
    netList_Reader.ReadLibpartSectionSetOpt( true );

    bool success = netList_Reader.ReadNetList( netfile );
    if( !success )
    {
        wxMessageBox( _("Netlist read error") );
        return false;
    }

    // Now copy footprints info into Cvpcb list:
    // We also remove footprint name if it is "$noname"
    // because this is a dummy name,, not an actual name
    COMPONENT_INFO_LIST& cmpInfo = netList_Reader.GetComponentInfoList();
    for( unsigned ii = 0; ii < cmpInfo.size(); ii++ )
    {
        m_components.push_back( cmpInfo[ii] );
        if( cmpInfo[ii]->m_Footprint == wxT( "$noname" ) )
            cmpInfo[ii]->m_Footprint.Empty();
    }
    cmpInfo.clear();    // cmpInfo is no more owner of the list.

    // Sort components by reference:
    sort( m_components.begin(), m_components.end() );

    // Now copy filters in m_components, if netlist type is KICAD
    // ( when the format is the "old" PCBNEW format, filters are already in
    // m_component list
    if( NETLIST_TYPE_KICAD == netList_Reader.GetNetlistType() )
    {
        for( unsigned ii = 0; ii < m_components.size(); ii++ )
        {
            LIPBART_INFO* libpart = netList_Reader.GetLibpart(m_components[ii].m_Libpart);
            if( libpart == NULL )
                continue;

            // now copy filter list
            m_components[ii].m_FootprintFilter = libpart->m_FootprintFilter;
        }
    }

    return 0;
}

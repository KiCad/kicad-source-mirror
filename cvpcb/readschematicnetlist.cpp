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

/* Read a netlist type Eeschema (New and Old format)
 * or OrcadPCB2 and build the component list
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <confirm.h>
#include <kicad_string.h>
#include <macros.h>
#include <wildcards_and_files_ext.h>

#include <cvpcb_mainframe.h>
#include <richio.h>

#include <netlist_reader.h>


int CVPCB_MAINFRAME::ReadSchematicNetlist()
{
    wxBusyCursor    dummy;           // Shows an hourglass while loading.
    NETLIST_READER* netlistReader;
    wxString        msg;
    wxString        compFootprintLinkFileName;
    wxFileName      fn = m_NetlistFileName;

    // Load the footprint association file if it has already been created.
    fn.SetExt( ComponentFileExtension );

    if( fn.FileExists() && fn.IsFileReadable() )
        compFootprintLinkFileName = fn.GetFullPath();

    m_netlist.Clear();

    try
    {
        netlistReader = NETLIST_READER::GetNetlistReader( &m_netlist,
                                                          m_NetlistFileName.GetFullPath(),
                                                          compFootprintLinkFileName );
        std::auto_ptr< NETLIST_READER > nlr( netlistReader );
        netlistReader->LoadNetlist();
    }
    catch( IO_ERROR& ioe )
    {
        msg = wxString::Format( _( "Error loading netlist.\n%s" ), ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return 1;
    }


    // We also remove footprint name if it is "$noname" because this is a dummy name,
    // not the actual name of the footprint.
    for( unsigned ii = 0; ii < m_netlist.GetCount(); ii++ )
    {
        if( m_netlist.GetComponent( ii )->GetFootprintLibName() == wxT( "$noname" ) )
            m_netlist.GetComponent( ii )->SetFootprintLibName( wxEmptyString );
    }

    // Sort components by reference:
    m_netlist.SortByReference();

    return 0;
}

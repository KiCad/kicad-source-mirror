/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.TXT for contributors.
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


#include <fctsys.h>
#include <build_version.h>
#include <confirm.h>

#include <schframe.h>
#include "netlist_exporter_kicad.h"

bool NETLIST_EXPORTER_KICAD::Write( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    std::auto_ptr<XNODE> xroot( makeRoot() );

    try
    {
        FILE_OUTPUTFORMATTER formatter( aOutFileName );

        xroot->Format( &formatter, 0 );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( NULL, ioe.errorText );
        return false;
    }

    return true;
}

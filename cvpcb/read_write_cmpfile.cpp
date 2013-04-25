/**
 * @file cvpcb/read_write_cmpfile.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kicad_string.h>
#include <appl_wxstruct.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>

#include <build_version.h>


/* File header. */
static char HeaderLinkFile[] = { "Cmp-Mod V01" };

/* Write the link file:
 * the header is:
 * Cmp-Mod V01 Created by CvPcb (2012-02-08 BZR 3403)-testing date = 10/02/2012 20:45:59
 * and write block per component like:
 * BeginCmp
 * TimeStamp = /322D3011;
 * Reference = BUS1;
 * ValeurCmp = BUSPC;
 * IdModule  = BUS_PC;
 * EndCmp
 */

bool CVPCB_MAINFRAME::WriteComponentLinkFile( const wxString& aFullFileName )
{
    COMPONENT*  component;
    FILE*       outputFile;
    wxFileName  fn( aFullFileName );
    wxString    Title = wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion();

    outputFile = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( outputFile == NULL )
        return false;

    int retval = 0;

    retval |= fprintf( outputFile, "%s", HeaderLinkFile );
    retval |= fprintf( outputFile, " Created by %s", TO_UTF8( Title ) );
    retval |= fprintf( outputFile, " date = %s\n", TO_UTF8( DateAndTime() ) );

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );
        retval |= fprintf( outputFile, "\nBeginCmp\n" );
        retval |= fprintf( outputFile, "TimeStamp = %s;\n", TO_UTF8( component->GetTimeStamp() ) );
        retval |= fprintf( outputFile, "Reference = %s;\n", TO_UTF8( component->GetReference() ) );
        retval |= fprintf( outputFile, "ValeurCmp = %s;\n", TO_UTF8( component->GetValue() ) );
        retval |= fprintf( outputFile, "IdModule  = %s;\n",
                           TO_UTF8( component->GetFootprintLibName() ) );
        retval |= fprintf( outputFile, "EndCmp\n" );
    }

    retval |= fprintf( outputFile, "\nEndListe\n" );
    fclose( outputFile );
    return retval >= 0;
}

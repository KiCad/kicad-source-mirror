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

    BOOST_FOREACH( COMPONENT_INFO& component, m_components )
    {
        retval |= fprintf( outputFile, "\nBeginCmp\n" );
        retval |= fprintf( outputFile, "TimeStamp = %s;\n", TO_UTF8( component.m_TimeStamp ) );
        retval |= fprintf( outputFile, "Reference = %s;\n", TO_UTF8( component.m_Reference ) );
        retval |= fprintf( outputFile, "ValeurCmp = %s;\n", TO_UTF8( component.m_Value ) );
        retval |= fprintf( outputFile, "IdModule  = %s;\n", TO_UTF8( component.m_Footprint ) );
        retval |= fprintf( outputFile, "EndCmp\n" );
    }

    retval |= fprintf( outputFile, "\nEndListe\n" );
    fclose( outputFile );
    return retval >= 0;
}

bool CVPCB_MAINFRAME::ReadComponentLinkFile( FILE * aFile )
{
    wxString    timestamp, valeur, ilib, namecmp, msg;
    bool        read_cmp_data = false, eof = false;
    char        Line[1024], * ident, * data;

    // Identification of the type of link file
    if( fgets( Line, sizeof(Line), aFile ) == 0 ||
        strnicmp( Line, HeaderLinkFile, 11 ) != 0 )
    {
        fclose( aFile );
        return false;
    }

    while( !eof && fgets( Line, sizeof(Line), aFile ) != 0 )
    {
        if( strnicmp( Line, "EndListe", 8 ) == 0 )
            break;

        /* Search the beginning of the component description. */
        if( strnicmp( Line, "BeginCmp", 8 ) != 0 )
            continue;

        timestamp.Empty();
        valeur.Empty();
        ilib.Empty();
        namecmp.Empty();
        read_cmp_data = true;

        while( !eof && read_cmp_data )
        {
            if( fgets( Line, 1024, aFile ) == 0 )
            {
                eof = true;
                break;
            }

            if( strnicmp( Line, "EndCmp", 6 ) == 0 )
            {
                read_cmp_data = true;
                break;
            }

            ident = strtok( Line, "=;\n\r" );
            data  = strtok( NULL, ";\n\r" );

            if( strnicmp( ident, "TimeStamp", 9 ) == 0 )
            {
                timestamp = FROM_UTF8( data );
                timestamp.Trim( true );
                timestamp.Trim( false );
                continue;
            }

            if( strnicmp( ident, "Reference", 9 ) == 0 )
            {
                namecmp = FROM_UTF8( data );
                namecmp.Trim( true );
                namecmp.Trim( false );
                continue;
            }

            if( strnicmp( ident, "ValeurCmp", 9 ) == 0 )
            {
                valeur = FROM_UTF8( data );
                valeur.Trim( true );
                valeur.Trim( false );
                continue;
            }

            if( strnicmp( ident, "IdModule", 8 ) == 0 )
            {
                ilib = FROM_UTF8( data );
                ilib.Trim( true );
                ilib.Trim( false );
                continue;
            }
        }   // End reading one component link block.

        // Search corresponding component info in list and update its parameters.
        BOOST_FOREACH( COMPONENT_INFO& component, m_components )
        {
            if( namecmp != component.m_Reference )
                continue;

            /* Copy the name of the corresponding module. */
            component.m_Footprint = ilib;
        }
    }

    fclose( aFile );
    return true;
}


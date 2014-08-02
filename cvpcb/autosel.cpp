/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file autosel.cpp
 */

// Routines for automatic selection of modules.

#include <fctsys.h>
#include <common.h>
#include <project.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <kicad_string.h>
#include <macros.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>

#define QUOTE   '\''

#define FMT_TITLE_LIB_LOAD_ERROR    _( "Library Load Error" )


class FOOTPRINT_ALIAS
{
public:
    int         m_Type;
    wxString    m_Name;
    wxString    m_FootprintName;

    FOOTPRINT_ALIAS() { m_Type = 0; }
};

typedef boost::ptr_vector< FOOTPRINT_ALIAS > FOOTPRINT_ALIAS_LIST;


/*
 * read the string between quotes and put it in aTarget
 * put text in aTarget
 * return a pointer to the last read char (the second quote if Ok)
 */
wxString GetQuotedText( wxString & text )
{
    int i = text.Find( QUOTE );

    if( wxNOT_FOUND == i )
        return wxT( "" );

    wxString shrt = text.Mid( i + 1 );
    i = shrt.Find( QUOTE );

    if( wxNOT_FOUND == i )
        return wxT( "" );

    text = shrt.Mid( i + 1 );
    return shrt.Mid( 0, i );
}


void CVPCB_MAINFRAME::AssocieModule( wxCommandEvent& event )
{
    FOOTPRINT_ALIAS_LIST aliases;
    FOOTPRINT_ALIAS*     alias;
    COMPONENT*           component;
    wxFileName           fn;
    wxString             msg, tmp;
    char                 Line[1024];
    FILE*                file;
    size_t               ii;
    SEARCH_STACK&        search = Prj().SchSearchS();

    if( m_netlist.IsEmpty() )
        return;

    // Find equivalents in all available files.
    for( ii = 0; ii < m_AliasLibNames.GetCount(); ii++ )
    {
        fn = m_AliasLibNames[ii];

        if( !fn.HasExt() )
        {
            fn.SetExt( FootprintAliasFileExtension );
            // above fails if filename has more than one point
        }
        else
        {
            fn.SetExt( fn.GetExt() + wxT( "." ) + FootprintAliasFileExtension );
        }

        tmp = search.FindValidPath( fn.GetFullPath() );

        if( !tmp )
        {
            msg.Printf( _( "Footprint alias library file '%s' could not be found in the "
                           "default search paths." ),
                        GetChars( fn.GetFullName() ) );
            wxMessageBox( msg, FMT_TITLE_LIB_LOAD_ERROR, wxOK | wxICON_ERROR );
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Error opening alias library '%s'." ), GetChars( tmp ) );
            wxMessageBox( msg, FMT_TITLE_LIB_LOAD_ERROR, wxOK | wxICON_ERROR );
            continue;
        }

        while( GetLine( file, Line, NULL, sizeof(Line) ) != NULL )
        {
            char* text = Line;
            wxString value, footprint, wtext = FROM_UTF8( Line );

            value = GetQuotedText( wtext );

            if( text == NULL || ( *text == 0 ) || value.IsEmpty() )
                continue;

            footprint = GetQuotedText( wtext );

            if( footprint.IsEmpty() )
                continue;

            value.Replace( wxT( " " ), wxT( "_" ) );

            alias = new FOOTPRINT_ALIAS();
            alias->m_Name = value;
            alias->m_FootprintName = footprint;
            aliases.push_back( alias );
        }

        fclose( file );
    }

    // Display the number of footprint aliases.
    msg.Printf( _( "%d footprint aliases found." ), aliases.size() );
    SetStatusText( msg, 0 );

    m_skipComponentSelect = true;
    ii = 0;

    for( unsigned kk = 0;  kk < m_netlist.GetCount();  kk++ )
    {
        component = m_netlist.GetComponent( kk );

        bool found = false;
        m_compListBox->SetSelection( ii++, true );

        if( !component->GetFPID().empty() )
            continue;

        BOOST_FOREACH( FOOTPRINT_ALIAS& alias, aliases )
        {

            if( alias.m_Name.CmpNoCase( component->GetValue() ) != 0 )
                continue;

            // filter alias so one can use multiple aliases (for polar and
            // nonpolar caps for example)
            const FOOTPRINT_INFO *module = m_footprints.GetModuleInfo( alias.m_FootprintName );

            if( module )
            {
                size_t filtercount = component->GetFootprintFilters().GetCount();
                found = ( 0 == filtercount ); // if no entries, do not filter

                for( size_t jj = 0; jj < filtercount && !found; jj++ )
                {
                    found = module->GetFootprintName().Matches( component->GetFootprintFilters()[jj] );
                }
            }
            else
            {
                msg.Printf( _( "Component %s: footprint %s not found in any of the project "
                               "footprint libraries." ),
                            GetChars( component->GetReference() ),
                            GetChars( alias.m_FootprintName ) );
                wxMessageBox( msg, _( "CvPcb Error" ), wxOK | wxICON_ERROR, this );
            }

            if( found )
            {
                SetNewPkg( alias.m_FootprintName );
                break;
            }

        }

        // obviously the last chance: there's only one filter matching one footprint
        if( !found && 1 == component->GetFootprintFilters().GetCount() )
        {
            // we do not need to analyse wildcards: single footprint do not
            // contain them and if there are wildcards it just will not match any
            const FOOTPRINT_INFO* module = m_footprints.GetModuleInfo( component->GetFootprintFilters()[0] );

            if( module )
            {
                SetNewPkg( component->GetFootprintFilters()[0] );
            }
        }
    }

    m_skipComponentSelect = false;
}

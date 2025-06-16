/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file auto_associate.cpp
 */

// This file handle automatic selection of footprints, from .equ files which give
// a footprint LIB_ID associated to a component value.
// These associations have this form:
// 'FT232BL'		'QFP:LQFP-32_7x7mm_Pitch0.8mm'


#include <kiface_base.h>
#include <string_utils.h>
#include <macros.h>

#include <auto_associate.h>
#include <cvpcb_association.h>
#include <cvpcb_mainframe.h>
#include <listboxes.h>
#include <project/project_file.h>
#include <wx/msgdlg.h>

#define QUOTE   '\''


/**
 * Read the string between quotes.
 *
 * @return a the quoted string.
 */
wxString GetQuotedText( wxString& text )
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


// A sort compare function, used to sort a FOOTPRINT_EQUIVALENCE_LIST by cmp values
// (m_ComponentValue member)
bool sortListbyCmpValue( const FOOTPRINT_EQUIVALENCE& ref, const FOOTPRINT_EQUIVALENCE& test )
{
    return ref.m_ComponentValue.Cmp( test.m_ComponentValue ) >= 0;
}


int CVPCB_MAINFRAME::buildEquivalenceList( FOOTPRINT_EQUIVALENCE_LIST& aList,
                                           wxString* aErrorMessages )
{
    char        line[1024];
    int         error_count = 0;
    FILE*       file;
    wxFileName  fn;
    wxString    tmp, error_msg;

    SEARCH_STACK& search  = Kiface().KifaceSearch();
    PROJECT_FILE& project = Prj().GetProjectFile();

    // Find equivalences in all available files, and populates the
    // equiv_List with all equivalences found in .equ files
    for( const wxString& equfile : project.m_EquivalenceFiles )
    {
        fn =  wxExpandEnvVars( equfile );

        if( fn.IsAbsolute() || fn.FileExists() )
            tmp = fn.GetFullPath();
        else
            tmp = search.FindValidPath( fn.GetFullPath() );

        if( !tmp )
        {
            error_count++;

            if( aErrorMessages )
            {
                error_msg.Printf( _( "Equivalence file '%s' could not be found." ),
                                  fn.GetFullName() );

                if( ! aErrorMessages->IsEmpty() )
                    *aErrorMessages << wxT( "\n\n" );

                *aErrorMessages += error_msg;
            }

            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == nullptr )
        {
            error_count++;

            if( aErrorMessages )
            {
                error_msg.Printf( _( "Error opening equivalence file '%s'." ), tmp );

                if( ! aErrorMessages->IsEmpty() )
                    *aErrorMessages << wxT( "\n\n" );

                *aErrorMessages += error_msg;
            }

            continue;
        }

        while( GetLine( file, line, nullptr, sizeof( line ) ) != nullptr )
        {
            if( *line == 0 )
                continue;

            wxString wtext = From_UTF8( line );
            wxString value = GetQuotedText( wtext );

            if( value.IsEmpty() )
                continue;

            wxString footprint = GetQuotedText( wtext );

            if( footprint.IsEmpty() )
                continue;

            value.Replace( wxT( " " ), wxT( "_" ) );

            FOOTPRINT_EQUIVALENCE* equivItem = new FOOTPRINT_EQUIVALENCE();
            equivItem->m_ComponentValue = value;
            equivItem->m_FootprintFPID = footprint;
            aList.push_back( equivItem );
        }

        fclose( file );
    }

    return error_count;
}


void CVPCB_MAINFRAME::AutomaticFootprintMatching()
{
    FOOTPRINT_EQUIVALENCE_LIST equivList;
    wxString                   msg;
    wxString                   error_msg;

    if( m_netlist.IsEmpty() )
        return;

    if( buildEquivalenceList( equivList, &error_msg ) )
        wxMessageBox( error_msg, _( "Equivalence File Load Error" ), wxOK | wxICON_WARNING, this );

    // Sort the association list by symbol value.  When sorted, finding duplicate definitions
    // (i.e. 2 or more items having the same symbol value) is easier.
    std::sort( equivList.begin(), equivList.end(), sortListbyCmpValue );

    // Display the number of footprint/symbol equivalences.
    msg.Printf( _( "%lu footprint/symbol equivalences found." ), (unsigned long)equivList.size() );
    SetStatusText( msg, 0 );

    // Now, associate each free component with a footprint
    m_skipComponentSelect = true;
    error_msg.Empty();

    bool firstAssoc = true;

    for( int kk = 0;  kk < (int) m_netlist.GetCount();  kk++ )
    {
        COMPONENT* component = m_netlist.GetComponent( kk );

        bool found = false;

        if( !component->GetFPID().empty() ) // the component has already a footprint
            continue;

        // Here a first attempt is made. We can have multiple equivItem of the same value.
        // When happens, using the footprint filter of components can remove the ambiguity by
        // filtering equivItem so one can use multiple equivList (for polar and non-polar caps
        // for example)
        wxString fpid_candidate;

        for( int idx = 0; idx < (int) equivList.size(); idx++ )
        {
            FOOTPRINT_EQUIVALENCE& equivItem = equivList[idx];

            if( equivItem.m_ComponentValue.CmpNoCase( component->GetValue() ) != 0 )
                continue;

            const FOOTPRINT_INFO* fp = m_FootprintsList->GetFootprintInfo( equivItem.m_FootprintFPID );

            bool equ_is_unique = true;
            int  next = idx+1;
            int  previous = idx-1;

            if( next < (int) equivList.size() && equivItem.m_ComponentValue == equivList[next].m_ComponentValue )
                equ_is_unique = false;

            if( previous >= 0 && equivItem.m_ComponentValue == equivList[previous].m_ComponentValue )
                equ_is_unique = false;

            // If the equivalence is unique, no ambiguity: use the association
            if( fp && equ_is_unique )
            {
                AssociateFootprint( CVPCB_ASSOCIATION( kk, equivItem.m_FootprintFPID ), firstAssoc );
                firstAssoc = false;
                found = true;
                break;
            }

            // Store the first candidate found in list, when equivalence is not unique
            // We use it later.
            if( fp && fpid_candidate.IsEmpty() )
                fpid_candidate = equivItem.m_FootprintFPID;

            // The equivalence is not unique: use the footprint filter to try to remove
            // ambiguity
            // if the footprint filter does not remove ambiguity, we will use fpid_candidate
            if( fp )
            {
                size_t filtercount = component->GetFootprintFilters().GetCount();
                found = ( filtercount == 0 ); // if no entries, do not filter

                for( size_t jj = 0; jj < filtercount && !found; jj++ )
                    found = fp->GetFootprintName().Matches( component->GetFootprintFilters()[jj] );
            }
            else
            {
                msg.Printf( _( "Component %s: footprint %s not found in any of the project footprint libraries." ),
                            component->GetReference(),
                            equivItem.m_FootprintFPID );

                if( !error_msg.IsEmpty() )
                    error_msg << wxT("\n\n");

                error_msg += msg;
            }

            if( found )
            {
                AssociateFootprint( CVPCB_ASSOCIATION( kk, equivItem.m_FootprintFPID ), firstAssoc );
                firstAssoc = false;
                break;
            }
        }

        if( found )
        {
            continue;
        }
        else if( !fpid_candidate.IsEmpty() )
        {
            AssociateFootprint( CVPCB_ASSOCIATION( kk, fpid_candidate ), firstAssoc );
            firstAssoc = false;
            continue;
        }

        // obviously the last chance: there's only one filter matching one footprint
        if( component->GetFootprintFilters().GetCount() == 1 )
        {
            // we do not need to analyze wildcards: single footprint do not
            // contain them and if there are wildcards it just will not match any
            if( m_FootprintsList->GetFootprintInfo( component->GetFootprintFilters()[0] ) )
            {
                AssociateFootprint( CVPCB_ASSOCIATION( kk, component->GetFootprintFilters()[0] ), firstAssoc );
                firstAssoc = false;
            }
        }
    }

    if( !error_msg.IsEmpty() )
        wxMessageBox( error_msg, _( "CvPcb Warning" ), wxOK | wxICON_WARNING, this );

    m_skipComponentSelect = false;
    m_symbolsListBox->Refresh();
}

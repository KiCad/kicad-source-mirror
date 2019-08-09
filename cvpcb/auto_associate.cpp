/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <kiface_i.h>
#include <kicad_string.h>
#include <macros.h>

#include <auto_associate.h>
#include <cvpcb_association.h>
#include <cvpcb_mainframe.h>
#include <listboxes.h>

#define QUOTE   '\''


/*
 * read the string between quotes and put it in aTarget
 * put text in aTarget
 * return a pointer to the last read char (the second quote if OK)
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


// read the .equ files and populate the list of equivalents
int CVPCB_MAINFRAME::buildEquivalenceList( FOOTPRINT_EQUIVALENCE_LIST& aList, wxString * aErrorMessages )
{
    char        line[1024];
    int         error_count = 0;
    FILE*       file;
    wxFileName  fn;
    wxString    tmp, error_msg;

    SEARCH_STACK& search = Kiface().KifaceSearch();

    // Find equivalences in all available files, and populates the
    // equiv_List with all equivalences found in .equ files
    for( unsigned ii = 0; ii < m_EquFilesNames.GetCount(); ii++ )
    {
        fn =  wxExpandEnvVars( m_EquFilesNames[ii] );

        tmp = search.FindValidPath( fn.GetFullPath() );

        if( !tmp )
        {
            error_count++;

            if( aErrorMessages )
            {
                error_msg.Printf( _( "Equivalence file \"%s\" could not be found in the "
                                     "default search paths." ),
                            GetChars( fn.GetFullName() ) );

                if( ! aErrorMessages->IsEmpty() )
                    *aErrorMessages << wxT("\n\n");

                *aErrorMessages += error_msg;
            }

            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            error_count++;

            if( aErrorMessages )
            {
                error_msg.Printf( _( "Error opening equivalence file \"%s\"." ), GetChars( tmp ) );

                if( ! aErrorMessages->IsEmpty() )
                    *aErrorMessages << wxT("\n\n");

                *aErrorMessages += error_msg;
            }

            continue;
        }

        while( GetLine( file, line, NULL, sizeof( line ) ) != NULL )
        {
            if( *line == 0 )
                continue;

            wxString wtext = FROM_UTF8( line );
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
    FOOTPRINT_EQUIVALENCE_LIST equiv_List;
    wxString             msg, error_msg;

    if( m_netlist.IsEmpty() )
        return;

    if( buildEquivalenceList( equiv_List, &error_msg ) )
        wxMessageBox( error_msg, _( "Equivalence File Load Error" ), wxOK |  wxICON_WARNING, this );

    // Sort the association list by component value.
    // When sorted, find duplicate definitions (i.e. 2 or more items
    // having the same component value) is more easy.
    std::sort( equiv_List.begin(), equiv_List.end(), sortListbyCmpValue );

    // Display the number of footprint/component equivalences.
    msg.Printf( _( "%lu footprint/cmp equivalences found." ), (unsigned long)equiv_List.size() );
    SetStatusText( msg, 0 );

    // Now, associate each free component with a footprint, when the association
    // is found in list
    m_skipComponentSelect = true;
    error_msg.Empty();

    bool firstAssoc = true;
    for( unsigned kk = 0;  kk < m_netlist.GetCount();  kk++ )
    {
        COMPONENT* component = m_netlist.GetComponent( kk );

        bool found = false;

        if( !component->GetFPID().empty() ) // the component has already a footprint
            continue;

        // Here a first attempt is made. We can have multiple equivItem of the same value.
        // When happens, using the footprint filter of components can remove the ambiguity by
        // filtering equivItem so one can use multiple equiv_List (for polar and
        // non-polar caps for example)
        wxString fpid_candidate;

        for( unsigned idx = 0; idx < equiv_List.size(); idx++ )
        {
            FOOTPRINT_EQUIVALENCE& equivItem = equiv_List[idx];

            if( equivItem.m_ComponentValue.CmpNoCase( component->GetValue() ) != 0 )
                continue;

            const FOOTPRINT_INFO *module = m_FootprintsList->GetModuleInfo( equivItem.m_FootprintFPID );

            bool equ_is_unique = true;
            unsigned next = idx+1;
            int  previous = idx-1;

            if( next < equiv_List.size() &&
                equivItem.m_ComponentValue == equiv_List[next].m_ComponentValue )
                equ_is_unique = false;

            if( previous >= 0 &&
                equivItem.m_ComponentValue == equiv_List[previous].m_ComponentValue )
                equ_is_unique = false;

            // If the equivalence is unique, no ambiguity: use the association
            if( module && equ_is_unique )
            {
                AssociateFootprint( CVPCB_ASSOCIATION( kk, equivItem.m_FootprintFPID ),
                        firstAssoc );
                firstAssoc = false;
                found = true;
                break;
            }

            // Store the first candidate found in list, when equivalence is not unique
            // We use it later.
            if( module && fpid_candidate.IsEmpty() )
                fpid_candidate = equivItem.m_FootprintFPID;

            // The equivalence is not unique: use the footprint filter to try to remove
            // ambiguity
            // if the footprint filter does not remove ambiguity, we will use fpid_candidate
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
                            GetChars( equivItem.m_FootprintFPID ) );

                if( ! error_msg.IsEmpty() )
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
            continue;
        else if( !fpid_candidate.IsEmpty() )
        {
            AssociateFootprint( CVPCB_ASSOCIATION( kk, fpid_candidate ), firstAssoc );
            firstAssoc = false;
            continue;
        }

        // obviously the last chance: there's only one filter matching one footprint
        if( 1 == component->GetFootprintFilters().GetCount() )
        {
            // we do not need to analyze wildcards: single footprint do not
            // contain them and if there are wildcards it just will not match any
            const FOOTPRINT_INFO* module = m_FootprintsList->GetModuleInfo( component->GetFootprintFilters()[0] );

            if( module )
            {
                AssociateFootprint( CVPCB_ASSOCIATION( kk, component->GetFootprintFilters()[0] ),
                        firstAssoc );
                firstAssoc = false;
            }
        }
    }

    if( !error_msg.IsEmpty() )
        wxMessageBox( error_msg, _( "CvPcb Warning" ), wxOK | wxICON_WARNING, this );

    m_skipComponentSelect = false;
    m_compListBox->Refresh();
}

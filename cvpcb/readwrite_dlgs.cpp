/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <confirm.h>
#include <footprint_library_adapter.h>
#include <dialogs/html_message_box.h>
#include <kiway.h>
#include <lib_id.h>
#include <richio.h>
#include <string_utils.h>

#include <cvpcb_mainframe.h>
#include <fp_conflict_assignment_selector.h>
#include <project_pcb.h>
#include <wx/msgdlg.h>


/**
 * Return true if the resultant LIB_ID has a certain nickname.
 *
 * The guess is only made if this footprint resides in only one library.
 *
 * @return int - 0 on success, 1 on not found, 2 on ambiguous i.e. multiple matches.
 */
static int guessNickname( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, LIB_ID* aFootprintId )
{
    if( aFootprintId->GetLibNickname().size() )
        return 0;

    wxString    nick;
    wxString    fpname = aFootprintId->GetLibItemName();

    std::vector<wxString> nicks = aAdapter->GetLibraryNames();

    // Search each library going through libraries alphabetically.
    for( unsigned libNdx = 0; libNdx < nicks.size(); ++libNdx )
    {
        for( const wxString& name : aAdapter->GetFootprintNames( nicks[libNdx], true ) )
        {
            if( fpname == name )
            {
                if( !nick )
                    nick = nicks[libNdx];
                else
                    return 2;       // duplicate, the guess would not be certain
            }
        }
    }

    if( nick.size() )
    {
        aFootprintId->SetLibNickname( nick );
        return 0;
    }

    return 1;
}


bool CVPCB_MAINFRAME::readNetListAndFpFiles( const std::string& aNetlist )
{
    wxString        msg;
    bool            hasMissingNicks = false;

    readSchematicNetlist( aNetlist );

    if( m_symbolsListBox == nullptr )
        return false;

    wxSafeYield();

    LoadFootprintFiles();

    BuildFootprintsList();
    BuildLibrariesList();

    m_symbolsListBox->Clear();

    if( m_netlist.AnyFootprintsLinked() )
    {
        for( unsigned i = 0; i < m_netlist.GetCount(); i++ )
        {
            COMPONENT* component = m_netlist.GetComponent( i );

            if( component->GetFPID().empty() )
                continue;

            if( component->GetFPID().IsLegacy() )
                hasMissingNicks = true;
        }
    }

    // Check if footprint links were generated before the footprint library table was implemented.
    if( hasMissingNicks )
    {
        msg = _( "Some of the assigned footprints are legacy entries with no library names. Would "
                 "you like KiCad to attempt to convert them to the new required LIB_ID format? "
                 "(If you answer no, then these assignments will be cleared and you will need to "
                 "re-assign them manually.)" );

        if( IsOK( this, msg ) )
        {
            msg.Clear();

            try
            {
                for( unsigned i = 0; i < m_netlist.GetCount(); i++ )
                {
                    COMPONENT* component = m_netlist.GetComponent( i );

                    if( component->GetFPID().IsLegacy() )
                    {
                        // get this first here, it's possibly obsoleted if we get it too soon.
                        FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

                        int guess = guessNickname( adapter, (LIB_ID*) &component->GetFPID() );

                        switch( guess )
                        {
                        case 0:
                            m_modified = true;
                            break;

                        case 1:
                            msg += wxString::Format( _( "Component '%s' footprint '%s' <b>not "
                                                        "found</b> in any library.\n" ),
                                                     component->GetReference(),
                                                     component->GetFPID().GetLibItemName().wx_str() );
                            break;

                        case 2:
                            msg += wxString::Format( _( "Component '%s' footprint '%s' was found "
                                                        "in <b>multiple</b> libraries.\n" ),
                                                     component->GetReference(),
                                                     component->GetFPID().GetLibItemName().wx_str() );
                            break;
                        }
                    }
                }
            }
            catch( const IO_ERROR& ioe )
            {
                msg = ioe.What();
                msg += wxT( "\n\n" );
                msg += _( "First check your footprint library table entries." );

                wxMessageBox( msg, _( "Problematic Footprint Library Tables" ) );
                return false;
            }

            if( msg.size() )
            {
                HTML_MESSAGE_BOX dlg( this, wxEmptyString );

                dlg.MessageSet( _( "The following errors occurred attempting to convert the "
                                   "footprint assignments:\n\n" ) );
                dlg.ListSet( msg );
                dlg.MessageSet( _( "\nYou will need to reassign them manually if you want them "
                                   "to be updated correctly the next time you import the "
                                   "netlist in Pcbnew." ) );

#if 1
                dlg.ShowModal();
#else
                dlg.Fit();

                // Modeless lets user watch while fixing the problems, but its not working.
                dlg.Show( true );
#endif
            }
        }
        else
        {
            // Clear the legacy footprint assignments.
            for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
            {
                COMPONENT* component = m_netlist.GetComponent( i );

                if( component->GetFPID().IsLegacy() )
                {
                    component->SetFPID( LIB_ID() );
                    m_modified = true;
                }
            }
        }
    }


    // Display a dialog to select footprint selection, if the netlist
    // and the .cmp file give 2 different valid footprints
    std::vector <int > m_indexes;   // indexes of footprints in netlist

    for( unsigned ii = 0; ii < m_netlist.GetCount(); ii++ )
    {
        COMPONENT* component = m_netlist.GetComponent( ii );

        if( component->GetAltFPID().empty() )
            continue;

        if( component->GetFPID().IsLegacy() || component->GetAltFPID().IsLegacy() )
            continue;

        m_indexes.push_back( ii );
    }

    // If a n assignment conflict is found,
    // open a dialog to chose between schematic assignment
    // and .cmp file assignment:
    if( m_indexes.size() > 0 )
    {
        DIALOG_FP_CONFLICT_ASSIGNMENT_SELECTOR dlg( this );

        for( unsigned ii = 0; ii < m_indexes.size(); ii++ )
        {
            COMPONENT* component = m_netlist.GetComponent( m_indexes[ii] );

            wxString cmpfpid = component->GetFPID().Format();
            wxString schfpid = component->GetAltFPID().Format();

            dlg.Add( component->GetReference(), schfpid, cmpfpid );
        }

        if( dlg.ShowModal() == wxID_OK )
        {
            // Update the fp selection:
            for( unsigned ii = 0; ii < m_indexes.size(); ii++ )
            {
                COMPONENT* component = m_netlist.GetComponent( m_indexes[ii] );

                int choice = dlg.GetSelection( component->GetReference() );

                if( choice == 0 )   // the schematic (alt fpid) is chosen:
                    component->SetFPID( component->GetAltFPID() );
            }
        }
    }

    int firstUnassigned = wxNOT_FOUND;

    // Populates the component list box:
    for( unsigned i = 0; i < m_netlist.GetCount(); i++ )
    {
        COMPONENT* component = m_netlist.GetComponent( i );

        msg = formatSymbolDesc( m_symbolsListBox->GetCount() + 1,
                                component->GetReference(),
                                component->GetValue(),
                                From_UTF8( component->GetFPID().Format().c_str() ) );

        m_symbolsListBox->AppendLine( msg );

        if( firstUnassigned == wxNOT_FOUND && component->GetFPID().empty() )
            firstUnassigned = i;

        if( !m_FootprintsList->GetFootprintInfo( component->GetFPID().Format().wx_str() ) )
            m_symbolsListBox->AppendWarning( i );
    }

    if( firstUnassigned >= 0 )
        m_symbolsListBox->SetSelection( firstUnassigned, true );

    DisplayStatus();

    return true;
}


bool CVPCB_MAINFRAME::SaveFootprintAssociation( bool doSaveSchematic )
{
    std::string      payload;
    STRING_FORMATTER sf;

    m_netlist.FormatCvpcbNetlist( &sf );

    payload = sf.GetString();
    Kiway().ExpressMail( FRAME_SCH, MAIL_ASSIGN_FOOTPRINTS, payload );

    if( doSaveSchematic )
    {
        payload = "";
        Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_SAVE, payload );

        if( payload == "success" )
            SetStatusText( _( "Schematic saved" ), 1 );
    }

    // Changes are saved, so reset the flag
    m_modified = false;

    return true;
}

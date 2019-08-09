/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jean-pierre.charras
 * Copyright (C) 2011-2016 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <confirm.h>
#include <fctsys.h>
#include <fp_lib_table.h>
#include <html_messagebox.h>
#include <kiway.h>
#include <lib_id.h>
#include <macros.h>

#include <cvpcb_mainframe.h>
#include <listboxes.h>
#include <fp_conflict_assignment_selector.h>


/// Return true if the resultant LIB_ID has a certain nickname.  The guess
/// is only made if this footprint resides in only one library.
/// @return int - 0 on success, 1 on not found, 2 on ambiguous i.e. multiple matches
static int guessNickname( FP_LIB_TABLE* aTbl, LIB_ID* aFootprintId )
{
    if( aFootprintId->GetLibNickname().size() )
        return 0;

    wxString    nick;
    wxString    fpname = aFootprintId->GetLibItemName();

    std::vector<wxString> nicks = aTbl->GetLogicalLibs();

    // Search each library going through libraries alphabetically.
    for( unsigned libNdx = 0;  libNdx<nicks.size();  ++libNdx )
    {
        wxArrayString fpnames;

        aTbl->FootprintEnumerate( fpnames, nicks[libNdx] );

        for( unsigned nameNdx = 0;  nameNdx<fpnames.size();   ++nameNdx )
        {
            if( fpname == fpnames[nameNdx] )
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


bool CVPCB_MAINFRAME::ReadNetListAndFpFiles( const std::string& aNetlist )
{
    wxString        msg;
    bool            hasMissingNicks = false;

    ReadSchematicNetlist( aNetlist );

    if( m_compListBox == NULL )
        return false;

    LoadProjectFile();

    wxSafeYield();

    LoadFootprintFiles();

    BuildFOOTPRINTS_LISTBOX();
    BuildLIBRARY_LISTBOX();

    m_compListBox->Clear();

    if( m_netlist.AnyFootprintsLinked() )
    {
        for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
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
        msg = _(
            "Some of the assigned footprints are legacy entries (are missing lib nicknames). "
            "Would you like CvPcb to attempt to convert them to the new required LIB_ID format? "
            "(If you answer no, then these assignments will be cleared out and you will "
            "have to re-assign these footprints yourself.)"
            );

        if( IsOK( this, msg ) )
        {
            msg.Clear();

            try
            {
                for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
                {
                    COMPONENT* component = m_netlist.GetComponent( i );

                    if( component->GetFPID().IsLegacy() )
                    {
                        // get this first here, it's possibly obsoleted if we get it too soon.
                        FP_LIB_TABLE*   tbl = Prj().PcbFootprintLibs( Kiway() );

                        int guess = guessNickname( tbl, (LIB_ID*) &component->GetFPID() );

                        switch( guess )
                        {
                        case 0:
                            DBG(printf("%s: guessed OK ref:%s  fpid:%s\n", __func__,
                                TO_UTF8( component->GetReference() ), component->GetFPID().Format().c_str() );)
                            m_modified = true;
                            break;

                        case 1:
                            msg += wxString::Format( _(
                                    "Component \"%s\" footprint \"%s\" was <b>not found</b> in any library.\n" ),
                                    GetChars( component->GetReference() ),
                                    GetChars( component->GetFPID().GetLibItemName() )
                                    );
                            break;

                        case 2:
                            msg += wxString::Format( _(
                                    "Component \"%s\" footprint \"%s\" was found in <b>multiple</b> libraries.\n" ),
                                    GetChars( component->GetReference() ),
                                    GetChars( component->GetFPID().GetLibItemName() )
                                    );
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
                dlg.Show( true );   // modeless lets user watch while fixing the problems, but its not working.
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
                    component->SetFPID( LIB_ID() /* empty */ );
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

        if( component->GetFPID().IsLegacy() || component->GetAltFPID().IsLegacy())
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

    // Populates the component list box:
    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        COMPONENT* component = m_netlist.GetComponent( i );

        msg.Printf( CMP_FORMAT, m_compListBox->GetCount() + 1,
                    GetChars( component->GetReference() ),
                    GetChars( component->GetValue() ),
                    GetChars( FROM_UTF8( component->GetFPID().Format().c_str() ) ) );

        m_compListBox->AppendLine( msg );
    }

    if( !m_netlist.IsEmpty() )
        m_compListBox->SetSelection( 0, true );

    DisplayStatus();

    return true;
}


bool CVPCB_MAINFRAME::SaveFootprintAssociation( bool doSaveSchematic )
{
    std::string      payload;
    STRING_FORMATTER sf;

    m_netlist.FormatBackAnnotation( &sf );

    payload = sf.GetString();
    Kiway().ExpressMail( FRAME_SCH, MAIL_BACKANNOTATE_FOOTPRINTS, payload );

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

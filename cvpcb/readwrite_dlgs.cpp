/**
 * @file cvpcb/readwrite_dlgs.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jean-pierre.charras
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
#include <kiway.h>
#include <common.h>
#include <confirm.h>
#include <build_version.h>
#include <macros.h>
#include <fpid.h>
#include <fp_lib_table.h>
#include <reporter.h>
#include <html_messagebox.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>
#include <wildcards_and_files_ext.h>
#include <fp_conflict_assignment_selector.h>


void CVPCB_MAINFRAME::SetNewPkg( const wxString& aFootprintName )
{
    COMPONENT* component;
    bool       hasFootprint = false;
    int        componentIndex;

    if( m_netlist.IsEmpty() )
        return;

    // If no component is selected, select the first one
    if( m_compListBox->GetFirstSelected() < 0 )
    {
        componentIndex = 0;
        m_compListBox->SetSelection( componentIndex, true );
    }

    // iterate over the selection
    while( m_compListBox->GetFirstSelected() != -1 )
    {
        // Get the component for the current iteration
        componentIndex = m_compListBox->GetFirstSelected();
        component = m_netlist.GetComponent( componentIndex );

        if( component == NULL )
            return;

        // Check to see if the component has already a footprint set.
        hasFootprint = !component->GetFPID().empty();

        FPID fpid;

        if( !aFootprintName.IsEmpty() )
        {
            wxCHECK_RET( fpid.Parse( aFootprintName ) < 0,
                         wxString::Format( wxT( "<%s> is not a valid FPID." ),
                                           GetChars( aFootprintName ) ) );
        }

        component->SetFPID( fpid );

        // create the new component description
        wxString   description = wxString::Format( CMP_FORMAT, componentIndex + 1,
                            GetChars( component->GetReference() ),
                            GetChars( component->GetValue() ),
                            GetChars( FROM_UTF8( component->GetFPID().Format().c_str() ) ) );

        // If the component hasn't had a footprint associated with it
        // it now has, so we decrement the count of components without
        // a footprint assigned.
        if( !hasFootprint )
            m_undefinedComponentCnt -= 1;

        // Set the new description and deselect the processed component
        m_compListBox->SetString( componentIndex, description );
        m_compListBox->SetSelection( componentIndex, false );
    }

    // Mark this "session" as modified
    m_modified = true;

    // select the next component, if there is one
    if( componentIndex < (m_compListBox->GetCount() - 1) )
        componentIndex++;

    m_compListBox->SetSelection( componentIndex, true );

    // update the statusbar
    DisplayStatus();
}


/// Return true if the resultant FPID has a certain nickname.  The guess
/// is only made if this footprint resides in only one library.
/// @return int - 0 on success, 1 on not found, 2 on ambiguous i.e. multiple matches
static int guessNickname( FP_LIB_TABLE* aTbl, FPID* aFootprintId )
{
    if( aFootprintId->GetLibNickname().size() )
        return 0;

    wxString    nick;
    wxString    fpname = aFootprintId->GetFootprintName();

    std::vector<wxString> nicks = aTbl->GetLogicalLibs();

    // Search each library going through libraries alphabetically.
    for( unsigned libNdx = 0;  libNdx<nicks.size();  ++libNdx )
    {
        wxArrayString fpnames = aTbl->FootprintEnumerate( nicks[libNdx] );

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


bool CVPCB_MAINFRAME::ReadNetListAndLinkFiles()
{
    wxString        msg;
    bool            hasMissingNicks = false;

    ReadSchematicNetlist();

    if( m_compListBox == NULL )
        return false;

    LoadProjectFile();
    LoadFootprintFiles();

    BuildFOOTPRINTS_LISTBOX();
    BuildLIBRARY_LISTBOX();

    m_compListBox->Clear();
    m_undefinedComponentCnt = 0;

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
        msg = wxT(
            "Some of the assigned footprints are legacy entries (are missing lib nicknames). "
            "Would you like CvPcb to attempt to convert them to the new required FPID format? "
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
                        FP_LIB_TABLE*   tbl = Prj().PcbFootprintLibs();

                        int guess = guessNickname( tbl, (FPID*) &component->GetFPID() );

                        switch( guess )
                        {
                        case 0:
                            DBG(printf("%s: guessed OK ref:%s  fpid:%s\n", __func__,
                                TO_UTF8( component->GetReference() ), component->GetFPID().Format().c_str() );)
                            m_modified = true;
                            break;

                        case 1:
                            msg += wxString::Format( _(
                                    "Component '%s' footprint '%s' was <b>not found</b> in any library.\n" ),
                                    GetChars( component->GetReference() ),
                                    GetChars( component->GetFPID().GetFootprintName() )
                                    );
                            break;

                        case 2:
                            msg += wxString::Format( _(
                                    "Component '%s' footprint '%s' was found in <b>multiple</b> libraries.\n" ),
                                    GetChars( component->GetReference() ),
                                    GetChars( component->GetFPID().GetFootprintName() )
                                    );
                            break;
                        }
                    }
                }
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg = ioe.errorText;
                msg += wxT( "\n\n" );
                msg += _( "First check your fp-lib-table entries." );

                wxMessageBox( msg, wxT( "Problematic fp-lib-tables" ) );
                return false;
            }

            if( msg.size() )
            {
                HTML_MESSAGE_BOX dlg( this, wxEmptyString );

                dlg.MessageSet( wxT( "The following errors occurred attempting to convert the "
                                     "footprint assignments:\n\n" ) );
                dlg.ListSet( msg );
                dlg.MessageSet( wxT( "\nYou will need to reassign them manually if you want them "
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
                    component->SetFPID( FPID() /* empty */ );
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

        if( component->GetFPID().empty() )
        {
            m_undefinedComponentCnt += 1;
            continue;
        }
    }

    if( !m_netlist.IsEmpty() )
        m_compListBox->SetSelection( 0, true );

    DisplayStatus();

    UpdateTitle();

    UpdateFileHistory( m_NetlistFileName.GetFullPath() );

    return true;
}


int CVPCB_MAINFRAME::SaveCmpLinkFile( const wxString& aFullFileName )
{
    wxFileName fn;

    if( !aFullFileName.IsEmpty() )
    {
        fn = m_NetlistFileName;
        fn.SetExt( ComponentFileExtension );
    }
    else
    {
        wxFileDialog dlg( this, _( "Save Component Footprint Link File" ),
                          Prj().GetProjectPath(),
                          wxT( "noname" ), ComponentFileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return -1;

        fn = dlg.GetPath();

        if( !fn.HasExt() )
            fn.SetExt( ComponentFileExtension );
    }

    if( !IsWritable( fn.GetFullPath() ) || WriteComponentLinkFile( fn.GetFullPath() ) == 0 )
    {
        DisplayError( this,
            wxString::Format( _( "Unable to create component footprint link file '%s'" ),
                             fn.GetFullPath() ) );
        return 0;
    }

    wxString msg = wxString::Format( _("File %s saved"), GetChars( fn.GetFullPath() ) );

    // Perhaps this replaces all of the above someday.
    {
        STRING_FORMATTER sf;

        m_netlist.FormatBackAnnotation( &sf );

        Kiway().ExpressMail( FRAME_SCH, MAIL_BACKANNOTATE_FOOTPRINTS, sf.GetString() );
    }

    SetStatusText( msg );
    return 1;
}

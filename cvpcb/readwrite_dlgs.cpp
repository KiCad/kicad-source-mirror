/**
 * @file cvpcb/readwrite_dlgs.cpp
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

#define titleComponentLibErr _( "Component Library Error" )

void CVPCB_MAINFRAME::SetNewPkg( const wxString& aFootprintName )
{
    COMPONENT* component;
    bool       hasFootprint = false;
    int        componentIndex;
    wxString   description;

    if( m_netlist.IsEmpty() )
        return;

    // If no component is selected, select the first one
    if( m_ListCmp->GetFirstSelected() < 0 )
    {
        componentIndex = 0;
        m_ListCmp->SetSelection( componentIndex, true );
    }

    // iterate over the selection
    while( m_ListCmp->GetFirstSelected() != -1 )
    {
        // Get the component for the current iteration
        componentIndex = m_ListCmp->GetFirstSelected();
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
        description.Printf( CMP_FORMAT, componentIndex + 1,
                            GetChars( component->GetReference() ),
                            GetChars( component->GetValue() ),
                            GetChars( FROM_UTF8( component->GetFPID().Format().c_str() ) ) );

        // If the component hasn't had a footprint associated with it
        // it now has, so we decrement the count of components without
        // a footprint assigned.
        if( !hasFootprint )
        {
            hasFootprint = true;
            m_undefinedComponentCnt -= 1;
        }

        // Set the new description and deselect the processed component
        m_ListCmp->SetString( componentIndex, description );
        m_ListCmp->SetSelection( componentIndex, false );
    }

    // Mark this "session" as modified
    m_modified = true;

    // select the next component, if there is one
    if( componentIndex < (m_ListCmp->GetCount() - 1) )
        componentIndex++;

    m_ListCmp->SetSelection( componentIndex, true );

    // update the statusbar
    DisplayStatus();
}


bool CVPCB_MAINFRAME::ReadNetListAndLinkFiles()
{
    COMPONENT* component;
    wxString   msg;
    bool       isLegacy = true;

    ReadSchematicNetlist();

    if( m_ListCmp == NULL )
        return false;

    LoadProjectFile( m_NetlistFileName.GetFullPath() );
    LoadFootprintFiles();
    BuildFOOTPRINTS_LISTBOX();
    BuildLIBRARY_LISTBOX();

    m_ListCmp->Clear();
    m_undefinedComponentCnt = 0;

    if( m_netlist.AnyFootprintsLinked() )
    {
        for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
        {
            component = m_netlist.GetComponent( i );

            if( component->GetFPID().empty() )
                continue;

            if( isLegacy )
            {
                if( !component->GetFPID().IsLegacy() )
                    isLegacy = false;
            }
        }
    }
    else
    {
        isLegacy = false;  // None of the components have footprints assigned.
    }

    wxString missingLibs;

    // Check if footprint links were generated before the footprint library table was implemented.
    if( isLegacy )
    {
        if( m_footprintLibTable->MissingLegacyLibs( m_ModuleLibNames, &missingLibs ) )
        {
            msg = wxT( "The following legacy libraries are defined in the project file "
                       "were not found in the footprint library table:\n\n" ) + missingLibs;
            msg += wxT( "\nDo you want to update the footprint library table before "
                        "attempting to update the assigned footprints?" );

            if( IsOK( this, msg ) )
            {
                wxCommandEvent cmd;

                OnEditFootprintLibraryTable( cmd );
            }
        }

        msg = wxT( "Some or all of the assigned footprints contain legacy entries.  Would you "
                   "like CvPcb to attempt to convert them to the new footprint library table "
                   "format?" );

        if( IsOK( this, msg ) )
        {
            msg.Clear();
            WX_STRING_REPORTER reporter( &msg );

            if( !m_footprintLibTable->ConvertFromLegacy( m_netlist, m_ModuleLibNames, &reporter ) )
            {
                HTML_MESSAGE_BOX dlg( this, wxEmptyString );

                dlg.MessageSet( wxT( "The following errors occurred attempt to convert the "
                                     "footprint assignments:\n\n" ) );
                dlg.ListSet( msg );
                dlg.MessageSet( wxT( "\nYou will need to reassign them manually if you want them "
                                     "to be updated correctly the next time you import the "
                                     "netlist in Pcbnew." ) );
                dlg.ShowModal();
            }

            m_modified = true;
        }
        else
        {
            // Clear the legacy footprint assignments.
            for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
            {
                FPID emptyFPID;
                component = m_netlist.GetComponent( i );
                component->SetFPID( emptyFPID );
                m_modified = true;
            }
        }
    }

    for( unsigned i = 0;  i < m_netlist.GetCount();  i++ )
    {
        component = m_netlist.GetComponent( i );

        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    GetChars( component->GetReference() ),
                    GetChars( component->GetValue() ),
                    GetChars( FROM_UTF8( component->GetFPID().Format().c_str() ) ) );

        m_ListCmp->AppendLine( msg );

        if( component->GetFPID().empty() )
        {
            m_undefinedComponentCnt += 1;
            continue;
        }
    }

    if( !m_netlist.IsEmpty() )
        m_ListCmp->SetSelection( 0, true );

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
        wxFileDialog dlg( this, _( "Save Component Footprint Link File" ), wxEmptyString,
                          _( "Unnamed file" ), ComponentFileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return -1;

        fn = dlg.GetPath();

        if( !fn.HasExt() )
            fn.SetExt( ComponentFileExtension );

        // Save the project specific footprint library table.
        if( !m_footprintLibTable->IsEmpty( false ) )
        {
            wxFileName fpLibFileName = fn;
            fpLibFileName.ClearExt();
            fpLibFileName.SetName( FP_LIB_TABLE::GetFileName() );

            if( fpLibFileName.FileExists()
              && IsOK( this, _( "A footprint library table already exists in this path.\n\nDo "
                                "you want to overwrite it?" ) ) )
            {
                try
                {
                    m_footprintLibTable->Save( fpLibFileName );
                }
                catch( IO_ERROR& ioe )
                {
                    DisplayError( this,
                                  wxString::Format( _( "An error occurred attempting to save the "
                                                       "footprint library table <%s>\n\n%s" ),
                                                    GetChars( fpLibFileName.GetFullPath() ),
                                                    GetChars( ioe.errorText ) ) );
                }
            }
        }
    }

    if( !IsWritable( fn.GetFullPath() ) )
        return 0;

    if( WriteComponentLinkFile( fn.GetFullPath() ) == 0 )
    {
        DisplayError( this, _( "Unable to create component footprint link file (.cmp)" ) );
        return 0;
    }

    wxString msg;
    msg.Printf( _("File %s saved"), GetChars( fn.GetFullPath() ) );
    SetStatusText( msg );
    return 1;
}

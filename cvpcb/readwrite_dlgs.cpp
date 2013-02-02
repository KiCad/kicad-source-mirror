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

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <cvstruct.h>
#include <wildcards_and_files_ext.h>

#define titleComponentLibErr _( "Component Library Error" )

void CVPCB_MAINFRAME::SetNewPkg( const wxString& aFootprintName )
{
    COMPONENT_INFO* component;
    bool       hasFootprint = false;
    int        componentIndex;
    wxString   description;

    if( m_components.empty() )
        return;

    // if no component is selected, select the first one

    if(m_ListCmp->GetFirstSelected() < 0)
    {
        componentIndex = 0;
        m_ListCmp->SetSelection( componentIndex, true );
    }

    // iterate over the selection

    while( m_ListCmp->GetFirstSelected() != -1)
    {
        // get the component for the current iteration

        componentIndex = m_ListCmp->GetFirstSelected();
        component = &m_components[componentIndex];

        if( component == NULL )
            return;

        // check to see if the component has allready a footprint set.

        hasFootprint = !(component->m_Footprint.IsEmpty());

        component->m_Footprint = aFootprintName;

        // create the new component description

        description.Printf( CMP_FORMAT, componentIndex + 1,
                    GetChars( component->m_Reference ),
                    GetChars( component->m_Value ),
                    GetChars( component->m_Footprint ) );

        // if the component hasn't had a footprint associated with it
        // it now has, so we decrement the count of components without
        // a footprint assigned.

        if( !hasFootprint )
        {
            hasFootprint = true;
            m_undefinedComponentCnt -= 1;
        }

        // set the new description and deselect the processed component
        m_ListCmp->SetString( componentIndex, description );
        m_ListCmp->SetSelection( componentIndex, false );
    }

    // mark this "session" as modified
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
    wxString   msg;
    int        error_level;

    error_level = ReadSchematicNetlist();

    if( error_level < 0 )
    {
        msg.Printf( _( "File <%s> does not appear to be a valid KiCad net list file." ),
                    GetChars( m_NetlistFileName.GetFullPath() ) );
        wxMessageBox( msg, _( "File Error" ), wxOK | wxICON_ERROR, this );
        m_NetlistFileName.Clear();
        UpdateTitle();
        return false;
    }

    LoadComponentLinkFile( m_NetlistFileName.GetFullPath() );

    if( m_ListCmp == NULL )
        return false;

    LoadProjectFile( m_NetlistFileName.GetFullPath() );
    LoadFootprintFiles();
    BuildFOOTPRINTS_LISTBOX();

    m_ListCmp->Clear();
    m_undefinedComponentCnt = 0;

    BOOST_FOREACH( COMPONENT_INFO& component, m_components )
    {
        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    GetChars( component.m_Reference ),
                    GetChars( component.m_Value ),
                    GetChars( component.m_Footprint ) );
        m_ListCmp->AppendLine( msg );

        if( component.m_Footprint.IsEmpty() )
            m_undefinedComponentCnt += 1;
    }

    if( !m_components.empty() )
        m_ListCmp->SetSelection( 0, true );

    DisplayStatus();

    UpdateTitle();

    UpdateFileHistory( m_NetlistFileName.GetFullPath() );

    return true;
}


bool CVPCB_MAINFRAME::LoadComponentLinkFile( const wxString& aFileName )
{
    FILE*       linkfile;
    wxFileName  fn = aFileName;

    fn.SetExt( ComponentFileExtension );

    linkfile = wxFopen( fn.GetFullPath(), wxT( "rt" ) );
    if( linkfile == NULL )
    {
        wxString msg;
        msg.Printf( _( "Cannot open CvPcb component file <%s>." ),
                    GetChars( fn.GetFullPath() ) );
        msg << wxT( "\n" ) << _( "This is normal if you are opening a new netlist file" );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        return false;
    }

    // read and close the file
    if( ! ReadComponentLinkFile( linkfile ) )
    {
        wxString msg;
        msg.Printf( _( " <%s> does not appear to be a valid KiCad component link file." ),
                    GetChars( fn.GetFullPath() ) );
        wxMessageBox( msg, titleComponentLibErr, wxOK | wxICON_ERROR );
        return false;
    }

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
        wxFileDialog dlg( this, _( "Save Component/Footprint Link File" ), wxGetCwd(),
                          wxEmptyString, ComponentFileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return -1;

        fn = dlg.GetPath();

        if( !fn.HasExt() )
            fn.SetExt( ComponentFileExtension );
    }

    if( !IsWritable( fn.GetFullPath() ) )
        return 0;

    if( WriteComponentLinkFile( fn.GetFullPath() ) == 0 )
    {
        DisplayError( this, _( "Unable to create component file (.cmp)" ) );
        return 0;
    }

    wxString msg;
    msg.Printf( _("File %s saved"), GetChars( fn.GetFullPath() ) );
    SetStatusText( msg );
    return 1;
}

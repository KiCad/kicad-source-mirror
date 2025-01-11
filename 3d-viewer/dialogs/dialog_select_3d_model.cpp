/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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


#include <set>
#include "dialog_select_3d_model.h"
#include "project.h"
#include "3d_cache/3d_info.h"
#include "3d_cache/3d_cache.h"
#include <3d_model_viewer/eda_3d_model_viewer.h>
#include <common_ogl/ogl_attr_list.h>
#include <dialogs/dialog_configure_paths.h>
#include <filename_resolver.h>
#include <pcbnew/footprint.h>
#include <wx_filename.h>

#include <wx/filedlg.h>

#include <kiplatform/ui.h>


DIALOG_SELECT_3DMODEL::DIALOG_SELECT_3DMODEL( wxWindow* aParent, S3D_CACHE* aCacheManager,
                                              FP_3DMODEL* aModelItem, wxString& prevModelSelectDir,
                                              int& prevModelWildcard ) :
        DIALOG_SELECT_3D_MODEL_BASE( aParent ),
        m_model( aModelItem ),
        m_cache( aCacheManager ),
        m_resolver( aCacheManager ? aCacheManager->GetResolver() : nullptr ),
        m_previousDir( prevModelSelectDir ),
        m_modelViewer( nullptr )
{
    m_modelViewer = new EDA_3D_MODEL_VIEWER(
            m_pane3Dviewer, OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ), m_cache );
    m_modelViewer->SetMinSize( wxSize( 400, -1 ) );
    m_Sizer3Dviewer->Add( m_modelViewer, 1, wxEXPAND|wxRIGHT, 5 );

    // create the filter list
    if( NULL != m_cache )
    {
        std::list< wxString > const* fl = m_cache->GetFileFilters();
        std::list< wxString >::const_iterator sL = fl->begin();

        // The files filter string build from list of loaded plugins:
        wxString filter;

        // The files filter extensions only build from list of loaded plugins:
        wxString ext_list;

        while( sL != fl->end() )
        {
            if( !filter.IsEmpty() )
                filter.Append( "|" );

            filter.Append( *sL );

            if( !ext_list.IsEmpty() )
                ext_list.Append( ";" );

            wxString ext = sL->AfterLast( '|' );

            if( ext.Len() > 3 ) // Skip *.*
                ext_list.Append( ext );

            ++sL;
        }

        if( !filter.empty() )
        {
            if( !ext_list.empty() )
            {
                wxString full_filter;
                full_filter.Printf( _( "All supported files (%s)" ), ext_list );
                full_filter << '|' << ext_list << '|' << filter;
                m_FileTree->SetFilter( full_filter );
            }
            else
            {
                m_FileTree->SetFilter( filter );
            }
        }
        else
        {
            m_FileTree->SetFilter( wxFileSelectorDefaultWildcardStr );
        }

        if( prevModelWildcard >= 0 && prevModelWildcard < (int)fl->size() )
        {
            m_FileTree->SetFilterIndex( prevModelWildcard );
        }
        else
        {
            prevModelWildcard = 0;
            m_FileTree->SetFilterIndex( 0 );
        }
    }
    else
    {
        m_FileTree->SetFilter( wxFileSelectorDefaultWildcardStr );
        prevModelWildcard = 0;
        m_FileTree->SetFilterIndex( 0 );
    }

    // Fix the filter box on the file selector widget so that it always shows the start of
    // the filter string in the combobox. Otherwise it will only show the end of the string,
    // which is empty for all but the all supported filters option.
    wxChoice* filterBox = m_FileTree->GetFilterListCtrl();
    KIPLATFORM::UI::EllipsizeChoiceBox( filterBox );

    m_FileTree->SetPath( m_previousDir );
    updateDirChoiceList();

    m_modelViewer->Refresh();
    m_modelViewer->SetFocus();

    finishDialogSettings();
}


bool DIALOG_SELECT_3DMODEL::TransferDataFromWindow()
{
    if( !m_model || !m_FileTree )
        return true;

    m_model->m_Scale.x = 1.0;
    m_model->m_Scale.y = 1.0;
    m_model->m_Scale.z = 1.0;

    m_model->m_Rotation.x = 0.0;
    m_model->m_Rotation.y = 0.0;
    m_model->m_Rotation.z = 0.0;

    m_model->m_Offset = m_model->m_Rotation;
    m_model->m_Filename.clear();

    wxString name = m_FileTree->GetFilePath();

    if( name.empty() )
        return true;

    m_previousDir = m_FileTree->GetPath();

    // file selection mode: retrieve the filename and specify a
    // path relative to one of the config paths
    wxFileName fname = m_FileTree->GetFilePath();
    fname.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
    m_model->m_Filename = m_resolver->ShortenPath( fname.GetFullPath() );

    return true;
}


void DIALOG_SELECT_3DMODEL::OnSelectionChanged( wxCommandEvent& event )
{
    if( m_modelViewer )
        m_modelViewer->Set3DModel( m_FileTree->GetFilePath() );
}


void DIALOG_SELECT_3DMODEL::OnFileActivated( wxCommandEvent& event )
{
    if( m_modelViewer )
        m_modelViewer->Set3DModel( m_FileTree->GetFilePath() );

    EmulateButtonClickIfPresent( wxID_OK );
}


void DIALOG_SELECT_3DMODEL::SetRootDir( wxCommandEvent& event )
{
    if( m_FileTree && m_dirChoices->GetSelection() > 0 )
    {
        m_FileTree->SetPath( m_dirChoices->GetString( m_dirChoices->GetSelection() ) );
        m_FileTree->UnselectAll();  // Ensure no unwanted selection
    }
}


void DIALOG_SELECT_3DMODEL::Cfg3DPaths( wxCommandEvent& event )
{
    DIALOG_CONFIGURE_PATHS dlg( this );

    if( dlg.ShowQuasiModal() == wxID_OK )
        updateDirChoiceList();
}


void DIALOG_SELECT_3DMODEL::updateDirChoiceList( void )
{
    if( !m_FileTree || !m_resolver || !m_dirChoices )
        return;

    std::list< SEARCH_PATH > const* md = m_resolver->GetPaths();
    std::list< SEARCH_PATH >::const_iterator sL = md->begin();
    std::set< wxString > cl;
    wxString prjDir;

    // extract the current project dir
    if( sL != md->end() )
    {
        prjDir = sL->m_Pathexp;
        ++sL;
    }

    while( sL != md->end() )
    {
        if( !sL->m_Pathexp.empty() && sL->m_Pathexp.compare( prjDir ) )
            cl.insert( sL->m_Pathexp );

        ++sL;
    }

    if( !cl.empty() )
    {
        unsigned int choice = 0;
        m_dirChoices->Clear();
        m_dirChoices->Append( "" ); //Insert a blank string at the beginning to allow selection

        if( !prjDir.empty() )
        {
            m_dirChoices->Append( prjDir );

            if( prjDir == m_FileTree->GetPath() )
                choice = 1;
        }

        std::set< wxString >::const_iterator sI = cl.begin();

        while( sI != cl.end() )
        {
            if( *sI == m_FileTree->GetPath() )
                choice = m_dirChoices->GetCount();

            m_dirChoices->Append( *sI );
            ++sI;
        }

        m_dirChoices->SetSelection( choice );
    }

    return;
}

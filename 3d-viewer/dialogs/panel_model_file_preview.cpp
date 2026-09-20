/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#include "panel_model_file_preview.h"

#include <3d_model_viewer/eda_3d_model_viewer.h>
#include <common_ogl/ogl_attr_list.h>
#include <plugins/3dapi/c3dmodel.h>
#include <trace_helpers.h>

#include <wx/app.h>
#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/translation.h>


struct PANEL_MODEL_FILE_PREVIEW::LIFETIME
{
    PANEL_MODEL_FILE_PREVIEW* panel = nullptr;
};


PANEL_MODEL_FILE_PREVIEW::PANEL_MODEL_FILE_PREVIEW( wxWindow* aParent, REQUEST aRequest, CANCEL aCancel,
                                                    bool aEmbedFile ) :
        wxPanel( aParent ),
        m_request( std::move( aRequest ) ),
        m_cancel( std::move( aCancel ) ),
        m_lifetime( std::make_shared<LIFETIME>() )
{
    m_lifetime->panel = this;
    auto* sizer = new wxBoxSizer( wxVERTICAL );
    m_viewer = new EDA_3D_MODEL_VIEWER( this, OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_4X ) );
    m_viewer->SetMinSize( FromDIP( wxSize( 520, 340 ) ) );
    const int border = FromDIP( 4 );
    sizer->Add( m_viewer, 1, wxEXPAND | wxALL, border );

    m_embedFile = new wxCheckBox( this, wxID_ANY, _( "Embed file" ) );
    m_embedFile->SetValue( aEmbedFile );
    sizer->Add( m_embedFile, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, border );

    SetSizerAndFit( sizer );

    // The owning dialog cannot change for this panel's lifetime, and the handler below runs at
    // idle rate, so resolve it once rather than casting on every tick.
    m_dialog = dynamic_cast<wxFileDialog*>( wxGetTopLevelParent( this ) );

    Bind( wxEVT_UPDATE_UI,
          [this]( wxUpdateUIEvent& aEvent )
          {
              aEvent.Skip();

              if( m_dialog )
                  SetSelectedFile( m_dialog->GetCurrentlySelectedFilename() );
          } );
}


PANEL_MODEL_FILE_PREVIEW::~PANEL_MODEL_FILE_PREVIEW()
{
    wxLogTrace( traceModelPreview, wxS( "destroy generation=%llu" ), static_cast<unsigned long long>( m_generation ) );
    m_lifetime->panel = nullptr;

    if( m_cancel )
        m_cancel();

    // The viewer holds a raw pointer into storage owned by m_modelOwner, so it has to go before
    // that owner is released; wxPanel would otherwise destroy it after our members.
    delete m_viewer;
    m_viewer = nullptr;
    m_modelOwner.reset();
}


void PANEL_MODEL_FILE_PREVIEW::SetSelectedFile( const wxString& aPath )
{
    if( aPath == m_selectedFile )
        return;

    m_selectedFile = aPath;
    const uint64_t generation = ++m_generation;
    wxLogTrace( traceModelPreview, wxS( "select generation=%llu path=%s" ),
                static_cast<unsigned long long>( generation ), aPath.c_str() );
    m_viewer->Clear3DModel();
    m_modelOwner.reset();

    if( aPath.IsEmpty() || !wxFileName::FileExists( aPath ) )
    {
        if( m_cancel )
            m_cancel();

        return;
    }

    if( !m_request )
        return;

    std::shared_ptr<LIFETIME> lifetime = m_lifetime;

    m_request( aPath, generation,
               [lifetime]( uint64_t aCompletedGeneration, RESULT aResult ) mutable
               {
                   wxAppConsole* app = wxTheApp;

                   if( !app )
                       return;

                   app->CallAfter(
                           [lifetime, aCompletedGeneration, result = std::move( aResult )]() mutable
                           {
                               if( lifetime->panel )
                                   lifetime->panel->ApplyResult( aCompletedGeneration, std::move( result ) );
                               else
                                   wxLogTrace( traceModelPreview, wxS( "discard destroyed generation=%llu" ),
                                               static_cast<unsigned long long>( aCompletedGeneration ) );
                           } );
               } );
}


bool PANEL_MODEL_FILE_PREVIEW::GetEmbedFile() const
{
    return m_embedFile->GetValue();
}


void PANEL_MODEL_FILE_PREVIEW::ApplyResult( uint64_t aGeneration, RESULT aResult )
{
    if( aGeneration != m_generation )
    {
        wxLogTrace( traceModelPreview, wxS( "discard stale generation=%llu current=%llu" ),
                    static_cast<unsigned long long>( aGeneration ), static_cast<unsigned long long>( m_generation ) );
        return;
    }

    const bool usable = aResult.model && aResult.model->m_Materials && aResult.model->m_Meshes
                        && aResult.model->m_MaterialsSize > 0 && aResult.model->m_MeshesSize > 0;

    if( aResult.status == S3D::MODEL_IMPORT_ERROR::NONE && aResult.owner && usable )
    {
        std::shared_ptr<const void> previousOwner = std::move( m_modelOwner );
        m_modelOwner = aResult.owner;
        m_viewer->Set3DModel( *aResult.model );
        wxLogTrace( traceModelPreview, wxS( "ready generation=%llu" ), static_cast<unsigned long long>( aGeneration ) );
        return;
    }

    m_viewer->Clear3DModel();
    m_modelOwner.reset();

    if( !aResult.message.IsEmpty() )
        wxLogTrace( traceModelPreview, wxS( "import diagnostic: %s" ), aResult.message.c_str() );
}

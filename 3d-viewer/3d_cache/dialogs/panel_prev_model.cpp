/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

#include <3d_model_viewer/c3d_model_viewer.h>
#include <3d_rendering/3d_render_ogl_legacy/c_ogl_3dmodel.h>
#include <common_ogl/cogl_att_list.h>
#include <cstdlib>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/valnum.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/glcanvas.h>
#include <wx/dirctrl.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "project.h"
#include "3d_cache.h"
#include "3d_info.h"
#include "3d_filename_resolver.h"
#include "plugins/3dapi/ifsg_api.h"
#include "panel_prev_model.h"


// ensure -360 < rotation < 360
static void checkRotation( double& rot )
{
    if( rot >= 360.0 )
    {
        int n = rot / 360.0;
        rot -= 360.0 * (double)n;
    }
    else if( rot <= -360.0 )
    {
        int n = -rot / 360.0;
        rot += 360.0 * (double)n;
    }

    return;
}


enum {
    ID_SCALEX = wxID_LAST + 1,
    ID_SCALEY,
    ID_SCALEZ,
    ID_ROTX,
    ID_ROTY,
    ID_ROTZ,
    ID_OFFX,
    ID_OFFY,
    ID_OFFZ,
    ID_3D_ISO,
    ID_3D_UPDATE,
    ID_3D_LEFT,
    ID_3D_RIGHT,
    ID_3D_FRONT,
    ID_3D_BACK,
    ID_3D_TOP,
    ID_3D_BOTTOM
};

wxBEGIN_EVENT_TABLE( PANEL_PREV_3D, wxPanel)
    EVT_TEXT_ENTER( ID_SCALEX, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_SCALEY, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_SCALEZ, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_ROTX, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_ROTY, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_ROTZ, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_OFFX, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_OFFY, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT_ENTER( ID_OFFZ, PANEL_PREV_3D::updateOrientation )
    EVT_BUTTON( ID_3D_ISO, PANEL_PREV_3D::View3DISO )
    EVT_BUTTON( ID_3D_UPDATE, PANEL_PREV_3D::View3DUpdate )
    EVT_BUTTON( ID_3D_LEFT, PANEL_PREV_3D::View3DLeft )
    EVT_BUTTON( ID_3D_RIGHT, PANEL_PREV_3D::View3DRight )
    EVT_BUTTON( ID_3D_FRONT, PANEL_PREV_3D::View3DFront )
    EVT_BUTTON( ID_3D_BACK, PANEL_PREV_3D::View3DBack )
    EVT_BUTTON( ID_3D_TOP, PANEL_PREV_3D::View3DTop )
    EVT_BUTTON( ID_3D_BOTTOM, PANEL_PREV_3D::View3DBottom )
wxEND_EVENT_TABLE()


PANEL_PREV_3D::PANEL_PREV_3D( wxWindow* aParent, S3D_CACHE* aCacheManager ) :
    wxPanel( aParent, -1 ), m_ModelManager( aCacheManager )
{
    if( NULL != m_ModelManager )
        m_resolver = m_ModelManager->GetResolver();
    else
        m_resolver = NULL;

    canvas = NULL;
    model = NULL;
    xscale = NULL;
    yscale = NULL;
    zscale = NULL;
    xrot = NULL;
    yrot = NULL;
    zrot = NULL;
    xoff = NULL;
    yoff = NULL;
    zoff = NULL;

    wxBoxSizer* mainBox = new wxBoxSizer( wxVERTICAL );
    wxStaticBoxSizer* vbox = new wxStaticBoxSizer( wxVERTICAL, this, _( "3D Preview" ) );
    m_FileTree = NULL;

    if( NULL != aParent )
        m_FileTree = (wxGenericDirCtrl*)
            aParent->FindWindowByLabel( wxT( "3D_MODEL_SELECTOR" ), aParent );

    wxFloatingPointValidator< float > valScale( 4 );
    valScale.SetRange( 0.0001, 9999 );
    wxFloatingPointValidator< float > valRotate( 2 );
    valRotate.SetRange( -180.0, 180.0 );
    wxFloatingPointValidator< float > valOffset( 6 );

    wxStaticBoxSizer* vbScale = new wxStaticBoxSizer( wxVERTICAL, this, _( "Scale" )  );
    wxStaticBoxSizer* vbRotate = new wxStaticBoxSizer( wxVERTICAL, this, _( "Rotation" ) );
    wxStaticBoxSizer* vbOffset = new wxStaticBoxSizer( wxVERTICAL, this, _( "Offset (inches)" ) );

    wxStaticBox* modScale = vbScale->GetStaticBox();
    wxStaticBox* modRotate = vbRotate->GetStaticBox();
    wxStaticBox* modOffset = vbOffset->GetStaticBox();

    wxBoxSizer* hbS1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbS2 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbS3 = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* txtS1 = new wxStaticText( modScale, -1, wxT( "X:" ) );
    wxStaticText* txtS2 = new wxStaticText( modScale, -1, wxT( "Y:" ) );
    wxStaticText* txtS3 = new wxStaticText( modScale, -1, wxT( "Z:" ) );
    xscale = new wxTextCtrl( modScale, ID_SCALEX, "1.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valScale );
    yscale = new wxTextCtrl( modScale, ID_SCALEY, "1.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valScale );
    zscale = new wxTextCtrl( modScale, ID_SCALEZ, "1.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valScale );
    xscale->SetMaxLength( 9 );
    yscale->SetMaxLength( 9 );
    zscale->SetMaxLength( 9 );
    hbS1->Add( txtS1, 0, wxALL, 2 );
    hbS1->Add( xscale, 0, wxALL, 2 );
    hbS2->Add( txtS2, 0, wxALL, 2 );
    hbS2->Add( yscale, 0, wxALL, 2 );
    hbS3->Add( txtS3, 0, wxALL, 2 );
    hbS3->Add( zscale, 0, wxALL, 2 );
    vbScale->Add( hbS1, 0, wxEXPAND | wxALL, 2 );
    vbScale->Add( hbS2, 0, wxEXPAND | wxALL, 2 );
    vbScale->Add( hbS3, 0, wxEXPAND | wxALL, 2 );

    wxBoxSizer* hbR1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbR2 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbR3 = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* txtR1 = new wxStaticText( modRotate, ID_ROTX, wxT( "X:" ) );
    wxStaticText* txtR2 = new wxStaticText( modRotate, ID_ROTY, wxT( "Y:" ) );
    wxStaticText* txtR3 = new wxStaticText( modRotate, ID_ROTZ, wxT( "Z:" ) );
    xrot = new wxTextCtrl( modRotate, ID_ROTX, "0.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valRotate );
    yrot = new wxTextCtrl( modRotate, ID_ROTY, "0.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valRotate );
    zrot = new wxTextCtrl( modRotate, ID_ROTZ, "0.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valRotate );
    xrot->SetMaxLength( 9 );
    yrot->SetMaxLength( 9 );
    zrot->SetMaxLength( 9 );
    hbR1->Add( txtR1, 0, wxALL, 2 );
    hbR1->Add( xrot, 0, wxALL, 2 );
    hbR2->Add( txtR2, 0, wxALL, 2 );
    hbR2->Add( yrot, 0, wxALL, 2 );
    hbR3->Add( txtR3, 0, wxALL, 2 );
    hbR3->Add( zrot, 0, wxALL, 2 );
    vbRotate->Add( hbR1, 0, wxEXPAND | wxALL, 2 );
    vbRotate->Add( hbR2, 0, wxEXPAND | wxALL, 2 );
    vbRotate->Add( hbR3, 0, wxEXPAND | wxALL, 2 );

    wxBoxSizer* hbO1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbO2 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbO3 = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* txtO1 = new wxStaticText( modOffset, -1, wxT( "X:" ) );
    wxStaticText* txtO2 = new wxStaticText( modOffset, -1, wxT( "Y:" ) );
    wxStaticText* txtO3 = new wxStaticText( modOffset, -1, wxT( "Z:" ) );
    xoff = new wxTextCtrl( modOffset, ID_OFFX, "0.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valOffset );
    yoff = new wxTextCtrl( modOffset, ID_OFFY, "0.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valOffset );
    zoff = new wxTextCtrl( modOffset, ID_OFFZ, "0.0", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valOffset );
    xoff->SetMaxLength( 9 );
    yoff->SetMaxLength( 9 );
    zoff->SetMaxLength( 9 );
    hbO1->Add( txtO1, 0, wxALL, 2 );
    hbO1->Add( xoff, 0, wxALL, 2 );
    hbO2->Add( txtO2, 0, wxALL, 2 );
    hbO2->Add( yoff, 0, wxALL, 2 );
    hbO3->Add( txtO3, 0, wxALL, 2 );
    hbO3->Add( zoff, 0, wxALL, 2 );
    vbOffset->Add( hbO1, 0, wxEXPAND | wxALL, 2 );
    vbOffset->Add( hbO2, 0, wxEXPAND | wxALL, 2 );
    vbOffset->Add( hbO3, 0, wxEXPAND | wxALL, 2 );

    // hbox holding orientation data and preview
    wxBoxSizer* hbox = new wxBoxSizer( wxHORIZONTAL );
    // vbox holding orientation data
    wxBoxSizer* vboxOrient = new wxBoxSizer( wxVERTICAL );
    // vbox holding the preview and view buttons
    wxBoxSizer* vboxPrev = new wxBoxSizer( wxVERTICAL );

    vboxOrient->Add( vbScale, 0, wxALL, 5 );
    vboxOrient->Add( vbRotate, 0, wxALL, 5 );
    vboxOrient->Add( vbOffset, 0, wxALL, 5 );
    vboxOrient->AddSpacer( 20 );

    // add preview items
    preview = new wxPanel( this, -1 );
    preview->SetMinSize( wxSize( 320, 200 ) );
    preview->SetBackgroundColour( wxColor( 0, 0, 0 ));
    vboxPrev->Add( preview, 1, wxEXPAND | wxALIGN_CENTER | wxLEFT | wxRIGHT, 5 );
    // buttons:
    wxButton* vFront = new wxButton( this, ID_3D_FRONT, wxT( "F" ) );
    wxButton* vBack = new wxButton( this, ID_3D_BACK, wxT( "B" ) );
    wxButton* vLeft = new wxButton( this, ID_3D_LEFT, wxT( "L" ) );
    wxButton* vRight = new wxButton( this, ID_3D_RIGHT, wxT( "R" ) );
    wxButton* vTop = new wxButton( this, ID_3D_TOP, wxT( "T" ) );
    wxButton* vBottom = new wxButton( this, ID_3D_BOTTOM, wxT( "B" ) );
    wxButton* vISO = new wxButton( this, ID_3D_ISO, wxT( "I" ) );
    wxButton* vUpdate = new wxButton( this, ID_3D_UPDATE, wxT( "U" ) );
    wxBoxSizer* hbBT = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbBB = new wxBoxSizer( wxHORIZONTAL );
    hbBT->Add( vISO, 0, wxCENTER | wxALL, 3 );
    hbBT->Add( vLeft, 0, wxCENTER | wxALL, 3 );
    hbBT->Add( vFront, 0, wxCENTER | wxALL, 3 );
    hbBT->Add( vTop, 0, wxCENTER | wxALL, 3 );
    hbBT->AddSpacer( 17 );
    hbBB->Add( vUpdate, 0, wxCENTER | wxALL, 3 );
    hbBB->Add( vRight, 0, wxCENTER | wxALL, 3 );
    hbBB->Add( vBack, 0, wxCENTER | wxALL, 3 );
    hbBB->Add( vBottom, 0, wxCENTER | wxALL, 3 );
    hbBB->AddSpacer( 17 );

    vboxPrev->AddSpacer( 7 );
    vboxPrev->Add( hbBT, 0 );
    vboxPrev->Add( hbBB, 0 );

    // XXX - Suppress the buttons until the Renderer code is ready.
    // vboxPrev->Hide( preview, true );
    vboxPrev->Hide( hbBT, true );
    vboxPrev->Hide( hbBB, true );

    hbox->Add( vboxOrient, 0, wxALL, 5 );
    hbox->Add( vboxPrev, 1, wxEXPAND );
    vbox->Add( hbox, 1, wxEXPAND );

    mainBox->Add( vbox, 1, wxEXPAND | wxALL, 5 );

    if( NULL != m_FileTree )
    {
        // NOTE: if/when the File Selector preview is implemented
        // we may need to hide the orientation boxes to ensure the
        // users have sufficient display area for the browser.
        // hbox->Hide( vboxOrient, true );
        // XXX -
        // NOTE: for now we always suppress the preview and model orientation
        // panels while in the file selector
        //mainBox->Hide( vbox, true );

        hbox->Hide( vboxOrient, true );
        vboxPrev->Hide( hbBT, true );
        vboxPrev->Hide( hbBB, true );
    }

    SetSizerAndFit( mainBox );
    Centre();

    return;
}


PANEL_PREV_3D::~PANEL_PREV_3D()
{
    if( NULL != canvas )
    {
        canvas->Clear3DModel();
        canvas->Refresh();
        canvas->Update();
    }

    model = NULL;

    return;
}


void PANEL_PREV_3D::View3DISO( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Isometric View\n";
    return;
}


void PANEL_PREV_3D::View3DUpdate( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Update 3D View\n";

    // update the model filename if appropriate
    if( NULL != m_FileTree )
    {
        wxString modelName = m_FileTree->GetFilePath();
        UpdateModelName( modelName );
    }

    return;
}


void PANEL_PREV_3D::View3DLeft( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Left View\n";
    return;
}


void PANEL_PREV_3D::View3DRight( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Right View\n";
    return;
}


void PANEL_PREV_3D::View3DFront( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Front View\n";
    return;
}


void PANEL_PREV_3D::View3DBack( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Back View\n";
    return;
}


void PANEL_PREV_3D::View3DTop( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Top View\n";
    return;
}


void PANEL_PREV_3D::View3DBottom( wxCommandEvent& event )
{
    // XXX - TO BE IMPLEMENTED
    // std::cout << "Switch to Bottom View\n";
    return;
}


void PANEL_PREV_3D::GetModelData( S3D_INFO* aModel )
{
    if( NULL == aModel )
        return;

    // XXX - due to cross-platform differences in wxWidgets, extracting
    // scale/rotation/offset only works as expected when the preview
    // panel is not embedded in a file selector dialog. This conditional
    // execution should be removed once the cross-platform issues are
    // fixed.
    if( NULL == m_FileTree )
    {
        SGPOINT scale;
        SGPOINT rotation;
        SGPOINT offset;

        getOrientationVars( scale, rotation, offset );

        aModel->scale = scale;
        aModel->offset = offset;
        aModel->rotation = rotation;
    }

    // return if we are not in file selection mode
    if( NULL == m_FileTree )
        return;

    // file selection mode: retrieve the filename and specify a
    // path relative to one of the config paths
    wxFileName fname = m_FileTree->GetFilePath();
    fname.Normalize();
    aModel->filename = m_resolver->ShortenPath( fname.GetFullPath() );

    return;
}


void PANEL_PREV_3D::SetModelData( S3D_INFO const* aModel )
{
    xscale->SetValue( wxString::FromDouble( aModel->scale.x ) );
    yscale->SetValue( wxString::FromDouble( aModel->scale.y ) );
    zscale->SetValue( wxString::FromDouble( aModel->scale.z ) );

    xrot->SetValue( wxString::FromDouble( aModel->rotation.x ) );
    yrot->SetValue( wxString::FromDouble( aModel->rotation.y ) );
    zrot->SetValue( wxString::FromDouble( aModel->rotation.z ) );

    xoff->SetValue( wxString::FromDouble( aModel->offset.x ) );
    yoff->SetValue( wxString::FromDouble( aModel->offset.y ) );
    zoff->SetValue( wxString::FromDouble( aModel->offset.z ) );

    UpdateModelName( aModel->filename );

    return;
}


void PANEL_PREV_3D::UpdateModelName( wxString const& aModelName )
{
    bool newModel = false;

    modelInfo.filename = aModelName;

    // if the model name is a directory simply clear the current model
    if( aModelName.empty() || wxFileName::DirExists( aModelName ) )
    {
        currentModelFile.clear();
        modelInfo.filename.clear();
    }
    else
    {
        wxString newModelFile;
        newModelFile = m_resolver->ResolvePath( aModelName );

        if( !newModelFile.empty() && newModelFile.Cmp( currentModelFile ) )
            newModel = true;

        currentModelFile = newModelFile;
        modelInfo.filename = currentModelFile;
    }

    if( currentModelFile.empty() || newModel )
    {
        if( NULL != canvas )
        {
            canvas->Clear3DModel();
            canvas->Refresh();
            canvas->Update();
        }

        model = NULL;

        if( currentModelFile.empty() )
            return;
    }

    if( NULL != m_ModelManager )
        model = m_ModelManager->GetModel( modelInfo.filename );
    else
        model = NULL;

    if( NULL == model )
    {
        if( NULL != canvas )
        {
            canvas->Refresh();
            canvas->Update();
        }

        return;
    }

    if( NULL == canvas )
    {
        canvas = new C3D_MODEL_VIEWER( preview,
            COGL_ATT_LIST::GetAttributesList( true ) );

        wxSizer* ws = new wxBoxSizer( wxHORIZONTAL );
        canvas->Set3DModel( *model );
        ws->Add( canvas, 1, wxEXPAND );
        preview->SetSizer( ws );
        preview->Layout();
        ws->FitInside( preview );

        // Fixes bug in Windows (XP and possibly others) where the canvas requires the focus
        // in order to receive mouse events.  Otherwise, the user has to click somewhere on
        // the canvas before it will respond to mouse wheel events.
        canvas->SetFocus();
        return;
    }

    canvas->Set3DModel( *model );
    canvas->Refresh();
    canvas->Update();
    canvas->SetFocus();

    return;
}


void PANEL_PREV_3D::UpdateWindowUI( long flags )
{
    /*
     XXX -
     NOTE: until we figure out how to ensure that a Paint Event is
     generated for the File Selector's UI, we cannot display any
     preview within the file browser.
    if( wxUPDATE_UI_RECURSE == flags && m_FileDlg && m_ModelManager )
    {
        // check for a change in the current model file
        S3D_INFO info;
        modelInfo = info;
        UpdateModelName( m_FileDlg->GetCurrentlySelectedFilename() );
    }
    // */

    wxPanel::UpdateWindowUI( flags );

    return;
}


void PANEL_PREV_3D::updateOrientation( wxCommandEvent &event )
{
    // note: process even if the canvas is NULL since the user may
    // edit the filename to provide a valid file
    SGPOINT scale;
    SGPOINT rotation;
    SGPOINT offset;

    getOrientationVars( scale, rotation, offset );

    modelInfo.scale = scale;
    modelInfo.offset = offset;
    modelInfo.rotation = rotation;

    if( NULL == canvas )
        return;

    canvas->Clear3DModel();

    if( NULL != m_ModelManager )
        model = m_ModelManager->GetModel( modelInfo.filename );
    else
        model = NULL;

    if( model )
    {
        canvas->Set3DModel( *model );
        canvas->Refresh();
        canvas->Update();
    }

    event.Skip();
    return;
}


void PANEL_PREV_3D::getOrientationVars( SGPOINT& scale, SGPOINT& rotation, SGPOINT& offset )
{
    if( NULL == xscale || NULL == yscale || NULL == zscale
        || NULL == xrot || NULL == yrot || NULL == zrot
        || NULL == xoff || NULL == yoff || NULL == zoff )
    {
        return;
    }

    xscale->GetValue().ToDouble( &scale.x );
    yscale->GetValue().ToDouble( &scale.y );
    zscale->GetValue().ToDouble( &scale.z );

    if( 0.0001 > scale.x || 0.0001 > scale.y || 0.0001 > scale.z )
    {
        wxString errmsg = _("[INFO] invalid scale values; setting all to 1.0");
        wxLogMessage( "%s", errmsg.ToUTF8() );

        scale.x = 1.0;
        scale.y = 1.0;
        scale.z = 1.0;
    }

    xrot->GetValue().ToDouble( &rotation.x );
    yrot->GetValue().ToDouble( &rotation.y );
    zrot->GetValue().ToDouble( &rotation.z );
    checkRotation( rotation.x );
    checkRotation( rotation.y );
    checkRotation( rotation.z );

    xoff->GetValue().ToDouble( &offset.x );
    yoff->GetValue().ToDouble( &offset.y );
    zoff->GetValue().ToDouble( &offset.z );

    return;
}

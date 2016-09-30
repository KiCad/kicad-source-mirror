/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2015-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  panel_prev_model.cpp
 */

#include <3d_canvas/eda_3d_canvas.h>
#include <common_ogl/cogl_att_list.h>
#include <cstdlib>

#include <wx/valnum.h>
#include <wx/tglbtn.h>

#include "project.h"
#include "panel_prev_model.h"
#include <class_board.h>


/**
 * @brief checkRotation - ensure -360 < rotation < 360
 * @param rot: in out parameter
 */
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
    ID_SCALEX = ID_KICAD_PANEL_PREV_MODEL_START,
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
    ID_3D_BOTTOM,
    ID_3D_END = ID_KICAD_PANEL_PREV_MODEL_END
};


wxBEGIN_EVENT_TABLE( PANEL_PREV_3D, wxPanel)
    EVT_TEXT( ID_SCALEX, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_SCALEY, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_SCALEZ, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_ROTX, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_ROTY, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_ROTZ, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_OFFX, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_OFFY, PANEL_PREV_3D::updateOrientation )
    EVT_TEXT( ID_OFFZ, PANEL_PREV_3D::updateOrientation )

    EVT_TOGGLEBUTTON( ID_3D_ISO, PANEL_PREV_3D::View3DISO )
    EVT_BUTTON( ID_3D_UPDATE, PANEL_PREV_3D::View3DUpdate )
    EVT_BUTTON( ID_3D_LEFT, PANEL_PREV_3D::View3DLeft )
    EVT_BUTTON( ID_3D_RIGHT, PANEL_PREV_3D::View3DRight )
    EVT_BUTTON( ID_3D_FRONT, PANEL_PREV_3D::View3DFront )
    EVT_BUTTON( ID_3D_BACK, PANEL_PREV_3D::View3DBack )
    EVT_BUTTON( ID_3D_TOP, PANEL_PREV_3D::View3DTop )
    EVT_BUTTON( ID_3D_BOTTOM, PANEL_PREV_3D::View3DBottom )
    EVT_CLOSE( PANEL_PREV_3D::OnCloseWindow )
wxEND_EVENT_TABLE()


PANEL_PREV_3D::PANEL_PREV_3D( wxWindow* aParent,
                              S3D_CACHE* aCacheManager,
                              MODULE* aModuleCopy,
                              std::vector<S3D_INFO> *aParentInfoList ) :
    wxPanel( aParent, -1 )
{
    if( NULL != aCacheManager )
        m_resolver = aCacheManager->GetResolver();
    else
        m_resolver = NULL;

    m_currentSelectedIdx = -1;
    m_parentInfoList = aParentInfoList;
    m_previewPane = NULL;
    xscale = NULL;
    yscale = NULL;
    zscale = NULL;
    xrot = NULL;
    yrot = NULL;
    zrot = NULL;
    xoff = NULL;
    yoff = NULL;
    zoff = NULL;
    currentModelFile.clear();

    wxBoxSizer* mainBox = new wxBoxSizer( wxVERTICAL );
    wxStaticBoxSizer* vbox = new wxStaticBoxSizer( wxVERTICAL, this, _( "3D Preview" ) );

    wxFloatingPointValidator< float > valScale( 4 );
    valScale.SetRange( 0.0f, 9999.0f );
    wxFloatingPointValidator< float > valRotate( 2 );
    valRotate.SetRange( -180.0f, 180.0f );
    wxFloatingPointValidator< float > valOffset( 4 );
    valOffset.SetRange( -9999.0f, 9999.0f );

    wxStaticBoxSizer* vbScale  = new wxStaticBoxSizer( wxVERTICAL, this, _( "Scale" )  );
    wxStaticBoxSizer* vbRotate = new wxStaticBoxSizer( wxVERTICAL, this, _( "Rotation (degrees)" ) );

    const wxString offsetString = _( "Offset " ) + "(" + GetUnitsLabel( g_UserUnit ) + ")";
    wxStaticBoxSizer* vbOffset = new wxStaticBoxSizer( wxVERTICAL, this, offsetString );

    wxStaticBox* modScale  = vbScale->GetStaticBox();
    wxStaticBox* modRotate = vbRotate->GetStaticBox();
    wxStaticBox* modOffset = vbOffset->GetStaticBox();

    wxBoxSizer* hbS1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbS2 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbS3 = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* txtS1 = new wxStaticText( modScale, -1, wxT( "X:" ) );
    wxStaticText* txtS2 = new wxStaticText( modScale, -1, wxT( "Y:" ) );
    wxStaticText* txtS3 = new wxStaticText( modScale, -1, wxT( "Z:" ) );

    xscale = new wxTextCtrl( modScale, ID_SCALEX, "1", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valScale );
    yscale = new wxTextCtrl( modScale, ID_SCALEY, "1", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valScale );
    zscale = new wxTextCtrl( modScale, ID_SCALEZ, "1", wxDefaultPosition, wxDefaultSize,
        wxTE_PROCESS_ENTER, valScale );

    xscale->SetMaxLength( 9 );
    yscale->SetMaxLength( 9 );
    zscale->SetMaxLength( 9 );
    hbS1->Add( txtS1, 0, wxALIGN_CENTER, 2 );
    hbS1->Add( xscale, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    hbS2->Add( txtS2, 0, wxALIGN_CENTER, 2 );
    hbS2->Add( yscale, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    hbS3->Add( txtS3, 0, wxALIGN_CENTER, 2 );
    hbS3->Add( zscale, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    vbScale->Add( hbS1, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );
    vbScale->Add( hbS2, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );
    vbScale->Add( hbS3, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );

    wxBoxSizer* hbR1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbR2 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbR3 = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* txtR1 = new wxStaticText( modRotate, -1, wxT( "X:" ) );
    wxStaticText* txtR2 = new wxStaticText( modRotate, -1, wxT( "Y:" ) );
    wxStaticText* txtR3 = new wxStaticText( modRotate, -1, wxT( "Z:" ) );

    xrot = new wxTextCtrl( modRotate, ID_ROTX, "0", wxDefaultPosition, wxDefaultSize,
                           wxTE_PROCESS_ENTER, valRotate );
    yrot = new wxTextCtrl( modRotate, ID_ROTY, "0", wxDefaultPosition, wxDefaultSize,
                           wxTE_PROCESS_ENTER, valRotate );
    zrot = new wxTextCtrl( modRotate, ID_ROTZ, "0", wxDefaultPosition, wxDefaultSize,
                           wxTE_PROCESS_ENTER, valRotate );

    xrot->SetMaxLength( 9 );
    yrot->SetMaxLength( 9 );
    zrot->SetMaxLength( 9 );
    hbR1->Add( txtR1, 1, wxALIGN_CENTER, 2 );
    hbR1->Add( xrot, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    hbR2->Add( txtR2, 1, wxALIGN_CENTER, 2 );
    hbR2->Add( yrot, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    hbR3->Add( txtR3, 1, wxALIGN_CENTER, 2 );
    hbR3->Add( zrot, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    vbRotate->Add( hbR1, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );
    vbRotate->Add( hbR2, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );
    vbRotate->Add( hbR3, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );

    wxBoxSizer* hbO1 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbO2 = new wxBoxSizer( wxHORIZONTAL );
    wxBoxSizer* hbO3 = new wxBoxSizer( wxHORIZONTAL );
    wxStaticText* txtO1 = new wxStaticText( modOffset, -1, wxT( "X:" ) );
    wxStaticText* txtO2 = new wxStaticText( modOffset, -1, wxT( "Y:" ) );
    wxStaticText* txtO3 = new wxStaticText( modOffset, -1, wxT( "Z:" ) );

    // The default offset is 0.0 or 0,0, depending on floating point separator:
    xoff = new wxTextCtrl( modOffset, ID_OFFX, "0", wxDefaultPosition, wxDefaultSize,
                           wxTE_PROCESS_ENTER, valOffset );
    yoff = new wxTextCtrl( modOffset, ID_OFFY, "0", wxDefaultPosition, wxDefaultSize,
                           wxTE_PROCESS_ENTER, valOffset );
    zoff = new wxTextCtrl( modOffset, ID_OFFZ, "0", wxDefaultPosition, wxDefaultSize,
                           wxTE_PROCESS_ENTER, valOffset );
    xoff->SetMaxLength( 10 );
    yoff->SetMaxLength( 10 );
    zoff->SetMaxLength( 10 );
    hbO1->Add( txtO1, 0, wxALIGN_CENTER, 2 );
    hbO1->Add( xoff, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    hbO2->Add( txtO2, 0, wxALIGN_CENTER, 2 );
    hbO2->Add( yoff, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    hbO3->Add( txtO3, 0, wxALIGN_CENTER, 2 );
    hbO3->Add( zoff, 0, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    vbOffset->Add( hbO1, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );
    vbOffset->Add( hbO2, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );
    vbOffset->Add( hbO3, 1, wxEXPAND | wxLEFT | wxRIGHT, 6 );

    // hbox holding orientation data and preview
    wxBoxSizer* hbox = new wxBoxSizer( wxHORIZONTAL );
    // vbox holding orientation data
    wxBoxSizer* vboxOrient = new wxBoxSizer( wxVERTICAL );
    // vbox holding the preview and view buttons
    wxBoxSizer* vboxPrev = new wxBoxSizer( wxVERTICAL );

    vboxOrient->Add( vbScale, 1, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    vboxOrient->Add( vbRotate, 1, wxEXPAND | wxLEFT | wxRIGHT, 2 );
    vboxOrient->Add( vbOffset, 1, wxEXPAND | wxLEFT | wxRIGHT, 2 );

    // add preview items
    wxPanel*preview = new wxPanel( this, -1 );
    preview->SetMinSize( wxSize( 400, 250 ) );
    preview->SetBackgroundColour( wxColor( 0, 0, 0 ));
    vboxPrev->Add( preview, 1, wxEXPAND | wxALL, 5 );

    // buttons:
    wxButton* vFront = new wxButton( this, ID_3D_FRONT );
    vFront->SetBitmap( KiBitmap( axis3d_front_xpm ) );

    wxButton* vBack = new wxButton( this, ID_3D_BACK );
    vBack->SetBitmap( KiBitmap( axis3d_back_xpm ) );

    wxButton* vLeft = new wxButton( this, ID_3D_LEFT );
    vLeft->SetBitmap( KiBitmap( axis3d_left_xpm ) );

    wxButton* vRight = new wxButton( this, ID_3D_RIGHT );
    vRight->SetBitmap( KiBitmap( axis3d_right_xpm ) );

    wxButton* vTop = new wxButton( this, ID_3D_TOP );
    vTop->SetBitmap( KiBitmap( axis3d_top_xpm ) );

    wxButton* vBottom = new wxButton( this, ID_3D_BOTTOM );
    vBottom->SetBitmap( KiBitmap( axis3d_bottom_xpm ) );

    wxToggleButton* vISO = new wxToggleButton( this, ID_3D_ISO, wxT("") );
    vISO->SetBitmap( KiBitmap( ortho_xpm ) );
    vISO->SetToolTip( _("Change to isometric perspective") );

    wxButton* vUpdate = new wxButton( this, ID_3D_UPDATE );
    vUpdate->SetBitmap( KiBitmap( reload_xpm ) );
    vUpdate->SetToolTip( _("Reload board and 3D models") );

    wxGridSizer* gridSizer = new wxGridSizer( 2, 4, 0, 0 );

    gridSizer->Add( vISO, 0, wxEXPAND, 3 );
    gridSizer->Add( vLeft, 0, wxEXPAND, 3 );
    gridSizer->Add( vFront, 0, wxEXPAND, 3 );
    gridSizer->Add( vTop, 0, wxEXPAND, 3 );

    gridSizer->Add( vUpdate, 0, wxEXPAND, 3 );
    gridSizer->Add( vRight, 0, wxEXPAND, 3 );
    gridSizer->Add( vBack, 0, wxEXPAND, 3 );
    gridSizer->Add( vBottom, 0, wxEXPAND, 3 );

    vboxPrev->AddSpacer( 7 );
    vboxPrev->Add( gridSizer, 0, wxCENTER, 0 );

    hbox->Add( vboxOrient, 0, 0, 2 );
    hbox->Add( vboxPrev, 1, wxEXPAND | wxALL, 12 );
    vbox->Add( hbox, 1, wxEXPAND | wxALL, 0 );

    mainBox->Add( vbox, 1, wxEXPAND | wxALL, 5 );

    SetSizerAndFit( mainBox );

    // Create a dummy board
    m_dummyBoard = new BOARD();
    m_dummyBoard->Add( (MODULE*)aModuleCopy );
    m_copyModule = aModuleCopy;

    // Set 3d viewer configuration for preview
    m_settings3Dviewer = new CINFO3D_VISU();

    // Create the 3D canvas
    m_previewPane = new EDA_3D_CANVAS( preview,
                                       COGL_ATT_LIST::GetAttributesList( true ),
                                       m_dummyBoard,
                                       *m_settings3Dviewer,
                                       aCacheManager );

    wxSizer* ws = new wxBoxSizer( wxHORIZONTAL );
    ws->Add( m_previewPane, 1, wxEXPAND );
    preview->SetSizer( ws );
    preview->Layout();
    ws->FitInside( preview );

    m_previewPane->SetFocus(); // Need to catch mouse and keyboard events
}


PANEL_PREV_3D::~PANEL_PREV_3D()
{
    delete m_settings3Dviewer;
    m_settings3Dviewer = NULL;

    delete m_dummyBoard;
    m_dummyBoard = NULL;

    delete m_previewPane;
    m_previewPane = NULL;
}


void PANEL_PREV_3D::OnCloseWindow( wxCloseEvent &event )
{
    if( m_previewPane )
        m_previewPane->Close();

    event.Skip();
}


void PANEL_PREV_3D::View3DISO( wxCommandEvent& event )
{
    if( m_settings3Dviewer )
    {
        m_settings3Dviewer->CameraGet().ToggleProjection();
        m_previewPane->Refresh();
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DUpdate( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->ReloadRequest();
        m_previewPane->Refresh();
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DLeft( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->SetView3D( 'X' );
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DRight( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->SetView3D( 'x' );
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DFront( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->SetView3D( 'Y' );
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DBack( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->SetView3D( 'y' );
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DTop( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->SetView3D( 'z' );
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::View3DBottom( wxCommandEvent& event )
{
    if( m_previewPane )
    {
        m_previewPane->SetView3D( 'Z' );
        m_previewPane->SetFocus();
    }
}


void PANEL_PREV_3D::SetModelDataIdx( int idx, bool aReloadPreviewModule )
{
    wxASSERT( m_parentInfoList != NULL );

    if( m_parentInfoList && (idx >= 0) )
    {
        wxASSERT( (unsigned int)idx < (*m_parentInfoList).size() );

        if( (unsigned int)idx < (*m_parentInfoList).size() )
        {
            m_currentSelectedIdx = -1;  // In case that we receive events on the
                                        // next updates, it will set first an
                                        // invalid selection

            const S3D_INFO *aModel = (const S3D_INFO *)&((*m_parentInfoList)[idx]);

            xscale->SetValue( wxString::Format( "%.4f", aModel->m_Scale.x ) );
            yscale->SetValue( wxString::Format( "%.4f", aModel->m_Scale.y ) );
            zscale->SetValue( wxString::Format( "%.4f", aModel->m_Scale.z ) );

            xrot->SetValue( wxString::Format( "%.2f", aModel->m_Rotation.x ) );
            yrot->SetValue( wxString::Format( "%.2f", aModel->m_Rotation.y ) );
            zrot->SetValue( wxString::Format( "%.2f", aModel->m_Rotation.z ) );

            switch( g_UserUnit )
            {
            case MILLIMETRES:
                xoff->SetValue( wxString::Format( "%.4f", aModel->m_Offset.x * 25.4 ) );
                yoff->SetValue( wxString::Format( "%.4f", aModel->m_Offset.y * 25.4 ) );
                zoff->SetValue( wxString::Format( "%.4f", aModel->m_Offset.z * 25.4 ) );
                break;

            case INCHES:
                xoff->SetValue( wxString::Format( "%.4f", aModel->m_Offset.x ) );
                yoff->SetValue( wxString::Format( "%.4f", aModel->m_Offset.y ) );
                zoff->SetValue( wxString::Format( "%.4f", aModel->m_Offset.z ) );
                break;

            case DEGREES:
            case UNSCALED_UNITS:
            default:
                wxASSERT(0);
            }

            UpdateModelName( aModel->m_Filename );

            if( aReloadPreviewModule && m_previewPane )
            {
                updateListOnModelCopy();

                m_previewPane->ReloadRequest();
                m_previewPane->Request_refresh();
            }

            m_currentSelectedIdx = idx;
        }
    }

    if( m_previewPane )
        m_previewPane->SetFocus();

    return;
}


void PANEL_PREV_3D::ResetModelData( bool aReloadPreviewModule )
{
    m_currentSelectedIdx = -1;

    xscale->SetValue( wxString::FromDouble( 1.0 ) );
    yscale->SetValue( wxString::FromDouble( 1.0 ) );
    zscale->SetValue( wxString::FromDouble( 1.0 ) );

    xrot->SetValue( wxString::FromDouble( 0.0 ) );
    yrot->SetValue( wxString::FromDouble( 0.0 ) );
    zrot->SetValue( wxString::FromDouble( 0.0 ) );

    xoff->SetValue( wxString::FromDouble( 0.0 ) );
    yoff->SetValue( wxString::FromDouble( 0.0 ) );
    zoff->SetValue( wxString::FromDouble( 0.0 ) );

    // This will update the model on the preview board with the current list of 3d shapes
    if( aReloadPreviewModule )
    {
        updateListOnModelCopy();

        if( m_previewPane )
        {
            m_previewPane->ReloadRequest();
            m_previewPane->Request_refresh();
        }
    }

    if( m_previewPane )
        m_previewPane->SetFocus();
}


void PANEL_PREV_3D::UpdateModelName( wxString const& aModelName )
{
    bool newModel = false;

    m_modelInfo.m_Filename = aModelName;

    // if the model name is a directory simply clear the current model
    if( aModelName.empty() || wxFileName::DirExists( aModelName ) )
    {
        currentModelFile.clear();
        m_modelInfo.m_Filename.clear();
    }
    else
    {
        wxString newModelFile;

        if( m_resolver )
            newModelFile = m_resolver->ResolvePath( aModelName );

        if( !newModelFile.empty() && newModelFile.Cmp( currentModelFile ) )
            newModel = true;

        currentModelFile = newModelFile;
    }

    if( currentModelFile.empty() || newModel )
    {
        updateListOnModelCopy();

        if( m_previewPane )
        {
            m_previewPane->ReloadRequest();
            m_previewPane->Refresh();
        }

        if( currentModelFile.empty() )
            return;
    }
    else
    {
        if( m_previewPane )
            m_previewPane->Refresh();
    }

    if( m_previewPane )
        m_previewPane->SetFocus();

    return;
}


void PANEL_PREV_3D::updateOrientation( wxCommandEvent &event )
{
    wxTextCtrl *textCtrl = (wxTextCtrl *)event.GetEventObject();

    if( textCtrl == NULL )
        return;

    if( textCtrl->GetLineLength(0) == 0 )   // This will skip the got and event with empty field
        return;

    if( textCtrl->GetLineLength(0) == 1 )
        if( (textCtrl->GetLineText(0).compare( "." ) == 0) ||
            (textCtrl->GetLineText(0).compare( "," ) == 0) )
            return;

    SGPOINT scale;
    SGPOINT rotation;
    SGPOINT offset;

    getOrientationVars( scale, rotation, offset );

    m_modelInfo.m_Scale = scale;
    m_modelInfo.m_Offset = offset;
    m_modelInfo.m_Rotation = rotation;

    if( m_currentSelectedIdx >= 0 )
    {
        // This will update the parent list with the new data
        (*m_parentInfoList)[m_currentSelectedIdx] = m_modelInfo;

        // It will update the copy model in the preview board
        updateListOnModelCopy();

        // Since the OpenGL render does not need to be reloaded to update the
        // shapes position, we just request to redraw again the canvas
        if( m_previewPane )
            m_previewPane->Refresh();
    }
}


void PANEL_PREV_3D::getOrientationVars( SGPOINT& aScale, SGPOINT& aRotation, SGPOINT& aOffset )
{
    if( NULL == xscale || NULL == yscale || NULL == zscale
        || NULL == xrot || NULL == yrot || NULL == zrot
        || NULL == xoff || NULL == yoff || NULL == zoff )
    {
        return;
    }

    xscale->GetValue().ToDouble( &aScale.x );
    yscale->GetValue().ToDouble( &aScale.y );
    zscale->GetValue().ToDouble( &aScale.z );

    xrot->GetValue().ToDouble( &aRotation.x );
    yrot->GetValue().ToDouble( &aRotation.y );
    zrot->GetValue().ToDouble( &aRotation.z );

    checkRotation( aRotation.x );
    checkRotation( aRotation.y );
    checkRotation( aRotation.z );

    xoff->GetValue().ToDouble( &aOffset.x );
    yoff->GetValue().ToDouble( &aOffset.y );
    zoff->GetValue().ToDouble( &aOffset.z );

    switch( g_UserUnit )
    {
    case MILLIMETRES:
        // Convert to Inches. Offset is stored in inches.
        aOffset.x = aOffset.x / 25.4;
        aOffset.y = aOffset.y / 25.4;
        aOffset.z = aOffset.z / 25.4;
        break;

    case INCHES:
        // It is already in Inches
        break;

    case DEGREES:
    case UNSCALED_UNITS:
    default:
        wxASSERT(0);
    }

    return;
}


bool PANEL_PREV_3D::ValidateWithMessage( wxString& aErrorMessage )
{
    bool invalidScale = false;
    #define MIN_SCALE 0.001
    #define MAX_SCALE 1000.0

    for( unsigned int idx = 0; idx < m_parentInfoList->size(); ++idx )
    {
        wxString msg;
        bool addError = false;
        S3D_INFO& s3dshape = (*m_parentInfoList)[idx];

        SGPOINT scale = s3dshape.m_Scale;

        if( MIN_SCALE > scale.x || MAX_SCALE < scale.x )
        {
            invalidScale = true;
            addError = true;
            msg += _( "Invalid X scale" );
        }

        if( MIN_SCALE > scale.y || MAX_SCALE < scale.y )
        {
            invalidScale = true;
            addError = true;

            if( !msg.IsEmpty() )
                msg += "\n";

            msg += _( "Invalid Y scale" );
        }

        if( MIN_SCALE > scale.z || MAX_SCALE < scale.z )
        {
            invalidScale = true;
            addError = true;

            if( !msg.IsEmpty() )
                msg += "\n";

            msg += _( "Invalid Z scale" );
        }

        if( addError )
        {
            msg.Prepend( s3dshape.m_Filename + "\n" );

            if( !aErrorMessage.IsEmpty() )
                aErrorMessage += "\n\n";

            aErrorMessage += msg;
        }
    }

    if( !aErrorMessage.IsEmpty() )
    {
        aErrorMessage += "\n\n";
        aErrorMessage += wxString::Format( "Min value = %.4f and max value = %.4f",
                                           MIN_SCALE, MAX_SCALE );
    }

    return invalidScale == false;
}

void PANEL_PREV_3D::updateListOnModelCopy()
{
    std::list<S3D_INFO>* draw3D  = &m_copyModule->Models();
    draw3D->clear();
    draw3D->insert( draw3D->end(), m_parentInfoList->begin(), m_parentInfoList->end() );
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <limits.h>
#include <bitmaps.h>

#include <wx/valnum.h>
#include <wx/tglbtn.h>

#include "project.h"
#include "panel_prev_model.h"
#include <class_board.h>


PANEL_PREV_3D::PANEL_PREV_3D( wxWindow* aParent, S3D_CACHE* aCacheManager,
                                  MODULE* aModuleCopy,
                                  std::vector<S3D_INFO> *aParentInfoList ):
                PANEL_PREV_3D_BASE( aParent, wxID_ANY )
    {
        initPanel();

        if( NULL != aCacheManager )
            m_resolver = aCacheManager->GetResolver();

        m_parentInfoList = aParentInfoList;

        m_dummyBoard->Add( (MODULE*)aModuleCopy );
        m_copyModule = aModuleCopy;

        // Set 3d viewer configuration for preview
        m_settings3Dviewer = new CINFO3D_VISU();

        // Create the 3D canvas
        m_previewPane = new EDA_3D_CANVAS( this,
                                           COGL_ATT_LIST::GetAttributesList( true ),
                                           m_dummyBoard,
                                           *m_settings3Dviewer,
                                           aCacheManager );

        m_SizerPanelView->Add( m_previewPane, 1, wxEXPAND );

        m_previewPane->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler(
                            PANEL_PREV_3D::onEnterPreviewCanvas ), NULL, this );
    }


PANEL_PREV_3D::~PANEL_PREV_3D()
{
    m_previewPane->Disconnect( wxEVT_ENTER_WINDOW,
            wxMouseEventHandler( PANEL_PREV_3D::onEnterPreviewCanvas ), NULL, this );

    delete m_settings3Dviewer;
    delete m_dummyBoard;
    delete m_previewPane;
}


void PANEL_PREV_3D::initPanel()
{
    m_resolver = NULL;
    currentModelFile.clear();
    m_dummyBoard = new BOARD();
    m_currentSelectedIdx = -1;

    // Set the bitmap of 3D view buttons:
    m_bpvTop->SetBitmap( KiBitmap( axis3d_top_xpm ) );
    m_bpvFront->SetBitmap( KiBitmap( axis3d_front_xpm ) );
    m_bpvBack->SetBitmap( KiBitmap( axis3d_back_xpm ) );
    m_bpvLeft->SetBitmap( KiBitmap( axis3d_left_xpm ) );
    m_bpvRight->SetBitmap( KiBitmap( axis3d_right_xpm ) );
    m_bpvBottom->SetBitmap( KiBitmap( axis3d_bottom_xpm ) );
    m_bpvISO->SetBitmap( KiBitmap( ortho_xpm ) );
    m_bpUpdate->SetBitmap( KiBitmap( reload_xpm ) );

    // Set the min and max values of spin buttons (mandatory on Linux)
    // They are not used, so they are set to min and max 32 bits int values
    // (the min and max values supported by a wxSpinButton)
    // It avoids blocking the up or down arrows when reaching this limit after
    // a few clicks.
    wxSpinButton* spinButtonList[] =
    {
        m_spinXscale, m_spinYscale, m_spinZscale,
        m_spinXrot, m_spinYrot, m_spinZrot,
        m_spinXoffset,m_spinYoffset, m_spinZoffset
    };

    for( int ii = 0; ii < 9; ii++ )
        spinButtonList[ii]->SetRange( INT_MIN, INT_MAX );
}




/**
 * @brief checkRotation
 * Ensure -MAX_ROTATION <= rotation <= MAX_ROTATION
 * aRotation will be normalized between -MAX_ROTATION and MAX_ROTATION
 * @param aRotation: in out parameter
 */
static void checkRotation( double& aRotation )
{
    if( aRotation > MAX_ROTATION )
    {
        int n = aRotation / MAX_ROTATION;
        aRotation -= MAX_ROTATION * n;
    }
    else if( aRotation < -MAX_ROTATION )
    {
        int n = -aRotation / MAX_ROTATION;
        aRotation += MAX_ROTATION * n;
    }
}

static bool validateFloatTextCtrl( wxTextCtrl* aTextCtrl )
 {
    if( aTextCtrl == NULL )
        return false;

    if( aTextCtrl->GetLineLength(0) == 0 )   // This will skip the got and event with empty field
        return false;

    if( aTextCtrl->GetLineLength(0) == 1 )
    {
        if( (aTextCtrl->GetLineText(0).compare( "." ) == 0) ||
            (aTextCtrl->GetLineText(0).compare( "," ) == 0) )
            return false;
    }

    return true;
}


static void incrementTextCtrl( wxTextCtrl* aTextCtrl, double aInc, double aMinval, double aMaxval )
{
    if( !validateFloatTextCtrl( aTextCtrl ) )
        return;

    double curr_value = 0;

    aTextCtrl->GetValue().ToDouble( &curr_value );
    curr_value += aInc;

    if( curr_value > aMaxval )
        curr_value = aMaxval;

    if( curr_value < aMinval )
        curr_value = aMinval;

    aTextCtrl->SetValue( wxString::Format( "%.4f", curr_value ) );
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


void PANEL_PREV_3D::onIncrementRot( wxSpinEvent& event )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xrot;

    if( spinCtrl == m_spinYrot )
        textCtrl = yrot;
    else if( spinCtrl == m_spinZrot )
        textCtrl = zrot;

    incrementTextCtrl( textCtrl, ROTATION_INCREMENT, -MAX_ROTATION, MAX_ROTATION );
}


void PANEL_PREV_3D::onDecrementRot( wxSpinEvent& event )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xrot;

    if( spinCtrl == m_spinYrot )
        textCtrl = yrot;
    else if( spinCtrl == m_spinZrot )
        textCtrl = zrot;

    incrementTextCtrl( textCtrl, -ROTATION_INCREMENT, -MAX_ROTATION, MAX_ROTATION );
}


void PANEL_PREV_3D::onIncrementScale( wxSpinEvent& event )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xscale;

    if( spinCtrl == m_spinYscale )
        textCtrl = yscale;
    else if( spinCtrl == m_spinZscale )
        textCtrl = zscale;

    incrementTextCtrl( textCtrl, SCALE_INCREMENT, 1/MAX_SCALE, MAX_SCALE );
}


void PANEL_PREV_3D::onDecrementScale( wxSpinEvent& event )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xscale;

    if( spinCtrl == m_spinYscale )
        textCtrl = yscale;
    else if( spinCtrl == m_spinZscale )
        textCtrl = zscale;

    incrementTextCtrl( textCtrl, -SCALE_INCREMENT, 1/MAX_SCALE, MAX_SCALE );
}


void PANEL_PREV_3D::onIncrementOffset( wxSpinEvent& event )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xoff;

    if( spinCtrl == m_spinYoffset )
        textCtrl = yoff;
    else if( spinCtrl == m_spinZoffset )
        textCtrl = zoff;

    double step = OFFSET_INCREMENT_MM;

    if( g_UserUnit == INCHES )
        step = OFFSET_INCREMENT_MIL/1000.0;

    incrementTextCtrl( textCtrl, step, -MAX_OFFSET, MAX_OFFSET );
}


void PANEL_PREV_3D::onDecrementOffset( wxSpinEvent& event )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xoff;

    if( spinCtrl == m_spinYoffset )
        textCtrl = yoff;
    else if( spinCtrl == m_spinZoffset )
        textCtrl = zoff;

    double step = OFFSET_INCREMENT_MM;

    if( g_UserUnit == INCHES )
        step = OFFSET_INCREMENT_MIL/1000.0;

    incrementTextCtrl( textCtrl, -step, -MAX_OFFSET, MAX_OFFSET );
}


void PANEL_PREV_3D::onMouseWheelScale( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = SCALE_INCREMENT;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    incrementTextCtrl( textCtrl, step, 1/MAX_SCALE, MAX_SCALE );
}


void PANEL_PREV_3D::onMouseWheelRot( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = ROTATION_INCREMENT_WHEEL;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    incrementTextCtrl( textCtrl, step, -MAX_ROTATION, MAX_ROTATION );
}

void PANEL_PREV_3D::onMouseWheelOffset( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = OFFSET_INCREMENT_MM;

    if( g_UserUnit == INCHES )
        step = OFFSET_INCREMENT_MIL/1000.0;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    incrementTextCtrl( textCtrl, step, -MAX_OFFSET, MAX_OFFSET );
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

    for( unsigned int idx = 0; idx < m_parentInfoList->size(); ++idx )
    {
        wxString msg;
        bool addError = false;
        S3D_INFO& s3dshape = (*m_parentInfoList)[idx];

        SGPOINT scale = s3dshape.m_Scale;

        if( 1/MAX_SCALE > scale.x || MAX_SCALE < scale.x )
        {
            invalidScale = true;
            addError = true;
            msg += _( "Invalid X scale" );
        }

        if( 1/MAX_SCALE > scale.y || MAX_SCALE < scale.y )
        {
            invalidScale = true;
            addError = true;

            if( !msg.IsEmpty() )
                msg += "\n";

            msg += _( "Invalid Y scale" );
        }

        if( 1/MAX_SCALE > scale.z || MAX_SCALE < scale.z )
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
                                           1/MAX_SCALE, MAX_SCALE );
    }

    return invalidScale == false;
}

void PANEL_PREV_3D::updateListOnModelCopy()
{
    std::list<S3D_INFO>* draw3D  = &m_copyModule->Models();
    draw3D->clear();
    draw3D->insert( draw3D->end(), m_parentInfoList->begin(), m_parentInfoList->end() );
}

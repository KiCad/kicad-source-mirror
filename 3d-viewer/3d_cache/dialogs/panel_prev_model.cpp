/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_prev_model.h"
#include <3d_canvas/eda_3d_canvas.h>
#include <common_ogl/cogl_att_list.h>
#include <class_board.h>
#include <base_units.h>
#include <bitmaps.h>
#include <dpi_scaling.h>
#include <pgm_base.h>
#include <project.h>


PANEL_PREV_3D::PANEL_PREV_3D( wxWindow* aParent, PCB_BASE_FRAME* aFrame, MODULE* aModule,
                              std::vector<MODULE_3D_SETTINGS> *aParentModelList ) :
    PANEL_PREV_3D_BASE( aParent, wxID_ANY )
{
    m_userUnits = aFrame->GetUserUnits();

    initPanel();

    // Initialize the color settings to draw the board and the footprint
    m_dummyBoard->SetGeneralSettings( &aFrame->Settings() );

    m_parentModelList = aParentModelList;

    m_dummyModule = new MODULE( *aModule );
    m_dummyBoard->Add( m_dummyModule );

    // Set 3d viewer configuration for preview
    m_settings3Dviewer = new CINFO3D_VISU();

    // Create the 3D canvas
    m_previewPane = new EDA_3D_CANVAS( this, COGL_ATT_LIST::GetAttributesList( true ),
                                       m_dummyBoard, *m_settings3Dviewer,
                                       aFrame->Prj().Get3DCacheManager() );

    loadCommonSettings();

    m_SizerPanelView->Add( m_previewPane, 1, wxEXPAND, 5 );
}


PANEL_PREV_3D::~PANEL_PREV_3D()
{
    delete m_settings3Dviewer;
    delete m_dummyBoard;
    delete m_previewPane;
}


void PANEL_PREV_3D::initPanel()
{
    m_dummyBoard = new BOARD();
    m_selected = -1;

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

    for( wxSpinButton* button : spinButtonList )
        button->SetRange(INT_MIN, INT_MAX );
}


void PANEL_PREV_3D::loadCommonSettings()
{
    wxCHECK_RET( m_previewPane, "Cannot load settings to null canvas" );

    wxConfigBase& cmnCfg = *Pgm().CommonSettings();

    {
        const DPI_SCALING dpi{ &cmnCfg, this };
        m_previewPane->SetScaleFactor( dpi.GetScaleFactor() );
    }

    {
        bool option;
        cmnCfg.Read( ENBL_MOUSEWHEEL_PAN_KEY, &option, false );
        m_settings3Dviewer->SetFlag( FL_MOUSEWHEEL_PANNING, option );
    }
}


/**
 * @brief rotationFromString
 * Ensure -MAX_ROTATION <= rotation <= MAX_ROTATION
 * aRotation will be normalized between -MAX_ROTATION and MAX_ROTATION
 */
static double rotationFromString( const wxString& aValue )
{
    double rotation = DoubleValueFromString( DEGREES, aValue ) / 10.0;

    if( rotation > MAX_ROTATION )
    {
        int n = rotation / MAX_ROTATION;
        rotation -= MAX_ROTATION * n;
    }
    else if( rotation < -MAX_ROTATION )
    {
        int n = -rotation / MAX_ROTATION;
        rotation += MAX_ROTATION * n;
    }

    return rotation;
}


wxString PANEL_PREV_3D::formatScaleValue( double aValue )
{
    return wxString::Format( "%.4f", aValue );
}


wxString PANEL_PREV_3D::formatRotationValue( double aValue )
{
    return wxString::Format( "%.2f %s", aValue, GetAbbreviatedUnitsLabel( DEGREES ) );
}


wxString PANEL_PREV_3D::formatOffsetValue( double aValue )
{
    // Convert from internal units (mm) to user units
    if( m_userUnits == INCHES )
        aValue /= 25.4f;

    return wxString::Format( "%.4f %s", aValue, GetAbbreviatedUnitsLabel( m_userUnits ) );
}


void PANEL_PREV_3D::SetSelectedModel( int idx )
{
    if( m_parentModelList && idx >= 0 && idx < (int) m_parentModelList->size() )
    {
        m_selected = idx;
        const MODULE_3D_SETTINGS& modelInfo = m_parentModelList->at( (unsigned) m_selected );

        // Use ChangeValue() instead of SetValue().  It's not the user making the change, so we
        // don't want to generate wxEVT_GRID_CELL_CHANGED events.

        xscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.x ) );
        yscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.y ) );
        zscale->ChangeValue( formatScaleValue( modelInfo.m_Scale.z ) );

        xrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.x ) );
        yrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.y ) );
        zrot->ChangeValue( formatRotationValue( modelInfo.m_Rotation.z ) );

        xoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.x ) );
        yoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.y ) );
        zoff->ChangeValue( formatOffsetValue( modelInfo.m_Offset.z ) );
    }
    else
    {
        m_selected = -1;

        xscale->ChangeValue( wxEmptyString );
        yscale->ChangeValue( wxEmptyString );
        zscale->ChangeValue( wxEmptyString );

        xrot->ChangeValue( wxEmptyString );
        yrot->ChangeValue( wxEmptyString );
        zrot->ChangeValue( wxEmptyString );

        xoff->ChangeValue( wxEmptyString );
        yoff->ChangeValue( wxEmptyString );
        zoff->ChangeValue( wxEmptyString );
    }
}


void PANEL_PREV_3D::updateOrientation( wxCommandEvent &event )
{
    if( m_parentModelList && m_selected >= 0 && m_selected < (int) m_parentModelList->size() )
    {
        // Write settings back to the parent
        MODULE_3D_SETTINGS* modelInfo = &m_parentModelList->at( (unsigned) m_selected );

        modelInfo->m_Scale.x = DoubleValueFromString( UNSCALED_UNITS, xscale->GetValue() );
        modelInfo->m_Scale.y = DoubleValueFromString( UNSCALED_UNITS, yscale->GetValue() );
        modelInfo->m_Scale.z = DoubleValueFromString( UNSCALED_UNITS, zscale->GetValue() );

        modelInfo->m_Rotation.x = rotationFromString( xrot->GetValue() );
        modelInfo->m_Rotation.y = rotationFromString( yrot->GetValue() );
        modelInfo->m_Rotation.z = rotationFromString( zrot->GetValue() );

        modelInfo->m_Offset.x = DoubleValueFromString( m_userUnits, xoff->GetValue() ) / IU_PER_MM;
        modelInfo->m_Offset.y = DoubleValueFromString( m_userUnits, yoff->GetValue() ) / IU_PER_MM;
        modelInfo->m_Offset.z = DoubleValueFromString( m_userUnits, zoff->GetValue() ) / IU_PER_MM;

        // Update the dummy module for the preview
        UpdateDummyModule( false );
    }
}


void PANEL_PREV_3D::doIncrementScale( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xscale;

    if( spinCtrl == m_spinYscale )
        textCtrl = yscale;
    else if( spinCtrl == m_spinZscale )
        textCtrl = zscale;

    double curr_value = DoubleValueFromString( UNSCALED_UNITS, textCtrl->GetValue() );

    curr_value += ( SCALE_INCREMENT * aSign );
    curr_value = std::max( 1/MAX_SCALE, curr_value );
    curr_value = std::min( curr_value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( curr_value ) );
}


void PANEL_PREV_3D::doIncrementRotation( wxSpinEvent& aEvent, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) aEvent.GetEventObject();
    wxTextCtrl* textCtrl = xrot;

    if( spinCtrl == m_spinYrot )
        textCtrl = yrot;
    else if( spinCtrl == m_spinZrot )
        textCtrl = zrot;

    double curr_value = DoubleValueFromString( DEGREES, textCtrl->GetValue() ) / 10.0;

    curr_value += ( ROTATION_INCREMENT * aSign );
    curr_value = std::max( -MAX_ROTATION, curr_value );
    curr_value = std::min( curr_value, MAX_ROTATION );

    textCtrl->SetValue( formatRotationValue( curr_value ) );
}


void PANEL_PREV_3D::doIncrementOffset( wxSpinEvent& event, double aSign )
{
    wxSpinButton* spinCtrl = (wxSpinButton*) event.GetEventObject();

    wxTextCtrl * textCtrl = xoff;

    if( spinCtrl == m_spinYoffset )
        textCtrl = yoff;
    else if( spinCtrl == m_spinZoffset )
        textCtrl = zoff;

    double step = OFFSET_INCREMENT_MM;

    if( m_userUnits == INCHES )
        step = OFFSET_INCREMENT_MIL/1000.0;

    double curr_value = DoubleValueFromString( m_userUnits, textCtrl->GetValue() ) / IU_PER_MM;

    curr_value += ( step * aSign );
    curr_value = std::max( -MAX_OFFSET, curr_value );
    curr_value = std::min( curr_value, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( curr_value ) );
}


void PANEL_PREV_3D::onMouseWheelScale( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = SCALE_INCREMENT;

    if( event.ShiftDown( ) )
        step = SCALE_INCREMENT_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = DoubleValueFromString( UNSCALED_UNITS, textCtrl->GetValue() );

    curr_value += step;
    curr_value = std::max( 1/MAX_SCALE, curr_value );
    curr_value = std::min( curr_value, MAX_SCALE );

    textCtrl->SetValue( formatScaleValue( curr_value ) );
}


void PANEL_PREV_3D::onMouseWheelRot( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = ROTATION_INCREMENT_WHEEL;

    if( event.ShiftDown( ) )
        step = ROTATION_INCREMENT_WHEEL_FINE;

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = DoubleValueFromString( DEGREES, textCtrl->GetValue() ) / 10.0;

    curr_value += step;
    curr_value = std::max( -MAX_ROTATION, curr_value );
    curr_value = std::min( curr_value, MAX_ROTATION );

    textCtrl->SetValue( formatRotationValue( curr_value ) );
}


void PANEL_PREV_3D::onMouseWheelOffset( wxMouseEvent& event )
{
    wxTextCtrl* textCtrl = (wxTextCtrl*) event.GetEventObject();

    double step = OFFSET_INCREMENT_MM;

    if( event.ShiftDown( ) )
        step = OFFSET_INCREMENT_MM_FINE;

    if( m_userUnits == INCHES )
    {
        step = OFFSET_INCREMENT_MIL/1000.0;
        if( event.ShiftDown( ) )
            step = OFFSET_INCREMENT_MIL_FINE/1000.0;
    }

    if( event.GetWheelRotation() >= 0 )
        step = -step;

    double curr_value = DoubleValueFromString( m_userUnits, textCtrl->GetValue() ) / IU_PER_MM;

    curr_value += step;
    curr_value = std::max( -MAX_OFFSET, curr_value );
    curr_value = std::min( curr_value, MAX_OFFSET );

    textCtrl->SetValue( formatOffsetValue( curr_value ) );
}


void PANEL_PREV_3D::UpdateDummyModule( bool aReloadRequired )
{
    m_dummyModule->Models().clear();

    for( size_t i = 0; i < m_parentModelList->size(); ++i )
    {
        if( m_parentModelList->at( i ).m_Preview )
        {
            m_dummyModule->Models().insert( m_dummyModule->Models().end(),
                                            m_parentModelList->at( i ) );
        }
    }

    if( aReloadRequired )
        m_previewPane->ReloadRequest();

    m_previewPane->Request_refresh();
}

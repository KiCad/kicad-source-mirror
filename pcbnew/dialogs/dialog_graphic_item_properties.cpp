/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras <jp.charras@wanadoo.fr>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dialog_graphic_item_properties.cpp
 */

/* Edit parameters values of graphic items type DRAWSEGMENTS:
 * Lines
 * Circles
 * Arcs
 * used as graphic elements found on non copper layers in boards
 * items on edge layers are considered as graphic items
 * Pcb texts are not always graphic items and are not handled here
 */
#include <fctsys.h>
#include <macros.h>
#include <gr_basic.h>
#include <confirm.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <base_units.h>

#include <class_board.h>
#include <class_drawsegment.h>

#include <dialog_graphic_item_properties_base.h>
#include <class_pcb_layer_box_selector.h>
#include <html_messagebox.h>


class DIALOG_GRAPHIC_ITEM_PROPERTIES : public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    PCB_EDIT_FRAME*       m_parent;
    wxDC*                 m_DC;
    DRAWSEGMENT*          m_item;
    BOARD_DESIGN_SETTINGS m_brdSettings;

public:
    DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_EDIT_FRAME* aParent, DRAWSEGMENT* aItem, wxDC* aDC );
    ~DIALOG_GRAPHIC_ITEM_PROPERTIES() {};

private:
    void initDlg();
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
    void OnLayerChoice( wxCommandEvent& event );
    bool itemValuesOK();
};

DIALOG_GRAPHIC_ITEM_PROPERTIES::DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_EDIT_FRAME* aParent,
                                                                DRAWSEGMENT* aItem, wxDC* aDC ):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_DC = aDC;
    m_item = aItem;
    m_brdSettings = m_parent->GetDesignSettings();
    initDlg();
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void PCB_EDIT_FRAME::InstallGraphicItemPropertiesDialog( DRAWSEGMENT* aItem, wxDC* aDC )
{
    wxCHECK_RET( aItem != NULL, wxT( "InstallGraphicItemPropertiesDialog() error: NULL item" ) );

    m_canvas->SetIgnoreMouseEvents( true );
    DIALOG_GRAPHIC_ITEM_PROPERTIES dlg( this, aItem, aDC );
    dlg.ShowModal();
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::initDlg()
{
    m_StandardButtonsSizerOK->SetDefault();

    // Set unit symbol
    wxStaticText* texts_unit[] =
    {
        m_StartPointXUnit,
        m_StartPointYUnit,
        m_EndPointXUnit,
        m_EndPointYUnit,
        m_ThicknessTextUnit,
        m_DefaulThicknessTextUnit,
        NULL
    };

    for( int ii = 0; ; ii++ )
    {
        if( texts_unit[ii] == NULL )
            break;

        texts_unit[ii]->SetLabel( GetAbbreviatedUnitsLabel() );
    }

    wxString msg;

    // Change texts according to the segment shape:
    switch( m_item->GetShape() )
    {
    case S_CIRCLE:
        SetTitle( _( "Circle Properties" ) );
        m_StartPointXLabel->SetLabel( _( "Center X" ) );
        m_StartPointYLabel->SetLabel( _( "Center Y" ) );
        m_EndPointXLabel->SetLabel( _( "Point X" ) );
        m_EndPointYLabel->SetLabel( _( "Point Y" ) );
        m_Angle_Text->Show( false );
        m_Angle_Ctrl->Show( false );
        m_AngleUnit->Show( false );
        break;

    case S_ARC:
        SetTitle( _( "Arc Properties" ) );
        m_StartPointXLabel->SetLabel( _( "Center X" ) );
        m_StartPointYLabel->SetLabel( _( "Center Y" ) );
        m_EndPointXLabel->SetLabel( _( "Start Point X" ) );
        m_EndPointYLabel->SetLabel( _( "Start Point Y" ) );

        // Here the angle is a double, but the UI is still working with integers.
        msg << int( m_item->GetAngle() );
        m_Angle_Ctrl->SetValue( msg );
        break;

    case S_SEGMENT:
        SetTitle( _( "Line Segment Properties" ) );

        // Fall through.
    default:
        m_Angle_Text->Show( false );
        m_Angle_Ctrl->Show( false );
        m_AngleUnit->Show( false );
        break;
    }

    PutValueInLocalUnits( *m_Center_StartXCtrl, m_item->GetStart().x );

    PutValueInLocalUnits( *m_Center_StartYCtrl, m_item->GetStart().y );

    PutValueInLocalUnits( *m_EndX_Radius_Ctrl, m_item->GetEnd().x );

    PutValueInLocalUnits( *m_EndY_Ctrl, m_item->GetEnd().y );

    PutValueInLocalUnits( *m_ThicknessCtrl, m_item->GetWidth() );

    int thickness;

    if( m_item->GetLayer() == Edge_Cuts )
        thickness = m_brdSettings.m_EdgeSegmentWidth;
    else
        thickness = m_brdSettings.m_DrawSegmentWidth;

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness );

    // Configure the layers list selector
    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetLayerSet( LSET::AllCuMask() );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    if( m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item was on an unknown layer.\n"
                         "It has been moved to the drawings layer. Please fix it." ) );
        m_LayerSelectionCtrl->SetLayerSelection( Dwgs_User );
    }
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnLayerChoice( wxCommandEvent& event )
{
    int thickness;

    if( m_LayerSelectionCtrl->GetLayerSelection() == Edge_Cuts )
        thickness = m_brdSettings.m_EdgeSegmentWidth;
    else
        thickness = m_brdSettings.m_DrawSegmentWidth;

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness );
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnOkClick( wxCommandEvent& event )
{
    if( !itemValuesOK() )
        return;

    m_parent->SaveCopyInUndoList( m_item, UR_CHANGED );

    wxString msg;

    if( m_DC )
        m_item->Draw( m_parent->GetCanvas(), m_DC, GR_XOR );

    msg = m_Center_StartXCtrl->GetValue();
    m_item->SetStartX( ValueFromString( g_UserUnit, msg ) );

    msg = m_Center_StartYCtrl->GetValue();
    m_item->SetStartY( ValueFromString( g_UserUnit, msg ) );

    msg = m_EndX_Radius_Ctrl->GetValue();
    m_item->SetEndX( ValueFromString( g_UserUnit, msg ) );

    msg = m_EndY_Ctrl->GetValue();
    m_item->SetEndY( ValueFromString( g_UserUnit, msg ) );

    msg = m_ThicknessCtrl->GetValue();
    m_item->SetWidth( ValueFromString( g_UserUnit, msg ) );

    msg = m_DefaultThicknessCtrl->GetValue();
    int thickness = ValueFromString( g_UserUnit, msg );

    m_item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    if( m_item->GetLayer() == Edge_Cuts )
        m_brdSettings.m_EdgeSegmentWidth = thickness;
    else
        m_brdSettings.m_DrawSegmentWidth = thickness;

    if( m_item->GetShape() == S_ARC )
    {
        double angle;
        m_Angle_Ctrl->GetValue().ToDouble( &angle );
        NORMALIZE_ANGLE_360( angle );
        m_item->SetAngle( angle );
    }

    m_parent->OnModify();

    if( m_DC )
        m_item->Draw( m_parent->GetCanvas(), m_DC, GR_OR );

    m_parent->SetMsgPanel( m_item );

    m_parent->SetDesignSettings( m_brdSettings );

    Close( true );
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::itemValuesOK()
{
    wxArrayString error_msgs;

    // Load the start and end points -- all types use these in the checks.
    int startx = ValueFromString( g_UserUnit, m_Center_StartXCtrl->GetValue() );
    int starty = ValueFromString( g_UserUnit, m_Center_StartYCtrl->GetValue() );
    int endx   = ValueFromString( g_UserUnit, m_EndX_Radius_Ctrl->GetValue() );
    int endy   = ValueFromString( g_UserUnit, m_EndY_Ctrl->GetValue() );

    // Type specific checks.
    switch( m_item->GetShape() )
    {
    case S_ARC:
        // Check angle of arc.
        double angle;
        m_Angle_Ctrl->GetValue().ToDouble( &angle );
        NORMALIZE_ANGLE_360( angle );

        if( angle == 0 )
        {
            error_msgs.Add( _( "The arc angle must be greater than zero." ) );
        }

        // Fall through.
    case S_CIRCLE:

        // Check radius.
        if( (startx == endx) && (starty == endy) )
        {
            error_msgs.Add( _( "The radius must be greater than zero." ) );
        }

        break;

    default:

        // Check start and end are not the same.
        if( (startx == endx) && (starty == endy) )
        {
            error_msgs.Add( _( "The start and end points cannot be the same." ) );
        }

        break;
    }

    // Check the item thickness.
    int thickness = ValueFromString( g_UserUnit, m_ThicknessCtrl->GetValue() );

    if( thickness <= 0 )
        error_msgs.Add( _( "The item thickness must be greater than zero." ) );

    // And the default thickness.
    thickness = ValueFromString( g_UserUnit, m_DefaultThicknessCtrl->GetValue() );

    if( thickness <= 0 )
        error_msgs.Add( _( "The default thickness must be greater than zero." ) );

    if( error_msgs.GetCount() )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error list" ) );
        dlg.ListSet( error_msgs );
        dlg.ShowModal();
    }

    return error_msgs.GetCount() == 0;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras <jp.charras@wanadoo.fr>
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


class DIALOG_GRAPHIC_ITEM_PROPERTIES: public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    wxDC* m_DC;
    DRAWSEGMENT* m_Item;
    BOARD_DESIGN_SETTINGS  m_brdSettings;

public:
    DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_EDIT_FRAME* aParent, DRAWSEGMENT * aItem, wxDC * aDC);
    ~DIALOG_GRAPHIC_ITEM_PROPERTIES() {};

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
    void OnLayerChoice( wxCommandEvent& event );
};

DIALOG_GRAPHIC_ITEM_PROPERTIES::DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_EDIT_FRAME* aParent,
                                                                DRAWSEGMENT * aItem, wxDC * aDC ):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_DC = aDC;
    m_Item = aItem;
    m_brdSettings = m_parent->GetDesignSettings();
    initDlg();
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void PCB_EDIT_FRAME::InstallGraphicItemPropertiesDialog( DRAWSEGMENT* aItem, wxDC* aDC )
{
    if ( aItem == NULL )
    {
        DisplayError( this, wxT( "InstallGraphicItemPropertiesDialog() error: NULL item" ) );
        return;
    }

    m_canvas->SetIgnoreMouseEvents( true );
    DIALOG_GRAPHIC_ITEM_PROPERTIES dlg( this, aItem, aDC );
    dlg.ShowModal();
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::initDlg( )
{
    m_StandardButtonsSizerOK->SetDefault();

    // Set unit symbol
    wxStaticText * texts_unit[] =
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
    switch ( m_Item->GetShape() )
    {
    case S_CIRCLE:
        m_StartPointXLabel->SetLabel( _( "Center X" ) );
        m_StartPointYLabel->SetLabel( _( "Center Y" ) );
        m_EndPointXLabel->SetLabel( _( "Point X" ) );
        m_EndPointYLabel->SetLabel( _( "Point Y" ) );
        m_Angle_Text->Show( false );
        m_Angle_Ctrl->Show( false );
        m_AngleUnit->Show( false );
        break;

    case S_ARC:
        m_StartPointXLabel->SetLabel( _( "Center X" ) );
        m_StartPointYLabel->SetLabel( _( "Center Y" ) );
        m_EndPointXLabel->SetLabel( _( "Start Point X" ) );
        m_EndPointYLabel->SetLabel( _( "Start Point Y" ) );

        // Here the angle is a double, but the UI is still working with integers.
        msg << int( m_Item->GetAngle() );
        m_Angle_Ctrl->SetValue( msg );
        break;

    default:
        m_Angle_Text->Show( false );
        m_Angle_Ctrl->Show( false );
        m_AngleUnit->Show( false );
        break;
    }

    PutValueInLocalUnits( *m_Center_StartXCtrl, m_Item->GetStart().x );

    PutValueInLocalUnits( *m_Center_StartYCtrl, m_Item->GetStart().y );

    PutValueInLocalUnits( *m_EndX_Radius_Ctrl, m_Item->GetEnd().x );

    PutValueInLocalUnits( *m_EndY_Ctrl, m_Item->GetEnd().y );

    PutValueInLocalUnits( *m_ThicknessCtrl, m_Item->GetWidth() );

    int thickness;

    if( m_Item->GetLayer() == Edge_Cuts )
        thickness =  m_brdSettings.m_EdgeSegmentWidth;
    else
        thickness =  m_brdSettings.m_DrawSegmentWidth;

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness );

    // Configure the layers list selector
    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetLayerSet( LSET::AllCuMask() );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    if( m_LayerSelectionCtrl->SetLayerSelection( m_Item->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item has an illegal layer id.\n"
                         "Now, forced on the drawings layer. Please, fix it" ) );
        m_LayerSelectionCtrl->SetLayerSelection( Dwgs_User );
    }
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnLayerChoice( wxCommandEvent& event )
{
    int thickness;

    if( m_LayerSelectionCtrl->GetLayerSelection() == Edge_Cuts )
        thickness =  m_brdSettings.m_EdgeSegmentWidth;
    else
        thickness =  m_brdSettings.m_DrawSegmentWidth;

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, thickness );
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::OnOkClick( wxCommandEvent& event )
{
    m_parent->SaveCopyInUndoList( m_Item, UR_CHANGED );

    wxString msg;

    if( m_DC )
        m_Item->Draw( m_parent->GetCanvas(), m_DC, GR_XOR );

    msg = m_Center_StartXCtrl->GetValue();
    m_Item->SetStartX( ValueFromString( g_UserUnit, msg ) );

    msg = m_Center_StartYCtrl->GetValue();
    m_Item->SetStartY( ValueFromString( g_UserUnit, msg ) );

    msg = m_EndX_Radius_Ctrl->GetValue();
    m_Item->SetEndX( ValueFromString( g_UserUnit, msg ) );

    msg = m_EndY_Ctrl->GetValue();
    m_Item->SetEndY( ValueFromString( g_UserUnit, msg ) );

    msg = m_ThicknessCtrl->GetValue();
    m_Item->SetWidth( ValueFromString( g_UserUnit, msg ) );

    msg = m_DefaultThicknessCtrl->GetValue();
    int thickness = ValueFromString( g_UserUnit, msg );

    m_Item->SetLayer( ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) );

    if( m_Item->GetLayer() == Edge_Cuts )
         m_brdSettings.m_EdgeSegmentWidth = thickness;
    else
         m_brdSettings.m_DrawSegmentWidth = thickness;

    if( m_Item->GetShape() == S_ARC )
    {
        double angle;
        m_Angle_Ctrl->GetValue().ToDouble( &angle );
        NORMALIZE_ANGLE_360(angle);
        m_Item->SetAngle( angle );
    }

    m_parent->OnModify();

    if( m_DC )
        m_Item->Draw( m_parent->GetCanvas(), m_DC, GR_OR );

    m_parent->SetMsgPanel( m_Item );

    m_parent->SetDesignSettings( m_brdSettings );

    Close( true );
}

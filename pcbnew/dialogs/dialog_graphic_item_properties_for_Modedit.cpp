/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 @file dialog_graphic_item_properties_for_Modedit.cpp
 */

/* Edit parameters values of graphic items in a footprint body:
 * Lines
 * Circles
 * Arcs
 * used as graphic elements found on non copper layers in boards
 * Footprint texts are not graphic items and are not handled here
 */
#include <fctsys.h>
#include <macros.h>
#include <confirm.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <class_board_design_settings.h>
#include <module_editor_frame.h>
#include <base_units.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <dialog_graphic_item_properties_base.h>
#include <class_pcb_layer_box_selector.h>

class DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES: public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    FOOTPRINT_EDIT_FRAME* m_parent;
    EDGE_MODULE* m_item;
    BOARD_DESIGN_SETTINGS  m_brdSettings;
    MODULE * m_module;

public:
    DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES( FOOTPRINT_EDIT_FRAME* aParent,
                                            EDGE_MODULE * aItem);
    ~DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES() {};

private:
    void initDlg( );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event ){ event.Skip(); }
    void OnLayerChoice( wxCommandEvent& event );
};

DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES(
                                                        FOOTPRINT_EDIT_FRAME* aParent,
                                                        EDGE_MODULE * aItem ):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_item = aItem;
    m_brdSettings = m_parent->GetDesignSettings();
    m_module = m_parent->GetBoard()->m_Modules;
    initDlg();
    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}

/*
 * Dialog to edit a graphic item of a footprint body.
 */
void FOOTPRINT_EDIT_FRAME::InstallFootprintBodyItemPropertiesDlg(EDGE_MODULE * aItem)
{
    if ( aItem == NULL )
    {
        wxMessageBox( wxT("InstallGraphicItemPropertiesDialog() error: NULL item"));
        return;
    }

    m_canvas->SetIgnoreMouseEvents( true );
    DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES* dialog =
        new DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES( this, aItem );
    dialog->ShowModal();
    dialog->Destroy();
    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );
}


void DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::initDlg()
{
    SetFocus();
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
    switch ( m_item->GetShape() )
    {
    case S_CIRCLE:
        m_StartPointXLabel->SetLabel(_("Center X"));
        m_StartPointYLabel->SetLabel(_("Center Y"));
        m_EndPointXLabel->SetLabel(_("Point X"));
        m_EndPointYLabel->SetLabel(_("Point Y"));
        m_Angle_Text->Show(false);
        m_Angle_Ctrl->Show(false);
        m_AngleUnit->Show(false);
        break;

    case S_ARC:
        m_StartPointXLabel->SetLabel(_("Center X"));
        m_StartPointYLabel->SetLabel(_("Center Y"));
        m_EndPointXLabel->SetLabel(_("Start Point X"));
        m_EndPointYLabel->SetLabel(_("Start Point Y"));

        // Here the angle is a double, but the UI is still working
        // with integers
        msg << int( m_item->GetAngle() );
        m_Angle_Ctrl->SetValue(msg);
        break;

    default:
        m_Angle_Text->Show(false);
        m_Angle_Ctrl->Show(false);
        m_AngleUnit->Show(false);
        break;
    }

    PutValueInLocalUnits( *m_Center_StartXCtrl, m_item->GetStart().x );

    PutValueInLocalUnits( *m_Center_StartYCtrl, m_item->GetStart().y );

    PutValueInLocalUnits( *m_EndX_Radius_Ctrl, m_item->GetEnd().x );

    PutValueInLocalUnits( *m_EndY_Ctrl, m_item->GetEnd().y );

    PutValueInLocalUnits( *m_ThicknessCtrl, m_item->GetWidth() );

    PutValueInLocalUnits( *m_DefaultThicknessCtrl, m_brdSettings.m_ModuleSegmentWidth );

    // Configure the layers list selector
    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetLayerSet( LSET::InternalCuMask().set( Edge_Cuts ) );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    if( m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item has an illegal layer id.\n"
                        "Now, forced on the front silk screen layer. Please, fix it" ) );
        m_LayerSelectionCtrl->SetLayerSelection( F_SilkS );
    }
}


/*******************************************************************/
void DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::OnLayerChoice( wxCommandEvent& event )
/*******************************************************************/
{
}

/*******************************************************************/
void DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::OnOkClick( wxCommandEvent& event )
/*******************************************************************/
/* Copy values in text control to the item parameters
*/
{
    LAYER_NUM layer = m_LayerSelectionCtrl->GetLayerSelection();

    if( IsCopperLayer( layer ) )
    {
        /* an edge is put on a copper layer: this it is very dangerous. a
         * confirmation is requested */
        if( !IsOK( NULL,
                   _( "The graphic item will be on a copper layer. This is very dangerous. Are you sure?" ) ) )
            return;
    }

    m_parent->SaveCopyInUndoList( m_module, UR_MODEDIT );
    m_module->SetLastEditTime();

    wxString msg;

    wxPoint coord;

    msg = m_Center_StartXCtrl->GetValue();
    coord.x = ValueFromString( g_UserUnit, msg );
    msg = m_Center_StartYCtrl->GetValue();
    coord.y = ValueFromString( g_UserUnit, msg );
    m_item->SetStart( coord );
    m_item->SetStart0( coord );

    msg = m_EndX_Radius_Ctrl->GetValue();
    coord.x = ValueFromString( g_UserUnit, msg );
    msg = m_EndY_Ctrl->GetValue();
    coord.y = ValueFromString( g_UserUnit, msg );
    m_item->SetEnd( coord );
    m_item->SetEnd0( coord );

    msg = m_ThicknessCtrl->GetValue();
    m_item->SetWidth( ValueFromString( g_UserUnit, msg ) );

    msg = m_DefaultThicknessCtrl->GetValue();
    int thickness = ValueFromString( g_UserUnit, msg );
    m_brdSettings.m_ModuleSegmentWidth = thickness;
    m_parent->SetDesignSettings( m_brdSettings );

    m_item->SetLayer( ToLAYER_ID( layer ) );

    if( m_item->GetShape() == S_ARC )
    {
        double angle;
        m_Angle_Ctrl->GetValue().ToDouble( &angle );
        NORMALIZE_ANGLE_360(angle);
        m_item->SetAngle( angle );
    }

    m_parent->OnModify();
    m_parent->SetMsgPanel( m_item );

    Close( true );
}

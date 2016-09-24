/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dialog_graphic_item_properties_for_Modedit.cpp
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
#include <wx/valnum.h>
#include <board_commit.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>

#include <dialog_graphic_item_properties_base.h>
#include <class_pcb_layer_box_selector.h>
#include <html_messagebox.h>

class DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES : public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    FOOTPRINT_EDIT_FRAME*  m_parent;
    EDGE_MODULE*           m_item;
    BOARD_DESIGN_SETTINGS  m_brdSettings;
    MODULE*                m_module;

    wxFloatingPointValidator<double>    m_AngleValidator;
    double                 m_AngleValue;

public:
    DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES( FOOTPRINT_EDIT_FRAME* aParent,
                                            EDGE_MODULE* aItem );
    ~DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES() {};

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
    void OnLayerChoice( wxCommandEvent& event );
    bool Validate() override;
};

DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES(
                                                        FOOTPRINT_EDIT_FRAME* aParent,
                                                        EDGE_MODULE * aItem ):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent ),
    m_AngleValidator( 1, &m_AngleValue ),
    m_AngleValue( 0.0 )
{
    m_parent = aParent;
    m_item = aItem;
    m_brdSettings = m_parent->GetDesignSettings();
    m_module = m_parent->GetBoard()->m_Modules;

    m_AngleValidator.SetRange( -360.0, 360.0 );
    m_AngleCtrl->SetValidator( m_AngleValidator );
    m_AngleValidator.SetWindow( m_AngleCtrl );

    SetFocus();
    m_StandardButtonsSizerOK->SetDefault();

    Layout();
    GetSizer()->SetSizeHints( this );
    Centre();
}


/*
 * Dialog to edit a graphic item of a footprint body.
 */
void FOOTPRINT_EDIT_FRAME::InstallFootprintBodyItemPropertiesDlg( EDGE_MODULE* aItem )
{
    if( aItem == NULL )
    {
        wxMessageBox( wxT( "InstallGraphicItemPropertiesDialog() error: NULL item" ) );
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


bool DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::TransferDataToWindow()
{
    // Set unit symbol
    wxStaticText* texts_unit[] =
    {
        m_StartPointXUnit,
        m_StartPointYUnit,
        m_EndPointXUnit,
        m_EndPointYUnit,
        m_ThicknessTextUnit,
        m_DefaulThicknessTextUnit,
    };

    for( size_t ii = 0; ii < DIM( texts_unit ); ii++ )
    {
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
        m_AngleText->Show( false );
        m_AngleCtrl->Show( false );
        m_AngleUnit->Show( false );
        break;

    case S_ARC:
        SetTitle( _( "Arc Properties" ) );
        m_StartPointXLabel->SetLabel( _( "Center X" ) );
        m_StartPointYLabel->SetLabel( _( "Center Y" ) );
        m_EndPointXLabel->SetLabel( _( "Start Point X" ) );
        m_EndPointYLabel->SetLabel( _( "Start Point Y" ) );

        m_AngleValue = m_item->GetAngle() / 10.0;
        break;

    case S_SEGMENT:
        SetTitle( _( "Line Segment Properties" ) );

        // Fall through.
    default:
        m_AngleText->Show( false );
        m_AngleCtrl->Show( false );
        m_AngleUnit->Show( false );
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
        wxMessageBox( _( "This item was on an unknown layer.\n"
                         "It has been moved to the front silk screen layer. Please fix it." ) );
        m_LayerSelectionCtrl->SetLayerSelection( F_SilkS );
    }

    return DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataToWindow();
}


void DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::OnLayerChoice( wxCommandEvent& event )
{
}


bool DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_module );

    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    LAYER_NUM layer = m_LayerSelectionCtrl->GetLayerSelection();

    if( IsCopperLayer( layer ) )
    {
        /* an edge is put on a copper layer: this it is very dangerous. a
         * confirmation is requested */
        if( !IsOK( NULL,
                   _( "The graphic item will be on a copper layer. This is very dangerous. Are you sure?" ) ) )
            return false;
    }

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
        m_item->SetAngle( m_AngleValue * 10.0 );
    }

    commit.Push( _( "Modify module graphic item" ) );

    m_parent->SetMsgPanel( m_item );

    return true;
}


bool DIALOG_MODEDIT_FP_BODY_ITEM_PROPERTIES::Validate()
{
    wxArrayString error_msgs;

    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::Validate() )
        return false;

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
        m_AngleCtrl->GetValue().ToDouble( &angle );
        NORMALIZE_ANGLE_360( angle );

        if( angle == 0 )
        {
            error_msgs.Add( _( "The arc angle must be greater than zero." ) );
        }

        // Fall through.
    case S_CIRCLE:

        // Check radius.
        if( (startx == endx) && (starty == endy) )
            error_msgs.Add( _( "The radius must be greater than zero." ) );

        break;

    default:

        // Check start and end are not the same.
        if( (startx == endx) && (starty == endy) )
            error_msgs.Add( _( "The start and end points cannot be the same." ) );

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

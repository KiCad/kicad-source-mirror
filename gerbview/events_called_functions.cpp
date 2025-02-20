/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <tool/selection.h>
#include <tools/gerbview_selection_tool.h>
#include <gerbview_painter.h>
#include <view/view.h>
#include "widgets/gerbview_layer_widget.h"
#include "widgets/dcode_selection_box.h"


// Event table:

BEGIN_EVENT_TABLE( GERBVIEW_FRAME, EDA_DRAW_FRAME )
    EVT_CLOSE( GERBVIEW_FRAME::OnCloseWindow )
    EVT_SIZE( GERBVIEW_FRAME::OnSize )

    // Menu Files:
    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, GERBVIEW_FRAME::OnGbrFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, GERBVIEW_FRAME::OnClearGbrFileHistory )

    EVT_MENU_RANGE( ID_GERBVIEW_DRILL_FILE1, ID_GERBVIEW_DRILL_FILEMAX,
                    GERBVIEW_FRAME::OnDrlFileHistory )
    EVT_MENU( ID_GERBVIEW_DRILL_FILE_LIST_CLEAR, GERBVIEW_FRAME::OnClearDrlFileHistory )

    EVT_MENU_RANGE( ID_GERBVIEW_ZIP_FILE1, ID_GERBVIEW_ZIP_FILEMAX,
                    GERBVIEW_FRAME::OnZipFileHistory )
    EVT_MENU( ID_GERBVIEW_ZIP_FILE_LIST_CLEAR, GERBVIEW_FRAME::OnClearZipFileHistory )

    EVT_MENU_RANGE( ID_GERBVIEW_JOB_FILE1, ID_GERBVIEW_JOB_FILEMAX,
                    GERBVIEW_FRAME::OnJobFileHistory )
    EVT_MENU( ID_GERBVIEW_JOB_FILE_LIST_CLEAR, GERBVIEW_FRAME::OnClearJobFileHistory )

    EVT_MENU( wxID_EXIT, GERBVIEW_FRAME::OnQuit )

    EVT_COMBOBOX( ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER, GERBVIEW_FRAME::OnSelectActiveLayer )

    EVT_SELECT_DCODE( ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE, GERBVIEW_FRAME::OnSelectActiveDCode )

    // Auxiliary horizontal toolbar
    EVT_CHOICE( ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE, GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_CHOICE( ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE, GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_CHOICE( ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE,
                GERBVIEW_FRAME::OnSelectHighlightChoice )
    EVT_CHOICE( ID_ON_ZOOM_SELECT, GERBVIEW_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, GERBVIEW_FRAME::OnSelectGrid )

    EVT_UPDATE_UI( ID_ON_GRID_SELECT, GERBVIEW_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE, GERBVIEW_FRAME::OnUpdateSelectDCode )

    // Drop files event
    EVT_DROP_FILES( GERBVIEW_FRAME::OnDropFiles )

END_EVENT_TABLE()


void GERBVIEW_FRAME::OnSelectHighlightChoice( wxCommandEvent& event )
{
    auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>( GetCanvas()->GetView()->GetPainter() )->GetSettings();

    switch( event.GetId() )
    {
    case ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE:
        settings->m_componentHighlightString = m_SelComponentBox->GetStringSelection();
        break;

    case ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE:
        settings->m_netHighlightString = m_SelNetnameBox->GetStringSelection();
        break;

    case ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE:
        settings->m_attributeHighlightString = m_SelAperAttributesBox->GetStringSelection();
        break;

    }

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::OnSelectActiveDCode( wxCommandEvent& event )
{
    GERBER_FILE_IMAGE* gerber_image = GetGbrImage( GetActiveLayer() );

    if( gerber_image )
    {
        int d_code = m_DCodeSelector->GetSelectedDCodeId();

        auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>(
                            GetCanvas()->GetView()->GetPainter() )->GetSettings();
        gerber_image->m_Selected_Tool = d_code;
        settings->m_dcodeHighlightValue = d_code;

        GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
        GetCanvas()->Refresh();
    }
}


void GERBVIEW_FRAME::OnSelectActiveLayer( wxCommandEvent& event )
{
    SetActiveLayer( event.GetSelection(), true );

    // Rebuild the DCode list in toolbar (but not the Layer Box) after change
    syncLayerBox( false );

    // Reinit highlighted dcode
    auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>
                        ( GetCanvas()->GetView()->GetPainter() )->GetSettings();
    int dcodeSelected = m_DCodeSelector->GetSelectedDCodeId();
    settings->m_dcodeHighlightValue = dcodeSelected;
    GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
    GetCanvas()->Refresh();
}


void GERBVIEW_FRAME::OnQuit( wxCommandEvent& event )
{
    Close( true );
}


void GERBVIEW_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_DRAW_FRAME::ShowChangedLanguage();

    m_LayersManager->SetLayersManagerTabsText();

    wxAuiPaneInfo& pane_info = m_auimgr.GetPane( m_LayersManager );
    pane_info.Caption( _( "Layers Manager" ) );
    m_auimgr.Update();

    ReFillLayerWidget();
}

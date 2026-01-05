/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#include "zone_preview_notebook.h"
#include <wx/debug.h>
#include <wx/imaglist.h>
#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/dcbuffer.h>
#include <board.h>
#include <zone.h>
#include <gal/color4d.h>
#include <widgets/color_swatch.h>
#include <settings/color_settings.h>
#include <zone_manager/zone_preview_canvas.h>
#include <pcb_edit_frame.h>


enum LAYER_ICON_SIZE
{
    WIDTH = 16,
    HEIGHT = 16,

};


class ZONE_PREVIEW_NOTEBOOK_PAGE : public wxPanel
{
public:
    ZONE_PREVIEW_NOTEBOOK_PAGE( wxWindow* aParent, BOARD* aBoard, ZONE* aZone, PCB_LAYER_ID aLayer,
                                GAL_DISPLAY_OPTIONS_IMPL& aOpts, EDA_DRAW_PANEL_GAL::GAL_TYPE aGalType ) :
            wxPanel( aParent ),
            m_layer( aLayer ),
            m_canvas( nullptr )
    {
        SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

        m_canvas = new ZONE_PREVIEW_CANVAS( aBoard, aZone->Clone( aLayer ), aLayer, this, aOpts, aGalType );
        GetSizer()->Add( m_canvas, 1, wxEXPAND );
        Layout();
        GetSizer()->Fit( this );
    }

    int  GetLayer() const { return m_layer; }
    ZONE_PREVIEW_CANVAS* GetCanvas() const { return m_canvas; }

private:
    int                  m_layer;
    ZONE_PREVIEW_CANVAS* m_canvas;
};


ZONE_PREVIEW_NOTEBOOK::ZONE_PREVIEW_NOTEBOOK( wxWindow* aParent, PCB_BASE_FRAME* aPcbFrame ) :
        wxNotebook( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_pcbFrame( aPcbFrame )
{
    Bind( wxEVT_BOOKCTRL_PAGE_CHANGED, &ZONE_PREVIEW_NOTEBOOK::OnPageChanged, this );

    wxSize swatchSize( LAYER_ICON_SIZE::WIDTH, LAYER_ICON_SIZE::HEIGHT );

    wxImageList* imageList = new wxImageList( swatchSize.x, swatchSize.y );

    for( int i = 0; i < PCB_LAYER_ID::PCB_LAYER_ID_COUNT; i++ )
    {
        const KIGFX::COLOR4D color = aPcbFrame->GetColorSettings()->GetColor( i );
        wxBitmap swatch = COLOR_SWATCH::MakeBitmap( color != COLOR4D::UNSPECIFIED ? color : COLOR4D::WHITE,
                                                    COLOR4D::UNSPECIFIED,
                                                    swatchSize,
                                                    { 5, 6 },
                                                    COLOR4D::WHITE,
                                                    { 1, 2, 2, 1 } );

        imageList->Add( swatch, *wxWHITE );
    }

    AssignImageList( imageList );

    Layout();
    GetSizer()->Fit( this );
}


void ZONE_PREVIEW_NOTEBOOK::OnZoneSelectionChanged( ZONE* aZone )
{
    int preferredLayer = UNDEFINED_LAYER;

    if( GetSelection() >= 0 && GetSelection() < (int) GetPageCount() )
        preferredLayer = static_cast<ZONE_PREVIEW_NOTEBOOK_PAGE*>( GetCurrentPage() )->GetLayer();

    while( GetPageCount() )
        RemovePage( 0 );

    if( !aZone )
        return;

    ZONE_PREVIEW_NOTEBOOK_PAGE* preferredPage = nullptr;

    for( PCB_LAYER_ID layer : aZone->GetLayerSet().UIOrder() )
    {
        BOARD*                      board = m_pcbFrame->GetBoard();
        wxString                    layerName = board->GetLayerName( layer );
        ZONE_PREVIEW_NOTEBOOK_PAGE* page = new ZONE_PREVIEW_NOTEBOOK_PAGE( this, board, aZone, layer,
                                                                           m_pcbFrame->GetGalDisplayOptions(),
                                                                           m_pcbFrame->GetCanvas()->GetBackend() );

        AddPage( page, layerName, false, layer );
        page->Layout();
        page->GetCanvas()->ZoomFitScreen();

        if( layer == preferredLayer )
            preferredPage = page;
    }

    if( !preferredPage )
        preferredPage = static_cast<ZONE_PREVIEW_NOTEBOOK_PAGE*>( GetPage( 0 ) );

    SetSelection( FindPage( preferredPage ) );

    // Reinit canvas size parameters and display
    PostSizeEvent();
}


void ZONE_PREVIEW_NOTEBOOK::OnPageChanged( wxNotebookEvent& aEvent )
{
    SetSelection( aEvent.GetSelection() );
    aEvent.Skip();

    // Reinit canvas size parameters and display
    PostSizeEvent();
}


void ZONE_PREVIEW_NOTEBOOK::FitCanvasToScreen()
{
    for( int ii = 0; ii < (int) GetPageCount(); ++ii )
        static_cast<ZONE_PREVIEW_NOTEBOOK_PAGE*>( GetPage( ii ) )->GetCanvas()->ZoomFitScreen();
}

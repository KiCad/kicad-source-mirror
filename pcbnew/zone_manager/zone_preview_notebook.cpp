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
#include <gal/color4d.h>
#include "zone_preview_canvas.h"
#include "settings/color_settings.h"
#include "zone_manager_preference.h"
#include <pcb_edit_frame.h>
#include "widgets/color_swatch.h"


class ZONE_PREVIEW_NOTEBOOK_PAGE : public wxPanel
{
public:
    ZONE_PREVIEW_NOTEBOOK_PAGE( wxWindow* aParent, int aLayer ) :
            wxPanel( aParent ),
            m_layer( aLayer )
    {
        SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
    }

    int  GetLayer() const { return m_layer; }

    /**
     * @brief Reuse the singleton zone preview canvas between different notebook pages
     */
    void AssignCanvas( ZONE_PREVIEW_CANVAS* aCanvas )
    {
        if( aCanvas->GetParent() )
            aCanvas->GetParent()->GetSizer()->Detach( aCanvas );

        aCanvas->Reparent( this );
        GetSizer()->Add( aCanvas, 1, wxEXPAND );
        Layout();
        GetSizer()->Fit( this );
    }

private:
    int m_layer;
};


ZONE_PREVIEW_NOTEBOOK::ZONE_PREVIEW_NOTEBOOK( wxWindow* aParent, PCB_BASE_FRAME* aPcbFrame ) :
        wxNotebook( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        m_pcbFrame( aPcbFrame ),
        m_previewCanvas( nullptr )
{
    Bind( wxEVT_BOOKCTRL_PAGE_CHANGED, &ZONE_PREVIEW_NOTEBOOK::OnPageChanged, this );

    wxSize swatchSize( ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::WIDTH,
                       ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::HEIGHT );

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
}


void ZONE_PREVIEW_NOTEBOOK::OnZoneSelectionChanged( ZONE* aZone )
{
    while( GetPageCount() )
        RemovePage( 0 );

    if( !aZone )
        return;

    const PCB_LAYER_ID firstLayer = aZone->GetFirstLayer();

    for( PCB_LAYER_ID layer : aZone->GetLayerSet().UIOrder() )
    {
        wxString layerName = m_pcbFrame->GetBoard()->GetLayerName( static_cast<PCB_LAYER_ID>( layer ) );

        if( auto existingContainer = m_zonePreviewPages.find( layer ); existingContainer != m_zonePreviewPages.end() )
        {
            AddPage( existingContainer->second, layerName, false, layer );
        }
        else
        {
            ZONE_PREVIEW_NOTEBOOK_PAGE* page = new ZONE_PREVIEW_NOTEBOOK_PAGE( this, layer );
            m_zonePreviewPages.try_emplace( layer, page );
            AddPage( page, layerName, false, layer );
        }
    }

    if( !m_previewCanvas )
    {
        m_previewCanvas = new ZONE_PREVIEW_CANVAS( m_pcbFrame->GetBoard(), m_zonePreviewPages[firstLayer],
                                                   m_pcbFrame->GetGalDisplayOptions() );
    }

    m_previewCanvas->ActivateSelectedZone( aZone );

    changePage( FindPage( m_zonePreviewPages[firstLayer] ) );
}


void ZONE_PREVIEW_NOTEBOOK::changePage( int aPageIdx )
{
    ZONE_PREVIEW_NOTEBOOK_PAGE* page = static_cast<ZONE_PREVIEW_NOTEBOOK_PAGE*>( GetPage( aPageIdx ) );

    page->AssignCanvas( m_previewCanvas );
    m_previewCanvas->OnLayerSelected( page->GetLayer() );
    SetSelection( aPageIdx );

    // Reinit canvas size parameters and display
    PostSizeEvent();

    CallAfter(
            [this]()
            {
                m_previewCanvas->ZoomFitScreen();
            } );
}


void ZONE_PREVIEW_NOTEBOOK::OnPageChanged( wxNotebookEvent& aEvent )
{
    changePage( aEvent.GetSelection() );
    aEvent.Skip();
}
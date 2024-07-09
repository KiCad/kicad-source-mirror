/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pane_zone_viewer.h"
#include <wx/artprov.h>
#include <wx/dataview.h>
#include <wx/colour.h>
#include <wx/debug.h>
#include <wx/event.h>
#include <wx/imaglist.h>
#include <wx/panel.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/dataview.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <board.h>
#include <gal/color4d.h>
#include "panel_zone_gal.h"
#include <wx/dataview.h>
#include "settings/color_settings.h"
#include "zone_manager_preference.h"
#include "zone_manager/panel_zone_gal.h"
#include <pcb_edit_frame.h>
#include "widgets/color_swatch.h"
class PANEL_ZONE_GAL_CONTAINER : public wxPanel
{
    wxBoxSizer*     m_sizer;
    PANEL_ZONE_GAL* m_gal{};
    int             m_layer;

public:
    PANEL_ZONE_GAL_CONTAINER( wxWindow* aParent, int aLayer ) :
            wxPanel( aParent ), m_sizer( new wxBoxSizer( wxHORIZONTAL ) ), m_layer( aLayer )
    {
        SetSizer( m_sizer );
    }
    int  GetLayer() const { return m_layer; }
    void TakeGAL( PANEL_ZONE_GAL*& now )
    {
        if( !m_gal )
            return;

        m_sizer->Detach( m_gal );
        now = m_gal;
        m_gal = nullptr;
    }

    /**
     * @brief Initialize the gal , shall only be called once while the gal is first constructed
     *
     * @param aGal The zone gal
     */
    void InitZoneGAL( PANEL_ZONE_GAL* aGal )
    {
        wxASSERT( !m_gal );
        m_gal = aGal;
        m_sizer->Add( m_gal, 1, wxEXPAND );
        Layout();
        m_sizer->Fit( this );
    }

    /**
     * @brief Reuse the only one zone gal between different container
     *
     * @param aGal The zone gal
     */
    void ResetZoneGAL( PANEL_ZONE_GAL* aGal )
    {
        if( aGal->GetParent() == this )
            return;

        static_cast<PANEL_ZONE_GAL_CONTAINER*>( aGal->GetParent() )->TakeGAL( m_gal );
        m_gal->Reparent( this );
        m_sizer->Add( m_gal, 1, wxEXPAND );
        Layout();
        m_sizer->Fit( this );
    }
};

PANE_ZONE_VIEWER::PANE_ZONE_VIEWER( wxWindow* aParent, PCB_BASE_FRAME* aPcbFrame ) :
        wxNotebook( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ), m_pcbFrame( aPcbFrame )
{
    Bind( wxEVT_BOOKCTRL_PAGE_CHANGED, &PANE_ZONE_VIEWER::OnNotebook, this );

    wxImageList* imageList = new wxImageList( ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::WIDTH,
                                              ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::HEIGHT );

    for( int i = 0; i < PCB_LAYER_ID::PCB_LAYER_ID_COUNT; i++ )
    {
        const KIGFX::COLOR4D color = aPcbFrame->GetColorSettings()->GetColor( i );
        imageList->Add( COLOR_SWATCH::MakeBitmap(
                color != COLOR4D::UNSPECIFIED ? color : COLOR4D::WHITE, COLOR4D::UNSPECIFIED,
                wxSize( ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::WIDTH,
                        ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::HEIGHT ),
                { 5, 6 }, COLOR4D::WHITE ) );
    }

    AssignImageList( imageList );
}


PANE_ZONE_VIEWER::~PANE_ZONE_VIEWER() = default;

void PANE_ZONE_VIEWER::ActivateSelectedZone( ZONE* aZone )
{
    while( GetPageCount() )
        RemovePage( 0 );

    if( !aZone )
        return;

    const PCB_LAYER_ID firstLayer = aZone->GetFirstLayer();

    for( PCB_LAYER_ID layer : aZone->GetLayerSet().Seq() )
    {
        wxString layerName =
                m_pcbFrame->GetBoard()->GetLayerName( static_cast<PCB_LAYER_ID>( layer ) );

        if( auto existingContainer = m_zoneContainers.find( layer );
            existingContainer != m_zoneContainers.end() )
        {
            AddPage( existingContainer->second, layerName, false, layer );
        }
        else
        {
            PANEL_ZONE_GAL_CONTAINER* container = new PANEL_ZONE_GAL_CONTAINER( this, layer );
            m_zoneContainers.try_emplace( layer, container );
            AddPage( container, layerName, false, layer );
        }
    }

    SetSelection( FindPage( m_zoneContainers[firstLayer] ) );

    if( !m_zoneGAL )
    {
        m_zoneGAL = ( new PANEL_ZONE_GAL( m_pcbFrame->GetBoard(),
                                          m_zoneContainers[aZone->GetFirstLayer()],
                                          m_pcbFrame->GetGalDisplayOptions() ) );
        m_zoneContainers[firstLayer]->InitZoneGAL( m_zoneGAL );
    }
    else
    {
        m_zoneContainers[firstLayer]->ResetZoneGAL( m_zoneGAL );
    }

    m_zoneGAL->ActivateSelectedZone( aZone );
}

void PANE_ZONE_VIEWER::OnNotebook( wxNotebookEvent& aEvent )
{
    const int                 idx = aEvent.GetSelection();
    PANEL_ZONE_GAL_CONTAINER* container = static_cast<PANEL_ZONE_GAL_CONTAINER*>( GetPage( idx ) );
    container->ResetZoneGAL( m_zoneGAL );
    m_zoneGAL->OnLayerSelected( container->GetLayer() );
    SetSelection( idx );
    aEvent.Skip();
}

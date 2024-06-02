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

#include <board.h>
#include <netinfo.h>
#include <vector>
#include <wx/string.h>
#include <settings/color_settings.h>
#include <wx/dcmemory.h>
#include <pcb_edit_frame.h>
#include <wx/variant.h>
#include "zone_manager_preference.h"
#include "zone_priority_container.h"
#include "model_zones_overview_table.h"

wxDEFINE_EVENT( EVT_ZONES_OVERVIEW_COUNT_CHANGE, wxCommandEvent );

void MODEL_ZONES_OVERVIEW_TABLE::SortZoneContainers()
{
    std::sort( m_filteredZoneContainers.begin(), m_filteredZoneContainers.end(),
               []( std::shared_ptr<ZONE_PRIORITY_CONTAINER> const& l,
                   std::shared_ptr<ZONE_PRIORITY_CONTAINER> const& r

               )
               {
                   return l->GetCurrentPriority() > r->GetCurrentPriority();
               } );
}


void MODEL_ZONES_OVERVIEW_TABLE::OnRowCountChange()
{
    wxCommandEvent rowCountChange( EVT_ZONES_OVERVIEW_COUNT_CHANGE );
    rowCountChange.SetInt( GetCount() );
    wxPostEvent( m_dialog, rowCountChange );
}


static wxBitmap MakeBitmapForLayers( LSEQ const& layers, COLOR_SETTINGS const& settings,
                                     const wxSize& aSize )
{
    wxBitmap   bitmap( aSize );
    wxBrush    brush;
    wxPen      pen;
    wxMemoryDC iconDC;

    iconDC.SelectObject( bitmap );
    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    const int                 layer_cout = layers.size();
    std::vector<PCB_LAYER_ID> layersToDraw;

    if( layer_cout > 4 )
    {
        for( const PCB_LAYER_ID& i :
             { layers[0], layers[1], layers[layer_cout - 1], layers[layer_cout - 2] } )
            layersToDraw.push_back( i );
    }
    else
    {
        layersToDraw = layers;
    }

    const int step = static_cast<int>( aSize.x / layersToDraw.size() );

    for( size_t i = 0; i < layersToDraw.size(); ++i )
    {
        const KIGFX::COLOR4D color = settings.GetColor( layersToDraw[i] );
        brush.SetColour( color.ToColour() );
        pen.SetColour( color.ToColour() );
        iconDC.SetBrush( brush );
        iconDC.SetPen( pen );
        iconDC.DrawRectangle( 0, i * step, aSize.x, step );
    }

    return bitmap;
}


MODEL_ZONES_OVERVIEW_TABLE::MODEL_ZONES_OVERVIEW_TABLE( ZONE_PRIORITY_CONTAINER_LIST aZones,
                                                        BOARD* a_pcb, PCB_BASE_FRAME* aPCB_FRAME,
                                                        wxWindow* a_dialog ) :
        m_allZoneContainers( aZones ),
        m_filteredZoneContainers( std::move( aZones ) ),
        m_pcb( a_pcb ),
        m_PCB_FRAME( aPCB_FRAME ),
        m_dialog( a_dialog ),
        m_sortByName( true ),
        m_sortByNet( true )
{
    Reset( m_filteredZoneContainers.size() );
}


MODEL_ZONES_OVERVIEW_TABLE::~MODEL_ZONES_OVERVIEW_TABLE() = default;


void MODEL_ZONES_OVERVIEW_TABLE::GetValueByRow( wxVariant& aVariant, unsigned aRow,
                                                unsigned aCol ) const
{
    if( static_cast<size_t>( aRow ) + 1 > m_filteredZoneContainers.size() )
        return;

    const ZONE& cur = m_filteredZoneContainers[aRow]->GetZone();

    switch( aCol )
    {
    case NAME:
        aVariant = cur.GetZoneName();
        break;

    case NET:
        aVariant = cur.GetNet()->GetNetname();
        break;

    case LAYERS:
    {
        wxArrayString layers;

        for( PCB_LAYER_ID layer : cur.GetLayerSet().Seq() )
            layers.Add( m_pcb->GetLayerName( layer ) );

        aVariant << wxDataViewIconText(
                wxJoin( layers, ',' ),
                MakeBitmapForLayers(
                        cur.GetLayerSet().Seq(), *m_PCB_FRAME->GetColorSettings(),
                        { LAYER_BAR_WIDTH, ZONE_MANAGER_PREFERENCE::LAYER_ICON_SIZE::HEIGHT } ) );
        break;
    }

    default:
        break;
    }
}


void MODEL_ZONES_OVERVIEW_TABLE::EnableFitterByName( bool aEnable )
{
    m_sortByName = aEnable;
}


void MODEL_ZONES_OVERVIEW_TABLE::EnableFitterByNet( bool aEnable )
{
    m_sortByNet = aEnable;
}


bool MODEL_ZONES_OVERVIEW_TABLE::SetValueByRow( const wxVariant& aVariant, unsigned aRow,
                                                unsigned aCol )
{
    return {};
}


unsigned int MODEL_ZONES_OVERVIEW_TABLE::GetCount() const
{
    return m_filteredZoneContainers.size();
}


ZONE* MODEL_ZONES_OVERVIEW_TABLE::GetZone( wxDataViewItem const& aItem ) const
{
    if( !aItem.IsOk() )
        return nullptr;

    unsigned int aRow = GetRow( aItem );

    if( aRow + 1 > GetCount() )
        return nullptr;

    return &m_filteredZoneContainers[aRow]->GetZone();
}


wxDataViewItem MODEL_ZONES_OVERVIEW_TABLE::GetItemByZone( ZONE* aZone ) const
{
    if( !aZone )
        return {};

    for( size_t i = 0; i < m_filteredZoneContainers.size(); i++ )
    {
        if( &m_filteredZoneContainers[i]->GetZone() == aZone )
            return GetItem( i );
    }

    return {};
}


std::optional<unsigned> MODEL_ZONES_OVERVIEW_TABLE::MoveZoneIndex( unsigned            aIndex,
                                                                   ZONE_INDEX_MOVEMENT aMovement )
{
    switch( aMovement )
    {
    case ZONE_INDEX_MOVEMENT::MOVE_UP:
        if( aIndex >= 1 && GetCount() > 1 )
            return SwapZonePriority( aIndex, aIndex - 1 );

        break;

    case ZONE_INDEX_MOVEMENT::MOVE_DOWN:
        if( aIndex + 1 < GetCount() )
            return SwapZonePriority( aIndex, aIndex + 1 );

        break;
    }

    return std::optional<unsigned>{};
}


std::optional<unsigned> MODEL_ZONES_OVERVIEW_TABLE::SwapZonePriority( unsigned aDragIndex,
                                                                      unsigned aDropIndex )
{
    for( const unsigned i : { aDragIndex, aDropIndex } )
    {
        if( !( i < GetCount() ) )
            return {};
    }

    if( aDragIndex == aDropIndex )
        return aDragIndex;

    std::swap( m_filteredZoneContainers[aDragIndex]->m_currentPriority,
               m_filteredZoneContainers[aDropIndex]->m_currentPriority );
    std::swap( m_filteredZoneContainers[aDragIndex], m_filteredZoneContainers[aDropIndex] );

    for( const unsigned int row : { aDragIndex, aDropIndex } )
        RowChanged( row );

    return aDropIndex;
}


wxDataViewItem MODEL_ZONES_OVERVIEW_TABLE::ApplyFilter( wxString const& aFilterText,
                                                        wxDataViewItem  aSelection )
{
    if( !GetAllZonesCount() )
        return {};

    wxString lowerFilterText = aFilterText.Strip( wxString::both ).Lower();

    if( lowerFilterText.empty() )
        return ClearFilter( aSelection );

    ZONE* selected_zone = GetZone( aSelection );
    m_filteredZoneContainers.clear();

    for( const auto& container : m_allZoneContainers )
    {
        const ZONE zone = container->GetZone();

        if( ( m_sortByName && zone.GetZoneName().Lower().Contains( lowerFilterText ) )
            || ( m_sortByNet && zone.GetNetname().Lower().Contains( lowerFilterText ) ) )
        {
            m_filteredZoneContainers.push_back( container );
        }
    }

    SortZoneContainers();
    Reset( GetCount() );
    OnRowCountChange();
    return GetItemByZone( selected_zone );
}


wxDataViewItem MODEL_ZONES_OVERVIEW_TABLE::ClearFilter( wxDataViewItem aSelection )
{
    if( !GetAllZonesCount() )
        return {};

    ZONE* zone = GetZone( aSelection );
    m_filteredZoneContainers = m_allZoneContainers;
    SortZoneContainers();
    Reset( GetCount() );
    OnRowCountChange();
    return GetItemByZone( zone );
}

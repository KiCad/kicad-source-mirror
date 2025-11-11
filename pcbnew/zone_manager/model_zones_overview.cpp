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

#include <board.h>
#include <netinfo.h>
#include <vector>
#include <wx/string.h>
#include <settings/color_settings.h>
#include <wx/dcmemory.h>
#include <pcb_edit_frame.h>
#include <wx/variant.h>
#include "model_zones_overview.h"


wxDEFINE_EVENT( EVT_ZONES_OVERVIEW_COUNT_CHANGE, wxCommandEvent );


void MODEL_ZONES_OVERVIEW::SortFilteredZones()
{
    std::sort( m_filteredZones.begin(), m_filteredZones.end(),
               [&]( ZONE* const& l, ZONE* const& r )
               {
                   return m_zoneSettingsBag.GetZonePriority( l ) > m_zoneSettingsBag.GetZonePriority( r );
               } );
}


void MODEL_ZONES_OVERVIEW::OnRowCountChange()
{
    wxCommandEvent rowCountChange( EVT_ZONES_OVERVIEW_COUNT_CHANGE );
    rowCountChange.SetInt( GetCount() );
    wxPostEvent( m_parent, rowCountChange );
}


static wxBitmap MakeBitmapForLayers( LSEQ const& layers, COLOR_SETTINGS const& settings, const wxSize& aSize )
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
        for( const PCB_LAYER_ID& i : { layers[0], layers[1], layers[layer_cout - 1], layers[layer_cout - 2] } )
            layersToDraw.push_back( i );
    }
    else
    {
        layersToDraw = layers;
    }

    const int step = static_cast<int>( aSize.y / layersToDraw.size() );

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


MODEL_ZONES_OVERVIEW::MODEL_ZONES_OVERVIEW( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                            ZONE_SETTINGS_BAG& aZoneSettingsBag ) :
        m_parent( aParent ),
        m_frame( aFrame ),
        m_zoneSettingsBag( aZoneSettingsBag ),
        m_sortByName( true ),
        m_sortByNet( true )
{
    m_filteredZones = m_zoneSettingsBag.GetClonedZoneList();
    Reset( m_filteredZones.size() );
}


void MODEL_ZONES_OVERVIEW::GetValueByRow( wxVariant& aVariant, unsigned aRow, unsigned aCol ) const
{
    if( static_cast<size_t>( aRow ) + 1 > m_filteredZones.size() )
        return;

    const ZONE& cur = *m_filteredZones[aRow];

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
            layers.Add( m_frame->GetBoard()->GetLayerName( layer ) );

        aVariant << wxDataViewIconText( wxJoin( layers, ',' ),
                                        MakeBitmapForLayers( cur.GetLayerSet().UIOrder(), *m_frame->GetColorSettings(),
                                                             wxSize( LAYER_BAR_WIDTH, LAYER_BAR_HEIGHT ) ) );
        break;
    }

    default:
        break;
    }
}


void MODEL_ZONES_OVERVIEW::EnableFitterByName( bool aEnable )
{
    m_sortByName = aEnable;
}


void MODEL_ZONES_OVERVIEW::EnableFitterByNet( bool aEnable )
{
    m_sortByNet = aEnable;
}


bool MODEL_ZONES_OVERVIEW::SetValueByRow( const wxVariant& aVariant, unsigned aRow, unsigned aCol )
{
    return {};
}


unsigned int MODEL_ZONES_OVERVIEW::GetCount() const
{
    return m_filteredZones.size();
}


ZONE* MODEL_ZONES_OVERVIEW::GetZone( wxDataViewItem const& aItem ) const
{
    if( !aItem.IsOk() )
        return nullptr;

    unsigned int aRow = GetRow( aItem );

    if( aRow + 1 > GetCount() )
        return nullptr;

    return m_filteredZones[aRow];
}


wxDataViewItem MODEL_ZONES_OVERVIEW::GetItemByZone( ZONE* aZone ) const
{
    if( !aZone )
        return {};

    for( size_t i = 0; i < m_filteredZones.size(); i++ )
    {
        if( m_filteredZones[i] == aZone )
            return GetItem( i );
    }

    return {};
}


std::optional<unsigned> MODEL_ZONES_OVERVIEW::MoveZoneIndex( unsigned aIndex, ZONE_INDEX_MOVEMENT aMovement )
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


std::optional<unsigned> MODEL_ZONES_OVERVIEW::SwapZonePriority( unsigned aDragIndex, unsigned aDropIndex )
{
    for( const unsigned i : { aDragIndex, aDropIndex } )
    {
        if( !( i < GetCount() ) )
            return {};
    }

    if( aDragIndex == aDropIndex )
        return aDragIndex;

    m_zoneSettingsBag.SwapPriority( m_filteredZones[aDragIndex], m_filteredZones[aDropIndex] );
    std::swap( m_filteredZones[aDragIndex], m_filteredZones[aDropIndex] );

    for( const unsigned int row : { aDragIndex, aDropIndex } )
        RowChanged( row );

    return aDropIndex;
}


wxDataViewItem MODEL_ZONES_OVERVIEW::ApplyFilter( wxString const& aFilterText, wxDataViewItem  aSelection )
{
    if( m_zoneSettingsBag.GetClonedZoneList().empty() )
        return {};

    wxString lowerFilterText = aFilterText.Strip( wxString::both ).Lower();

    if( lowerFilterText.empty() )
        return ClearFilter( aSelection );

    ZONE* selected_zone = GetZone( aSelection );
    m_filteredZones.clear();

    for( ZONE* zone : m_zoneSettingsBag.GetClonedZoneList() )
    {
        if( ( m_sortByName && zone->GetZoneName().Lower().Contains( lowerFilterText ) )
            || ( m_sortByNet && zone->GetNetname().Lower().Contains( lowerFilterText ) ) )
        {
            m_filteredZones.push_back( zone );
        }
    }

    SortFilteredZones();
    Reset( GetCount() );
    OnRowCountChange();
    return GetItemByZone( selected_zone );
}


wxDataViewItem MODEL_ZONES_OVERVIEW::ClearFilter( wxDataViewItem aSelection )
{
    if( m_zoneSettingsBag.GetClonedZoneList().empty() )
        return {};

    ZONE* zone = GetZone( aSelection );
    m_filteredZones = m_zoneSettingsBag.GetClonedZoneList();
    SortFilteredZones();
    Reset( GetCount() );
    OnRowCountChange();
    return GetItemByZone( zone );
}

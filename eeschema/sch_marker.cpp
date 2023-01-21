/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <trigo.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <base_units.h>
#include <erc_settings.h>
#include <sch_marker.h>
#include <schematic.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <erc_item.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_file_versions.h>

/// Factor to convert the maker unit shape to internal units:
#define SCALING_FACTOR schIUScale.mmToIU( 0.15 )


SCH_MARKER::SCH_MARKER( std::shared_ptr<ERC_ITEM> aItem, const VECTOR2I& aPos ) :
        SCH_ITEM( nullptr, SCH_MARKER_T ),
        MARKER_BASE( SCALING_FACTOR, aItem, MARKER_BASE::MARKER_ERC )
{
    if( m_rcItem )
        m_rcItem->SetParent( this );

    m_Pos = aPos;

    m_isLegacyMarker = false;
}


SCH_MARKER::~SCH_MARKER()
{
    if( m_rcItem )
        m_rcItem->SetParent( nullptr );
}


EDA_ITEM* SCH_MARKER::Clone() const
{
    return new SCH_MARKER( *this );
}


void SCH_MARKER::SwapData( SCH_ITEM* aItem )
{
    std::swap( *((SCH_MARKER*) this), *((SCH_MARKER*) aItem ) );
}


wxString SCH_MARKER::Serialize() const
{
    std::shared_ptr<ERC_ITEM> erc = std::static_pointer_cast<ERC_ITEM>( m_rcItem );
    wxString                  sheetSpecificPath, mainItemPath, auxItemPath;

    if( erc->IsSheetSpecific() )
        sheetSpecificPath = erc->GetSpecificSheetPath().Path().AsString();

    if( erc->MainItemHasSheetPath() )
        mainItemPath = erc->GetMainItemSheetPath().Path().AsString();

    if( erc->AuxItemHasSheetPath() )
        auxItemPath = erc->GetAuxItemSheetPath().Path().AsString();

    return wxString::Format( wxT( "%s|%d|%d|%s|%s|%s|%s|%s" ), m_rcItem->GetSettingsKey(), m_Pos.x,
                             m_Pos.y, m_rcItem->GetMainItemID().AsString(),
                             m_rcItem->GetAuxItemID().AsString(), sheetSpecificPath, mainItemPath,
                             auxItemPath );
}


SCH_MARKER* SCH_MARKER::Deserialize( SCHEMATIC* schematic, const wxString& data )
{
    wxArrayString props = wxSplit( data, '|' );
    VECTOR2I      markerPos( (int) strtol( props[1].c_str(), nullptr, 10 ),
                             (int) strtol( props[2].c_str(), nullptr, 10 ) );

    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( props[0] );

    if( !ercItem )
        return nullptr;

    ercItem->SetItems( KIID( props[3] ), KIID( props[4] ) );

    bool isLegacyMarker = true;

    // Deserialize sheet / item specific paths - we are not able to use the file version to determine
    // if markers are legacy as there could be a file opened with a prior version but which has
    // new markers - this code is called not just during schematic load, but also to match new
    // ERC exceptions to exclusions.
    if( props.size() == 8 )
    {
        isLegacyMarker = false;

        SCH_SHEET_LIST sheetPaths = schematic->GetSheets();

        if( !props[5].IsEmpty() )
        {
            KIID_PATH                     sheetSpecificKiidPath( props[5] );
            std::optional<SCH_SHEET_PATH> sheetSpecificPath =
                    sheetPaths.GetSheetPathByKIIDPath( sheetSpecificKiidPath, true );
            if( sheetSpecificPath.has_value() )
                ercItem->SetSheetSpecificPath( sheetSpecificPath.value() );
        }

        if( !props[6].IsEmpty() )
        {
            KIID_PATH                     mainItemKiidPath( props[6] );
            std::optional<SCH_SHEET_PATH> mainItemPath =
                    sheetPaths.GetSheetPathByKIIDPath( mainItemKiidPath, true );
            if( mainItemPath.has_value() )
            {
                if( props[7].IsEmpty() )
                {
                    ercItem->SetItemsSheetPaths( mainItemPath.value() );
                }
                else
                {
                    KIID_PATH                     auxItemKiidPath( props[7] );
                    std::optional<SCH_SHEET_PATH> auxItemPath =
                            sheetPaths.GetSheetPathByKIIDPath( auxItemKiidPath, true );

                    if( auxItemPath.has_value() )
                        ercItem->SetItemsSheetPaths( mainItemPath.value(), auxItemPath.value() );
                }
            }
        }
    }

    SCH_MARKER* marker = new SCH_MARKER( ercItem, markerPos );
    marker->SetIsLegacyMarker( isLegacyMarker );

    return marker;
}


#if defined(DEBUG)

void SCH_MARKER::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML:
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << GetPos() << "/>\n";
}

#endif


void SCH_MARKER::ViewGetLayers( int aLayers[], int& aCount ) const
{
    wxCHECK_RET( Schematic(), "No SCHEMATIC set for SCH_MARKER!" );

    // Don't display sheet-specific markers when SCH_SHEET_PATHs do not match
    std::shared_ptr<ERC_ITEM> ercItem = std::static_pointer_cast<ERC_ITEM>( GetRCItem() );

    if( ercItem->IsSheetSpecific()
        && ( ercItem->GetSpecificSheetPath() != Schematic()->CurrentSheet() ) )
    {
        aCount = 0;
        return;
    }

    aCount = 2;

    if( IsExcluded() )
    {
        aLayers[0] = LAYER_ERC_EXCLUSION;
    }
    else
    {
        switch( Schematic()->ErcSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
        {
        default:
        case SEVERITY::RPT_SEVERITY_ERROR:   aLayers[0] = LAYER_ERC_ERR;  break;
        case SEVERITY::RPT_SEVERITY_WARNING: aLayers[0] = LAYER_ERC_WARN; break;
        }
    }

    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


SCH_LAYER_ID SCH_MARKER::GetColorLayer() const
{
    if( IsExcluded() )
        return LAYER_ERC_EXCLUSION;

    wxCHECK_MSG( Schematic(), LAYER_ERC_ERR, "No SCHEMATIC set for SCH_MARKER!" );

    switch( Schematic()->ErcSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
    {
    default:
    case SEVERITY::RPT_SEVERITY_ERROR:   return LAYER_ERC_ERR;
    case SEVERITY::RPT_SEVERITY_WARNING: return LAYER_ERC_WARN;
    }
}


KIGFX::COLOR4D SCH_MARKER::getColor() const
{
    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();
    return colors->GetColor( GetColorLayer() );
}


SEVERITY SCH_MARKER::GetSeverity() const
{
    if( IsExcluded() )
        return RPT_SEVERITY_EXCLUSION;

    ERC_ITEM* item = static_cast<ERC_ITEM*>( m_rcItem.get() );

    return Schematic()->ErcSettings().GetSeverity( item->GetErrorCode() );
}


void SCH_MARKER::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& aOffset )
{
    PrintMarker( aSettings, aOffset );
}


bool SCH_MARKER::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return SCH_ITEM::Matches( m_rcItem->GetErrorMessage(), aSearchData );
}


const BOX2I SCH_MARKER::GetBoundingBox() const
{
    return GetBoundingBoxMarker();
}


void SCH_MARKER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Electrical Rule Check Error" ), m_rcItem->GetErrorMessage() );
}


BITMAPS SCH_MARKER::GetMenuImage() const
{
    return BITMAPS::erc;
}


void SCH_MARKER::Rotate( const VECTOR2I& aCenter )
{
    // Marker geometry isn't user-editable
}


void SCH_MARKER::MirrorVertically( int aCenter )
{
    // Marker geometry isn't user-editable
}


void SCH_MARKER::MirrorHorizontally( int aCenter )
{
    // Marker geometry isn't user-editable
}


bool SCH_MARKER::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return HitTestMarker( aPosition, aAccuracy );
}

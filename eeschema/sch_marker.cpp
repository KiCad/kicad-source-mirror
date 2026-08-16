/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <sch_draw_panel.h>
#include <google/protobuf/any.pb.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/schematic/schematic_rules.pb.h>
#include <trigo.h>
#include <widgets/msgpanel.h>
#include <bitmaps.h>
#include <base_units.h>
#include <eda_draw_frame.h>
#include <erc/erc_settings.h>
#include <sch_marker.h>
#include <schematic.h>
#include <widgets/ui_common.h>
#include <widgets/wx_data_view_hyperlink_renderer.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <erc/erc_item.h>
#include <sch_screen.h>

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


void SCH_MARKER::swapData( SCH_ITEM* aItem )
{
    SCH_MARKER* item = static_cast<SCH_MARKER*>( aItem );

    std::swap( m_isLegacyMarker, item->m_isLegacyMarker );
    std::swap( m_Pos, item->m_Pos );

    std::swap( m_markerType, item->m_markerType );
    std::swap( m_excluded, item->m_excluded );
    std::swap( m_comment, item->m_comment );
    std::swap( m_rcItem, item->m_rcItem );

    std::swap( m_scalingFactor, item->m_scalingFactor );
    std::swap( m_shapeBoundingBox, item->m_shapeBoundingBox );
}


static void ToProto( kiapi::schematic::ErcMarker& aMsg, const SCH_MARKER& aMarker )
{
    std::shared_ptr<ERC_ITEM> erc = std::static_pointer_cast<ERC_ITEM>( aMarker.GetRCItem() );

    aMsg.set_error_type(
            ToProtoEnum<ERCE_T, kiapi::schematic::ErcErrorType>( static_cast<ERCE_T>( erc->GetErrorCode() ) ) );

    kiapi::common::PackVector2( *aMsg.mutable_position(), aMarker.GetPos(), schIUScale );

    if( erc->IsSheetSpecific() )
        kiapi::common::PackSheetPath( *aMsg.mutable_sheet_specific_path(), erc->GetSpecificSheetPath().Path() );

    if( erc->MainItemHasSheetPath() )
        kiapi::common::PackSheetPath( *aMsg.mutable_main_item_sheet_path(), erc->GetMainItemSheetPath().Path() );

    if( erc->AuxItemHasSheetPath() )
        kiapi::common::PackSheetPath( *aMsg.mutable_aux_item_sheet_path(), erc->GetAuxItemSheetPath().Path() );

    if( erc->GetErrorCode() == ERCE_GENERIC_WARNING
            || erc->GetErrorCode() == ERCE_GENERIC_ERROR
            || erc->GetErrorCode() == ERCE_UNRESOLVED_VARIABLE )
    {
        SCH_ITEM* sch_item = aMarker.Schematic()->ResolveItem( erc->GetMainItemID() );
        SCH_ITEM* parent = sch_item ? static_cast<SCH_ITEM*>( sch_item->GetParent() ) : nullptr;
        EDA_TEXT* text_item = dynamic_cast<EDA_TEXT*>( sch_item );

        // SCH_FIELDs and SCH_ITEMs inside LIB_SYMBOLs don't have persistent KIIDs.  So the
        // exclusion must refer to the parent's KIID, and include the text of the original text
        // item for later look-up.
        if( parent && parent->IsType( { SCH_SYMBOL_T, SCH_LABEL_T, SCH_SHEET_T } ) && text_item )
        {
            aMsg.add_items()->set_value( parent->m_Uuid.AsStdString() );
            aMsg.mutable_child()->set_text_value( text_item->GetText().ToUTF8() );
        }
    }

    for( const KIID& id : erc->GetIDs() )
    {
        if( id != niluuid )
            aMsg.add_items()->set_value( id.AsStdString() );
    }
}


SCH_MARKER* SCH_MARKER::FromProto( const kiapi::schematic::ErcMarker& aMsg, const SCH_SHEET_LIST& aSheetList )
{
    ERCE_T code = FromProtoEnum<ERCE_T, kiapi::schematic::ErcErrorType>( aMsg.error_type() );

    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( code );

    if( !ercItem )
        return nullptr;

    VECTOR2I pos = kiapi::common::UnpackVector2( aMsg.position(), schIUScale );

    if( aMsg.has_sheet_specific_path() )
    {
        KIID_PATH                     path = kiapi::common::UnpackSheetPath( aMsg.sheet_specific_path() );
        std::optional<SCH_SHEET_PATH> sheetPath = aSheetList.GetSheetPathByKIIDPath( path, true );

        if( sheetPath.has_value() )
            ercItem->SetSheetSpecificPath( sheetPath.value() );
    }

    if( aMsg.has_main_item_sheet_path() )
    {
        KIID_PATH                     path = kiapi::common::UnpackSheetPath( aMsg.main_item_sheet_path() );
        std::optional<SCH_SHEET_PATH> mainPath = aSheetList.GetSheetPathByKIIDPath( path, true );

        if( mainPath.has_value() )
        {
            if( aMsg.has_aux_item_sheet_path() )
            {
                KIID_PATH                     auxPath =
                        kiapi::common::UnpackSheetPath( aMsg.aux_item_sheet_path() );
                std::optional<SCH_SHEET_PATH> auxPathResolved =
                        aSheetList.GetSheetPathByKIIDPath( auxPath, true );

                if( auxPathResolved.has_value() )
                    ercItem->SetItemsSheetPaths( mainPath.value(), auxPathResolved.value() );
            }
            else
            {
                ercItem->SetItemsSheetPaths( mainPath.value() );
            }
        }
    }

    if( aMsg.has_child() && aMsg.items_size() > 0 )
    {
        SCH_ITEM* parent = aSheetList.ResolveItem( KIID( aMsg.items( 0 ).value() ) );

        if( !parent )
            return nullptr;

        wxString text = wxString::FromUTF8( aMsg.child().text_value() );
        KIID     uuid = niluuid;

        parent->RunOnChildren(
                [&]( SCH_ITEM* child )
                {
                    if( EDA_TEXT* text_item = dynamic_cast<EDA_TEXT*>( child ) )
                    {
                        if( text_item->GetText() == text )
                            uuid = child->m_Uuid;
                    }
                },
                RECURSE_MODE::NO_RECURSE );

        if( uuid == niluuid && parent->Type() == SCH_SYMBOL_T )
        {
            static_cast<SCH_SYMBOL*>( parent )->GetLibSymbolRef()->RunOnChildren(
                    [&]( SCH_ITEM* child )
                    {
                        if( child->Type() == SCH_FIELD_T )
                        {
                            // Match only on SCH_SYMBOL fields, not LIB_SYMBOL fields.
                        }
                        else if( EDA_TEXT* text_item = dynamic_cast<EDA_TEXT*>( child ) )
                        {
                            if( text_item->GetText() == text )
                                uuid = child->m_Uuid;
                        }
                    },
                    RECURSE_MODE::NO_RECURSE );
        }

        if( uuid != niluuid )
            ercItem->SetItems( uuid );
        else
            return nullptr;
    }
    else
    {
        KIID mainId = aMsg.items_size() > 0 ? KIID( aMsg.items( 0 ).value() ) : niluuid;
        KIID auxId  = aMsg.items_size() > 1 ? KIID( aMsg.items( 1 ).value() ) : niluuid;

        ercItem->SetItems( mainId, auxId );
    }

    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), pos );
    marker->SetIsLegacyMarker( false );

    return marker;
}


void SCH_MARKER::Serialize( google::protobuf::Any& aContainer ) const
{
    kiapi::schematic::ErcMarker msg;
    ToProto( msg, *this );
    aContainer.PackFrom( msg );
}


bool SCH_MARKER::Deserialize( const google::protobuf::Any& aContainer )
{
    kiapi::schematic::ErcMarker msg;

    if( !aContainer.UnpackTo( &msg ) )
        return false;

    if( !Schematic() )
        return false;

    std::unique_ptr<SCH_MARKER> tmp( SCH_MARKER::FromProto( msg, Schematic()->Hierarchy() ) );

    if( !tmp )
        return false;

    swapData( tmp.get() );
    return true;
}


SCH_MARKER* SCH_MARKER::FromLegacyString( const SCH_SHEET_LIST& aSheetList, const wxString& aData )
{
    wxArrayString props = wxSplit( aData, '|' );
    VECTOR2I      markerPos( (int) strtol( props[1].c_str(), nullptr, 10 ),
                             (int) strtol( props[2].c_str(), nullptr, 10 ) );

    std::shared_ptr<ERC_ITEM> ercItem = ERC_ITEM::Create( props[0] );

    if( !ercItem )
        return nullptr;

    if( ercItem->GetErrorCode() == ERCE_GENERIC_WARNING
            || ercItem->GetErrorCode() == ERCE_GENERIC_ERROR
            || ercItem->GetErrorCode() == ERCE_UNRESOLVED_VARIABLE )
    {
        // SCH_FIELDs and SCH_ITEMs inside LIB_SYMBOLs don't have persistent KIIDs.  So the
        // exclusion will contain the parent's KIID in prop[3], and the text of the original
        // text item in prop[4].

        if( !props[4].IsEmpty() )
        {
            KIID      uuid = niluuid;
            SCH_ITEM* parent = aSheetList.ResolveItem( KIID( props[3] ) );

            // Check fields and pins for a match
            parent->RunOnChildren(
                    [&]( SCH_ITEM* child )
                    {
                        if( EDA_TEXT* text_item = dynamic_cast<EDA_TEXT*>( child ) )
                        {
                            if( text_item->GetText() == props[4] )
                                uuid = child->m_Uuid;
                        }
                    },
                    RECURSE_MODE::NO_RECURSE );

            // If it's a symbol, we must also check non-overridden LIB_SYMBOL text children
            if( uuid == niluuid && parent->Type() == SCH_SYMBOL_T )
            {
                static_cast<SCH_SYMBOL*>( parent )->GetLibSymbolRef()->RunOnChildren(
                        [&]( SCH_ITEM* child )
                        {
                            if( child->Type() == SCH_FIELD_T )
                            {
                                // Match only on SCH_SYMBOL fields, not LIB_SYMBOL fields.
                            }
                            else if( EDA_TEXT* text_item = dynamic_cast<EDA_TEXT*>( child ) )
                            {
                                if( text_item->GetText() == props[4] )
                                    uuid = child->m_Uuid;
                            }
                        },
                        RECURSE_MODE::NO_RECURSE );
            }

            if( uuid != niluuid )
                ercItem->SetItems( uuid );
            else
                return nullptr;
        }
        else
        {
            ercItem->SetItems( KIID( props[3] ) );
        }
    }
    else
    {
        ercItem->SetItems( KIID( props[3] ), KIID( props[4] ) );
    }

    bool isLegacyMarker = true;

    // Deserialize sheet / item specific paths - we are not able to use the file version to
    // determine if markers are legacy as there could be a file opened with a prior version
    // but which has new markers - this code is called not just during schematic load, but
    // also to match new ERC exceptions to exclusions.
    if( props.size() == 8 )
    {
        isLegacyMarker = false;

        if( !props[5].IsEmpty() )
        {
            KIID_PATH                     sheetSpecificKiidPath( props[5] );
            std::optional<SCH_SHEET_PATH> sheetSpecificPath =
                    aSheetList.GetSheetPathByKIIDPath( sheetSpecificKiidPath, true );

            if( sheetSpecificPath.has_value() )
                ercItem->SetSheetSpecificPath( sheetSpecificPath.value() );
        }

        if( !props[6].IsEmpty() )
        {
            KIID_PATH                     mainItemKiidPath( props[6] );
            std::optional<SCH_SHEET_PATH> mainItemPath =
                    aSheetList.GetSheetPathByKIIDPath( mainItemKiidPath, true );

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
                            aSheetList.GetSheetPathByKIIDPath( auxItemKiidPath, true );

                    if( auxItemPath.has_value() )
                        ercItem->SetItemsSheetPaths( mainItemPath.value(), auxItemPath.value() );
                }
            }
        }
    }

    SCH_MARKER* marker = new SCH_MARKER( std::move( ercItem ), markerPos );
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


std::vector<int> SCH_MARKER::ViewGetLayers() const
{
    wxCHECK2_MSG( Schematic(), return {}, "No SCHEMATIC set for SCH_MARKER!" );

    // Don't display sheet-specific markers when SCH_SHEET_PATHs do not match
    std::shared_ptr<ERC_ITEM> ercItem = std::static_pointer_cast<ERC_ITEM>( GetRCItem() );

    if( ercItem->IsSheetSpecific()
        && ( ercItem->GetSpecificSheetPath() != Schematic()->CurrentSheet() ) )
    {
        return {};
    }

    std::vector<int> layers( 2 );

    if( IsExcluded() )
    {
        layers[0] = LAYER_ERC_EXCLUSION;
    }
    else
    {
        switch( Schematic()->ErcSettings().GetSeverity( m_rcItem->GetErrorCode() ) )
        {
        case SEVERITY::RPT_SEVERITY_IGNORE:  return {};
        case SEVERITY::RPT_SEVERITY_WARNING: layers[0] = LAYER_ERC_WARN; break;
        default:
        case SEVERITY::RPT_SEVERITY_ERROR:   layers[0] = LAYER_ERC_ERR;  break;
        }
    }

    layers[1] = LAYER_SELECTION_SHADOWS;
    return layers;
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
    return ::GetColorSettings( DEFAULT_THEME )->GetColor( GetColorLayer() );
}


SEVERITY SCH_MARKER::GetSeverity() const
{
    if( IsExcluded() )
        return RPT_SEVERITY_EXCLUSION;

    ERC_ITEM* item = static_cast<ERC_ITEM*>( m_rcItem.get() );

    return Schematic()->ErcSettings().GetSeverity( item->GetErrorCode() );
}


bool SCH_MARKER::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return SCH_ITEM::Matches( m_rcItem->GetErrorMessage( true ), aSearchData );
}


const BOX2I SCH_MARKER::GetBoundingBox() const
{
    return GetBoundingBoxMarker();
}


void SCH_MARKER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Type" ), _( "Marker" ) );
    aList.emplace_back( _( "Violation" ), HYPERLINK_DV_RENDERER::StripMarkup( m_rcItem->GetErrorMessage( true ) ) );

    switch( GetSeverity() )
    {
    case RPT_SEVERITY_IGNORE:
        aList.emplace_back( _( "Severity" ), _( "Ignore" ) );
        break;
    case RPT_SEVERITY_WARNING:
        aList.emplace_back( _( "Severity" ), _( "Warning" ) );
        break;
    case RPT_SEVERITY_ERROR:
        aList.emplace_back( _( "Severity" ), _( "Error" ) );
        break;
    default:
        break;
    }

    if( GetMarkerType() == MARKER_DRAWING_SHEET )
    {
        aList.emplace_back( _( "Drawing Sheet" ), wxEmptyString );
    }
    else
    {
        wxString  mainText;
        wxString  auxText;
        EDA_ITEM* mainItem = nullptr;
        EDA_ITEM* auxItem = nullptr;

        if( m_rcItem->GetMainItemID() != niluuid )
            mainItem = aFrame->ResolveItem( m_rcItem->GetMainItemID() );

        if( m_rcItem->GetAuxItemID() != niluuid )
            auxItem = aFrame->ResolveItem( m_rcItem->GetAuxItemID() );

        if( mainItem )
            mainText = mainItem->GetItemDescription( aFrame, true );

        if( auxItem )
            auxText = auxItem->GetItemDescription( aFrame, true );

        aList.emplace_back( mainText, auxText );
    }

    if( IsExcluded() )
        aList.emplace_back( _( "Excluded" ), m_comment );
}


BITMAPS SCH_MARKER::GetMenuImage() const
{
    return BITMAPS::erc;
}


void SCH_MARKER::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
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

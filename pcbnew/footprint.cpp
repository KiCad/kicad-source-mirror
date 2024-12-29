/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <magic_enum.hpp>

#include <unordered_set>

#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <convert_basic_shapes_to_polygon.h>
#include <convert_shape_list_to_polygon.h>
#include <drc/drc_item.h>
#include <embedded_files.h>
#include <font/font.h>
#include <font/outline_font.h>
#include <footprint.h>
#include <geometry/convex_hull.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <i18n_utility.h>
#include <lset.h>
#include <macros.h>
#include <pad.h>
#include <pcb_dimension.h>
#include <pcb_edit_frame.h>
#include <pcb_field.h>
#include <pcb_group.h>
#include <pcb_marker.h>
#include <pcb_reference_image.h>
#include <pcb_textbox.h>
#include <pcb_track.h>
#include <refdes_utils.h>
#include <string_utils.h>
#include <view/view.h>
#include <zone.h>

#include <google/protobuf/any.pb.h>
#include <api/board/board_types.pb.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/api_pcb_utils.h>
#include <wx/log.h>


FOOTPRINT::FOOTPRINT( BOARD* parent ) :
        BOARD_ITEM_CONTAINER((BOARD_ITEM*) parent, PCB_FOOTPRINT_T ),
        m_boundingBoxCacheTimeStamp( 0 ),
        m_textExcludedBBoxCacheTimeStamp( 0 ),
        m_hullCacheTimeStamp( 0 ),
        m_initial_comments( nullptr ),
        m_componentClass( nullptr )
{
    m_attributes   = 0;
    m_layer        = F_Cu;
    m_orient       = ANGLE_0;
    m_fpStatus     = FP_PADS_are_LOCKED;
    m_arflag       = 0;
    m_link         = 0;
    m_lastEditTime = 0;
    m_zoneConnection          = ZONE_CONNECTION::INHERITED;
    m_fileFormatVersionAtLoad = 0;
    m_embedFonts = false;

    // These are the mandatory fields for the editor to work
    for( int i = 0; i < MANDATORY_FIELDS; i++ )
    {
        PCB_FIELD* field;

        if( i == FOOTPRINT_FIELD )
        {
            m_fields.push_back( nullptr );
        }
        else
        {
            field = new PCB_FIELD( this, i );
            m_fields.push_back( field );
        }

        switch( i )
        {
        case FOOTPRINT_FIELD: break;
        case REFERENCE_FIELD:
            field->SetLayer( F_SilkS );
            field->SetVisible( true );
            break;
        case VALUE_FIELD:
            field->SetLayer( F_Fab );
            field->SetVisible( true );
            break;
        default:
            field->SetLayer( F_Fab );
            field->SetVisible( false );
            break;
        }
    }

    m_3D_Drawings.clear();
}


FOOTPRINT::FOOTPRINT( const FOOTPRINT& aFootprint ) :
    BOARD_ITEM_CONTAINER( aFootprint )
{
    m_pos          = aFootprint.m_pos;
    m_fpid         = aFootprint.m_fpid;
    m_attributes   = aFootprint.m_attributes;
    m_fpStatus     = aFootprint.m_fpStatus;
    m_orient       = aFootprint.m_orient;
    m_lastEditTime = aFootprint.m_lastEditTime;
    m_link         = aFootprint.m_link;
    m_path         = aFootprint.m_path;
    m_embedFonts   = aFootprint.m_embedFonts;

    m_cachedBoundingBox              = aFootprint.m_cachedBoundingBox;
    m_boundingBoxCacheTimeStamp      = aFootprint.m_boundingBoxCacheTimeStamp;
    m_cachedTextExcludedBBox         = aFootprint.m_cachedTextExcludedBBox;
    m_textExcludedBBoxCacheTimeStamp = aFootprint.m_textExcludedBBoxCacheTimeStamp;
    m_cachedHull                     = aFootprint.m_cachedHull;
    m_hullCacheTimeStamp             = aFootprint.m_hullCacheTimeStamp;

    m_clearance                      = aFootprint.m_clearance;
    m_solderMaskMargin               = aFootprint.m_solderMaskMargin;
    m_solderPasteMargin              = aFootprint.m_solderPasteMargin;
    m_solderPasteMarginRatio         = aFootprint.m_solderPasteMarginRatio;
    m_zoneConnection                 = aFootprint.m_zoneConnection;
    m_netTiePadGroups                = aFootprint.m_netTiePadGroups;
    m_fileFormatVersionAtLoad        = aFootprint.m_fileFormatVersionAtLoad;

    m_componentClass                 = aFootprint.m_componentClass;

    std::map<BOARD_ITEM*, BOARD_ITEM*> ptrMap;

    // Copy fields
    for( PCB_FIELD* field : aFootprint.m_fields )
    {
        if( field )
        {
            PCB_FIELD* newField = static_cast<PCB_FIELD*>( field->Clone() );
            ptrMap[field] = newField;
            Add( newField, ADD_MODE::APPEND ); // Append to ensure indexes are identical
        }
        else
        {
            m_fields.push_back( nullptr );
        }
    }

    // Copy pads
    for( PAD* pad : aFootprint.Pads() )
    {
        PAD* newPad = static_cast<PAD*>( pad->Clone() );
        ptrMap[ pad ] = newPad;
        Add( newPad, ADD_MODE::APPEND ); // Append to ensure indexes are identical
    }

    // Copy zones
    for( ZONE* zone : aFootprint.Zones() )
    {
        ZONE* newZone = static_cast<ZONE*>( zone->Clone() );
        ptrMap[ zone ] = newZone;
        Add( newZone, ADD_MODE::APPEND ); // Append to ensure indexes are identical

        // Ensure the net info is OK and especially uses the net info list
        // living in the current board
        // Needed when copying a fp from fp editor that has its own board
        // Must be NETINFO_LIST::ORPHANED_ITEM for a keepout that has no net.
        newZone->SetNetCode( -1 );
    }

    // Copy drawings
    for( BOARD_ITEM* item : aFootprint.GraphicalItems() )
    {
        BOARD_ITEM* newItem = static_cast<BOARD_ITEM*>( item->Clone() );
        ptrMap[ item ] = newItem;
        Add( newItem, ADD_MODE::APPEND ); // Append to ensure indexes are identical
    }

    // Copy groups
    for( PCB_GROUP* group : aFootprint.Groups() )
    {
        PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( group->Clone() );
        ptrMap[ group ] = newGroup;
        Add( newGroup, ADD_MODE::APPEND ); // Append to ensure indexes are identical
    }

    // Rebuild groups
    for( PCB_GROUP* group : aFootprint.Groups() )
    {
        PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( ptrMap[ group ] );

        newGroup->GetItems().clear();

        for( BOARD_ITEM* member : group->GetItems() )
        {
            if( ptrMap.count( member ) )
                newGroup->AddItem( ptrMap[ member ] );
        }
    }

    for( auto& [ name, file ] : aFootprint.EmbeddedFileMap() )
        AddFile( new EMBEDDED_FILES::EMBEDDED_FILE( *file ) );

    // Copy auxiliary data
    m_3D_Drawings   = aFootprint.m_3D_Drawings;
    m_libDescription = aFootprint.m_libDescription;
    m_keywords      = aFootprint.m_keywords;
    m_privateLayers = aFootprint.m_privateLayers;

    m_arflag        = 0;

    m_initial_comments = aFootprint.m_initial_comments ?
                         new wxArrayString( *aFootprint.m_initial_comments ) : nullptr;
}


FOOTPRINT::FOOTPRINT( FOOTPRINT&& aFootprint ) :
    BOARD_ITEM_CONTAINER( aFootprint )
{
    *this = std::move( aFootprint );
}


FOOTPRINT::~FOOTPRINT()
{
    // Untangle group parents before doing any deleting
    for( PCB_GROUP* group : m_groups )
    {
        for( BOARD_ITEM* item : group->GetItems() )
            item->SetParentGroup( nullptr );
    }

    // Clean up the owned elements
    delete m_initial_comments;

    for( PCB_FIELD* f : m_fields )
        delete f;

    m_fields.clear();

    for( PAD* p : m_pads )
        delete p;

    m_pads.clear();

    for( ZONE* zone : m_zones )
        delete zone;

    m_zones.clear();

    for( PCB_GROUP* group : m_groups )
        delete group;

    m_groups.clear();

    for( BOARD_ITEM* d : m_drawings )
        delete d;

    m_drawings.clear();

    if( BOARD* board = GetBoard() )
        board->IncrementTimeStamp();
}


void FOOTPRINT::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::board;
    types::FootprintInstance footprint;

    footprint.mutable_id()->set_value( m_Uuid.AsStdString() );
    footprint.mutable_position()->set_x_nm( GetPosition().x );
    footprint.mutable_position()->set_y_nm( GetPosition().y );
    footprint.mutable_orientation()->set_value_degrees( GetOrientationDegrees() );
    footprint.set_layer( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( GetLayer() ) );
    footprint.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                                     : kiapi::common::types::LockedState::LS_UNLOCKED );

    google::protobuf::Any buf;
    GetField( REFERENCE_FIELD )->Serialize( buf );
    buf.UnpackTo( footprint.mutable_reference_field() );
    GetField( VALUE_FIELD )->Serialize( buf );
    buf.UnpackTo( footprint.mutable_value_field() );
    GetField( DATASHEET_FIELD )->Serialize( buf );
    buf.UnpackTo( footprint.mutable_datasheet_field() );
    GetField( DESCRIPTION_FIELD )->Serialize( buf );
    buf.UnpackTo( footprint.mutable_description_field() );

    types::FootprintAttributes* attrs = footprint.mutable_attributes();

    attrs->set_not_in_schematic( IsBoardOnly() );
    attrs->set_exclude_from_position_files( IsExcludedFromPosFiles() );
    attrs->set_exclude_from_bill_of_materials( IsExcludedFromBOM() );
    attrs->set_exempt_from_courtyard_requirement( AllowMissingCourtyard() );
    attrs->set_do_not_populate( IsDNP() );

    types::Footprint* def = footprint.mutable_definition();

    def->mutable_id()->CopyFrom( kiapi::common::LibIdToProto( GetFPID() ) );
    // anchor?
    def->mutable_attributes()->set_description( GetLibDescription().ToStdString() );
    def->mutable_attributes()->set_keywords( GetKeywords().ToStdString() );

    // TODO: serialize library mandatory fields

    types::FootprintDesignRuleOverrides* overrides = def->mutable_overrides();

    if( GetLocalClearance().has_value() )
        overrides->mutable_copper_clearance()->set_value_nm( *GetLocalClearance() );

    if( GetLocalSolderMaskMargin().has_value() )
        overrides->mutable_solder_mask()->mutable_solder_mask_margin()->set_value_nm( *GetLocalSolderMaskMargin() );

    if( GetLocalSolderPasteMargin().has_value() )
        overrides->mutable_solder_paste()->mutable_solder_paste_margin()->set_value_nm( *GetLocalSolderPasteMargin() );

    if( GetLocalSolderPasteMarginRatio().has_value() )
        overrides->mutable_solder_paste()->mutable_solder_paste_margin_ratio()->set_value( *GetLocalSolderPasteMarginRatio() );

    overrides->set_zone_connection(
            ToProtoEnum<ZONE_CONNECTION, types::ZoneConnectionStyle>( GetLocalZoneConnection() ) );

    for( const wxString& group : GetNetTiePadGroups() )
    {
        types::NetTieDefinition* netTie = def->add_net_ties();
        wxStringTokenizer tokenizer( group, " " );

        while( tokenizer.HasMoreTokens() )
            netTie->add_pad_number( tokenizer.GetNextToken().ToStdString() );
    }

    for( PCB_LAYER_ID layer : GetPrivateLayers().Seq() )
        def->add_private_layers( ToProtoEnum<PCB_LAYER_ID, types::BoardLayer>( layer ) );

    for( const PCB_FIELD* item : m_fields )
    {
        if( !item )
            continue;

        if( item->GetId() < MANDATORY_FIELDS )
            continue;

        google::protobuf::Any* itemMsg = def->add_items();
        item->Serialize( *itemMsg );
    }

    for( const PAD* item : Pads() )
    {
        google::protobuf::Any* itemMsg = def->add_items();
        item->Serialize( *itemMsg );
    }

    for( const BOARD_ITEM* item : GraphicalItems() )
    {
        google::protobuf::Any* itemMsg = def->add_items();
        item->Serialize( *itemMsg );
    }

    for( const ZONE* item : Zones() )
    {
        google::protobuf::Any* itemMsg = def->add_items();
        item->Serialize( *itemMsg );
    }

    for( const FP_3DMODEL& model : Models() )
    {
        google::protobuf::Any* itemMsg = def->add_items();
        types::Footprint3DModel modelMsg;
        modelMsg.set_filename( model.m_Filename.ToUTF8() );
        kiapi::common::PackVector3D( *modelMsg.mutable_scale(), model.m_Scale );
        kiapi::common::PackVector3D( *modelMsg.mutable_rotation(), model.m_Rotation );
        kiapi::common::PackVector3D( *modelMsg.mutable_offset(), model.m_Offset );
        modelMsg.set_visible( model.m_Show );
        modelMsg.set_opacity( model.m_Opacity );
        itemMsg->PackFrom( modelMsg );
    }

    aContainer.PackFrom( footprint );
}


bool FOOTPRINT::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::board;
    types::FootprintInstance footprint;

    if( !aContainer.UnpackTo( &footprint ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( footprint.id().value() );
    SetPosition( VECTOR2I( footprint.position().x_nm(), footprint.position().y_nm() ) );
    SetOrientationDegrees( footprint.orientation().value_degrees() );
    SetLayer( FromProtoEnum<PCB_LAYER_ID, types::BoardLayer>( footprint.layer() ) );
    SetLocked( footprint.locked() == kiapi::common::types::LockedState::LS_LOCKED );

    google::protobuf::Any buf;
    types::Field mandatoryField;

    if( footprint.has_reference_field() )
    {
        mandatoryField = footprint.reference_field();
        mandatoryField.mutable_id()->set_id( REFERENCE_FIELD );
        buf.PackFrom( mandatoryField );
        GetField( REFERENCE_FIELD )->Deserialize( buf );
    }

    if( footprint.has_value_field() )
    {
        mandatoryField = footprint.value_field();
        mandatoryField.mutable_id()->set_id( VALUE_FIELD );
        buf.PackFrom( mandatoryField );
        GetField( VALUE_FIELD )->Deserialize( buf );
    }

    if( footprint.has_datasheet_field() )
    {
        mandatoryField = footprint.datasheet_field();
        mandatoryField.mutable_id()->set_id( DATASHEET_FIELD );
        buf.PackFrom( mandatoryField );
        GetField( DATASHEET_FIELD )->Deserialize( buf );
    }

    if( footprint.has_description_field() )
    {
        mandatoryField = footprint.description_field();
        mandatoryField.mutable_id()->set_id( DESCRIPTION_FIELD );
        buf.PackFrom( mandatoryField );
        GetField( DESCRIPTION_FIELD )->Deserialize( buf );
    }

    SetBoardOnly( footprint.attributes().not_in_schematic() );
    SetExcludedFromBOM( footprint.attributes().exclude_from_bill_of_materials() );
    SetExcludedFromPosFiles( footprint.attributes().exclude_from_position_files() );
    SetAllowMissingCourtyard( footprint.attributes().exempt_from_courtyard_requirement() );
    SetDNP( footprint.attributes().do_not_populate() );

    // Definition
    SetFPID( kiapi::common::LibIdFromProto( footprint.definition().id() ) );
    // TODO: how should anchor be handled?
    SetLibDescription( footprint.definition().attributes().description() );
    SetKeywords( footprint.definition().attributes().keywords() );

    const types::FootprintDesignRuleOverrides& overrides = footprint.overrides();

    if( overrides.has_copper_clearance() )
        SetLocalClearance( overrides.copper_clearance().value_nm() );
    else
        SetLocalClearance( std::nullopt );

    if( overrides.has_solder_mask() && overrides.solder_mask().has_solder_mask_margin() )
        SetLocalSolderMaskMargin( overrides.solder_mask().solder_mask_margin().value_nm() );
    else
        SetLocalSolderMaskMargin( std::nullopt );

    if( overrides.has_solder_paste() )
    {
        const types::SolderPasteOverrides& pasteSettings = overrides.solder_paste();

        if( pasteSettings.has_solder_paste_margin() )
            SetLocalSolderPasteMargin( pasteSettings.solder_paste_margin().value_nm() );
        else
            SetLocalSolderPasteMargin( std::nullopt );

        if( pasteSettings.has_solder_paste_margin_ratio() )
            SetLocalSolderPasteMarginRatio( pasteSettings.solder_paste_margin_ratio().value() );
        else
            SetLocalSolderPasteMarginRatio( std::nullopt );
    }

    SetLocalZoneConnection( FromProtoEnum<ZONE_CONNECTION>( overrides.zone_connection() ) );

    for( const types::NetTieDefinition& netTieMsg : footprint.definition().net_ties() )
    {
        wxString group;

        for( const std::string& pad : netTieMsg.pad_number() )
            group.Append( wxString::Format( wxT( "%s " ), pad ) );

        group.Trim();
        AddNetTiePadGroup( group );
    }

    LSET privateLayers;

    for( int layerMsg : footprint.definition().private_layers() )
    {
        auto layer = static_cast<types::BoardLayer>( layerMsg );
        privateLayers.set( FromProtoEnum<PCB_LAYER_ID, types::BoardLayer>( layer ) );
    }

    SetPrivateLayers( privateLayers );

    // Footprint items
    for( PCB_FIELD* field : m_fields )
    {
        if( field && field->GetId() >= MANDATORY_FIELDS )
            Remove( field );
    }

    Pads().clear();
    GraphicalItems().clear();
    Zones().clear();
    Groups().clear();
    Models().clear();

    for( const google::protobuf::Any& itemMsg : footprint.definition().items() )
    {
        std::optional<KICAD_T> type = kiapi::common::TypeNameFromAny( itemMsg );

        if( !type )
        {
            // Bit of a hack here, but eventually 3D models should be promoted to a first-class
            // object, at which point they can get their own serialization
            if( itemMsg.type_url() == "type.googleapis.com/kiapi.board.types.Footprint3DModel" )
            {
                types::Footprint3DModel modelMsg;

                if( !itemMsg.UnpackTo( &modelMsg ) )
                    continue;

                FP_3DMODEL model;

                model.m_Filename = wxString::FromUTF8( modelMsg.filename() );
                model.m_Show = modelMsg.visible();
                model.m_Opacity = modelMsg.opacity();
                model.m_Scale = kiapi::common::UnpackVector3D( modelMsg.scale() );
                model.m_Rotation = kiapi::common::UnpackVector3D( modelMsg.rotation() );
                model.m_Offset = kiapi::common::UnpackVector3D( modelMsg.offset() );

                Models().push_back( model );
            }
            else
            {
                wxLogTrace( traceApi, wxString::Format( wxS( "Attempting to unpack unknown type %s "
                                                             "from footprint message, skipping" ),
                                                        itemMsg.type_url() ) );
            }

            continue;
        }

        std::unique_ptr<BOARD_ITEM> item = CreateItemForType( *type, this );

        if( item && item->Deserialize( itemMsg ) )
            Add( item.release(), ADD_MODE::APPEND );
    }

    return true;
}


PCB_FIELD* FOOTPRINT::GetField( MANDATORY_FIELD_T aFieldType )
{
    PCB_FIELD* field = m_fields[aFieldType];
    wxASSERT_MSG( field, wxT( "Requesting a null field (likely FOOTPRINT)" ) );

    return m_fields[aFieldType];
}


const PCB_FIELD* FOOTPRINT::GetField( MANDATORY_FIELD_T aFieldType ) const
{
    const PCB_FIELD* field = m_fields[aFieldType];
    wxASSERT_MSG( field, wxT( "Requesting a null field (likely FOOTPRINT)" ) );

    return m_fields[aFieldType];
}


PCB_FIELD* FOOTPRINT::GetFieldById( int aFieldId )
{
    for( PCB_FIELD* field : m_fields )
    {
        if( field && field->GetId() == aFieldId )
            return field;
    }

    return nullptr;
}

bool FOOTPRINT::HasFieldByName( const wxString& aFieldName ) const
{
    for( PCB_FIELD* field : m_fields )
    {
        if( field && field->GetCanonicalName() == aFieldName )
            return true;
    }

    return false;
}

PCB_FIELD* FOOTPRINT::GetFieldByName( const wxString& aFieldName )
{
    if( aFieldName.empty() )
        return nullptr;

    for( PCB_FIELD* field : m_fields )
    {
        if( field && field->GetName() == aFieldName )
            return field;
    }

    return nullptr;
}


wxString FOOTPRINT::GetFieldText( const wxString& aFieldName ) const
{
    for( const PCB_FIELD* field : m_fields )
    {
        if( field && ( aFieldName == field->GetName() || aFieldName == field->GetCanonicalName() ) )
            return field->GetText();
    }

    return wxEmptyString;
}


std::vector<PCB_FIELD*> FOOTPRINT::GetFields( bool aVisibleOnly ) const
{
    std::vector<PCB_FIELD*> fields;

    for( PCB_FIELD* field : m_fields )
    {
        // FOOTPRINT field doesn't exist on FOOTPRINT, but it's kept in the m_fields vector
        // for the moment so that the MANDATORY_FIELD ids line up with vector indices.
        // This could be cleaned up by making m_fields a map in the future.
        if( !field || field->GetId() == FOOTPRINT_FIELD )
            continue;

        if( aVisibleOnly )
        {
            if( !field->IsVisible() || field->GetText().IsEmpty() )
                continue;
        }

        fields.push_back( field );
    }

    return fields;
}


void FOOTPRINT::GetFields( std::vector<PCB_FIELD*>& aVector, bool aVisibleOnly ) const
{
    aVector = GetFields( aVisibleOnly );
}


PCB_FIELD* FOOTPRINT::AddField( const PCB_FIELD& aField )
{
    int newNdx = m_fields.size();

    m_fields.push_back( new PCB_FIELD( aField ) );
    return m_fields[newNdx];
}


void FOOTPRINT::RemoveField( const wxString& aFieldName )
{
    for( unsigned i = MANDATORY_FIELDS; i < m_fields.size(); ++i )
    {
        if( aFieldName == m_fields[i]->GetName( false ) )
        {
            m_fields.erase( m_fields.begin() + i );
            return;
        }
    }
}


void FOOTPRINT::ApplyDefaultSettings( const BOARD& board, bool aStyleFields, bool aStyleText,
                                      bool aStyleShapes )
{
    if( aStyleFields )
    {
        for( PCB_FIELD* field : m_fields )
        {
            if( field )
                field->StyleFromSettings( board.GetDesignSettings() );
        }
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        switch( item->Type() )
        {
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
            if( aStyleText )
                item->StyleFromSettings( board.GetDesignSettings() );

            break;

        case PCB_SHAPE_T:
            if( aStyleShapes && !item->IsOnCopperLayer() )
                item->StyleFromSettings( board.GetDesignSettings() );

            break;

        default:
            break;
        }
    }
}


bool FOOTPRINT::FixUuids()
{
    // replace null UUIDs if any by a valid uuid
    std::vector< BOARD_ITEM* > item_list;

    for( PCB_FIELD* field : m_fields )
    {
        if( field )
            item_list.push_back( field );
    }

    for( PAD* pad : m_pads )
        item_list.push_back( pad );

    for( BOARD_ITEM* gr_item : m_drawings )
        item_list.push_back( gr_item );

    // Note: one cannot fix null UUIDs inside the group, but it should not happen
    // because null uuids can be found in old footprints, therefore without group
    for( PCB_GROUP* group : m_groups )
        item_list.push_back( group );

    // Probably notneeded, because old fp do not have zones. But just in case.
    for( ZONE* zone : m_zones )
        item_list.push_back( zone );

    bool changed = false;

    for( BOARD_ITEM* item : item_list )
    {
        if( item->m_Uuid == niluuid )
        {
            const_cast<KIID&>( item->m_Uuid ) = KIID();
            changed = true;
        }
    }

    return changed;
}


FOOTPRINT& FOOTPRINT::operator=( FOOTPRINT&& aOther )
{
    BOARD_ITEM::operator=( aOther );

    m_pos           = aOther.m_pos;
    m_fpid          = aOther.m_fpid;
    m_attributes    = aOther.m_attributes;
    m_fpStatus      = aOther.m_fpStatus;
    m_orient        = aOther.m_orient;
    m_lastEditTime  = aOther.m_lastEditTime;
    m_link          = aOther.m_link;
    m_path          = aOther.m_path;

    m_cachedBoundingBox              = aOther.m_cachedBoundingBox;
    m_boundingBoxCacheTimeStamp      = aOther.m_boundingBoxCacheTimeStamp;
    m_cachedTextExcludedBBox         = aOther.m_cachedTextExcludedBBox;
    m_textExcludedBBoxCacheTimeStamp = aOther.m_textExcludedBBoxCacheTimeStamp;
    m_cachedHull                     = aOther.m_cachedHull;
    m_hullCacheTimeStamp             = aOther.m_hullCacheTimeStamp;

    m_clearance                      = aOther.m_clearance;
    m_solderMaskMargin               = aOther.m_solderMaskMargin;
    m_solderPasteMargin              = aOther.m_solderPasteMargin;
    m_solderPasteMarginRatio         = aOther.m_solderPasteMarginRatio;
    m_zoneConnection                 = aOther.m_zoneConnection;
    m_netTiePadGroups                = aOther.m_netTiePadGroups;

    m_componentClass                 = aOther.m_componentClass;

    // Move the fields
    m_fields.clear();

    for( PCB_FIELD* field : aOther.m_fields )
    {
        if( field )
            Add( field );
        else
            m_fields.push_back( nullptr );
    }

    // Move the pads
    m_pads.clear();

    for( PAD* pad : aOther.Pads() )
        Add( pad );

    aOther.Pads().clear();

    // Move the zones
    m_zones.clear();

    for( ZONE* item : aOther.Zones() )
    {
        Add( item );

        // Ensure the net info is OK and especially uses the net info list
        // living in the current board
        // Needed when copying a fp from fp editor that has its own board
        // Must be NETINFO_LIST::ORPHANED_ITEM for a keepout that has no net.
        item->SetNetCode( -1 );
    }

    aOther.Zones().clear();

    // Move the drawings
    m_drawings.clear();

    for( BOARD_ITEM* item : aOther.GraphicalItems() )
        Add( item );

    aOther.GraphicalItems().clear();

    // Move the groups
    m_groups.clear();

    for( PCB_GROUP* group : aOther.Groups() )
        Add( group );

    aOther.Groups().clear();

    // Copy auxiliary data
    m_3D_Drawings      = aOther.m_3D_Drawings;
    m_libDescription   = aOther.m_libDescription;
    m_keywords         = aOther.m_keywords;
    m_privateLayers    = aOther.m_privateLayers;

    m_initial_comments = aOther.m_initial_comments;

    // Clear the other item's containers since this is a move
    aOther.m_fields.clear();
    aOther.Pads().clear();
    aOther.Zones().clear();
    aOther.GraphicalItems().clear();
    aOther.m_initial_comments = nullptr;

    return *this;
}


FOOTPRINT& FOOTPRINT::operator=( const FOOTPRINT& aOther )
{
    BOARD_ITEM::operator=( aOther );

    m_pos           = aOther.m_pos;
    m_fpid          = aOther.m_fpid;
    m_attributes    = aOther.m_attributes;
    m_fpStatus      = aOther.m_fpStatus;
    m_orient        = aOther.m_orient;
    m_lastEditTime  = aOther.m_lastEditTime;
    m_link          = aOther.m_link;
    m_path          = aOther.m_path;

    m_cachedBoundingBox              = aOther.m_cachedBoundingBox;
    m_boundingBoxCacheTimeStamp      = aOther.m_boundingBoxCacheTimeStamp;
    m_cachedTextExcludedBBox         = aOther.m_cachedTextExcludedBBox;
    m_textExcludedBBoxCacheTimeStamp = aOther.m_textExcludedBBoxCacheTimeStamp;
    m_cachedHull                     = aOther.m_cachedHull;
    m_hullCacheTimeStamp             = aOther.m_hullCacheTimeStamp;

    m_clearance                      = aOther.m_clearance;
    m_solderMaskMargin               = aOther.m_solderMaskMargin;
    m_solderPasteMargin              = aOther.m_solderPasteMargin;
    m_solderPasteMarginRatio         = aOther.m_solderPasteMarginRatio;
    m_zoneConnection                 = aOther.m_zoneConnection;
    m_netTiePadGroups                = aOther.m_netTiePadGroups;

    m_componentClass                 = aOther.m_componentClass;

    std::map<BOARD_ITEM*, BOARD_ITEM*> ptrMap;

    // Copy fields
    m_fields.clear();

    for( PCB_FIELD* field : aOther.m_fields )
    {
        if( field )
        {
            PCB_FIELD* newField = new PCB_FIELD( *field );
            ptrMap[field] = newField;
            Add( newField );
        }
        else
        {
            m_fields.push_back( nullptr );
        }
    }

    // Copy pads
    m_pads.clear();

    for( PAD* pad : aOther.Pads() )
    {
        PAD* newPad = new PAD( *pad );
        ptrMap[ pad ] = newPad;
        Add( newPad );
    }

    // Copy zones
    m_zones.clear();

    for( ZONE* zone : aOther.Zones() )
    {
        ZONE* newZone = static_cast<ZONE*>( zone->Clone() );
        ptrMap[ zone ] = newZone;
        Add( newZone );

        // Ensure the net info is OK and especially uses the net info list
        // living in the current board
        // Needed when copying a fp from fp editor that has its own board
        // Must be NETINFO_LIST::ORPHANED_ITEM for a keepout that has no net.
        newZone->SetNetCode( -1 );
    }

    // Copy drawings
    m_drawings.clear();

    for( BOARD_ITEM* item : aOther.GraphicalItems() )
    {
        BOARD_ITEM* newItem = static_cast<BOARD_ITEM*>( item->Clone() );
        ptrMap[ item ] = newItem;
        Add( newItem );
    }

    // Copy groups
    m_groups.clear();

    for( PCB_GROUP* group : aOther.Groups() )
    {
        PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( group->Clone() );
        newGroup->GetItems().clear();

        for( BOARD_ITEM* member : group->GetItems() )
            newGroup->AddItem( ptrMap[ member ] );

        Add( newGroup );
    }

    // Copy auxiliary data
    m_3D_Drawings   = aOther.m_3D_Drawings;
    m_libDescription = aOther.m_libDescription;
    m_keywords      = aOther.m_keywords;
    m_privateLayers = aOther.m_privateLayers;

    m_initial_comments = aOther.m_initial_comments ?
                            new wxArrayString( *aOther.m_initial_comments ) : nullptr;

    return *this;
}


bool FOOTPRINT::IsConflicting() const
{
    return HasFlag( COURTYARD_CONFLICT );
}


void FOOTPRINT::GetContextualTextVars( wxArrayString* aVars ) const
{
    aVars->push_back( wxT( "REFERENCE" ) );
    aVars->push_back( wxT( "VALUE" ) );
    aVars->push_back( wxT( "LAYER" ) );
    aVars->push_back( wxT( "FOOTPRINT_LIBRARY" ) );
    aVars->push_back( wxT( "FOOTPRINT_NAME" ) );
    aVars->push_back( wxT( "SHORT_NET_NAME(<pad_number>)" ) );
    aVars->push_back( wxT( "NET_NAME(<pad_number>)" ) );
    aVars->push_back( wxT( "NET_CLASS(<pad_number>)" ) );
    aVars->push_back( wxT( "PIN_NAME(<pad_number>)" ) );
}


bool FOOTPRINT::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( GetBoard() && GetBoard()->GetBoardUse() == BOARD_USE::FPHOLDER )
        return false;

    if( token->IsSameAs( wxT( "REFERENCE" ) ) )
    {
        *token = Reference().GetShownText( false, aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "VALUE" ) ) )
    {
        *token = Value().GetShownText( false, aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "LAYER" ) ) )
    {
        *token = GetLayerName();
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_LIBRARY" ) ) )
    {
        *token = m_fpid.GetUniStringLibNickname();
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        *token = m_fpid.GetUniStringLibItemName();
        return true;
    }
    else if( token->StartsWith( wxT( "SHORT_NET_NAME(" ) )
                 || token->StartsWith( wxT( "NET_NAME(" ) )
                 || token->StartsWith( wxT( "NET_CLASS(" ) )
                 || token->StartsWith( wxT( "PIN_NAME(" ) ) )
    {
        wxString padNumber = token->AfterFirst( '(' );
        padNumber = padNumber.BeforeLast( ')' );

        for( PAD* pad : Pads() )
        {
            if( pad->GetNumber() == padNumber )
            {
                if( token->StartsWith( wxT( "SHORT_NET_NAME" ) ) )
                    *token = pad->GetShortNetname();
                else if( token->StartsWith( wxT( "NET_NAME" ) ) )
                    *token = pad->GetNetname();
                else if( token->StartsWith( wxT( "NET_CLASS" ) ) )
                    *token = pad->GetNetClassVariableSubstitutionName();
                else
                    *token = pad->GetPinFunction();

                return true;
            }
        }
    }
    else if( HasFieldByName( *token ) )
    {
        *token = GetFieldText( *token );
        return true;
    }

    if( GetBoard() && GetBoard()->ResolveTextVar( token, aDepth + 1 ) )
        return true;

    return false;
}


void FOOTPRINT::ClearAllNets()
{
    // Force the ORPHANED dummy net info for all pads.
    // ORPHANED dummy net does not depend on a board
    for( PAD* pad : m_pads )
        pad->SetNetCode( NETINFO_LIST::ORPHANED );
}


void FOOTPRINT::Add( BOARD_ITEM* aBoardItem, ADD_MODE aMode, bool aSkipConnectivity )
{
    switch( aBoardItem->Type() )
    {
    case PCB_FIELD_T:
        // Always append fields
        m_fields.push_back( static_cast<PCB_FIELD*>( aBoardItem ) );

        break;

    case PCB_TEXT_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_SHAPE_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_REFERENCE_IMAGE_T:
        if( aMode == ADD_MODE::APPEND )
            m_drawings.push_back( aBoardItem );
        else
            m_drawings.push_front( aBoardItem );
        break;

    case PCB_PAD_T:
        if( aMode == ADD_MODE::APPEND )
            m_pads.push_back( static_cast<PAD*>( aBoardItem ) );
        else
            m_pads.push_front( static_cast<PAD*>( aBoardItem ) );
        break;

    case PCB_ZONE_T:
        if( aMode == ADD_MODE::APPEND )
            m_zones.push_back( static_cast<ZONE*>( aBoardItem ) );
        else
            m_zones.insert( m_zones.begin(), static_cast<ZONE*>( aBoardItem ) );
        break;

    case PCB_GROUP_T:
        if( aMode == ADD_MODE::APPEND )
            m_groups.push_back( static_cast<PCB_GROUP*>( aBoardItem ) );
        else
            m_groups.insert( m_groups.begin(), static_cast<PCB_GROUP*>( aBoardItem ) );
        break;

    default:
    {
        wxString msg;
        msg.Printf( wxT( "FOOTPRINT::Add() needs work: BOARD_ITEM type (%d) not handled" ),
                    aBoardItem->Type() );
        wxFAIL_MSG( msg );

        return;
    }
    }

    aBoardItem->ClearEditFlags();
    aBoardItem->SetParent( this );
}


void FOOTPRINT::Remove( BOARD_ITEM* aBoardItem, REMOVE_MODE aMode )
{
    switch( aBoardItem->Type() )
    {
    case PCB_FIELD_T:
    {
        for( auto it = m_fields.begin(); it != m_fields.end(); ++it )
        {
            if( *it == aBoardItem )
            {
                m_fields.erase( it );
                break;
            }
        }
    }
    break;

    case PCB_TEXT_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_SHAPE_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_REFERENCE_IMAGE_T:
        for( auto it = m_drawings.begin(); it != m_drawings.end(); ++it )
        {
            if( *it == aBoardItem )
            {
                m_drawings.erase( it );
                break;
            }
        }

        break;

    case PCB_PAD_T:
        for( auto it = m_pads.begin(); it != m_pads.end(); ++it )
        {
            if( *it == static_cast<PAD*>( aBoardItem ) )
            {
                m_pads.erase( it );
                break;
            }
        }

        break;

    case PCB_ZONE_T:
        for( auto it = m_zones.begin(); it != m_zones.end(); ++it )
        {
            if( *it == static_cast<ZONE*>( aBoardItem ) )
            {
                m_zones.erase( it );
                break;
            }
        }

        break;

    case PCB_GROUP_T:
        for( auto it = m_groups.begin(); it != m_groups.end(); ++it )
        {
            if( *it == static_cast<PCB_GROUP*>( aBoardItem ) )
            {
                m_groups.erase( it );
                break;
            }
        }

        break;

    default:
    {
        wxString msg;
        msg.Printf( wxT( "FOOTPRINT::Remove() needs work: BOARD_ITEM type (%d) not handled" ),
                    aBoardItem->Type() );
        wxFAIL_MSG( msg );
    }
    }

    aBoardItem->SetFlags( STRUCT_DELETED );

    PCB_GROUP* parentGroup = aBoardItem->GetParentGroup();

    if( parentGroup && !( parentGroup->GetFlags() & STRUCT_DELETED ) )
        parentGroup->RemoveItem( aBoardItem );
}


double FOOTPRINT::GetArea( int aPadding ) const
{
    BOX2I bbox = GetBoundingBox( false );

    double w = std::abs( static_cast<double>( bbox.GetWidth() ) ) + aPadding;
    double h = std::abs( static_cast<double>( bbox.GetHeight() ) ) + aPadding;
    return w * h;
}


int FOOTPRINT::GetLikelyAttribute() const
{
    int smd_count = 0;
    int tht_count = 0;

    for( PAD* pad : m_pads )
    {
        switch( pad->GetProperty() )
        {
        case PAD_PROP::FIDUCIAL_GLBL:
        case PAD_PROP::FIDUCIAL_LOCAL:
            continue;

        case PAD_PROP::HEATSINK:
        case PAD_PROP::CASTELLATED:
        case PAD_PROP::MECHANICAL:
            continue;

        case PAD_PROP::NONE:
        case PAD_PROP::BGA:
        case PAD_PROP::TESTPOINT:
            break;
        }

        switch( pad->GetAttribute() )
        {
        case PAD_ATTRIB::PTH:
            tht_count++;
            break;

        case PAD_ATTRIB::SMD:
            if( pad->IsOnCopperLayer() )
                smd_count++;

            break;

        default:
            break;
        }
    }

    // Footprints with plated through-hole pads should usually be marked through hole even if they
    // also have SMD because they might not be auto-placed.  Exceptions to this might be shielded
    if( tht_count > 0 )
        return FP_THROUGH_HOLE;

    if( smd_count > 0 )
        return FP_SMD;

    return 0;
}


wxString FOOTPRINT::GetTypeName() const
{
    if( ( m_attributes & FP_SMD ) == FP_SMD )
        return _( "SMD" );

    if( ( m_attributes & FP_THROUGH_HOLE ) == FP_THROUGH_HOLE )
        return _( "Through hole" );

    return _( "Other" );
}


BOX2I FOOTPRINT::GetFpPadsLocalBbox() const
{
    BOX2I bbox;

    // We want the bounding box of the footprint pads at rot 0, not flipped
    // Create such a image:
    FOOTPRINT dummy( *this );

    dummy.SetPosition( VECTOR2I( 0, 0 ) );
    dummy.SetOrientation( ANGLE_0 );

    if( dummy.IsFlipped() )
        dummy.Flip( VECTOR2I( 0, 0 ), FLIP_DIRECTION::TOP_BOTTOM );

    for( PAD* pad : dummy.Pads() )
        bbox.Merge( pad->GetBoundingBox() );

    // Remove the parent and the group from the dummy footprint before deletion
    dummy.SetParent( nullptr );
    dummy.SetParentGroup( nullptr );

    return bbox;
}


bool FOOTPRINT::TextOnly() const
{
    for( BOARD_ITEM* item : m_drawings )
    {
        if( m_privateLayers.test( item->GetLayer() ) )
            continue;

        if( item->Type() != PCB_FIELD_T && item->Type() != PCB_TEXT_T )
            return false;
    }

    return true;
}


const BOX2I FOOTPRINT::GetBoundingBox() const
{
    return GetBoundingBox( true );
}


const BOX2I FOOTPRINT::GetBoundingBox( bool aIncludeText ) const
{
    const BOARD* board = GetBoard();

    if( board )
    {
        if( aIncludeText )
        {
            if( m_boundingBoxCacheTimeStamp >= board->GetTimeStamp() )
                return m_cachedBoundingBox;
        }
        else
        {
            if( m_textExcludedBBoxCacheTimeStamp >= board->GetTimeStamp() )
                return m_cachedTextExcludedBBox;
        }
    }

    std::vector<PCB_TEXT*> texts;
    bool                   isFPEdit = board && board->IsFootprintHolder();

    BOX2I bbox( m_pos );
    bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) );   // Give a min size to the bbox

    // Calculate the footprint side
    PCB_LAYER_ID footprintSide = GetSide();

    for( BOARD_ITEM* item : m_drawings )
    {
        if( m_privateLayers.test( item->GetLayer() ) && !isFPEdit )
            continue;

        // We want the bitmap bounding box just in the footprint editor
        // so it will start with the correct initial zoom
        if( item->Type() == PCB_REFERENCE_IMAGE_T && !isFPEdit )
            continue;

        // Handle text separately
        if( item->Type() == PCB_TEXT_T )
        {
            texts.push_back( static_cast<PCB_TEXT*>( item ) );
            continue;
        }

        // If we're not including text then drop annotations as well -- unless, of course, it's
        // an unsided footprint -- in which case it's likely to be nothing *but* annotations.
        if( !aIncludeText && footprintSide != UNDEFINED_LAYER )
        {
            if( BaseType( item->Type() ) == PCB_DIMENSION_T )
                continue;

            if( item->GetLayer() == Cmts_User || item->GetLayer() == Dwgs_User
                || item->GetLayer() == Eco1_User || item->GetLayer() == Eco2_User )
            {
                continue;
            }
        }

        bbox.Merge( item->GetBoundingBox() );
    }

    for( PCB_FIELD* field : m_fields )
    {
        // Reference and value get their own processing
        if( field && !field->IsReference() && !field->IsValue() )
            texts.push_back( field );
    }

    for( PAD* pad : m_pads )
        bbox.Merge( pad->GetBoundingBox() );

    for( ZONE* zone : m_zones )
        bbox.Merge( zone->GetBoundingBox() );

    bool noDrawItems = ( m_drawings.empty() && m_pads.empty() && m_zones.empty() );

    // Groups do not contribute to the rect, only their members
    if( aIncludeText || noDrawItems )
    {
        // Only PCB_TEXT and PCB_FIELD items are independently selectable;
        // PCB_TEXTBOX items go in with other graphic items above.
        for( PCB_TEXT* text : texts )
        {
            if( !isFPEdit && m_privateLayers.test( text->GetLayer() ) )
                continue;

            if( text->IsVisible() )
                bbox.Merge( text->GetBoundingBox() );
        }

        // This can be further optimized when aIncludeInvisibleText is true, but currently
        // leaving this as is until it's determined there is a noticeable speed hit.
        bool   valueLayerIsVisible = true;
        bool   refLayerIsVisible   = true;

        if( board )
        {
            // The first "&&" conditional handles the user turning layers off as well as layers
            // not being present in the current PCB stackup.  Values, references, and all
            // footprint text can also be turned off via the GAL meta-layers, so the 2nd and
            // 3rd "&&" conditionals handle that.
            valueLayerIsVisible = board->IsLayerVisible( Value().GetLayer() )
                                  && board->IsElementVisible( LAYER_FP_VALUES )
                                  && board->IsElementVisible( LAYER_FP_TEXT );

            refLayerIsVisible = board->IsLayerVisible( Reference().GetLayer() )
                                && board->IsElementVisible( LAYER_FP_REFERENCES )
                                && board->IsElementVisible( LAYER_FP_TEXT );
        }


        if( ( Value().IsVisible() && valueLayerIsVisible ) || noDrawItems )
        {
            bbox.Merge( Value().GetBoundingBox() );
        }

        if( ( Reference().IsVisible() && refLayerIsVisible ) || noDrawItems )
        {
            bbox.Merge( Reference().GetBoundingBox() );
        }
    }

    if( board )
    {
        if( aIncludeText || noDrawItems )
        {
            m_boundingBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedBoundingBox = bbox;
        }
        else
        {
            m_textExcludedBBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedTextExcludedBBox = bbox;
        }
    }

    return bbox;
}


const BOX2I FOOTPRINT::GetLayerBoundingBox( LSET aLayers ) const
{
    std::vector<PCB_TEXT*> texts;
    const BOARD* board = GetBoard();
    bool         isFPEdit = board && board->IsFootprintHolder();

    // Start with an uninitialized bounding box
    BOX2I bbox;

    for( BOARD_ITEM* item : m_drawings )
    {
        if( m_privateLayers.test( item->GetLayer() ) && !isFPEdit )
            continue;

        if( ( aLayers & item->GetLayerSet() ).none() )
            continue;

        // We want the bitmap bounding box just in the footprint editor
        // so it will start with the correct initial zoom
        if( item->Type() == PCB_REFERENCE_IMAGE_T && !isFPEdit )
            continue;

        bbox.Merge( item->GetBoundingBox() );
    }

    for( PAD* pad : m_pads )
    {
        if( ( aLayers & pad->GetLayerSet() ).none() )
            continue;

        bbox.Merge( pad->GetBoundingBox() );
    }

    for( ZONE* zone : m_zones )
    {
        if( ( aLayers & zone->GetLayerSet() ).none() )
            continue;

        bbox.Merge( zone->GetBoundingBox() );
    }

    return bbox;
}


SHAPE_POLY_SET FOOTPRINT::GetBoundingHull() const
{
    const BOARD* board = GetBoard();
    bool         isFPEdit = board && board->IsFootprintHolder();

    if( board )
    {
        if( m_hullCacheTimeStamp >= board->GetTimeStamp() )
            return m_cachedHull;
    }

    SHAPE_POLY_SET rawPolys;
    SHAPE_POLY_SET hull;

    for( BOARD_ITEM* item : m_drawings )
    {
        if( !isFPEdit && m_privateLayers.test( item->GetLayer() ) )
            continue;

        if( item->Type() != PCB_FIELD_T && item->Type() != PCB_REFERENCE_IMAGE_T )
        {
            item->TransformShapeToPolygon( rawPolys, UNDEFINED_LAYER, 0, ARC_LOW_DEF,
                                           ERROR_OUTSIDE );
        }

        // We intentionally exclude footprint fields from the bounding hull.
    }

    for( PAD* pad : m_pads )
    {
        pad->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                pad->TransformShapeToPolygon( rawPolys, aLayer, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
            } );

        // In case hole is larger than pad
        pad->TransformHoleToPolygon( rawPolys, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
    }

    for( ZONE* zone : m_zones )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            const SHAPE_POLY_SET& layerPoly = *zone->GetFilledPolysList( layer );

            for( int ii = 0; ii < layerPoly.OutlineCount(); ii++ )
            {
                const SHAPE_LINE_CHAIN& poly = layerPoly.COutline( ii );
                rawPolys.AddOutline( poly );
            }
        }
    }

    // If there are some graphic items, build the actual hull.
    // However if no items, create a minimal polygon (can happen if a footprint
    // is created with no item: it contains only 2 texts.
    if( rawPolys.OutlineCount() == 0 )
    {
        // generate a small dummy rectangular outline around the anchor
        const int halfsize = pcbIUScale.mmToIU( 1.0 );

        rawPolys.NewOutline();

        // add a square:
        rawPolys.Append( GetPosition().x - halfsize,  GetPosition().y - halfsize );
        rawPolys.Append( GetPosition().x + halfsize,  GetPosition().y - halfsize );
        rawPolys.Append( GetPosition().x + halfsize,  GetPosition().y + halfsize );
        rawPolys.Append( GetPosition().x - halfsize,  GetPosition().y + halfsize );
    }

    std::vector<VECTOR2I> convex_hull;
    BuildConvexHull( convex_hull, rawPolys );

    m_cachedHull.RemoveAllContours();
    m_cachedHull.NewOutline();

    for( const VECTOR2I& pt : convex_hull )
        m_cachedHull.Append( pt );

    if( board )
        m_hullCacheTimeStamp = board->GetTimeStamp();

    return m_cachedHull;
}


void FOOTPRINT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg, msg2;

    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( UnescapeString( Reference().GetText() ),
                        UnescapeString( Value().GetText() ) );

    if( aFrame->IsType( FRAME_FOOTPRINT_VIEWER )
        || aFrame->IsType( FRAME_FOOTPRINT_CHOOSER )
        || aFrame->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        size_t     padCount = GetPadCount( DO_NOT_INCLUDE_NPTH );

        aList.emplace_back( _( "Library" ), GetFPID().GetLibNickname().wx_str() );

        aList.emplace_back( _( "Footprint Name" ), GetFPID().GetLibItemName().wx_str() );

        aList.emplace_back( _( "Pads" ), wxString::Format( wxT( "%zu" ), padCount ) );

        aList.emplace_back( wxString::Format( _( "Doc: %s" ), GetLibDescription() ),
                            wxString::Format( _( "Keywords: %s" ), GetKeywords() ) );

        return;
    }

    // aFrame is the board editor:

    switch( GetSide() )
    {
    case F_Cu: aList.emplace_back( _( "Board Side" ), _( "Front" ) );          break;
    case B_Cu: aList.emplace_back( _( "Board Side" ), _( "Back (Flipped)" ) ); break;
    default:   /* unsided: user-layers only, etc. */                           break;
    }

    auto addToken = []( wxString* aStr, const wxString& aAttr )
                    {
                        if( !aStr->IsEmpty() )
                            *aStr += wxT( ", " );

                        *aStr += aAttr;
                    };

    wxString status;
    wxString attrs;

    if( IsLocked() )
        addToken( &status, _( "Locked" ) );

    if( m_fpStatus & FP_is_PLACED )
        addToken( &status, _( "autoplaced" ) );

    if( m_attributes & FP_BOARD_ONLY )
        addToken( &attrs, _( "not in schematic" ) );

    if( m_attributes & FP_EXCLUDE_FROM_POS_FILES )
        addToken( &attrs, _( "exclude from pos files" ) );

    if( m_attributes & FP_EXCLUDE_FROM_BOM )
        addToken( &attrs, _( "exclude from BOM" ) );

    if( m_attributes & FP_DNP )
        addToken( &attrs, _( "DNP" ) );

    aList.emplace_back( _( "Status: " ) + status, _( "Attributes:" ) + wxS( " " ) + attrs );

    aList.emplace_back( _( "Rotation" ), wxString::Format( wxT( "%.4g" ),
                                                           GetOrientation().AsDegrees() ) );

    if( m_componentClass )
    {
        aList.emplace_back( _( "Component Class" ), m_componentClass->GetName() );
    }

    msg.Printf( _( "Footprint: %s" ), m_fpid.GetUniStringLibId() );
    msg2.Printf( _( "3D-Shape: %s" ), m_3D_Drawings.empty() ? _( "<none>" )
                                                            : m_3D_Drawings.front().m_Filename );
    aList.emplace_back( msg, msg2 );

    msg.Printf( _( "Doc: %s" ), m_libDescription );
    msg2.Printf( _( "Keywords: %s" ), m_keywords );
    aList.emplace_back( msg, msg2 );
}


PCB_LAYER_ID FOOTPRINT::GetSide() const
{
    if( const BOARD* board = GetBoard() )
    {
        if( board->IsFootprintHolder() )
            return UNDEFINED_LAYER;
    }

    // Test pads first; they're the most likely to return a quick answer.
    for( PAD* pad : m_pads )
    {
        if( ( LSET::SideSpecificMask() & pad->GetLayerSet() ).any() )
            return GetLayer();
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        if( LSET::SideSpecificMask().test( item->GetLayer() ) )
            return GetLayer();
    }

    for( ZONE* zone : m_zones )
    {
        if( ( LSET::SideSpecificMask() & zone->GetLayerSet() ).any() )
            return GetLayer();
    }

    return UNDEFINED_LAYER;
}


bool FOOTPRINT::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    // If we have any pads, fall back on normal checking
    for( PAD* pad : m_pads )
    {
        if( pad->IsOnLayer( aLayer ) )
            return true;
    }

    for( ZONE* zone : m_zones )
    {
        if( zone->IsOnLayer( aLayer ) )
            return true;
    }

    for( PCB_FIELD* field : m_fields )
    {
        if( field && field->IsOnLayer( aLayer ) )
            return true;
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->IsOnLayer( aLayer ) )
            return true;
    }

    return false;
}


bool FOOTPRINT::HitTestOnLayer( const VECTOR2I& aPosition, PCB_LAYER_ID aLayer, int aAccuracy ) const
{
    for( PAD* pad : m_pads )
    {
        if( pad->IsOnLayer( aLayer ) && pad->HitTest( aPosition, aAccuracy ) )
            return true;
    }

    for( ZONE* zone : m_zones )
    {
        if( zone->IsOnLayer( aLayer ) && zone->HitTest( aPosition, aAccuracy ) )
            return true;
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() != PCB_TEXT_T && item->IsOnLayer( aLayer )
            && item->HitTest( aPosition, aAccuracy ) )
        {
            return true;
        }
    }

    return false;
}


bool FOOTPRINT::HitTestOnLayer( const BOX2I& aRect, bool aContained, PCB_LAYER_ID aLayer, int aAccuracy ) const
{
    std::vector<BOARD_ITEM*> items;

    for( PAD* pad : m_pads )
    {
        if( pad->IsOnLayer( aLayer ) )
            items.push_back( pad );
    }

    for( ZONE* zone : m_zones )
    {
        if( zone->IsOnLayer( aLayer ) )
            items.push_back( zone );
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() != PCB_TEXT_T && item->IsOnLayer( aLayer ) )
            items.push_back( item );
    }

    // If we require the elements to be contained in the rect and any of them are not,
    // we can return false;
    // Conversely, if we just require any of the elements to have a hit, we can return true
    // when the first one is found.
    for( BOARD_ITEM* item : items )
    {
        if( !aContained && item->HitTest( aRect, aContained, aAccuracy ) )
            return true;
        else if( aContained && !item->HitTest( aRect, aContained, aAccuracy ) )
            return false;
    }

    // If we didn't exit in the loop, that means that we did not return false for aContained or
    // we did not return true for !aContained.  So we can just return the bool with a test of
    // whether there were any elements or not.
    return !items.empty() && aContained;
}


bool FOOTPRINT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox( false );
    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool FOOTPRINT::HitTestAccurate( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return GetBoundingHull().Collide( aPosition, aAccuracy );
}


bool FOOTPRINT::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
    {
        return arect.Contains( GetBoundingBox( false ) );
    }
    else
    {
        // If the rect does not intersect the bounding box, skip any tests
        if( !aRect.Intersects( GetBoundingBox( false ) ) )
            return false;

        // If there are no pads, zones, or drawings, allow intersection with text
        if( m_pads.empty() && m_zones.empty() && m_drawings.empty() )
            return GetBoundingBox( true ).Intersects( arect );

        // Determine if any elements in the FOOTPRINT intersect the rect
        for( PAD* pad : m_pads )
        {
            if( pad->HitTest( arect, false, 0 ) )
                return true;
        }

        for( ZONE* zone : m_zones )
        {
            if( zone->HitTest( arect, false, 0 ) )
                return true;
        }

        // PCB fields are selectable on their own, so they don't get tested

        for( BOARD_ITEM* item : m_drawings )
        {
            // Text items are selectable on their own, and are therefore excluded from this
            // test.  TextBox items are NOT selectable on their own, and so MUST be included
            // here. Bitmaps aren't selectable since they aren't displayed.
            if( item->Type() != PCB_TEXT_T && item->HitTest( arect, false, 0 ) )
                return true;
        }

        // Groups are not hit-tested; only their members

        // No items were hit
        return false;
    }
}


PAD* FOOTPRINT::FindPadByNumber( const wxString& aPadNumber, PAD* aSearchAfterMe ) const
{
    bool can_select = aSearchAfterMe ? false : true;

    for( PAD* pad : m_pads )
    {
        if( !can_select && pad == aSearchAfterMe )
        {
            can_select = true;
            continue;
        }

        if( can_select && pad->GetNumber() == aPadNumber )
            return pad;
    }

    return nullptr;
}


PAD* FOOTPRINT::GetPad( const VECTOR2I& aPosition, LSET aLayerMask )
{
    for( PAD* pad : m_pads )
    {
        // ... and on the correct layer.
        if( !( pad->GetLayerSet() & aLayerMask ).any() )
            continue;

        if( pad->HitTest( aPosition ) )
            return pad;
    }

    return nullptr;
}


std::vector<const PAD*> FOOTPRINT::GetPads( const wxString& aPadNumber, const PAD* aIgnore ) const
{
    std::vector<const PAD*> retv;

    for( const PAD* pad : m_pads )
    {
        if( ( aIgnore && aIgnore == pad ) || ( pad->GetNumber() != aPadNumber ) )
            continue;

        retv.push_back( pad );
    }

    return retv;
}


unsigned FOOTPRINT::GetPadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    if( aIncludeNPTH )
        return m_pads.size();

    unsigned cnt = 0;

    for( PAD* pad : m_pads )
    {
        if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
            continue;

        cnt++;
    }

    return cnt;
}


std::set<wxString> FOOTPRINT::GetUniquePadNumbers( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    std::set<wxString> usedNumbers;

    // Create a set of used pad numbers
    for( PAD* pad : m_pads )
    {
        // Skip pads not on copper layers (used to build complex
        // solder paste shapes for instance)
        if( ( pad->GetLayerSet() & LSET::AllCuMask() ).none() )
            continue;

        // Skip pads with no name, because they are usually "mechanical"
        // pads, not "electrical" pads
        if( pad->GetNumber().IsEmpty() )
            continue;

        if( !aIncludeNPTH )
        {
            // skip NPTH
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                continue;
        }

        usedNumbers.insert( pad->GetNumber() );
    }

    return usedNumbers;
}


unsigned FOOTPRINT::GetUniquePadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
{
    return GetUniquePadNumbers( aIncludeNPTH ).size();
}


void FOOTPRINT::Add3DModel( FP_3DMODEL* a3DModel )
{
    if( nullptr == a3DModel )
        return;

    if( !a3DModel->m_Filename.empty() )
        m_3D_Drawings.push_back( *a3DModel );
}


// see footprint.h
INSPECT_RESULT FOOTPRINT::Visit( INSPECTOR inspector, void* testData,
                                 const std::vector<KICAD_T>& aScanTypes )
{
#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    bool drawingsScanned = false;

    for( KICAD_T scanType : aScanTypes )
    {
        switch( scanType )
        {
        case PCB_FOOTPRINT_T:
            if( inspector( this, testData ) == INSPECT_RESULT::QUIT )
                return INSPECT_RESULT::QUIT;

            break;

        case PCB_PAD_T:
            if( IterateForward<PAD*>( m_pads, inspector, testData, { scanType } )
                    == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }

            break;

        case PCB_ZONE_T:
            if( IterateForward<ZONE*>( m_zones, inspector, testData, { scanType } )
                    == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }

            break;

        case PCB_FIELD_T:
            if( IterateForward<PCB_FIELD*>( m_fields, inspector, testData, { scanType } )
                == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }

            break;

        case PCB_TEXT_T:
        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_SHAPE_T:
        case PCB_TEXTBOX_T:
        case PCB_TABLE_T:
        case PCB_TABLECELL_T:
            if( !drawingsScanned )
            {
                if( IterateForward<BOARD_ITEM*>( m_drawings, inspector, testData, aScanTypes )
                        == INSPECT_RESULT::QUIT )
                {
                    return INSPECT_RESULT::QUIT;
                }

                drawingsScanned = true;
            }

            break;

        case PCB_GROUP_T:
            if( IterateForward<PCB_GROUP*>( m_groups, inspector, testData, { scanType } )
                    == INSPECT_RESULT::QUIT )
            {
                return INSPECT_RESULT::QUIT;
            }

            break;

        default:
            break;
        }
    }

    return INSPECT_RESULT::CONTINUE;
}


wxString FOOTPRINT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    wxString reference = GetReference();

    if( reference.IsEmpty() )
        reference = _( "<no reference designator>" );

    return wxString::Format( _( "Footprint %s" ), reference );
}


BITMAPS FOOTPRINT::GetMenuImage() const
{
    return BITMAPS::module;
}


EDA_ITEM* FOOTPRINT::Clone() const
{
    return new FOOTPRINT( *this );
}


void FOOTPRINT::RunOnChildren( const std::function<void ( BOARD_ITEM* )>& aFunction ) const
{
    try
    {
        for( PCB_FIELD* field : m_fields )
        {
            if( field )
                aFunction( field );
        }

        for( PAD* pad : m_pads )
            aFunction( pad );

        for( ZONE* zone : m_zones )
            aFunction( zone );

        for( PCB_GROUP* group : m_groups )
            aFunction( group );

        for( BOARD_ITEM* drawing : m_drawings )
            aFunction( drawing );
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error running FOOTPRINT::RunOnChildren" ) );
    }
}


void FOOTPRINT::RunOnDescendants( const std::function<void( BOARD_ITEM* )>& aFunction,
                                  int aDepth ) const
{
    // Avoid freezes with infinite recursion
    if( aDepth > 20 )
        return;

    try
    {
        for( PCB_FIELD* field : m_fields )
        {
            if( field )
                aFunction( field );
        }

        for( PAD* pad : m_pads )
            aFunction( pad );

        for( ZONE* zone : m_zones )
            aFunction( zone  );

        for( PCB_GROUP* group : m_groups )
        {
            aFunction( group );
            group->RunOnDescendants( aFunction, aDepth + 1 );
        }

        for( BOARD_ITEM* drawing : m_drawings )
        {
            aFunction( drawing );
            drawing->RunOnDescendants( aFunction, aDepth + 1 );
        }
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error running FOOTPRINT::RunOnDescendants" ) );
    }
}


std::vector<int> FOOTPRINT::ViewGetLayers() const
{
    std::vector<int> layers;

    layers.reserve( 6 );
    layers.push_back( LAYER_ANCHOR );

    switch( m_layer )
    {
    default:
        wxASSERT_MSG( false, wxT( "Illegal layer" ) );   // do you really have footprints placed
                                                         // on other layers?
        KI_FALLTHROUGH;

    case F_Cu:
        layers.push_back( LAYER_FOOTPRINTS_FR );
        break;

    case B_Cu:
        layers.push_back( LAYER_FOOTPRINTS_BK );
        break;
    }

    if( IsLocked() )
        layers.push_back( LAYER_LOCKED_ITEM_SHADOW );

    if( IsConflicting() )
        layers.push_back( LAYER_CONFLICTS_SHADOW );

    // If there are no pads, and only drawings on a silkscreen layer, then report the silkscreen
    // layer as well so that the component can be edited with the silkscreen layer
    bool f_silk = false, b_silk = false, non_silk = false;

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->GetLayer() == F_SilkS )
            f_silk = true;
        else if( item->GetLayer() == B_SilkS )
            b_silk = true;
        else
            non_silk = true;
    }

    if( ( f_silk || b_silk ) && !non_silk && m_pads.empty() )
    {
        if( f_silk )
            layers.push_back( F_SilkS );

        if( b_silk )
            layers.push_back( B_SilkS );
    }

    return layers;
}


double FOOTPRINT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // The locked shadow shape is shown only if the footprint itself is visible
        if( ( m_layer == F_Cu ) && aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
            return 0.0;

        if( ( m_layer == B_Cu ) && aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
            return 0.0;

        return std::numeric_limits<double>::max();
    }

    if( aLayer == LAYER_CONFLICTS_SHADOW && IsConflicting() )
    {
        // The locked shadow shape is shown only if the footprint itself is visible
        if( ( m_layer == F_Cu ) && aView->IsLayerVisible( LAYER_FOOTPRINTS_FR ) )
            return 0.0;

        if( ( m_layer == B_Cu ) && aView->IsLayerVisible( LAYER_FOOTPRINTS_BK ) )
            return 0.0;

        return std::numeric_limits<double>::max();
    }

    int layer = ( m_layer == F_Cu ) ? LAYER_FOOTPRINTS_FR :
                ( m_layer == B_Cu ) ? LAYER_FOOTPRINTS_BK : LAYER_ANCHOR;

    // Currently this is only pertinent for the anchor layer; everything else is drawn from the
    // children.
    // The "good" value is experimentally chosen.
    #define MINIMAL_ZOOM_LEVEL_FOR_VISIBILITY 1.5

    if( aView->IsLayerVisible( layer ) )
        return MINIMAL_ZOOM_LEVEL_FOR_VISIBILITY;

    return std::numeric_limits<double>::max();
}


const BOX2I FOOTPRINT::ViewBBox() const
{
    BOX2I area = GetBoundingBox( true );

    // Inflate in case clearance lines are drawn around pads, etc.
    if( const BOARD* board = GetBoard() )
    {
        int biggest_clearance = board->GetMaxClearanceValue();
        area.Inflate( biggest_clearance );
    }

    return area;
}


bool FOOTPRINT::IsLibNameValid( const wxString & aName )
{
    const wxChar * invalids = StringLibNameInvalidChars( false );

    if( aName.find_first_of( invalids ) != std::string::npos )
        return false;

    return true;
}


const wxChar* FOOTPRINT::StringLibNameInvalidChars( bool aUserReadable )
{
    // This list of characters is also duplicated in validators.cpp and
    // lib_id.cpp
    // TODO: Unify forbidden character lists - Warning, invalid filename characters are not the same
    // as invalid LIB_ID characters.  We will need to separate the FP filenames from FP names before this
    // can be unified
    static const wxChar invalidChars[] = wxT("%$<>\t\n\r\"\\/:");
    static const wxChar invalidCharsReadable[] = wxT("% $ < > 'tab' 'return' 'line feed' \\ \" / :");

    if( aUserReadable )
        return invalidCharsReadable;
    else
        return invalidChars;
}


void FOOTPRINT::Move( const VECTOR2I& aMoveVector )
{
    if( aMoveVector.x == 0 && aMoveVector.y == 0 )
        return;

    VECTOR2I newpos = m_pos + aMoveVector;
    SetPosition( newpos );
}


void FOOTPRINT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    if( aAngle == ANGLE_0 )
        return;

    EDA_ANGLE orientation = GetOrientation();
    EDA_ANGLE newOrientation = orientation + aAngle;
    VECTOR2I  newpos = m_pos;
    RotatePoint( newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( newOrientation );

    for( PCB_FIELD* field : m_fields )
    {
        if( field )
            field->KeepUpright();
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() == PCB_TEXT_T )
            static_cast<PCB_TEXT*>( item )->KeepUpright();
    }
}


void FOOTPRINT::SetLayerAndFlip( PCB_LAYER_ID aLayer )
{
    wxASSERT( aLayer == F_Cu || aLayer == B_Cu );

    if( aLayer != GetLayer() )
        Flip( GetPosition(), FLIP_DIRECTION::LEFT_RIGHT );
}


void FOOTPRINT::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    // Move footprint to its final position:
    VECTOR2I finalPos = m_pos;

    // Now Flip the footprint.
    // Flipping a footprint is a specific transform: it is not mirrored like a text.
    // We have to change the side, and ensure the footprint rotation is modified according to the
    // transform, because this parameter is used in pick and place files, and when updating the
    // footprint from library.
    // When flipped around the X axis (Y coordinates changed) orientation is negated
    // When flipped around the Y axis (X coordinates changed) orientation is 180 - old orient.
    // Because it is specific to a footprint, we flip around the X axis, and after rotate 180 deg

    MIRROR( finalPos.y, aCentre.y );     /// Mirror the Y position (around the X axis)

    SetPosition( finalPos );

    // Flip layer
    BOARD_ITEM::SetLayer( GetBoard()->FlipLayer( GetLayer() ) );

    // Calculate the new orientation, and then clear it for pad flipping.
    EDA_ANGLE newOrientation = -m_orient;
    newOrientation.Normalize180();
    m_orient = ANGLE_0;

    // Mirror fields to other side of board.
    for( PCB_FIELD* field : m_fields )
    {
        if( field )
            field->Flip( m_pos, FLIP_DIRECTION::TOP_BOTTOM );
    }

    // Mirror pads to other side of board.
    for( PAD* pad : m_pads )
        pad->Flip( m_pos, FLIP_DIRECTION::TOP_BOTTOM );

    // Now set the new orientation.
    m_orient = newOrientation;

    // Mirror zones to other side of board.
    for( ZONE* zone : m_zones )
        zone->Flip( m_pos, FLIP_DIRECTION::TOP_BOTTOM );

    // Reverse mirror footprint graphics and texts.
    for( BOARD_ITEM* item : m_drawings )
        item->Flip( m_pos, FLIP_DIRECTION::TOP_BOTTOM );

    // Now rotate 180 deg if required
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        Rotate( aCentre, ANGLE_180 );

    m_boundingBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;

    m_cachedHull.Mirror( m_pos, aFlipDirection );

    // Re-use cached geometry, just update the hashes. See SetPosition().
    std::swap( m_courtyard_cache_front, m_courtyard_cache_back );
    std::swap( m_courtyard_cache_back_hash, m_courtyard_cache_front_hash );
}


void FOOTPRINT::SetPosition( const VECTOR2I& aPos )
{
    VECTOR2I delta = aPos - m_pos;

    m_pos += delta;

    for( PCB_FIELD* field : m_fields )
    {
        if( field )
            field->EDA_TEXT::Offset( delta );
    }

    for( PAD* pad : m_pads )
        pad->SetPosition( pad->GetPosition() + delta );

    for( ZONE* zone : m_zones )
        zone->Move( delta );

    for( BOARD_ITEM* item : m_drawings )
        item->Move( delta );

    m_cachedBoundingBox.Move( delta );
    m_cachedTextExcludedBBox.Move( delta );
    m_cachedHull.Move( delta );

    // The geometry work has been conserved by using Move(). But the hashes
    // need to be updated, otherwise the cached polygons will still be rebuild.
    m_courtyard_cache_back.Move( delta );
    m_courtyard_cache_back_hash = m_courtyard_cache_back.GetHash();
    m_courtyard_cache_front.Move( delta );
    m_courtyard_cache_front_hash = m_courtyard_cache_front.GetHash();
}


void FOOTPRINT::MoveAnchorPosition( const VECTOR2I& aMoveVector )
{
    /*
     * Move the reference point of the footprint
     * the footprints elements (pads, outlines, edges .. ) are moved
     * but:
     * - the footprint position is not modified.
     * - the relative (local) coordinates of these items are modified
     * - Draw coordinates are updated
     */

    // Update (move) the relative coordinates relative to the new anchor point.
    VECTOR2I moveVector = aMoveVector;
    RotatePoint( moveVector, -GetOrientation() );

    // Update field local coordinates
    for( PCB_FIELD* field : m_fields )
    {
        if( field )
            field->Move( moveVector );
    }

    // Update the pad local coordinates.
    for( PAD* pad : m_pads )
        pad->Move( moveVector );

    // Update the draw element coordinates.
    for( BOARD_ITEM* item : GraphicalItems() )
        item->Move( moveVector );

    // Update the keepout zones
    for( ZONE* zone : Zones() )
        zone->Move( moveVector );

    // Update the 3D models
    for( FP_3DMODEL& model : Models() )
    {
        model.m_Offset.x += pcbIUScale.IUTomm( moveVector.x );
        model.m_Offset.y -= pcbIUScale.IUTomm( moveVector.y );
    }

    m_cachedBoundingBox.Move( moveVector );
    m_cachedTextExcludedBBox.Move( moveVector );
    m_cachedHull.Move( moveVector );

    // The geometry work have been conserved by using Move(). But the hashes
    // need to be updated, otherwise the cached polygons will still be rebuild.
    m_courtyard_cache_back.Move( moveVector );
    m_courtyard_cache_back_hash = m_courtyard_cache_back.GetHash();
    m_courtyard_cache_front.Move( moveVector );
    m_courtyard_cache_front_hash = m_courtyard_cache_front.GetHash();
}


void FOOTPRINT::SetOrientation( const EDA_ANGLE& aNewAngle )
{
    EDA_ANGLE angleChange = aNewAngle - m_orient;  // change in rotation

    m_orient = aNewAngle;
    m_orient.Normalize180();

    for( PCB_FIELD* field : m_fields )
    {
        if( field )
            field->Rotate( GetPosition(), angleChange );
    }

    for( PAD* pad : m_pads )
        pad->Rotate( GetPosition(), angleChange );

    for( ZONE* zone : m_zones )
        zone->Rotate( GetPosition(), angleChange );

    for( BOARD_ITEM* item : m_drawings )
        item->Rotate( GetPosition(), angleChange );

    m_boundingBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;
    m_hullCacheTimeStamp = 0;
    m_courtyard_cache_front_hash.Clear();
    m_courtyard_cache_back_hash.Clear();
}


BOARD_ITEM* FOOTPRINT::Duplicate() const
{
    FOOTPRINT* dupe = static_cast<FOOTPRINT*>( BOARD_ITEM::Duplicate() );

    dupe->RunOnDescendants( [&]( BOARD_ITEM* child )
                            {
                                const_cast<KIID&>( child->m_Uuid ) = KIID();
                            });

    return dupe;
}


BOARD_ITEM* FOOTPRINT::DuplicateItem( const BOARD_ITEM* aItem, bool aAddToFootprint )
{
    BOARD_ITEM* new_item = nullptr;

    switch( aItem->Type() )
    {
    case PCB_PAD_T:
    {
        PAD* new_pad = new PAD( *static_cast<const PAD*>( aItem ) );
        const_cast<KIID&>( new_pad->m_Uuid ) = KIID();

        if( aAddToFootprint )
            m_pads.push_back( new_pad );

        new_item = new_pad;
        break;
    }

    case PCB_ZONE_T:
    {
        ZONE* new_zone = new ZONE( *static_cast<const ZONE*>( aItem ) );
        const_cast<KIID&>( new_zone->m_Uuid ) = KIID();

        if( aAddToFootprint )
            m_zones.push_back( new_zone );

        new_item = new_zone;
        break;
    }

    case PCB_FIELD_T:
    case PCB_TEXT_T:
    {
        PCB_TEXT* new_text = new PCB_TEXT( *static_cast<const PCB_TEXT*>( aItem ) );
        const_cast<KIID&>( new_text->m_Uuid ) = KIID();

        if( aItem->Type() == PCB_FIELD_T )
        {
            switch( static_cast<const PCB_FIELD*>( aItem )->GetId() )
            {
            case REFERENCE_FIELD: new_text->SetText( wxT( "${REFERENCE}" ) ); break;

            case VALUE_FIELD: new_text->SetText( wxT( "${VALUE}" ) ); break;

            case DATASHEET_FIELD: new_text->SetText( wxT( "${DATASHEET}" ) ); break;
            }
        }

        if( aAddToFootprint )
            Add( new_text );

        new_item = new_text;
        break;
    }

    case PCB_SHAPE_T:
    {
        PCB_SHAPE* new_shape = new PCB_SHAPE( *static_cast<const PCB_SHAPE*>( aItem ) );
        const_cast<KIID&>( new_shape->m_Uuid ) = KIID();

        if( aAddToFootprint )
            Add( new_shape );

        new_item = new_shape;
        break;
    }

    case PCB_TEXTBOX_T:
    {
        PCB_TEXTBOX* new_textbox = new PCB_TEXTBOX( *static_cast<const PCB_TEXTBOX*>( aItem ) );
        const_cast<KIID&>( new_textbox->m_Uuid ) = KIID();

        if( aAddToFootprint )
            Add( new_textbox );

        new_item = new_textbox;
        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    {
        PCB_DIMENSION_BASE* dimension = static_cast<PCB_DIMENSION_BASE*>( aItem->Duplicate() );

        if( aAddToFootprint )
            Add( dimension );

        new_item = dimension;
        break;
    }

    case PCB_GROUP_T:
    {
        PCB_GROUP* group = static_cast<const PCB_GROUP*>( aItem )->DeepDuplicate();

        if( aAddToFootprint )
        {
            group->RunOnDescendants(
                    [&]( BOARD_ITEM* aCurrItem )
                    {
                        Add( aCurrItem );
                    } );

            Add( new_item );
        }

        new_item = group;
        break;
    }

    case PCB_FOOTPRINT_T:
        // Ignore the footprint itself
        break;

    default:
        // Un-handled item for duplication
        wxFAIL_MSG( wxT( "Duplication not supported for items of class " ) + aItem->GetClass() );
        break;
    }

    return new_item;
}


wxString FOOTPRINT::GetNextPadNumber( const wxString& aLastPadNumber ) const
{
    std::set<wxString> usedNumbers;

    // Create a set of used pad numbers
    for( PAD* pad : m_pads )
        usedNumbers.insert( pad->GetNumber() );

    // Pad numbers aren't technically reference designators, but the formatting is close enough
    // for these to give us what we need.
    wxString prefix = UTIL::GetRefDesPrefix( aLastPadNumber );
    int      num = GetTrailingInt( aLastPadNumber );

    while( usedNumbers.count( wxString::Format( wxT( "%s%d" ), prefix, num ) ) )
        num++;

    return wxString::Format( wxT( "%s%d" ), prefix, num );
}


void FOOTPRINT::AutoPositionFields()
{
    // Auto-position reference and value
    BOX2I bbox = GetBoundingBox( false );
    bbox.Inflate( pcbIUScale.mmToIU( 0.2 ) ); // Gap between graphics and text

    if( Reference().GetPosition() == VECTOR2I( 0, 0 ) )
    {
        Reference().SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        Reference().SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        Reference().SetTextAngle( ANGLE_0 );

        Reference().SetX( bbox.GetCenter().x );
        Reference().SetY( bbox.GetTop() - Reference().GetTextSize().y / 2 );
    }

    if( Value().GetPosition() == VECTOR2I( 0, 0 ) )
    {
        Value().SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        Value().SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        Value().SetTextAngle( ANGLE_0 );

        Value().SetX( bbox.GetCenter().x );
        Value().SetY( bbox.GetBottom() + Value().GetTextSize().y / 2 );
    }
}


void FOOTPRINT::IncrementReference( int aDelta )
{
    const wxString& refdes = GetReference();

    SetReference( wxString::Format( wxT( "%s%i" ),
                                    UTIL::GetRefDesPrefix( refdes ),
                                    GetTrailingInt( refdes ) + aDelta ) );
}


// Calculate the area of a PolySet, polygons with hole are allowed.
static double polygonArea( SHAPE_POLY_SET& aPolySet )
{
    // Ensure all outlines are closed, before calculating the SHAPE_POLY_SET area
    for( int ii = 0; ii < aPolySet.OutlineCount(); ii++ )
    {
        SHAPE_LINE_CHAIN& outline = aPolySet.Outline( ii );
        outline.SetClosed( true );

        for( int jj = 0; jj < aPolySet.HoleCount( ii ); jj++ )
            aPolySet.Hole( ii, jj ).SetClosed( true );
    }

    return aPolySet.Area();
}


double FOOTPRINT::GetCoverageArea( const BOARD_ITEM* aItem, const GENERAL_COLLECTOR& aCollector  )
{
    int            textMargin = aCollector.GetGuide()->Accuracy();
    SHAPE_POLY_SET poly;

    if( aItem->Type() == PCB_MARKER_T )
    {
        const PCB_MARKER* marker = static_cast<const PCB_MARKER*>( aItem );
        SHAPE_LINE_CHAIN  markerShape;

        marker->ShapeToPolygon( markerShape );
        return markerShape.Area();
    }
    else if( aItem->Type() == PCB_GROUP_T || aItem->Type() == PCB_GENERATOR_T )
    {
        double combinedArea = 0.0;

        for( BOARD_ITEM* member : static_cast<const PCB_GROUP*>( aItem )->GetItems() )
            combinedArea += GetCoverageArea( member, aCollector );

        return combinedArea;
    }
    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( aItem );

        poly = footprint->GetBoundingHull();
    }
    else if( aItem->Type() == PCB_FIELD_T || aItem->Type() == PCB_TEXT_T )
    {
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( aItem );

        text->TransformTextToPolySet( poly, textMargin, ARC_LOW_DEF, ERROR_INSIDE );
    }
    else if( aItem->Type() == PCB_TEXTBOX_T )
    {
        const PCB_TEXTBOX* tb = static_cast<const PCB_TEXTBOX*>( aItem );

        tb->TransformTextToPolySet( poly, textMargin, ARC_LOW_DEF, ERROR_INSIDE );
    }
    else if( aItem->Type() == PCB_SHAPE_T )
    {
        // Approximate "linear" shapes with just their width squared, as we don't want to consider
        // a linear shape as being much bigger than another for purposes of selection filtering
        // just because it happens to be really long.

        const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );

        switch( shape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
        case SHAPE_T::ARC:
        case SHAPE_T::BEZIER:
            return shape->GetWidth() * shape->GetWidth();

        case SHAPE_T::RECTANGLE:
        case SHAPE_T::CIRCLE:
        case SHAPE_T::POLY:
        {
            if( !shape->IsFilled() )
                return shape->GetWidth() * shape->GetWidth();

            KI_FALLTHROUGH;
        }

        default:
            shape->TransformShapeToPolygon( poly, UNDEFINED_LAYER, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
        }
    }
    else if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_ARC_T )
    {
        double width = static_cast<const PCB_TRACK*>( aItem )->GetWidth();
        return width * width;
    }
    else if( aItem->Type() == PCB_PAD_T )
    {
        static_cast<const PAD*>( aItem )->Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                SHAPE_POLY_SET padPoly;
                aItem->TransformShapeToPolygon( padPoly, aLayer, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
                poly.BooleanAdd( padPoly );
            } );
    }
    else
    {
        aItem->TransformShapeToPolygon( poly, UNDEFINED_LAYER, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
    }

    return polygonArea( poly );
}


double FOOTPRINT::CoverageRatio( const GENERAL_COLLECTOR& aCollector ) const
{
    int textMargin = aCollector.GetGuide()->Accuracy();

    SHAPE_POLY_SET footprintRegion( GetBoundingHull() );
    SHAPE_POLY_SET coveredRegion;

    TransformPadsToPolySet( coveredRegion, UNDEFINED_LAYER, 0, ARC_LOW_DEF, ERROR_OUTSIDE );

    TransformFPShapesToPolySet( coveredRegion, UNDEFINED_LAYER, textMargin, ARC_LOW_DEF,
                                ERROR_OUTSIDE,
                                true,  /* include text */
                                false, /* include shapes */
                                false  /* include private items */ );

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        const BOARD_ITEM* item = aCollector[i];

        switch( item->Type() )
        {
        case PCB_FIELD_T:
        case PCB_TEXT_T:
        case PCB_TEXTBOX_T:
        case PCB_SHAPE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            if( item->GetParent() != this )
            {
                item->TransformShapeToPolygon( coveredRegion, UNDEFINED_LAYER, 0, ARC_LOW_DEF,
                                               ERROR_OUTSIDE );
            }
            break;

        case PCB_FOOTPRINT_T:
            if( item != this )
            {
                const FOOTPRINT* footprint = static_cast<const FOOTPRINT*>( item );
                coveredRegion.AddOutline( footprint->GetBoundingHull().Outline( 0 ) );
            }
            break;

        default:
            break;
        }
    }

    coveredRegion.BooleanIntersection( footprintRegion );

    double footprintRegionArea = polygonArea( footprintRegion );
    double uncoveredRegionArea = footprintRegionArea - polygonArea( coveredRegion );
    double coveredArea = footprintRegionArea - uncoveredRegionArea;
    double ratio = ( coveredArea / footprintRegionArea );

    // Test for negative ratio (should not occur).
    // better to be conservative (this will result in the disambiguate dialog)
    if( ratio < 0.0 )
        return 1.0;

    return std::min( ratio, 1.0 );
}


std::shared_ptr<SHAPE> FOOTPRINT::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();

    // There are several possible interpretations here:
    // 1) the bounding box (without or without invisible items)
    // 2) just the pads and "edges" (ie: non-text graphic items)
    // 3) the courtyard

    // We'll go with (2) for now, unless the caller is clearly looking for (3)

    if( aLayer == F_CrtYd || aLayer == B_CrtYd )
    {
        const SHAPE_POLY_SET& courtyard = GetCourtyard( aLayer );

        if( courtyard.OutlineCount() == 0 )    // malformed/empty polygon
            return shape;

        shape->AddShape( new SHAPE_SIMPLE( courtyard.COutline( 0 ) ) );
    }
    else
    {
        for( PAD* pad : Pads() )
            shape->AddShape( pad->GetEffectiveShape( aLayer, aFlash )->Clone() );

        for( BOARD_ITEM* item : GraphicalItems() )
        {
            if( item->Type() == PCB_SHAPE_T )
                shape->AddShape( item->GetEffectiveShape( aLayer, aFlash )->Clone() );
        }
    }

    return shape;
}


const SHAPE_POLY_SET& FOOTPRINT::GetCourtyard( PCB_LAYER_ID aLayer ) const
{
    std::lock_guard<std::mutex> lock( m_courtyard_cache_mutex );

    if( m_courtyard_cache_front_hash != m_courtyard_cache_front.GetHash()
        || m_courtyard_cache_back_hash != m_courtyard_cache_back.GetHash() )
    {
        const_cast<FOOTPRINT*>(this)->BuildCourtyardCaches();
    }

    return GetCachedCourtyard( aLayer );
}


const SHAPE_POLY_SET& FOOTPRINT::GetCachedCourtyard( PCB_LAYER_ID aLayer ) const
{
    if( IsBackLayer( aLayer ) )
        return m_courtyard_cache_back;
    else
        return m_courtyard_cache_front;
}


void FOOTPRINT::BuildCourtyardCaches( OUTLINE_ERROR_HANDLER* aErrorHandler )
{
    m_courtyard_cache_front.RemoveAllContours();
    m_courtyard_cache_back.RemoveAllContours();
    ClearFlags( MALFORMED_COURTYARDS );

    // Build the courtyard area from graphic items on the courtyard.
    // Only PCB_SHAPE_T have meaning, graphic texts are ignored.
    // Collect items:
    std::vector<PCB_SHAPE*> list_front;
    std::vector<PCB_SHAPE*> list_back;
    std::map<int, int>      front_width_histogram;
    std::map<int, int>      back_width_histogram;

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( item->GetLayer() == B_CrtYd && item->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            list_back.push_back( shape );
            back_width_histogram[ shape->GetStroke().GetWidth() ]++;
        }

        if( item->GetLayer() == F_CrtYd && item->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );
            list_front.push_back( shape );
            front_width_histogram[ shape->GetStroke().GetWidth() ]++;
        }
    }

    if( !list_front.size() && !list_back.size() )
        return;

    int maxError = pcbIUScale.mmToIU( 0.005 );        // max error for polygonization
    int chainingEpsilon = pcbIUScale.mmToIU( 0.02 );  // max dist from one endPt to next startPt

    if( ConvertOutlineToPolygon( list_front, m_courtyard_cache_front, maxError, chainingEpsilon,
                                 true, aErrorHandler ) )
    {
        int width = 0;

        // Touching courtyards, or courtyards -at- the clearance distance are legal.
        // Use maxError here because that is the allowed deviation when transforming arcs/circles to
        // polygons.
        m_courtyard_cache_front.Inflate( -maxError, CORNER_STRATEGY::CHAMFER_ACUTE_CORNERS, maxError );

        m_courtyard_cache_front.CacheTriangulation( false );
        auto max = std::max_element( front_width_histogram.begin(), front_width_histogram.end(),
                                     []( const std::pair<int, int>& a, const std::pair<int, int>& b )
                                     {
                                         return a.second < b.second;
                                     } );

        if( max != front_width_histogram.end() )
            width = max->first;

        if( width == 0 )
            width = pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH );

        if( m_courtyard_cache_front.OutlineCount() > 0 )
            m_courtyard_cache_front.Outline( 0 ).SetWidth( width );
    }
    else
    {
        SetFlags( MALFORMED_F_COURTYARD );
    }

    if( ConvertOutlineToPolygon( list_back, m_courtyard_cache_back, maxError, chainingEpsilon, true,
                                 aErrorHandler ) )
    {
        int width = 0;

        // Touching courtyards, or courtyards -at- the clearance distance are legal.
        m_courtyard_cache_back.Inflate( -maxError, CORNER_STRATEGY::CHAMFER_ACUTE_CORNERS, maxError );

        m_courtyard_cache_back.CacheTriangulation( false );
        auto max = std::max_element( back_width_histogram.begin(), back_width_histogram.end(),
                                     []( const std::pair<int, int>& a, const std::pair<int, int>& b )
                                     {
                                         return a.second < b.second;
                                     } );

        if( max != back_width_histogram.end() )
            width = max->first;

        if( width == 0 )
            width = pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH );

        if( m_courtyard_cache_back.OutlineCount() > 0 )
            m_courtyard_cache_back.Outline( 0 ).SetWidth( width );
    }
    else
    {
        SetFlags( MALFORMED_B_COURTYARD );
    }

    m_courtyard_cache_front_hash = m_courtyard_cache_front.GetHash();
    m_courtyard_cache_back_hash = m_courtyard_cache_back.GetHash();
}


void FOOTPRINT::BuildNetTieCache()
{
    m_netTieCache.clear();
    std::map<wxString, int> map = MapPadNumbersToNetTieGroups();
    std::map<PCB_LAYER_ID, std::vector<PCB_SHAPE*>> layer_shapes;

    std::for_each( m_drawings.begin(), m_drawings.end(),
                   [&]( BOARD_ITEM* item )
                   {
                       if( item->Type() != PCB_SHAPE_T )
                           return;

                       for( PCB_LAYER_ID layer : item->GetLayerSet() )
                           layer_shapes[layer].push_back( static_cast<PCB_SHAPE*>( item ) );
                   } );

    for( size_t ii = 0; ii < m_pads.size(); ++ii )
    {
        PAD* pad = m_pads[ ii ];
        bool has_nettie = false;

        auto it = map.find( pad->GetNumber() );

        if( it == map.end() || it->second < 0 )
            continue;

        for( size_t jj = ii + 1; jj < m_pads.size(); ++jj )
        {
            PAD* other = m_pads[ jj ];

            auto it2 = map.find( other->GetNumber() );

            if( it2 == map.end() || it2->second < 0 )
                continue;

            if( it2->second == it->second )
            {
                m_netTieCache[pad].insert( pad->GetNetCode() );
                m_netTieCache[pad].insert( other->GetNetCode() );
                m_netTieCache[other].insert( other->GetNetCode() );
                m_netTieCache[other].insert( pad->GetNetCode() );
                has_nettie = true;
            }
        }

        if( !has_nettie )
            continue;

        for( auto& [ layer, shapes ] : layer_shapes )
        {
            auto pad_shape = pad->GetEffectiveShape( layer );

            for( auto other_shape : shapes )
            {
                auto shape = other_shape->GetEffectiveShape( layer );

                if( pad_shape->Collide( shape.get() ) )
                {
                    std::set<int>& nettie = m_netTieCache[pad];
                    m_netTieCache[other_shape].insert( nettie.begin(), nettie.end() );
                }
            }
        }
    }
}


std::map<wxString, int> FOOTPRINT::MapPadNumbersToNetTieGroups() const
{
    std::map<wxString, int> padNumberToGroupIdxMap;

    for( const PAD* pad : m_pads )
        padNumberToGroupIdxMap[ pad->GetNumber() ] = -1;

    auto processPad =
            [&]( wxString aPad, int aGroup )
            {
                aPad.Trim( true ).Trim( false );

                if( !aPad.IsEmpty() )
                    padNumberToGroupIdxMap[ aPad ] = aGroup;
            };

    for( int ii = 0; ii < (int) m_netTiePadGroups.size(); ++ii )
    {
        wxString group( m_netTiePadGroups[ ii ] );
        bool esc = false;
        wxString pad;

        for( wxUniCharRef ch : group )
        {
            if( esc )
            {
                esc = false;
                pad.Append( ch );
                continue;
            }

            switch( static_cast<unsigned char>( ch ) )
            {
            case '\\':
                esc = true;
                break;

            case ',':
                processPad( pad, ii );
                pad.Clear();
                break;

            default:
                pad.Append( ch );
                break;
            }
        }

        processPad( pad, ii );
    }

    return padNumberToGroupIdxMap;
}


std::vector<PAD*> FOOTPRINT::GetNetTiePads( PAD* aPad ) const
{
    // First build a map from pad numbers to allowed-shorting-group indexes.  This ends up being
    // something like O(3n), but it still beats O(n^2) for large numbers of pads.

    std::map<wxString, int> padToNetTieGroupMap = MapPadNumbersToNetTieGroups();
    int                     groupIdx = padToNetTieGroupMap[ aPad->GetNumber() ];
    std::vector<PAD*>       otherPads;

    if( groupIdx >= 0 )
    {
        for( PAD* pad : m_pads )
        {
            if( padToNetTieGroupMap[ pad->GetNumber() ] == groupIdx )
                otherPads.push_back( pad );
        }
    }

    return otherPads;
}


void FOOTPRINT::CheckFootprintAttributes( const std::function<void( const wxString& )>& aErrorHandler )
{
    int likelyAttr = ( GetLikelyAttribute() & ( FP_SMD | FP_THROUGH_HOLE ) );
    int setAttr = ( GetAttributes() & ( FP_SMD | FP_THROUGH_HOLE ) );

    if( setAttr && likelyAttr && setAttr != likelyAttr )
    {
        wxString msg;

        switch( likelyAttr )
        {
        case FP_THROUGH_HOLE:
            msg.Printf( _( "(expected 'Through hole'; actual '%s')" ), GetTypeName() );
            break;
        case FP_SMD:
            msg.Printf( _( "(expected 'SMD'; actual '%s')" ), GetTypeName() );
            break;
        }

        if( aErrorHandler )
            (aErrorHandler)( msg );
    }
}


void FOOTPRINT::CheckPads( UNITS_PROVIDER* aUnitsProvider,
                           const std::function<void( const PAD*, int,
                                                     const wxString& )>& aErrorHandler )
{
    if( aErrorHandler == nullptr )
        return;

    for( PAD* pad: Pads() )
    {
        pad->CheckPad( aUnitsProvider,
                [&]( int errorCode, const wxString& msg )
                {
                    aErrorHandler( pad, errorCode, msg );
                } );
    }
}


void FOOTPRINT::CheckShortingPads( const std::function<void( const PAD*, const PAD*,
                                                             int aErrorCode,
                                                             const VECTOR2I& )>& aErrorHandler )
{
    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    for( PAD* pad : Pads() )
    {
        std::vector<PAD*> netTiePads = GetNetTiePads( pad );

        for( PAD* other : Pads() )
        {
            if( other == pad )
                continue;

            // store canonical order so we don't collide in both directions (a:b and b:a)
            PAD* a = pad;
            PAD* b = other;

            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                std::swap( a, b );

            if( checkedPairs.find( { a, b } ) == checkedPairs.end() )
            {
                checkedPairs[ { a, b } ] = 1;

                if( pad->HasDrilledHole() && other->HasDrilledHole() )
                {
                    VECTOR2I pos = pad->GetPosition();

                    if( pad->GetPosition() == other->GetPosition() )
                    {
                        aErrorHandler( pad, other, DRCE_DRILLED_HOLES_COLOCATED, pos );
                    }
                    else
                    {
                        std::shared_ptr<SHAPE_SEGMENT> holeA = pad->GetEffectiveHoleShape();
                        std::shared_ptr<SHAPE_SEGMENT> holeB = other->GetEffectiveHoleShape();

                        if( holeA->Collide( holeB->GetSeg(), 0 ) )
                            aErrorHandler( pad, other, DRCE_DRILLED_HOLES_TOO_CLOSE, pos );
                    }
                }

                if( pad->SameLogicalPadAs( other ) || alg::contains( netTiePads, other ) )
                    continue;

                if( !( ( pad->GetLayerSet() & other->GetLayerSet() ) & LSET::AllCuMask() ).any() )
                    continue;

                if( pad->GetBoundingBox().Intersects( other->GetBoundingBox() ) )
                {
                    VECTOR2I pos;

                    for( PCB_LAYER_ID l : pad->Padstack().RelevantShapeLayers( other->Padstack() ) )
                    {
                        SHAPE*   padShape = pad->GetEffectiveShape( l ).get();
                        SHAPE*   otherShape = other->GetEffectiveShape( l ).get();

                        if( padShape->Collide( otherShape, 0, nullptr, &pos ) )
                            aErrorHandler( pad, other, DRCE_SHORTING_ITEMS, pos );
                    }
                }
            }
        }
    }
}


void FOOTPRINT::CheckNetTies( const std::function<void( const BOARD_ITEM* aItem,
                                                        const BOARD_ITEM* bItem,
                                                        const BOARD_ITEM* cItem,
                                                        const VECTOR2I& )>& aErrorHandler )
{
    // First build a map from pad numbers to allowed-shorting-group indexes.  This ends up being
    // something like O(3n), but it still beats O(n^2) for large numbers of pads.

    std::map<wxString, int> padNumberToGroupIdxMap = MapPadNumbersToNetTieGroups();

    // Now collect all the footprint items which are on copper layers

    std::vector<BOARD_ITEM*> copperItems;

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->IsOnCopperLayer() )
            copperItems.push_back( item );

        item->RunOnDescendants(
                [&]( BOARD_ITEM* descendent )
                {
                    if( descendent->IsOnCopperLayer() )
                        copperItems.push_back( descendent );
                } );
    }

    for( ZONE* zone : m_zones )
    {
        if( !zone->GetIsRuleArea() && zone->IsOnCopperLayer() )
            copperItems.push_back( zone );
    }

    for( PCB_FIELD* field : m_fields )
    {
        if( field && field->IsOnCopperLayer() )
            copperItems.push_back( field );
    }

    for( PCB_LAYER_ID layer : { F_Cu, In1_Cu, B_Cu } )
    {
        // Next, build a polygon-set for the copper on this layer.  We don't really care about
        // nets here, we just want to end up with a set of outlines describing the distinct
        // copper polygons of the footprint.

        SHAPE_POLY_SET                         copperOutlines;
        std::map<int, std::vector<const PAD*>> outlineIdxToPadsMap;

        for( BOARD_ITEM* item : copperItems )
        {
            if( item->IsOnLayer( layer ) )
            {
                item->TransformShapeToPolygon( copperOutlines, layer, 0, ARC_HIGH_DEF,
                                               ERROR_OUTSIDE );
            }
        }

        copperOutlines.Simplify();

        // Index each pad to the outline in the set that it is part of.

        for( const PAD* pad : m_pads )
        {
            for( int ii = 0; ii < copperOutlines.OutlineCount(); ++ii )
            {
                if( pad->GetEffectiveShape( layer )->Collide( &copperOutlines.Outline( ii ), 0 ) )
                    outlineIdxToPadsMap[ ii ].emplace_back( pad );
            }
        }

        // Finally, ensure that each outline which contains multiple pads has all its pads
        // listed in an allowed-shorting group.

        for( const auto& [ outlineIdx, pads ] : outlineIdxToPadsMap )
        {
            if( pads.size() > 1 )
            {
                const PAD* firstPad = pads[0];
                int        firstGroupIdx = padNumberToGroupIdxMap[ firstPad->GetNumber() ];

                for( size_t ii = 1; ii < pads.size(); ++ii )
                {
                    const PAD* thisPad = pads[ii];
                    int        thisGroupIdx = padNumberToGroupIdxMap[ thisPad->GetNumber() ];

                    if( thisGroupIdx < 0 || thisGroupIdx != firstGroupIdx )
                    {
                        BOARD_ITEM* shortingItem = nullptr;
                        VECTOR2I    pos = ( firstPad->GetPosition() + thisPad->GetPosition() ) / 2;

                        pos = copperOutlines.Outline( outlineIdx ).NearestPoint( pos );

                        for( BOARD_ITEM* item : copperItems )
                        {
                            if( item->HitTest( pos, 1 ) )
                            {
                                shortingItem = item;
                                break;
                            }
                        }

                        if( shortingItem )
                            aErrorHandler( shortingItem, firstPad, thisPad, pos );
                        else
                            aErrorHandler( firstPad, thisPad, nullptr, pos );
                    }
                }
            }
        }
    }
}


void FOOTPRINT::CheckNetTiePadGroups( const std::function<void( const wxString& )>& aErrorHandler )
{
    std::set<wxString> padNumbers;
    wxString           msg;

    for( const auto& [ padNumber, _ ] : MapPadNumbersToNetTieGroups() )
    {
        const PAD* pad = FindPadByNumber( padNumber );

        if( !pad )
        {
            msg.Printf( _( "(net-tie pad group contains unknown pad number %s)" ), padNumber );
            aErrorHandler( msg );
        }
        else if( !padNumbers.insert( pad->GetNumber() ).second )
        {
            msg.Printf( _( "(pad %s appears in more than one net-tie pad group)" ), padNumber );
            aErrorHandler( msg );
        }
    }
}


void FOOTPRINT::swapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == PCB_FOOTPRINT_T );

    FOOTPRINT* image = static_cast<FOOTPRINT*>( aImage );

    std::swap( *this, *image );

    RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                child->SetParent( this );
            } );

    image->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                child->SetParent( image );
            } );
}


bool FOOTPRINT::HasThroughHolePads() const
{
    for( PAD* pad : Pads() )
    {
        if( pad->GetAttribute() != PAD_ATTRIB::SMD )
            return true;
    }

    return false;
}


bool FOOTPRINT::operator==( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != PCB_FOOTPRINT_T )
        return false;

    const FOOTPRINT& other = static_cast<const FOOTPRINT&>( aOther );

    return *this == other;
}


bool FOOTPRINT::operator==( const FOOTPRINT& aOther ) const
{
    if( m_pads.size() != aOther.m_pads.size() )
        return false;

    for( size_t ii = 0; ii < m_pads.size(); ++ii )
    {
        if( !( *m_pads[ii] == *aOther.m_pads[ii] ) )
            return false;
    }

    if( m_drawings.size() != aOther.m_drawings.size() )
        return false;

    for( size_t ii = 0; ii < m_drawings.size(); ++ii )
    {
        if( !( *m_drawings[ii] == *aOther.m_drawings[ii] ) )
            return false;
    }

    if( m_zones.size() != aOther.m_zones.size() )
        return false;

    for( size_t ii = 0; ii < m_zones.size(); ++ii )
    {
        if( !( *m_zones[ii] == *aOther.m_zones[ii] ) )
            return false;
    }

    if( m_fields.size() != aOther.m_fields.size() )
        return false;

    for( size_t ii = 0; ii < m_fields.size(); ++ii )
    {
        if( m_fields[ii] )
        {
            if( !( *m_fields[ii] == *aOther.m_fields[ii] ) )
                return false;
        }
    }

    return true;
}


double FOOTPRINT::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != PCB_FOOTPRINT_T )
        return 0.0;

    const FOOTPRINT& other = static_cast<const FOOTPRINT&>( aOther );

    double similarity = 1.0;

    for( const PAD* pad : m_pads)
    {
        const PAD* otherPad = other.FindPadByNumber( pad->GetNumber() );

        if( !otherPad )
            continue;

        similarity *= pad->Similarity( *otherPad );
    }

    return similarity;
}


bool FOOTPRINT::cmp_drawings::operator()( const BOARD_ITEM* itemA, const BOARD_ITEM* itemB ) const
{
    if( itemA->Type() != itemB->Type() )
        return itemA->Type() < itemB->Type();

    if( itemA->GetLayer() != itemB->GetLayer() )
        return itemA->GetLayer() < itemB->GetLayer();

    if( itemA->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* dwgA = static_cast<const PCB_SHAPE*>( itemA );
        const PCB_SHAPE* dwgB = static_cast<const PCB_SHAPE*>( itemB );

        if( dwgA->GetShape() != dwgB->GetShape() )
            return dwgA->GetShape() < dwgB->GetShape();

        // GetStart() and GetEnd() have no meaning with polygons.
        // We cannot use them for sorting polygons
        if( dwgA->GetShape() != SHAPE_T::POLY )
        {
            if( dwgA->GetStart().x != dwgB->GetStart().x )
                return dwgA->GetStart().x < dwgB->GetStart().x;
            if( dwgA->GetStart().y != dwgB->GetStart().y )
                return dwgA->GetStart().y < dwgB->GetStart().y;

            if( dwgA->GetEnd().x != dwgB->GetEnd().x )
                return dwgA->GetEnd().x < dwgB->GetEnd().x;
            if( dwgA->GetEnd().y != dwgB->GetEnd().y )
                return dwgA->GetEnd().y < dwgB->GetEnd().y;
        }

        if( dwgA->GetShape() == SHAPE_T::ARC )
        {
            if( dwgA->GetCenter().x != dwgB->GetCenter().x )
                return dwgA->GetCenter().x < dwgB->GetCenter().x;
            if( dwgA->GetCenter().y != dwgB->GetCenter().y )
                return dwgA->GetCenter().y < dwgB->GetCenter().y;
        }
        else if( dwgA->GetShape() == SHAPE_T::BEZIER )
        {
            if( dwgA->GetBezierC1().x != dwgB->GetBezierC1().x )
                return dwgA->GetBezierC1().x < dwgB->GetBezierC1().x;
            if( dwgA->GetBezierC1().y != dwgB->GetBezierC1().y )
                return dwgA->GetBezierC1().y < dwgB->GetBezierC1().y;

            if( dwgA->GetBezierC2().x != dwgB->GetBezierC2().x )
                return dwgA->GetBezierC2().x < dwgB->GetBezierC2().x;
            if( dwgA->GetBezierC2().y != dwgB->GetBezierC2().y )
                return dwgA->GetBezierC2().y < dwgB->GetBezierC2().y;
        }
        else if( dwgA->GetShape() == SHAPE_T::POLY )
        {
            if( dwgA->GetPolyShape().TotalVertices() != dwgB->GetPolyShape().TotalVertices() )
                return dwgA->GetPolyShape().TotalVertices() < dwgB->GetPolyShape().TotalVertices();

            for( int ii = 0; ii < dwgA->GetPolyShape().TotalVertices(); ++ii )
            {
                if( dwgA->GetPolyShape().CVertex( ii ).x != dwgB->GetPolyShape().CVertex( ii ).x )
                    return dwgA->GetPolyShape().CVertex( ii ).x
                           < dwgB->GetPolyShape().CVertex( ii ).x;
                if( dwgA->GetPolyShape().CVertex( ii ).y != dwgB->GetPolyShape().CVertex( ii ).y )
                    return dwgA->GetPolyShape().CVertex( ii ).y
                           < dwgB->GetPolyShape().CVertex( ii ).y;
            }
        }

        if( dwgA->GetWidth() != dwgB->GetWidth() )
            return dwgA->GetWidth() < dwgB->GetWidth();
    }

    if( itemA->m_Uuid != itemB->m_Uuid )
        return itemA->m_Uuid < itemB->m_Uuid;

    return itemA < itemB;
}


bool FOOTPRINT::cmp_pads::operator()( const PAD* aFirst, const PAD* aSecond ) const
{
    if( aFirst->GetNumber() != aSecond->GetNumber() )
        return StrNumCmp( aFirst->GetNumber(), aSecond->GetNumber() ) < 0;

    if( aFirst->GetFPRelativePosition().x != aSecond->GetFPRelativePosition().x )
        return aFirst->GetFPRelativePosition().x < aSecond->GetFPRelativePosition().x;
    if( aFirst->GetFPRelativePosition().y != aSecond->GetFPRelativePosition().y )
        return aFirst->GetFPRelativePosition().y < aSecond->GetFPRelativePosition().y;

    std::optional<bool> padCopperMatches;

    // Pick the "most complex" padstack to iterate
    const PAD* checkPad = aFirst;

    if( aSecond->Padstack().Mode() == PADSTACK::MODE::CUSTOM
        || ( aSecond->Padstack().Mode() == PADSTACK::MODE::FRONT_INNER_BACK &&
             aFirst->Padstack().Mode() == PADSTACK::MODE::NORMAL ) )
    {
        checkPad = aSecond;
    }

    checkPad->Padstack().ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            if( aFirst->GetSize( aLayer ).x != aSecond->GetSize( aLayer ).x )
                padCopperMatches = aFirst->GetSize( aLayer ).x < aSecond->GetSize( aLayer ).x;
            else if( aFirst->GetSize( aLayer ).y != aSecond->GetSize( aLayer ).y )
                padCopperMatches = aFirst->GetSize( aLayer ).y < aSecond->GetSize( aLayer ).y;
            else if( aFirst->GetShape( aLayer ) != aSecond->GetShape( aLayer ) )
                padCopperMatches = aFirst->GetShape( aLayer ) < aSecond->GetShape( aLayer );
        } );

    if( padCopperMatches.has_value() )
        return *padCopperMatches;

    if( aFirst->GetLayerSet().Seq() != aSecond->GetLayerSet().Seq() )
        return aFirst->GetLayerSet().Seq() < aSecond->GetLayerSet().Seq();

    if( aFirst->m_Uuid != aSecond->m_Uuid )
        return aFirst->m_Uuid < aSecond->m_Uuid;

    return aFirst < aSecond;
}


#if 0
bool FOOTPRINT::cmp_padstack::operator()( const PAD* aFirst, const PAD* aSecond ) const
{
    if( aFirst->GetSize().x != aSecond->GetSize().x )
        return aFirst->GetSize().x < aSecond->GetSize().x;
    if( aFirst->GetSize().y != aSecond->GetSize().y )
        return aFirst->GetSize().y < aSecond->GetSize().y;

    if( aFirst->GetShape() != aSecond->GetShape() )
        return aFirst->GetShape() < aSecond->GetShape();

    if( aFirst->GetLayerSet().Seq() != aSecond->GetLayerSet().Seq() )
        return aFirst->GetLayerSet().Seq() < aSecond->GetLayerSet().Seq();

    if( aFirst->GetDrillSizeX() != aSecond->GetDrillSizeX() )
        return aFirst->GetDrillSizeX() < aSecond->GetDrillSizeX();

    if( aFirst->GetDrillSizeY() != aSecond->GetDrillSizeY() )
        return aFirst->GetDrillSizeY() < aSecond->GetDrillSizeY();

    if( aFirst->GetDrillShape() != aSecond->GetDrillShape() )
        return aFirst->GetDrillShape() < aSecond->GetDrillShape();

    if( aFirst->GetAttribute() != aSecond->GetAttribute() )
        return aFirst->GetAttribute() < aSecond->GetAttribute();

    if( aFirst->GetOrientation() != aSecond->GetOrientation() )
        return aFirst->GetOrientation() < aSecond->GetOrientation();

    if( aFirst->GetSolderMaskExpansion() != aSecond->GetSolderMaskExpansion() )
        return aFirst->GetSolderMaskExpansion() < aSecond->GetSolderMaskExpansion();

    if( aFirst->GetSolderPasteMargin() != aSecond->GetSolderPasteMargin() )
        return aFirst->GetSolderPasteMargin() < aSecond->GetSolderPasteMargin();

    if( aFirst->GetLocalSolderMaskMargin() != aSecond->GetLocalSolderMaskMargin() )
        return aFirst->GetLocalSolderMaskMargin() < aSecond->GetLocalSolderMaskMargin();

    const std::shared_ptr<SHAPE_POLY_SET>& firstShape = aFirst->GetEffectivePolygon( ERROR_INSIDE );
    const std::shared_ptr<SHAPE_POLY_SET>& secondShape = aSecond->GetEffectivePolygon( ERROR_INSIDE );

    if( firstShape->VertexCount() != secondShape->VertexCount() )
        return firstShape->VertexCount() < secondShape->VertexCount();

    for( int ii = 0; ii < firstShape->VertexCount(); ++ii )
    {
        if( firstShape->CVertex( ii ).x != secondShape->CVertex( ii ).x )
            return firstShape->CVertex( ii ).x < secondShape->CVertex( ii ).x;
        if( firstShape->CVertex( ii ).y != secondShape->CVertex( ii ).y )
            return firstShape->CVertex( ii ).y < secondShape->CVertex( ii ).y;
    }

    return false;
}
#endif


bool FOOTPRINT::cmp_zones::operator()( const ZONE* aFirst, const ZONE* aSecond ) const
{
    if( aFirst->GetAssignedPriority() != aSecond->GetAssignedPriority() )
        return aFirst->GetAssignedPriority() < aSecond->GetAssignedPriority();

    if( aFirst->GetLayerSet().Seq() != aSecond->GetLayerSet().Seq() )
        return aFirst->GetLayerSet().Seq() < aSecond->GetLayerSet().Seq();

    if( aFirst->Outline()->TotalVertices() != aSecond->Outline()->TotalVertices() )
        return aFirst->Outline()->TotalVertices() < aSecond->Outline()->TotalVertices();

    for( int ii = 0; ii < aFirst->Outline()->TotalVertices(); ++ii )
    {
        if( aFirst->Outline()->CVertex( ii ).x != aSecond->Outline()->CVertex( ii ).x )
            return aFirst->Outline()->CVertex( ii ).x < aSecond->Outline()->CVertex( ii ).x;
        if( aFirst->Outline()->CVertex( ii ).y != aSecond->Outline()->CVertex( ii ).y )
            return aFirst->Outline()->CVertex( ii ).y < aSecond->Outline()->CVertex( ii ).y;
    }

    if( aFirst->m_Uuid != aSecond->m_Uuid )
        return aFirst->m_Uuid < aSecond->m_Uuid;

    return aFirst < aSecond;
}


void FOOTPRINT::TransformPadsToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                        int aClearance, int aMaxError, ERROR_LOC aErrorLoc,
                                        bool aSkipNPTHPadsWihNoCopper, bool aSkipPlatedPads,
                                        bool aSkipNonPlatedPads ) const
{
    auto processPad =
        [&]( const PAD* pad, PCB_LAYER_ID padLayer )
        {
            VECTOR2I clearance( aClearance, aClearance );

            switch( aLayer )
            {
            case F_Cu:
                if( aSkipPlatedPads && pad->FlashLayer( F_Mask ) )
                    return;

                if( aSkipNonPlatedPads && !pad->FlashLayer( F_Mask ) )
                    return;

                break;

            case B_Cu:
                if( aSkipPlatedPads && pad->FlashLayer( B_Mask ) )
                    return;

                if( aSkipNonPlatedPads && !pad->FlashLayer( B_Mask ) )
                    return;

                break;

            case F_Mask:
            case B_Mask:
                clearance.x += pad->GetSolderMaskExpansion( padLayer );
                clearance.y += pad->GetSolderMaskExpansion( padLayer );
                break;

            case F_Paste:
            case B_Paste:
                clearance += pad->GetSolderPasteMargin( padLayer );
                break;

            default:
                break;
            }

            // Our standard TransformShapeToPolygon() routines can't handle differing x:y clearance
            // values (which get generated when a relative paste margin is used with an oblong pad).
            // So we apply this huge hack and fake a larger pad to run the transform on.
            // Of course being a hack it falls down when dealing with custom shape pads (where the
            // size is only the size of the anchor), so for those we punt and just use clearance.x.

            if( ( clearance.x < 0 || clearance.x != clearance.y )
                    && pad->GetShape( padLayer ) != PAD_SHAPE::CUSTOM )
            {
                VECTOR2I dummySize = pad->GetSize( padLayer ) + clearance + clearance;

                if( dummySize.x <= 0 || dummySize.y <= 0 )
                    return;

                PAD dummy( *pad );
                dummy.SetSize( padLayer, dummySize );
                dummy.TransformShapeToPolygon( aBuffer, padLayer, 0, aMaxError, aErrorLoc );
            }
            else
            {
                pad->TransformShapeToPolygon( aBuffer, padLayer, clearance.x, aMaxError,
                                              aErrorLoc );
            }
        };

    for( const PAD* pad : m_pads )
    {
        if( !pad->FlashLayer( aLayer ) )
            continue;

        if( aLayer == UNDEFINED_LAYER )
        {
            pad->Padstack().ForEachUniqueLayer(
                    [&]( PCB_LAYER_ID l )
                    {
                        processPad( pad, l );
                    } );
        }
        else
        {
            processPad( pad, aLayer );
        }
    }
}


void FOOTPRINT::TransformFPShapesToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                            int aClearance, int aError, ERROR_LOC aErrorLoc,
                                            bool aIncludeText, bool aIncludeShapes,
                                            bool aIncludePrivateItems ) const
{
    std::vector<const PCB_TEXT*> texts; // List of PCB_TEXTs to convert

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( GetPrivateLayers().test( item->GetLayer() ) && !aIncludePrivateItems )
            continue;

        if( item->Type() == PCB_TEXT_T && aIncludeText )
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

            if( aLayer != UNDEFINED_LAYER && text->GetLayer() == aLayer && text->IsVisible() )
                texts.push_back( text );
        }

        if( item->Type() == PCB_TEXTBOX_T && aIncludeText )
        {
            PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( item );

            if( aLayer != UNDEFINED_LAYER && textbox->GetLayer() == aLayer && textbox->IsVisible() )
            {
                // border
                if( textbox->IsBorderEnabled() )
                    textbox->PCB_SHAPE::TransformShapeToPolygon( aBuffer, aLayer, 0, aError, aErrorLoc );
                // text
                textbox->TransformTextToPolySet( aBuffer, 0, aError, aErrorLoc );
            }
        }

        if( item->Type() == PCB_SHAPE_T && aIncludeShapes )
        {
            const PCB_SHAPE* outline = static_cast<PCB_SHAPE*>( item );

            if( aLayer != UNDEFINED_LAYER && outline->GetLayer() == aLayer )
                outline->TransformShapeToPolygon( aBuffer, aLayer, 0, aError, aErrorLoc );
        }
    }

    if( aIncludeText )
    {
        for( const PCB_FIELD* field : m_fields )
        {
            if( field && field->GetLayer() == aLayer && field->IsVisible() )
                texts.push_back( field );
        }
    }

    for( const PCB_TEXT* text : texts )
        text->TransformTextToPolySet( aBuffer, aClearance, aError, aErrorLoc );
}


void FOOTPRINT::EmbedFonts()
{
    using OUTLINE_FONT = KIFONT::OUTLINE_FONT;
    using EMBEDDING_PERMISSION = OUTLINE_FONT::EMBEDDING_PERMISSION;

    std::set<OUTLINE_FONT*> fonts;

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( auto* text = dynamic_cast<EDA_TEXT*>( item ) )
        {
            if( auto* font = text->GetFont(); font && !font->IsStroke() )
            {
                auto* outline = static_cast<OUTLINE_FONT*>( font );
                auto permission = outline->GetEmbeddingPermission();

                if( permission == EMBEDDING_PERMISSION::EDITABLE
                    || permission == EMBEDDING_PERMISSION::INSTALLABLE )
                {
                    fonts.insert( outline );
                }
            }
        }
    }

    for( auto* font : fonts )
    {
        auto file = GetEmbeddedFiles()->AddFile( font->GetFileName(), false );
        file->type = EMBEDDED_FILES::EMBEDDED_FILE::FILE_TYPE::FONT;
    }
}


wxString FOOTPRINT::GetComponentClassAsString() const
{
    if( m_componentClass )
    {
        return m_componentClass->GetFullName();
    }

    return wxEmptyString;
}


static struct FOOTPRINT_DESC
{
    FOOTPRINT_DESC()
    {
        ENUM_MAP<ZONE_CONNECTION>& zcMap = ENUM_MAP<ZONE_CONNECTION>::Instance();

        if( zcMap.Choices().GetCount() == 0 )
        {
            zcMap.Undefined( ZONE_CONNECTION::INHERITED );
            zcMap.Map( ZONE_CONNECTION::INHERITED, _HKI( "Inherited" ) )
                 .Map( ZONE_CONNECTION::NONE, _HKI( "None" ) )
                 .Map( ZONE_CONNECTION::THERMAL, _HKI( "Thermal reliefs" ) )
                 .Map( ZONE_CONNECTION::FULL, _HKI( "Solid" ) )
                 .Map( ZONE_CONNECTION::THT_THERMAL, _HKI( "Thermal reliefs for PTH" ) );
        }

        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask().Seq() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        wxPGChoices fpLayers;       // footprints might be placed only on F.Cu & B.Cu
        fpLayers.Add( LSET::Name( F_Cu ), F_Cu );
        fpLayers.Add( LSET::Name( B_Cu ), B_Cu );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( FOOTPRINT );
        propMgr.AddTypeCast( new TYPE_CAST<FOOTPRINT, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<FOOTPRINT, BOARD_ITEM_CONTAINER> );
        propMgr.InheritsAfter( TYPE_HASH( FOOTPRINT ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( FOOTPRINT ), TYPE_HASH( BOARD_ITEM_CONTAINER ) );

        auto layer = new PROPERTY_ENUM<FOOTPRINT, PCB_LAYER_ID>( _HKI( "Layer" ),
                    &FOOTPRINT::SetLayerAndFlip, &FOOTPRINT::GetLayer );
        layer->SetChoices( fpLayers );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ), layer );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, double>( _HKI( "Orientation" ),
                    &FOOTPRINT::SetOrientationDegrees, &FOOTPRINT::GetOrientationDegrees,
                    PROPERTY_DISPLAY::PT_DEGREE ) );

        const wxString groupFields = _HKI( "Fields" );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Reference" ),
                    &FOOTPRINT::SetReference, &FOOTPRINT::GetReferenceAsString ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Value" ),
                    &FOOTPRINT::SetValue, &FOOTPRINT::GetValueAsString ),
                    groupFields );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Library Link" ),
                    NO_SETTER( FOOTPRINT, wxString ), &FOOTPRINT::GetFPIDAsString ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Library Description" ),
                    NO_SETTER( FOOTPRINT, wxString ), &FOOTPRINT::GetLibDescription ),
                    groupFields );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Keywords" ),
                    NO_SETTER( FOOTPRINT, wxString ), &FOOTPRINT::GetKeywords ),
                    groupFields );

        // Used only in DRC engine
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>(
                                     _HKI( "Component Class" ), NO_SETTER( FOOTPRINT, wxString ),
                                     &FOOTPRINT::GetComponentClassAsString ) )
                .SetIsHiddenFromLibraryEditors()
                .SetIsHiddenFromPropertiesManager();

        const wxString groupAttributes = _HKI( "Attributes" );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Not in Schematic" ),
                    &FOOTPRINT::SetBoardOnly, &FOOTPRINT::IsBoardOnly ), groupAttributes );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Exclude From Position Files" ),
                    &FOOTPRINT::SetExcludedFromPosFiles, &FOOTPRINT::IsExcludedFromPosFiles ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Exclude From Bill of Materials" ),
                    &FOOTPRINT::SetExcludedFromBOM, &FOOTPRINT::IsExcludedFromBOM ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Do not Populate" ),
                    &FOOTPRINT::SetDNP, &FOOTPRINT::IsDNP ),
                    groupAttributes );

        const wxString groupOverrides = _HKI( "Overrides" );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>(
                    _HKI( "Exempt From Courtyard Requirement" ),
                    &FOOTPRINT::SetAllowMissingCourtyard, &FOOTPRINT::AllowMissingCourtyard ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, std::optional<int>>(
                    _HKI( "Clearance Override" ),
                    &FOOTPRINT::SetLocalClearance, &FOOTPRINT::GetLocalClearance,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, std::optional<int>>(
                    _HKI( "Solderpaste Margin Override" ),
                    &FOOTPRINT::SetLocalSolderPasteMargin, &FOOTPRINT::GetLocalSolderPasteMargin,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, std::optional<double>>(
                    _HKI( "Solderpaste Margin Ratio Override" ),
                    &FOOTPRINT::SetLocalSolderPasteMarginRatio,
                    &FOOTPRINT::GetLocalSolderPasteMarginRatio,
                    PROPERTY_DISPLAY::PT_RATIO ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY_ENUM<FOOTPRINT, ZONE_CONNECTION>(
                    _HKI( "Zone Connection Style" ),
                    &FOOTPRINT::SetLocalZoneConnection, &FOOTPRINT::GetLocalZoneConnection ),
                    groupOverrides );
    }
} _FOOTPRINT_DESC;

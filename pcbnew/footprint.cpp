/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/mirror.h>
#include <confirm.h>
#include <refdes_utils.h>
#include <bitmaps.h>
#include <unordered_set>
#include <string_utils.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <macros.h>
#include <pad.h>
#include <pcb_marker.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <pcb_dimension.h>
#include <pcb_bitmap.h>
#include <pcb_textbox.h>
#include <footprint.h>
#include <zone.h>
#include <view/view.h>
#include <i18n_utility.h>
#include <drc/drc_item.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_simple.h>
#include <convert_shape_list_to_polygon.h>
#include <geometry/convex_hull.h>
#include "convert_basic_shapes_to_polygon.h"


FOOTPRINT::FOOTPRINT( BOARD* parent ) :
        BOARD_ITEM_CONTAINER((BOARD_ITEM*) parent, PCB_FOOTPRINT_T ),
        m_boundingBoxCacheTimeStamp( 0 ),
        m_visibleBBoxCacheTimeStamp( 0 ),
        m_textExcludedBBoxCacheTimeStamp( 0 ),
        m_hullCacheTimeStamp( 0 ),
        m_initial_comments( nullptr ),
        m_courtyard_cache_timestamp( 0 )
{
    m_attributes   = 0;
    m_layer        = F_Cu;
    m_orient       = ANGLE_0;
    m_fpStatus     = FP_PADS_are_LOCKED;
    m_arflag       = 0;
    m_link         = 0;
    m_lastEditTime = 0;
    m_localClearance              = 0;
    m_localSolderMaskMargin       = 0;
    m_localSolderPasteMargin      = 0;
    m_localSolderPasteMarginRatio = 0.0;
    m_zoneConnection              = ZONE_CONNECTION::INHERITED;
    m_fileFormatVersionAtLoad     = 0;

    // These are special and mandatory text fields
    m_reference = new PCB_TEXT( this, PCB_TEXT::TEXT_is_REFERENCE );
    m_value = new PCB_TEXT( this, PCB_TEXT::TEXT_is_VALUE );

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

    m_cachedBoundingBox              = aFootprint.m_cachedBoundingBox;
    m_boundingBoxCacheTimeStamp      = aFootprint.m_boundingBoxCacheTimeStamp;
    m_cachedVisibleBBox              = aFootprint.m_cachedVisibleBBox;
    m_visibleBBoxCacheTimeStamp      = aFootprint.m_visibleBBoxCacheTimeStamp;
    m_cachedTextExcludedBBox         = aFootprint.m_cachedTextExcludedBBox;
    m_textExcludedBBoxCacheTimeStamp = aFootprint.m_textExcludedBBoxCacheTimeStamp;
    m_cachedHull                     = aFootprint.m_cachedHull;
    m_hullCacheTimeStamp             = aFootprint.m_hullCacheTimeStamp;

    m_localClearance                 = aFootprint.m_localClearance;
    m_localSolderMaskMargin          = aFootprint.m_localSolderMaskMargin;
    m_localSolderPasteMargin         = aFootprint.m_localSolderPasteMargin;
    m_localSolderPasteMarginRatio    = aFootprint.m_localSolderPasteMarginRatio;
    m_zoneConnection                 = aFootprint.m_zoneConnection;
    m_netTiePadGroups                = aFootprint.m_netTiePadGroups;
    m_fileFormatVersionAtLoad        = aFootprint.m_fileFormatVersionAtLoad;

    std::map<BOARD_ITEM*, BOARD_ITEM*> ptrMap;

    // Copy reference and value.
    m_reference = new PCB_TEXT( *aFootprint.m_reference );
    m_reference->SetParent( this );
    ptrMap[ aFootprint.m_reference ] = m_reference;

    m_value = new PCB_TEXT( *aFootprint.m_value );
    m_value->SetParent( this );
    ptrMap[ aFootprint.m_value ] = m_value;

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

    // Copy auxiliary data
    m_3D_Drawings   = aFootprint.m_3D_Drawings;
    m_doc           = aFootprint.m_doc;
    m_keywords      = aFootprint.m_keywords;
    m_properties    = aFootprint.m_properties;
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
    delete m_reference;
    delete m_value;
    delete m_initial_comments;

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


bool FOOTPRINT::FixUuids()
{
    // replace null UUIDs if any by a valid uuid
    std::vector< BOARD_ITEM* > item_list;

    item_list.push_back( m_reference );
    item_list.push_back( m_value );

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
    m_cachedVisibleBBox              = aOther.m_cachedVisibleBBox;
    m_visibleBBoxCacheTimeStamp      = aOther.m_visibleBBoxCacheTimeStamp;
    m_cachedTextExcludedBBox         = aOther.m_cachedTextExcludedBBox;
    m_textExcludedBBoxCacheTimeStamp = aOther.m_textExcludedBBoxCacheTimeStamp;
    m_cachedHull                     = aOther.m_cachedHull;
    m_hullCacheTimeStamp             = aOther.m_hullCacheTimeStamp;

    m_localClearance                 = aOther.m_localClearance;
    m_localSolderMaskMargin          = aOther.m_localSolderMaskMargin;
    m_localSolderPasteMargin         = aOther.m_localSolderPasteMargin;
    m_localSolderPasteMarginRatio    = aOther.m_localSolderPasteMarginRatio;
    m_zoneConnection                 = aOther.m_zoneConnection;
    m_netTiePadGroups                = aOther.m_netTiePadGroups;

    // Move reference and value
    m_reference = aOther.m_reference;
    m_reference->SetParent( this );
    m_value = aOther.m_value;
    m_value->SetParent( this );


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
    m_doc              = aOther.m_doc;
    m_keywords         = aOther.m_keywords;
    m_properties       = aOther.m_properties;
    m_privateLayers    = aOther.m_privateLayers;

    m_initial_comments = aOther.m_initial_comments;

    // Clear the other item's containers since this is a move
    aOther.Pads().clear();
    aOther.Zones().clear();
    aOther.GraphicalItems().clear();
    aOther.m_value            = nullptr;
    aOther.m_reference        = nullptr;
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
    m_cachedVisibleBBox              = aOther.m_cachedVisibleBBox;
    m_visibleBBoxCacheTimeStamp      = aOther.m_visibleBBoxCacheTimeStamp;
    m_cachedTextExcludedBBox         = aOther.m_cachedTextExcludedBBox;
    m_textExcludedBBoxCacheTimeStamp = aOther.m_textExcludedBBoxCacheTimeStamp;
    m_cachedHull                     = aOther.m_cachedHull;
    m_hullCacheTimeStamp             = aOther.m_hullCacheTimeStamp;

    m_localClearance                 = aOther.m_localClearance;
    m_localSolderMaskMargin          = aOther.m_localSolderMaskMargin;
    m_localSolderPasteMargin         = aOther.m_localSolderPasteMargin;
    m_localSolderPasteMarginRatio    = aOther.m_localSolderPasteMarginRatio;
    m_zoneConnection                 = aOther.m_zoneConnection;
    m_netTiePadGroups                = aOther.m_netTiePadGroups;

    // Copy reference and value
    *m_reference = *aOther.m_reference;
    m_reference->SetParent( this );
    *m_value = *aOther.m_value;
    m_value->SetParent( this );

    std::map<BOARD_ITEM*, BOARD_ITEM*> ptrMap;

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
    m_doc           = aOther.m_doc;
    m_keywords      = aOther.m_keywords;
    m_properties    = aOther.m_properties;
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
        *token = m_reference->GetShownText( false, aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "VALUE" ) ) )
    {
        *token = m_value->GetShownText( false, aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "LAYER" ) ) )
    {
        *token = GetLayerName();
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_LIBRARY" ) ) )
    {
        *token = m_fpid.GetLibNickname();
        return true;
    }
    else if( token->IsSameAs( wxT( "FOOTPRINT_NAME" ) ) )
    {
        *token = m_fpid.GetLibItemName();
        return true;
    }
    else if( token->StartsWith( wxT( "NET_NAME(" ) )
                || token->StartsWith( wxT( "NET_CLASS(" ) )
                || token->StartsWith( wxT( "PIN_NAME(" ) ) )
    {
        wxString padNumber = token->AfterFirst( '(' );
        padNumber = padNumber.BeforeLast( ')' );

        for( PAD* pad : Pads() )
        {
            if( pad->GetNumber() == padNumber )
            {
                if( token->StartsWith( wxT( "NET_NAME" ) ) )
                    *token = pad->GetNetname();
                else if( token->StartsWith( wxT( "NET_CLASS" ) ) )
                    *token = pad->GetNetClassName();
                else
                    *token = pad->GetPinFunction();

                return true;
            }
        }
    }
    else if( m_properties.count( *token ) )
    {
        *token = m_properties.at( *token );
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
    case PCB_TEXT_T:
        // Only user text can be added this way.
        wxASSERT( static_cast<PCB_TEXT*>( aBoardItem )->GetType() == PCB_TEXT::TEXT_is_DIVERS );
        KI_FALLTHROUGH;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_SHAPE_T:
    case PCB_TEXTBOX_T:
    case PCB_BITMAP_T:
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
    case PCB_TEXT_T:
        // Only user text can be removed this way.
        wxCHECK_RET( static_cast<PCB_TEXT*>( aBoardItem )->GetType() == PCB_TEXT::TEXT_is_DIVERS,
                     wxT( "Please report this bug: Invalid remove operation on required text" ) );
        KI_FALLTHROUGH;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_LEADER_T:
    case PCB_SHAPE_T:
    case PCB_TEXTBOX_T:
    case PCB_BITMAP_T:
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
    BOX2I bbox = GetBoundingBox( false, false );

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

    if( smd_count > 0 )
        return FP_SMD;

    if( tht_count > 0 )
        return FP_THROUGH_HOLE;

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
        dummy.Flip( VECTOR2I( 0, 0 ), false );

    for( PAD* pad : dummy.Pads() )
        bbox.Merge( pad->GetBoundingBox() );

    dummy.SetParent( nullptr );

    return bbox;
}


const BOX2I FOOTPRINT::GetBoundingBox() const
{
    return GetBoundingBox( true, true );
}


const BOX2I FOOTPRINT::GetBoundingBox( bool aIncludeText, bool aIncludeInvisibleText ) const
{
    const BOARD* board = GetBoard();
    bool         isFPEdit = board && board->IsFootprintHolder();

    if( board )
    {
        if( aIncludeText && aIncludeInvisibleText )
        {
            if( m_boundingBoxCacheTimeStamp >= board->GetTimeStamp() )
                return m_cachedBoundingBox;
        }
        else if( aIncludeText )
        {
            if( m_visibleBBoxCacheTimeStamp >= board->GetTimeStamp() )
                return m_cachedVisibleBBox;
        }
        else
        {
            if( m_textExcludedBBoxCacheTimeStamp >= board->GetTimeStamp() )
                return m_cachedTextExcludedBBox;
        }
    }

    BOX2I bbox( m_pos );
    bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) );   // Give a min size to the bbox

    for( BOARD_ITEM* item : m_drawings )
    {
        if( m_privateLayers.test( item->GetLayer() ) && !isFPEdit )
            continue;

        // We want the bitmap bounding box just in the footprint editor
        // so it will start with the correct initial zoom
        if( item->Type() == PCB_BITMAP_T && !isFPEdit )
            continue;

        // Handle text separately
        if( item->Type() == PCB_TEXT_T )
            continue;

        // Treat dimension objects as text
        if( !aIncludeText && BaseType( item->Type() ) == PCB_DIMENSION_T )
            continue;

        bbox.Merge( item->GetBoundingBox() );
    }

    for( PAD* pad : m_pads )
        bbox.Merge( pad->GetBoundingBox() );

    for( ZONE* zone : m_zones )
        bbox.Merge( zone->GetBoundingBox() );

    bool noDrawItems = ( m_drawings.empty() && m_pads.empty() && m_zones.empty() );

    // Groups do not contribute to the rect, only their members
    if( aIncludeText || noDrawItems )
    {
        for( BOARD_ITEM* item : m_drawings )
        {
            if( !isFPEdit && m_privateLayers.test( item->GetLayer() ) )
                continue;

            // Only PCB_TEXT items are independently selectable; PCB_TEXTBOX items go in with
            // other graphic items above.
            if( item->Type() == PCB_TEXT_T )
                bbox.Merge( item->GetBoundingBox() );
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
            valueLayerIsVisible = board->IsLayerVisible( m_value->GetLayer() )
                                  && board->IsElementVisible( LAYER_MOD_VALUES )
                                  && board->IsElementVisible( LAYER_MOD_TEXT );

            refLayerIsVisible = board->IsLayerVisible( m_reference->GetLayer() )
                                && board->IsElementVisible( LAYER_MOD_REFERENCES )
                                && board->IsElementVisible( LAYER_MOD_TEXT );
        }


        if( ( m_value->IsVisible() && valueLayerIsVisible )
                || aIncludeInvisibleText
                || noDrawItems )
        {
            bbox.Merge( m_value->GetBoundingBox() );
        }

        if( ( m_reference->IsVisible() && refLayerIsVisible )
                || aIncludeInvisibleText
                || noDrawItems )
        {
            bbox.Merge( m_reference->GetBoundingBox() );
        }
    }

    if( board )
    {
        if( ( aIncludeText && aIncludeInvisibleText ) || noDrawItems )
        {
            m_boundingBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedBoundingBox = bbox;
        }
        else if( aIncludeText )
        {
            m_visibleBBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedVisibleBBox = bbox;
        }
        else
        {
            m_textExcludedBBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedTextExcludedBBox = bbox;
        }
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

        if( item->Type() != PCB_TEXT_T && item->Type() != PCB_BITMAP_T )
        {
            item->TransformShapeToPolygon( rawPolys, UNDEFINED_LAYER, 0, ARC_LOW_DEF,
                                           ERROR_OUTSIDE );
        }

        // We intentionally exclude footprint text from the bounding hull.
    }

    for( PAD* pad : m_pads )
    {
        pad->TransformShapeToPolygon( rawPolys, UNDEFINED_LAYER, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
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
    aList.emplace_back( UnescapeString( m_reference->GetText() ),
                        UnescapeString( m_value->GetText() ) );

    if( aFrame->IsType( FRAME_FOOTPRINT_VIEWER )
        || aFrame->IsType( FRAME_FOOTPRINT_VIEWER_MODAL )
        || aFrame->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        size_t     padCount = GetPadCount( DO_NOT_INCLUDE_NPTH );

        aList.emplace_back( _( "Library" ), GetFPID().GetLibNickname().wx_str() );

        aList.emplace_back( _( "Footprint Name" ), GetFPID().GetLibItemName().wx_str() );

        aList.emplace_back( _( "Pads" ), wxString::Format( wxT( "%zu" ), padCount ) );

        aList.emplace_back( wxString::Format( _( "Doc: %s" ), GetDescription() ),
                            wxString::Format( _( "Keywords: %s" ), GetKeywords() ) );

        return;
    }

    // aFrame is the board editor:
    aList.emplace_back( _( "Board Side" ), IsFlipped() ? _( "Back (Flipped)" ) : _( "Front" ) );

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

    msg.Printf( _( "Footprint: %s" ), m_fpid.GetUniStringLibId() );
    msg2.Printf( _( "3D-Shape: %s" ), m_3D_Drawings.empty() ? _( "<none>" )
                                                            : m_3D_Drawings.front().m_Filename );
    aList.emplace_back( msg, msg2 );

    msg.Printf( _( "Doc: %s" ), m_doc );
    msg2.Printf( _( "Keywords: %s" ), m_keywords );
    aList.emplace_back( msg, msg2 );
}


bool FOOTPRINT::IsOnLayer( PCB_LAYER_ID aLayer, bool aIncludeCourtyards ) const
{
    static const LSET courtyardLayers( 2, B_CrtYd, F_CrtYd );

    if( aIncludeCourtyards && courtyardLayers[aLayer] )
        return !GetCourtyard( aLayer ).IsEmpty();

    // If we have any pads, fall back on normal checking
    if( !m_pads.empty() )
        return m_layer == aLayer;

    // No pads?  Check if this entire footprint exists on the given layer
    for( ZONE* zone : m_zones )
    {
        if( !zone->IsOnLayer( aLayer ) )
            return false;
    }

    for( BOARD_ITEM* item : m_drawings )
    {
        if( !item->IsOnLayer( aLayer ) )
            return false;
    }

    return true;
}


bool FOOTPRINT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox( false, false );
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
        return arect.Contains( GetBoundingBox( false, false ) );
    }
    else
    {
        // If the rect does not intersect the bounding box, skip any tests
        if( !aRect.Intersects( GetBoundingBox( false, false ) ) )
            return false;

        // The empty footprint dummy rectangle intersects the selection area.
        if( m_pads.empty() && m_zones.empty() && m_drawings.empty() )
            return GetBoundingBox( true, false ).Intersects( arect );

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


unsigned FOOTPRINT::GetUniquePadCount( INCLUDE_NPTH_T aIncludeNPTH ) const
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

    return usedNumbers.size();
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

        case PCB_TEXT_T:
            if( inspector( m_reference, testData ) == INSPECT_RESULT::QUIT )
                return INSPECT_RESULT::QUIT;

            if( inspector( m_value, testData ) == INSPECT_RESULT::QUIT )
                return INSPECT_RESULT::QUIT;

            // Intentionally fall through since m_Drawings can hold PCB_TEXT_T also
            KI_FALLTHROUGH;

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_LEADER_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_SHAPE_T:
        case PCB_TEXTBOX_T:
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


wxString FOOTPRINT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
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


void FOOTPRINT::RunOnChildren( const std::function<void ( BOARD_ITEM*)>& aFunction ) const
{
    try
    {
        for( PAD* pad : m_pads )
            aFunction( static_cast<BOARD_ITEM*>( pad ) );

        for( ZONE* zone : m_zones )
            aFunction( static_cast<ZONE*>( zone ) );

        for( PCB_GROUP* group : m_groups )
            aFunction( static_cast<PCB_GROUP*>( group ) );

        for( BOARD_ITEM* drawing : m_drawings )
            aFunction( static_cast<BOARD_ITEM*>( drawing ) );

        aFunction( static_cast<BOARD_ITEM*>( m_reference ) );
        aFunction( static_cast<BOARD_ITEM*>( m_value ) );
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( wxT( "Error running FOOTPRINT::RunOnChildren" ) );
    }
}


void FOOTPRINT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_ANCHOR;

    switch( m_layer )
    {
    default:
        wxASSERT_MSG( false, wxT( "Illegal layer" ) );   // do you really have footprints placed
                                                         // on other layers?
        KI_FALLTHROUGH;

    case F_Cu:
        aLayers[1] = LAYER_MOD_FR;
        break;

    case B_Cu:
        aLayers[1] = LAYER_MOD_BK;
        break;
    }

    if( IsLocked() )
        aLayers[ aCount++ ] = LAYER_LOCKED_ITEM_SHADOW;

    if( IsConflicting() )
        aLayers[ aCount++ ] = LAYER_CONFLICTS_SHADOW;

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
            aLayers[ aCount++ ] = F_SilkS;

        if( b_silk )
            aLayers[ aCount++ ] = B_SilkS;
    }
}


double FOOTPRINT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // The locked shadow shape is shown only if the footprint itself is visible
        if( ( m_layer == F_Cu ) && aView->IsLayerVisible( LAYER_MOD_FR ) )
            return 0.0;

        if( ( m_layer == B_Cu ) && aView->IsLayerVisible( LAYER_MOD_BK ) )
            return 0.0;

        return std::numeric_limits<double>::max();
    }

    if( aLayer == LAYER_CONFLICTS_SHADOW && IsConflicting() )
    {
        // The locked shadow shape is shown only if the footprint itself is visible
        if( ( m_layer == F_Cu ) && aView->IsLayerVisible( LAYER_MOD_FR ) )
            return 0.0;

        if( ( m_layer == B_Cu ) && aView->IsLayerVisible( LAYER_MOD_BK ) )
            return 0.0;

        return std::numeric_limits<double>::max();
    }

    int layer = ( m_layer == F_Cu ) ? LAYER_MOD_FR :
                ( m_layer == B_Cu ) ? LAYER_MOD_BK : LAYER_ANCHOR;

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
    BOX2I area = GetBoundingBox( true, true );

    // Inflate in case clearance lines are drawn around pads, etc.
    if( const BOARD* board = GetBoard() )
    {
        int biggest_clearance = board->GetDesignSettings().GetBiggestClearanceValue();
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
    VECTOR2I newpos = m_pos + aMoveVector;
    SetPosition( newpos );
}


void FOOTPRINT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE orientation = GetOrientation();
    EDA_ANGLE newOrientation = orientation + aAngle;
    VECTOR2I  newpos = m_pos;
    RotatePoint( newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( newOrientation );

    m_reference->KeepUpright( orientation, newOrientation );
    m_value->KeepUpright( orientation, newOrientation );

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() == PCB_TEXT_T )
            static_cast<PCB_TEXT*>( item )->KeepUpright( orientation, newOrientation );
    }

    m_boundingBoxCacheTimeStamp = 0;
    m_visibleBBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;
    m_hullCacheTimeStamp = 0;
    m_courtyard_cache_timestamp = 0;
}


void FOOTPRINT::SetLayerAndFlip( PCB_LAYER_ID aLayer )
{
    wxASSERT( aLayer == F_Cu || aLayer == B_Cu );

    if( aLayer != GetLayer() )
        Flip( GetPosition(), true );
}


void FOOTPRINT::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
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
    BOARD_ITEM::SetLayer( FlipLayer( GetLayer() ) );

    // Reverse mirror orientation.
    m_orient = -m_orient;
    m_orient.Normalize180();

    // Mirror pads to other side of board.
    for( PAD* pad : m_pads )
        pad->Flip( m_pos, false );

    // Mirror zones to other side of board.
    for( ZONE* zone : m_zones )
        zone->Flip( m_pos, false );

    // Mirror reference and value.
    m_reference->Flip( m_pos, false );
    m_value->Flip( m_pos, false );

    // Reverse mirror footprint graphics and texts.
    for( BOARD_ITEM* item : m_drawings )
        item->Flip( m_pos, false );

    // Now rotate 180 deg if required
    if( aFlipLeftRight )
        Rotate( aCentre, ANGLE_180 );

    m_boundingBoxCacheTimeStamp = 0;
    m_visibleBBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;
    m_courtyard_cache_timestamp = 0;

    m_cachedHull.Mirror( aFlipLeftRight, !aFlipLeftRight, m_pos );

    std::swap( m_courtyard_cache_front, m_courtyard_cache_back );
}


void FOOTPRINT::SetPosition( const VECTOR2I& aPos )
{
    VECTOR2I delta = aPos - m_pos;

    m_pos += delta;

    m_reference->EDA_TEXT::Offset( delta );
    m_value->EDA_TEXT::Offset( delta );

    for( PAD* pad : m_pads )
        pad->SetPosition( pad->GetPosition() + delta );

    for( ZONE* zone : m_zones )
        zone->Move( delta );

    for( BOARD_ITEM* item : m_drawings )
        item->Move( delta );

    m_cachedBoundingBox.Move( delta );
    m_cachedVisibleBBox.Move( delta );
    m_cachedTextExcludedBBox.Move( delta );
    m_courtyard_cache_back.Move( delta );
    m_courtyard_cache_front.Move( delta );
    m_cachedHull.Move( delta );
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

    // Update of the reference and value.
    m_reference->Move( moveVector );
    m_value->Move( moveVector );

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
    m_cachedVisibleBBox.Move( moveVector );
    m_cachedTextExcludedBBox.Move( moveVector );
    m_cachedHull.Move( moveVector );
}


void FOOTPRINT::SetOrientation( const EDA_ANGLE& aNewAngle )
{
    EDA_ANGLE angleChange = aNewAngle - m_orient;  // change in rotation

    m_orient = aNewAngle;
    m_orient.Normalize180();

    for( PAD* pad : m_pads )
        pad->Rotate( GetPosition(), angleChange );

    for( ZONE* zone : m_zones )
        zone->Rotate( GetPosition(), angleChange );

    // Update of the reference and value.
    m_reference->Rotate( GetPosition(), angleChange );
    m_value->Rotate( GetPosition(), angleChange );

    for( BOARD_ITEM* item : m_drawings )
        item->Rotate( GetPosition(), angleChange );

    m_boundingBoxCacheTimeStamp = 0;
    m_visibleBBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;
    m_courtyard_cache_timestamp = 0;

    m_cachedHull.Rotate( angleChange, GetPosition() );
}


BOARD_ITEM* FOOTPRINT::Duplicate() const
{
    FOOTPRINT* dupe = static_cast<FOOTPRINT*>( BOARD_ITEM::Duplicate() );

    dupe->RunOnChildren( [&]( BOARD_ITEM* child )
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

    case PCB_TEXT_T:
    {
        PCB_TEXT* new_text = new PCB_TEXT( *static_cast<const PCB_TEXT*>( aItem ) );
        const_cast<KIID&>( new_text->m_Uuid ) = KIID();

        if( new_text->GetType() == PCB_TEXT::TEXT_is_REFERENCE )
        {
            new_text->SetText( wxT( "${REFERENCE}" ) );
            new_text->SetType( PCB_TEXT::TEXT_is_DIVERS );
        }
        else if( new_text->GetType() == PCB_TEXT::TEXT_is_VALUE )
        {
            new_text->SetText( wxT( "${VALUE}" ) );
            new_text->SetType( PCB_TEXT::TEXT_is_DIVERS );
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
        new_item = static_cast<const PCB_GROUP*>( aItem )->DeepDuplicate();
        break;

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
    int textMargin = KiROUND( 5 * aCollector.GetGuide()->OnePixelInIU() );
    SHAPE_POLY_SET poly;

    if( aItem->Type() == PCB_MARKER_T )
    {
        const PCB_MARKER* marker = static_cast<const PCB_MARKER*>( aItem );
        SHAPE_LINE_CHAIN  markerShape;

        marker->ShapeToPolygon( markerShape );
        return markerShape.Area();
    }
    else if( aItem->Type() == PCB_GROUP_T )
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
    else if( aItem->Type() == PCB_TEXT_T )
    {
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( aItem );

        text->TransformTextToPolySet( poly, UNDEFINED_LAYER, textMargin, ARC_LOW_DEF, ERROR_OUTSIDE );
    }
    else if( aItem->Type() == PCB_TEXTBOX_T )
    {
        const PCB_TEXTBOX* tb = static_cast<const PCB_TEXTBOX*>( aItem );

        tb->TransformTextToPolySet( poly, UNDEFINED_LAYER, textMargin, ARC_LOW_DEF, ERROR_OUTSIDE );
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

        case SHAPE_T::RECT:
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
    else
    {
        aItem->TransformShapeToPolygon( poly, UNDEFINED_LAYER, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
    }

    return polygonArea( poly );
}


double FOOTPRINT::CoverageRatio( const GENERAL_COLLECTOR& aCollector ) const
{
    int textMargin = KiROUND( 5 * aCollector.GetGuide()->OnePixelInIU() );

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
    if( GetBoard() && GetBoard()->GetTimeStamp() > m_courtyard_cache_timestamp )
        const_cast<FOOTPRINT*>( this )->BuildCourtyardCaches();

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

    m_courtyard_cache_timestamp = GetBoard()->GetTimeStamp();

    // Build the courtyard area from graphic items on the courtyard.
    // Only PCB_SHAPE_T have meaning, graphic texts are ignored.
    // Collect items:
    std::vector<PCB_SHAPE*> list_front;
    std::vector<PCB_SHAPE*> list_back;

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( item->GetLayer() == B_CrtYd && item->Type() == PCB_SHAPE_T )
            list_back.push_back( static_cast<PCB_SHAPE*>( item ) );

        if( item->GetLayer() == F_CrtYd && item->Type() == PCB_SHAPE_T )
            list_front.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    if( !list_front.size() && !list_back.size() )
        return;

    int errorMax = pcbIUScale.mmToIU( 0.02 );         // max error for polygonization
    int chainingEpsilon = pcbIUScale.mmToIU( 0.02 );  // max dist from one endPt to next startPt

    if( ConvertOutlineToPolygon( list_front, m_courtyard_cache_front, errorMax, chainingEpsilon,
                                 true, aErrorHandler ) )
    {
        // Touching courtyards, or courtyards -at- the clearance distance are legal.
        m_courtyard_cache_front.Inflate( -1, SHAPE_POLY_SET::CHAMFER_ACUTE_CORNERS );

        m_courtyard_cache_front.CacheTriangulation( false );
    }
    else
    {
        SetFlags( MALFORMED_F_COURTYARD );
    }

    if( ConvertOutlineToPolygon( list_back, m_courtyard_cache_back, errorMax, chainingEpsilon,
                                 true, aErrorHandler ) )
    {
        // Touching courtyards, or courtyards -at- the clearance distance are legal.
        m_courtyard_cache_back.Inflate( -1, SHAPE_POLY_SET::CHAMFER_ACUTE_CORNERS );

        m_courtyard_cache_back.CacheTriangulation( false );
    }
    else
    {
        SetFlags( MALFORMED_B_COURTYARD );
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


void FOOTPRINT::CheckPads( const std::function<void( const PAD*, int,
                                                     const wxString& )>& aErrorHandler )
{
    if( aErrorHandler == nullptr )
        return;

    for( PAD* pad: Pads() )
    {
        if( pad->GetAttribute() == PAD_ATTRIB::PTH ||  pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            if( pad->GetDrillSizeX() < 1 || pad->GetDrillSizeY() < 1 )
                (aErrorHandler)( pad, DRCE_PAD_TH_WITH_NO_HOLE, wxEmptyString );
        }

        if( pad->GetAttribute() == PAD_ATTRIB::PTH )
        {
            if( !pad->IsOnCopperLayer() )
            {
                (aErrorHandler)( pad, DRCE_PADSTACK, _( "(PTH pad has no copper layers)" ) );
            }
            else
            {
                LSET           lset = pad->GetLayerSet() & LSET::AllCuMask();
                PCB_LAYER_ID   layer = lset.Seq().at( 0 );
                SHAPE_POLY_SET padOutline;

                pad->TransformShapeToPolygon( padOutline, layer, 0, ARC_HIGH_DEF, ERROR_INSIDE );

                std::shared_ptr<SHAPE_SEGMENT> hole = pad->GetEffectiveHoleShape();
                SHAPE_POLY_SET                 holeOutline;

                TransformOvalToPolygon( holeOutline, hole->GetSeg().A, hole->GetSeg().B,
                                        hole->GetWidth(), ARC_HIGH_DEF, ERROR_INSIDE );

                padOutline.BooleanSubtract( holeOutline, SHAPE_POLY_SET::POLYGON_MODE::PM_FAST );

                if( padOutline.IsEmpty() )
                    aErrorHandler( pad, DRCE_PADSTACK, _( "(PTH pad's hole leaves no copper)" ) );
            }
        }

        if( pad->GetAttribute() == PAD_ATTRIB::SMD )
        {
            if( pad->IsOnLayer( F_Cu ) && pad->IsOnLayer( B_Cu ) )
            {
                aErrorHandler( pad, DRCE_PADSTACK,
                               _( "(SMD pad appears on both front and back copper)" ) );
            }
            else if( pad->IsOnLayer( F_Cu ) )
            {
                if( pad->IsOnLayer( B_Mask ) )
                {
                    aErrorHandler( pad, DRCE_PADSTACK,
                                   _( "(SMD pad copper and mask layers don't match)" ) );
                }
                else if( pad->IsOnLayer( B_Paste ) )
                {
                    aErrorHandler( pad, DRCE_PADSTACK,
                                   _( "(SMD pad copper and paste layers don't match)" ) );
                }
            }
            else if( pad->IsOnLayer( B_Cu ) )
            {
                if( pad->IsOnLayer( F_Mask ) )
                {
                    aErrorHandler( pad, DRCE_PADSTACK,
                                   _( "(SMD pad copper and mask layers don't match)" ) );
                }
                else if( pad->IsOnLayer( F_Paste ) )
                {
                    aErrorHandler( pad, DRCE_PADSTACK,
                                   _( "(SMD pad copper and paste layers don't match)" ) );
                }
            }
        }
    }
}


void FOOTPRINT::CheckShortingPads( const std::function<void( const PAD*, const PAD*,
                                                             const VECTOR2I& )>& aErrorHandler )
{
    std::unordered_map<PTR_PTR_CACHE_KEY, int> checkedPairs;

    for( PAD* pad : Pads() )
    {
        std::vector<PAD*> netTiePads = GetNetTiePads( pad );

        for( PAD* other : Pads() )
        {
            if( other == pad || pad->SameLogicalPadAs( other ) )
                continue;

            if( alg::contains( netTiePads, other ) )
                continue;

            if( !( ( pad->GetLayerSet() & other->GetLayerSet() ) & LSET::AllCuMask() ).any() )
                continue;

            // store canonical order so we don't collide in both directions (a:b and b:a)
            PAD* a = pad;
            PAD* b = other;

            if( static_cast<void*>( a ) > static_cast<void*>( b ) )
                std::swap( a, b );

            if( checkedPairs.find( { a, b } ) == checkedPairs.end() )
            {
                checkedPairs[ { a, b } ] = 1;

                if( pad->GetBoundingBox().Intersects( other->GetBoundingBox() ) )
                {
                    VECTOR2I pos;
                    SHAPE*   padShape = pad->GetEffectiveShape().get();
                    SHAPE*   otherShape = other->GetEffectiveShape().get();

                    if( padShape->Collide( otherShape, 0, nullptr, &pos ) )
                        aErrorHandler( pad, other, pos );
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
    }

    for( ZONE* zone : m_zones )
    {
        if( !zone->GetIsRuleArea() && zone->IsOnCopperLayer() )
            copperItems.push_back( zone );
    }

    if( m_reference->IsOnCopperLayer() )
        copperItems.push_back( m_reference );

    if( m_value->IsOnCopperLayer() )
        copperItems.push_back( m_value );

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

        copperOutlines.Simplify( SHAPE_POLY_SET::PM_FAST );

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

    auto ret = MapPadNumbersToNetTieGroups();

    for( auto [ padNumber, _ ] : ret )
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

    std::swap( *this, *static_cast<FOOTPRINT*>( aImage ) );
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


#define TEST( a, b ) { if( a != b ) return a < b; }
#define TEST_PT( a, b ) { if( a.x != b.x ) return a.x < b.x; if( a.y != b.y ) return a.y < b.y; }


bool FOOTPRINT::cmp_drawings::operator()( const BOARD_ITEM* itemA, const BOARD_ITEM* itemB ) const
{
    TEST( itemA->Type(), itemB->Type() );
    TEST( itemA->GetLayer(), itemB->GetLayer() );

    if( itemA->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* dwgA = static_cast<const PCB_SHAPE*>( itemA );
        const PCB_SHAPE* dwgB = static_cast<const PCB_SHAPE*>( itemB );

        TEST( dwgA->GetShape(), dwgB->GetShape() );

        TEST_PT( dwgA->GetStart(), dwgB->GetStart() );
        TEST_PT( dwgA->GetEnd(), dwgB->GetEnd() );

        if( dwgA->GetShape() == SHAPE_T::ARC )
        {
            TEST_PT( dwgA->GetCenter(), dwgB->GetCenter() );
        }
        else if( dwgA->GetShape() == SHAPE_T::BEZIER )
        {
            TEST_PT( dwgA->GetBezierC1(), dwgB->GetBezierC1() );
            TEST_PT( dwgA->GetBezierC2(), dwgB->GetBezierC2() );
        }
        else if( dwgA->GetShape() == SHAPE_T::POLY )
        {
            TEST( dwgA->GetPolyShape().TotalVertices(), dwgB->GetPolyShape().TotalVertices() );

            for( int ii = 0; ii < dwgA->GetPolyShape().TotalVertices(); ++ii )
                TEST_PT( dwgA->GetPolyShape().CVertex( ii ), dwgB->GetPolyShape().CVertex( ii ) );
        }

        TEST( dwgA->GetWidth(), dwgB->GetWidth() );
    }

    TEST( itemA->m_Uuid, itemB->m_Uuid );   // should be always the case for valid boards

    return itemA < itemB;
}


bool FOOTPRINT::cmp_pads::operator()( const PAD* aFirst, const PAD* aSecond ) const
{
    if( aFirst->GetNumber() != aSecond->GetNumber() )
        return StrNumCmp( aFirst->GetNumber(), aSecond->GetNumber() ) < 0;

    TEST_PT( aFirst->GetFPRelativePosition(), aSecond->GetFPRelativePosition() );
    TEST_PT( aFirst->GetSize(), aSecond->GetSize() );
    TEST( aFirst->GetShape(), aSecond->GetShape() );
    TEST( aFirst->GetLayerSet().Seq(), aSecond->GetLayerSet().Seq() );

    TEST( aFirst->m_Uuid, aSecond->m_Uuid );   // should be always the case for valid boards

    return aFirst < aSecond;
}


bool FOOTPRINT::cmp_zones::operator()( const ZONE* aFirst, const ZONE* aSecond ) const
{
    TEST( aFirst->GetAssignedPriority(), aSecond->GetAssignedPriority() );
    TEST( aFirst->GetLayerSet().Seq(), aSecond->GetLayerSet().Seq() );

    TEST( aFirst->Outline()->TotalVertices(), aSecond->Outline()->TotalVertices() );

    for( int ii = 0; ii < aFirst->Outline()->TotalVertices(); ++ii )
        TEST_PT( aFirst->Outline()->CVertex( ii ), aSecond->Outline()->CVertex( ii ) );

    TEST( aFirst->m_Uuid, aSecond->m_Uuid );   // should be always the case for valid boards

    return aFirst < aSecond;
}


#undef TEST


void FOOTPRINT::TransformPadsToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                        int aClearance, int aMaxError, ERROR_LOC aErrorLoc,
                                        bool aSkipNPTHPadsWihNoCopper, bool aSkipPlatedPads,
                                        bool aSkipNonPlatedPads ) const
{
    for( const PAD* pad : m_pads )
    {
        if( !pad->FlashLayer( aLayer ) )
            continue;

        VECTOR2I clearance( aClearance, aClearance );

        switch( aLayer )
        {
        case F_Cu:
            if( aSkipPlatedPads && pad->FlashLayer( F_Mask ) )
                continue;

            if( aSkipNonPlatedPads && !pad->FlashLayer( F_Mask ) )
                continue;

            break;

        case B_Cu:
            if( aSkipPlatedPads && pad->FlashLayer( B_Mask ) )
                continue;

            if( aSkipNonPlatedPads && !pad->FlashLayer( B_Mask ) )
                continue;

            break;

        case F_Mask:
        case B_Mask:
            clearance.x += pad->GetSolderMaskExpansion();
            clearance.y += pad->GetSolderMaskExpansion();
            break;

        case F_Paste:
        case B_Paste:
            clearance += pad->GetSolderPasteMargin();
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
                && pad->GetShape() != PAD_SHAPE::CUSTOM )
        {
            VECTOR2I dummySize = pad->GetSize() + clearance + clearance;

            if( dummySize.x <= 0 || dummySize.y <= 0 )
                continue;

            PAD dummy( *pad );
            dummy.SetSize( dummySize );
            dummy.TransformShapeToPolygon( aBuffer, aLayer, 0, aMaxError, aErrorLoc );
        }
        else
        {
            pad->TransformShapeToPolygon( aBuffer, aLayer, clearance.x, aMaxError, aErrorLoc );
        }
    }
}


void FOOTPRINT::TransformFPShapesToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                            int aClearance, int aError, ERROR_LOC aErrorLoc,
                                            bool aIncludeText, bool aIncludeShapes,
                                            bool aIncludePrivateItems ) const
{
    std::vector<PCB_TEXT*> texts;  // List of PCB_TEXTs to convert

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
                textbox->TransformShapeToPolygon( aBuffer, aLayer, 0, aError, aErrorLoc );
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
        if( Reference().GetLayer() == aLayer && Reference().IsVisible() )
            texts.push_back( &Reference() );

        if( Value().GetLayer() == aLayer && Value().IsVisible() )
            texts.push_back( &Value() );
    }

    for( const PCB_TEXT* text : texts )
        text->TransformTextToPolySet( aBuffer, aLayer, aClearance, aError, aErrorLoc );
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

            for( LSEQ seq = LSET::AllLayersMask().Seq(); seq; ++seq )
                layerEnum.Map( *seq, LSET::Name( *seq ) );
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

        const wxString groupFootprint = _HKI( "Footprint Properties" );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Reference" ),
                    &FOOTPRINT::SetReference, &FOOTPRINT::GetReferenceAsString ),
                    groupFootprint );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Value" ),
                    &FOOTPRINT::SetValue, &FOOTPRINT::GetValueAsString ),
                    groupFootprint );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Library link" ),
                    NO_SETTER( FOOTPRINT, wxString ), &FOOTPRINT::GetFPIDAsString ),
                    groupFootprint );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Description" ),
                    NO_SETTER( FOOTPRINT, wxString ), &FOOTPRINT::GetDescription ),
                    groupFootprint );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Keywords" ),
                    NO_SETTER( FOOTPRINT, wxString ), &FOOTPRINT::GetKeywords ),
                    groupFootprint );

        const wxString groupAttributes = _HKI( "Fabrication Attributes" );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Not in schematic" ),
                    &FOOTPRINT::SetBoardOnly, &FOOTPRINT::IsBoardOnly ), groupAttributes );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Exclude from position files" ),
                    &FOOTPRINT::SetExcludedFromPosFiles, &FOOTPRINT::IsExcludedFromPosFiles ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Exclude from bill of materials" ),
                    &FOOTPRINT::SetExcludedFromBOM, &FOOTPRINT::IsExcludedFromBOM ),
                    groupAttributes );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>( _HKI( "Do not populate" ),
                    &FOOTPRINT::SetDNP, &FOOTPRINT::IsDNP ),
                    groupAttributes );

        const wxString groupOverrides = _HKI( "Overrides" );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, bool>(
                    _HKI( "Exempt from courtyard requirement" ),
                    &FOOTPRINT::SetAllowMissingCourtyard, &FOOTPRINT::AllowMissingCourtyard ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, int>( _HKI( "Clearance Override" ),
                    &FOOTPRINT::SetLocalClearance, &FOOTPRINT::GetLocalClearance,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, int>( _HKI( "Solderpaste Margin Override" ),
                    &FOOTPRINT::SetLocalSolderPasteMargin, &FOOTPRINT::GetLocalSolderPasteMargin,
                    PROPERTY_DISPLAY::PT_SIZE ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, double>(
                    _HKI( "Solderpaste Margin Ratio Override" ),
                    &FOOTPRINT::SetLocalSolderPasteMarginRatio,
                    &FOOTPRINT::GetLocalSolderPasteMarginRatio ),
                    groupOverrides );
        propMgr.AddProperty( new PROPERTY_ENUM<FOOTPRINT, ZONE_CONNECTION>(
                    _HKI( "Zone Connection Style" ),
                    &FOOTPRINT::SetZoneConnection, &FOOTPRINT::GetZoneConnection ),
                    groupOverrides );
    }
} _FOOTPRINT_DESC;

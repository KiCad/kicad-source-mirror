/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <core/mirror.h>
#include <confirm.h>
#include <refdes_utils.h>
#include <bitmaps.h>
#include <unordered_set>
#include <string_utils.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <fp_shape.h>
#include <macros.h>
#include <pad.h>
#include <pcb_text.h>
#include <pcb_marker.h>
#include <pcb_group.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <view/view.h>
#include <geometry/shape_null.h>
#include <i18n_utility.h>
#include <convert_shape_list_to_polygon.h>
#include <geometry/convex_hull.h>

FOOTPRINT::FOOTPRINT( BOARD* parent ) :
        BOARD_ITEM_CONTAINER((BOARD_ITEM*) parent, PCB_FOOTPRINT_T ),
        m_boundingBoxCacheTimeStamp( 0 ),
        m_visibleBBoxCacheTimeStamp( 0 ),
        m_textExcludedBBoxCacheTimeStamp( 0 ),
        m_hullCacheTimeStamp( 0 ),
        m_initial_comments( nullptr )
{
    m_attributes   = 0;
    m_layer        = F_Cu;
    m_orient       = 0;
    m_fpStatus     = FP_PADS_are_LOCKED;
    m_arflag       = 0;
    m_rot90Cost    = m_rot180Cost = 0;
    m_link         = 0;
    m_lastEditTime = 0;
    m_localClearance              = 0;
    m_localSolderMaskMargin       = 0;
    m_localSolderPasteMargin      = 0;
    m_localSolderPasteMarginRatio = 0.0;
    m_zoneConnection              = ZONE_CONNECTION::INHERITED; // Use zone setting by default
    m_thermalWidth = 0;     // Use zone setting by default
    m_thermalGap = 0;       // Use zone setting by default

    // These are special and mandatory text fields
    m_reference = new FP_TEXT( this, FP_TEXT::TEXT_is_REFERENCE );
    m_value = new FP_TEXT( this, FP_TEXT::TEXT_is_VALUE );

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
    m_rot90Cost    = aFootprint.m_rot90Cost;
    m_rot180Cost   = aFootprint.m_rot180Cost;
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
    m_thermalWidth                   = aFootprint.m_thermalWidth;
    m_thermalGap                     = aFootprint.m_thermalGap;

    std::map<BOARD_ITEM*, BOARD_ITEM*> ptrMap;

    // Copy reference and value.
    m_reference = new FP_TEXT( *aFootprint.m_reference );
    m_reference->SetParent( this );
    ptrMap[ aFootprint.m_reference ] = m_reference;

    m_value = new FP_TEXT( *aFootprint.m_value );
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
    for( FP_ZONE* zone : aFootprint.Zones() )
    {
        FP_ZONE* newZone = static_cast<FP_ZONE*>( zone->Clone() );
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

    // Copy auxiliary data: 3D_Drawings info
    m_3D_Drawings = aFootprint.m_3D_Drawings;

    m_doc         = aFootprint.m_doc;
    m_keywords    = aFootprint.m_keywords;
    m_properties  = aFootprint.m_properties;

    m_arflag = 0;

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
    // Clean up the owned elements
    delete m_reference;
    delete m_value;
    delete m_initial_comments;

    for( PAD* p : m_pads )
        delete p;

    m_pads.clear();

    for( FP_ZONE* zone : m_fp_zones )
        delete zone;

    m_fp_zones.clear();

    for( PCB_GROUP* group : m_fp_groups )
        delete group;

    m_fp_groups.clear();

    for( BOARD_ITEM* d : m_drawings )
        delete d;

    m_drawings.clear();
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
    for( PCB_GROUP* group : m_fp_groups )
        item_list.push_back( group );

    // Probably notneeded, because old fp do not have zones. But just in case.
    for( FP_ZONE* zone : m_fp_zones )
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
    m_rot90Cost     = aOther.m_rot90Cost;
    m_rot180Cost    = aOther.m_rot180Cost;
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
    m_thermalWidth                   = aOther.m_thermalWidth;
    m_thermalGap                     = aOther.m_thermalGap;

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
    m_fp_zones.clear();

    for( FP_ZONE* item : aOther.Zones() )
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
    m_fp_groups.clear();

    for( PCB_GROUP* group : aOther.Groups() )
        Add( group );

    aOther.Groups().clear();

    // Copy auxiliary data: 3D_Drawings info
    m_3D_Drawings.clear();
    m_3D_Drawings = aOther.m_3D_Drawings;
    m_doc         = aOther.m_doc;
    m_keywords    = aOther.m_keywords;
    m_properties  = aOther.m_properties;

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
    m_rot90Cost     = aOther.m_rot90Cost;
    m_rot180Cost    = aOther.m_rot180Cost;
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
    m_thermalWidth                   = aOther.m_thermalWidth;
    m_thermalGap                     = aOther.m_thermalGap;

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
    m_fp_zones.clear();

    for( FP_ZONE* zone : aOther.Zones() )
    {
        FP_ZONE* newZone = static_cast<FP_ZONE*>( zone->Clone() );
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
    m_fp_groups.clear();

    for( PCB_GROUP* group : aOther.Groups() )
    {
        PCB_GROUP* newGroup = static_cast<PCB_GROUP*>( group->Clone() );
        newGroup->GetItems().clear();

        for( BOARD_ITEM* member : group->GetItems() )
            newGroup->AddItem( ptrMap[ member ] );

        Add( newGroup );
    }

    // Copy auxiliary data: 3D_Drawings info
    m_3D_Drawings.clear();
    m_3D_Drawings = aOther.m_3D_Drawings;
    m_doc         = aOther.m_doc;
    m_keywords    = aOther.m_keywords;
    m_properties  = aOther.m_properties;

    m_initial_comments = aOther.m_initial_comments ?
                            new wxArrayString( *aOther.m_initial_comments ) : nullptr;

    return *this;
}


void FOOTPRINT::GetContextualTextVars( wxArrayString* aVars ) const
{
    aVars->push_back( wxT( "REFERENCE" ) );
    aVars->push_back( wxT( "VALUE" ) );
    aVars->push_back( wxT( "LAYER" ) );
}


bool FOOTPRINT::ResolveTextVar( wxString* token, int aDepth ) const
{
    if( token->IsSameAs( wxT( "REFERENCE" ) ) )
    {
        *token = m_reference->GetShownText( aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "VALUE" ) ) )
    {
        *token = m_value->GetShownText( aDepth + 1 );
        return true;
    }
    else if( token->IsSameAs( wxT( "LAYER" ) ) )
    {
        *token = GetLayerName();
        return true;
    }
    else if( m_properties.count( *token ) )
    {
        *token = m_properties.at( *token );
        return true;
    }

    return false;
}


void FOOTPRINT::ClearAllNets()
{
    // Force the ORPHANED dummy net info for all pads.
    // ORPHANED dummy net does not depend on a board
    for( PAD* pad : m_pads )
        pad->SetNetCode( NETINFO_LIST::ORPHANED );
}


void FOOTPRINT::Add( BOARD_ITEM* aBoardItem, ADD_MODE aMode )
{
    switch( aBoardItem->Type() )
    {
    case PCB_FP_TEXT_T:
        // Only user text can be added this way.
        wxASSERT( static_cast<FP_TEXT*>( aBoardItem )->GetType() == FP_TEXT::TEXT_is_DIVERS );
        KI_FALLTHROUGH;

    case PCB_FP_SHAPE_T:
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

    case PCB_FP_ZONE_T:
        if( aMode == ADD_MODE::APPEND )
            m_fp_zones.push_back( static_cast<FP_ZONE*>( aBoardItem ) );
        else
            m_fp_zones.insert( m_fp_zones.begin(), static_cast<FP_ZONE*>( aBoardItem ) );
        break;

    case PCB_GROUP_T:
        if( aMode == ADD_MODE::APPEND )
            m_fp_groups.push_back( static_cast<PCB_GROUP*>( aBoardItem ) );
        else
            m_fp_groups.insert( m_fp_groups.begin(), static_cast<PCB_GROUP*>( aBoardItem ) );
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
    case PCB_FP_TEXT_T:
        // Only user text can be removed this way.
        wxCHECK_RET(
                static_cast<FP_TEXT*>( aBoardItem )->GetType() == FP_TEXT::TEXT_is_DIVERS,
                "Please report this bug: Invalid remove operation on required text" );
        KI_FALLTHROUGH;

    case PCB_FP_SHAPE_T:
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

    case PCB_FP_ZONE_T:
        for( auto it = m_fp_zones.begin(); it != m_fp_zones.end(); ++it )
        {
            if( *it == static_cast<FP_ZONE*>( aBoardItem ) )
            {
                m_fp_zones.erase( it );
                break;
            }
        }

        break;

    case PCB_GROUP_T:
        for( auto it = m_fp_groups.begin(); it != m_fp_groups.end(); ++it )
        {
            if( *it == static_cast<PCB_GROUP*>( aBoardItem ) )
            {
                m_fp_groups.erase( it );
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
    EDA_RECT bbox = GetBoundingBox( false, false );

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
        switch( pad->GetAttribute() )
        {
        case PAD_ATTRIB::PTH:
            tht_count++;
            break;
        case PAD_ATTRIB::SMD:
            smd_count++;
            break;
        default:
            break;
        }
    }

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


EDA_RECT FOOTPRINT::GetFpPadsLocalBbox() const
{
    EDA_RECT area;

    // We want the bounding box of the footprint pads at rot 0, not flipped
    // Create such a image:
    FOOTPRINT dummy( *this );

    dummy.SetPosition( wxPoint( 0, 0 ) );

    if( dummy.IsFlipped() )
        dummy.Flip( wxPoint( 0, 0 ) , false );

    if( dummy.GetOrientation() )
        dummy.SetOrientation( 0 );

    for( PAD* pad : dummy.Pads() )
        area.Merge( pad->GetBoundingBox() );

    return area;
}


const EDA_RECT FOOTPRINT::GetBoundingBox() const
{
    return GetBoundingBox( true, true );
}


const EDA_RECT FOOTPRINT::GetBoundingBox( bool aIncludeText, bool aIncludeInvisibleText ) const
{
    const BOARD* board = GetBoard();

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

    EDA_RECT area;

    area.SetOrigin( m_pos );
    area.SetEnd( m_pos );
    area.Inflate( Millimeter2iu( 0.25 ) );   // Give a min size to the area

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() == PCB_FP_SHAPE_T )
            area.Merge( item->GetBoundingBox() );
    }

    for( PAD* pad : m_pads )
        area.Merge( pad->GetBoundingBox() );

    for( FP_ZONE* zone : m_fp_zones )
        area.Merge( zone->GetBoundingBox() );

    bool noDrawItems = ( m_drawings.empty() && m_pads.empty() && m_fp_zones.empty() );

    // Groups do not contribute to the rect, only their members
    if( aIncludeText || noDrawItems )
    {
        for( BOARD_ITEM* item : m_drawings )
        {
            if( item->Type() == PCB_FP_TEXT_T )
                area.Merge( item->GetBoundingBox() );
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
          || aIncludeInvisibleText || noDrawItems )
            area.Merge( m_value->GetBoundingBox() );

        if( ( m_reference->IsVisible() && refLayerIsVisible )
          || aIncludeInvisibleText || noDrawItems )
            area.Merge( m_reference->GetBoundingBox() );
    }

    if( board )
    {
        if( ( aIncludeText && aIncludeInvisibleText ) || noDrawItems )
        {
            m_boundingBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedBoundingBox = area;
        }
        else if( aIncludeText )
        {
            m_visibleBBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedVisibleBBox = area;
        }
        else
        {
            m_textExcludedBBoxCacheTimeStamp = board->GetTimeStamp();
            m_cachedTextExcludedBBox = area;
        }
    }

    return area;
}


SHAPE_POLY_SET FOOTPRINT::GetBoundingHull() const
{
    const BOARD* board = GetBoard();

    if( board )
    {
        if( m_hullCacheTimeStamp >= board->GetTimeStamp() )
            return m_cachedHull;
    }

    SHAPE_POLY_SET rawPolys;
    SHAPE_POLY_SET hull;

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() == PCB_FP_SHAPE_T )
        {
            item->TransformShapeWithClearanceToPolygon( rawPolys, UNDEFINED_LAYER, 0, ARC_LOW_DEF,
                                                        ERROR_OUTSIDE );
        }

        // We intentionally exclude footprint text from the bounding hull.
    }

    for( PAD* pad : m_pads )
    {
        pad->TransformShapeWithClearanceToPolygon( rawPolys, UNDEFINED_LAYER, 0, ARC_LOW_DEF,
                                                   ERROR_OUTSIDE );
        // In case hole is larger than pad
        pad->TransformHoleWithClearanceToPolygon( rawPolys, 0, ARC_LOW_DEF, ERROR_OUTSIDE );
    }

    for( FP_ZONE* zone : m_fp_zones )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            SHAPE_POLY_SET layerPoly = zone->GetFilledPolysList( layer );

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
        const int halfsize = Millimeter2iu( 1.0 );

        rawPolys.NewOutline();

        // add a square:
        rawPolys.Append( GetPosition().x - halfsize,  GetPosition().y - halfsize );
        rawPolys.Append( GetPosition().x + halfsize,  GetPosition().y - halfsize );
        rawPolys.Append( GetPosition().x + halfsize,  GetPosition().y + halfsize );
        rawPolys.Append( GetPosition().x - halfsize,  GetPosition().y + halfsize );
    }

    std::vector<wxPoint> convex_hull;
    BuildConvexHull( convex_hull, rawPolys );

    m_cachedHull.RemoveAllContours();
    m_cachedHull.NewOutline();

    for( const wxPoint& pt : convex_hull )
        m_cachedHull.Append( pt );

    if( board )
        m_hullCacheTimeStamp = board->GetTimeStamp();

    return m_cachedHull;
}


void FOOTPRINT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg, msg2;

    aList.emplace_back( m_reference->GetShownText(), m_value->GetShownText() );

    if( aFrame->IsType( FRAME_FOOTPRINT_VIEWER )
        || aFrame->IsType( FRAME_FOOTPRINT_VIEWER_MODAL )
        || aFrame->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        wxDateTime date( static_cast<time_t>( m_lastEditTime ) );

        // Date format: see http://www.cplusplus.com/reference/ctime/strftime
        if( m_lastEditTime && date.IsValid() )
            msg = date.Format( wxT( "%b %d, %Y" ) ); // Abbreviated_month_name Day, Year
        else
            msg = _( "Unknown" );

        aList.emplace_back( _( "Last Change" ), msg );
    }
    else if( aFrame->IsType( FRAME_PCB_EDITOR ) )
    {
        aList.emplace_back( _( "Board Side" ), IsFlipped() ? _( "Back (Flipped)" ) : _( "Front" ) );
    }

    auto addToken = []( wxString* aStr, const wxString& aAttr )
                    {
                        if( !aStr->IsEmpty() )
                            *aStr += wxT( ", " );

                        *aStr += aAttr;
                    };

    wxString status;
    wxString attrs;

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        addToken( &status, _( "Locked" ) );

    if( m_fpStatus & FP_is_PLACED )
        addToken( &status, _( "autoplaced" ) );

    if( m_attributes & FP_BOARD_ONLY )
        addToken( &attrs, _( "not in schematic" ) );

    if( m_attributes & FP_EXCLUDE_FROM_POS_FILES )
        addToken( &attrs, _( "exclude from pos files" ) );

    if( m_attributes & FP_EXCLUDE_FROM_BOM )
        addToken( &attrs, _( "exclude from BOM" ) );

    aList.emplace_back( _( "Status: " ) + status, _( "Attributes:" ) + wxS( " " ) + attrs );

    aList.emplace_back( _( "Rotation" ), wxString::Format( "%.4g", GetOrientationDegrees() ) );

    msg.Printf( _( "Footprint: %s" ), m_fpid.GetUniStringLibId() );
    msg2.Printf( _( "3D-Shape: %s" ), m_3D_Drawings.empty() ? _( "<none>" )
                                                            : m_3D_Drawings.front().m_Filename );
    aList.emplace_back( msg, msg2 );

    msg.Printf( _( "Doc: %s" ), m_doc );
    msg2.Printf( _( "Keywords: %s" ), m_keywords );
    aList.emplace_back( msg, msg2 );
}


bool FOOTPRINT::IsOnLayer( PCB_LAYER_ID aLayer ) const
{
    // If we have any pads, fall back on normal checking
    if( !m_pads.empty() )
        return m_layer == aLayer;

    // No pads?  Check if this entire footprint exists on the given layer
    for( FP_ZONE* zone : m_fp_zones )
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


bool FOOTPRINT::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox( false, false );
    return rect.Inflate( aAccuracy ).Contains( aPosition );
}


bool FOOTPRINT::HitTestAccurate( const wxPoint& aPosition, int aAccuracy ) const
{
    return GetBoundingHull().Collide( aPosition, aAccuracy );
}


bool FOOTPRINT::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
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
        if( m_pads.empty() && m_fp_zones.empty() && m_drawings.empty() )
            return GetBoundingBox( true, false ).Intersects( arect );

        // Determine if any elements in the FOOTPRINT intersect the rect
        for( PAD* pad : m_pads )
        {
            if( pad->HitTest( arect, false, 0 ) )
                return true;
        }

        for( FP_ZONE* zone : m_fp_zones )
        {
            if( zone->HitTest( arect, false, 0 ) )
                return true;
        }

        for( BOARD_ITEM* item : m_drawings )
        {
            if( item->Type() != PCB_FP_TEXT_T && item->HitTest( arect, false, 0 ) )
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


PAD* FOOTPRINT::GetPad( const wxPoint& aPosition, LSET aLayerMask )
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


PAD* FOOTPRINT::GetTopLeftPad()
{
    PAD* topLeftPad = m_pads.front();

    for( PAD* p : m_pads )
    {
        wxPoint pnt = p->GetPosition(); // GetPosition() returns the center of the pad

        if( ( pnt.x < topLeftPad->GetPosition().x ) ||
            ( topLeftPad->GetPosition().x == pnt.x && pnt.y < topLeftPad->GetPosition().y ) )
        {
            topLeftPad = p;
        }
    }

    return topLeftPad;
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
SEARCH_RESULT FOOTPRINT::Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] )
{
    KICAD_T        stype;
    SEARCH_RESULT  result = SEARCH_RESULT::CONTINUE;
    const KICAD_T* p    = scanTypes;
    bool           done = false;

#if 0 && defined(DEBUG)
    std::cout << GetClass().mb_str() << ' ';
#endif

    while( !done )
    {
        stype = *p;

        switch( stype )
        {
        case PCB_FOOTPRINT_T:
            result = inspector( this, testData );  // inspect me
            ++p;
            break;

        case PCB_PAD_T:
            result = IterateForward<PAD*>( m_pads, inspector, testData, p );
            ++p;
            break;

        case PCB_FP_ZONE_T:
            result = IterateForward<FP_ZONE*>( m_fp_zones, inspector, testData, p );
            ++p;
            break;

        case PCB_FP_TEXT_T:
            result = inspector( m_reference, testData );

            if( result == SEARCH_RESULT::QUIT )
                break;

            result = inspector( m_value, testData );

            if( result == SEARCH_RESULT::QUIT )
                break;

            // Intentionally fall through since m_Drawings can hold PCB_FP_SHAPE_T also
            KI_FALLTHROUGH;

        case PCB_FP_SHAPE_T:
            result = IterateForward<BOARD_ITEM*>( m_drawings, inspector, testData, p );

            // skip over any types handled in the above call.
            for( ; ; )
            {
                switch( stype = *++p )
                {
                case PCB_FP_TEXT_T:
                case PCB_FP_SHAPE_T:
                    continue;

                default:
                    ;
                }

                break;
            }

            break;

        case PCB_GROUP_T:
            result = IterateForward<PCB_GROUP*>( m_fp_groups, inspector, testData, p );
            ++p;
            break;

        default:
            done = true;
            break;
        }

        if( result == SEARCH_RESULT::QUIT )
            break;
    }

    return result;
}


wxString FOOTPRINT::GetSelectMenuText( EDA_UNITS aUnits ) const
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

        for( FP_ZONE* zone : m_fp_zones )
            aFunction( static_cast<FP_ZONE*>( zone ) );

        for( PCB_GROUP* group : m_fp_groups )
            aFunction( static_cast<PCB_GROUP*>( group ) );

        for( BOARD_ITEM* drawing : m_drawings )
            aFunction( static_cast<BOARD_ITEM*>( drawing ) );

        aFunction( static_cast<BOARD_ITEM*>( m_reference ) );
        aFunction( static_cast<BOARD_ITEM*>( m_value ) );
    }
    catch( std::bad_function_call& )
    {
        wxFAIL_MSG( "Error running FOOTPRINT::RunOnChildren" );
    }
}


void FOOTPRINT::GetAllDrawingLayers( int aLayers[], int& aCount, bool aIncludePads ) const
{
    std::unordered_set<int> layers;

    for( BOARD_ITEM* item : m_drawings )
        layers.insert( static_cast<int>( item->GetLayer() ) );

    if( aIncludePads )
    {
        for( PAD* pad : m_pads )
        {
            int pad_layers[KIGFX::VIEW::VIEW_MAX_LAYERS], pad_layers_count;
            pad->ViewGetLayers( pad_layers, pad_layers_count );

            for( int i = 0; i < pad_layers_count; i++ )
                layers.insert( pad_layers[i] );
        }
    }

    aCount = layers.size();
    int i = 0;

    for( int layer : layers )
        aLayers[i++] = layer;
}


void FOOTPRINT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 2;
    aLayers[0] = LAYER_ANCHOR;

    switch( m_layer )
    {
    default:
        wxASSERT_MSG( false, "Illegal layer" );    // do you really have footprints placed on
                                                   // other layers?
        KI_FALLTHROUGH;

    case F_Cu:
        aLayers[1] = LAYER_MOD_FR;
        break;

    case B_Cu:
        aLayers[1] = LAYER_MOD_BK;
        break;
    }

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
    EDA_RECT area = GetBoundingBox( true, true );

    // Add the Clearance shape size: (shape around the pads when the clearance is shown.  Not
    // optimized, but the draw cost is small (perhaps smaller than optimization).
    const BOARD* board = GetBoard();

    if( board )
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
    // TODO: Unify forbidden character lists
    static const wxChar invalidChars[] = wxT("%$<>\t\n\r\"\\/:");
    static const wxChar invalidCharsReadable[] = wxT("% $ < > 'tab' 'return' 'line feed' \\ \" / :");

    if( aUserReadable )
        return invalidCharsReadable;
    else
        return invalidChars;
}


void FOOTPRINT::Move( const wxPoint& aMoveVector )
{
    wxPoint newpos = m_pos + aMoveVector;
    SetPosition( newpos );
}


void FOOTPRINT::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    double  orientation = GetOrientation();
    double  newOrientation = orientation + aAngle;
    wxPoint newpos = m_pos;
    RotatePoint( &newpos, aRotCentre, aAngle );
    SetPosition( newpos );
    SetOrientation( newOrientation );

    m_reference->KeepUpright( orientation, newOrientation );
    m_value->KeepUpright( orientation, newOrientation );

    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() == PCB_FP_TEXT_T )
            static_cast<FP_TEXT*>( item )->KeepUpright( orientation, newOrientation  );
    }

    m_boundingBoxCacheTimeStamp = 0;
    m_visibleBBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;
    m_hullCacheTimeStamp = 0;
}


void FOOTPRINT::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    // Move footprint to its final position:
    wxPoint finalPos = m_pos;

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
    SetLayer( FlipLayer( GetLayer() ) );

    // Reverse mirror orientation.
    m_orient = -m_orient;

    NORMALIZE_ANGLE_180( m_orient );

    // Mirror pads to other side of board.
    for( PAD* pad : m_pads )
        pad->Flip( m_pos, false );

    // Mirror zones to other side of board.
    for( ZONE* zone : m_fp_zones )
        zone->Flip( m_pos, false );

    // Mirror reference and value.
    m_reference->Flip( m_pos, false );
    m_value->Flip( m_pos, false );

    // Reverse mirror footprint graphics and texts.
    for( BOARD_ITEM* item : m_drawings )
    {
        switch( item->Type() )
        {
        case PCB_FP_SHAPE_T:
            static_cast<FP_SHAPE*>( item )->Flip( m_pos, false );
            break;

        case PCB_FP_TEXT_T:
            static_cast<FP_TEXT*>( item )->Flip( m_pos, false );
            break;

        default:
            wxMessageBox( wxT( "FOOTPRINT::Flip() error: Unknown Draw Type" ) );
            break;
        }
    }

    // Now rotate 180 deg if required
    if( aFlipLeftRight )
        Rotate( aCentre, 1800.0 );

    m_boundingBoxCacheTimeStamp = 0;
    m_visibleBBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;

    m_cachedHull.Mirror( aFlipLeftRight, !aFlipLeftRight, m_pos );

    std::swap( m_poly_courtyard_front, m_poly_courtyard_back );
}


void FOOTPRINT::SetPosition( const wxPoint& aPos )
{
    wxPoint delta = aPos - m_pos;

    m_pos += delta;

    m_reference->EDA_TEXT::Offset( delta );
    m_value->EDA_TEXT::Offset( delta );

    for( PAD* pad : m_pads )
        pad->SetPosition( pad->GetPosition() + delta );

    for( ZONE* zone : m_fp_zones )
        zone->Move( delta );

    for( BOARD_ITEM* item : m_drawings )
    {
        switch( item->Type() )
        {
        case PCB_FP_SHAPE_T:
        {
            FP_SHAPE* shape = static_cast<FP_SHAPE*>( item );
            shape->SetDrawCoord();
            break;
        }

        case PCB_FP_TEXT_T:
        {
            FP_TEXT* text = static_cast<FP_TEXT*>( item );
            text->EDA_TEXT::Offset( delta );
            break;
        }

        default:
            wxMessageBox( wxT( "Draw type undefined." ) );
            break;
        }
    }

    m_cachedBoundingBox.Move( delta );
    m_cachedVisibleBBox.Move( delta );
    m_cachedTextExcludedBBox.Move( delta );
    m_cachedHull.Move( delta );
}


void FOOTPRINT::MoveAnchorPosition( const wxPoint& aMoveVector )
{
    /* Move the reference point of the footprint
     * the footprints elements (pads, outlines, edges .. ) are moved
     * but:
     * - the footprint position is not modified.
     * - the relative (local) coordinates of these items are modified
     * - Draw coordinates are updated
     */


    // Update (move) the relative coordinates relative to the new anchor point.
    wxPoint moveVector = aMoveVector;
    RotatePoint( &moveVector, -GetOrientation() );

    // Update of the reference and value.
    m_reference->SetPos0( m_reference->GetPos0() + moveVector );
    m_reference->SetDrawCoord();
    m_value->SetPos0( m_value->GetPos0() + moveVector );
    m_value->SetDrawCoord();

    // Update the pad local coordinates.
    for( PAD* pad : m_pads )
    {
        pad->SetPos0( pad->GetPos0() + moveVector );
        pad->SetDrawCoord();
    }

    // Update the draw element coordinates.
    for( BOARD_ITEM* item : GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_FP_SHAPE_T:
        {
            FP_SHAPE* shape = static_cast<FP_SHAPE*>( item );
            shape->Move( moveVector );
        }
            break;

        case PCB_FP_TEXT_T:
        {
            FP_TEXT* text = static_cast<FP_TEXT*>( item );
            text->SetPos0( text->GetPos0() + moveVector );
            text->SetDrawCoord();
        }
            break;

        default:
            break;
        }
    }

    // Update the keepout zones
    for( ZONE* zone : Zones() )
    {
        zone->Move( moveVector );
    }

    // Update the 3D models
    for( FP_3DMODEL& model : Models() )
    {
        model.m_Offset.x += Iu2Millimeter( moveVector.x );
        model.m_Offset.y -= Iu2Millimeter( moveVector.y );
    }

    m_cachedBoundingBox.Move( moveVector );
    m_cachedVisibleBBox.Move( moveVector );
    m_cachedTextExcludedBBox.Move( moveVector );
    m_cachedHull.Move( moveVector );
}


void FOOTPRINT::SetOrientation( double aNewAngle )
{
    double angleChange = aNewAngle - m_orient;  // change in rotation

    NORMALIZE_ANGLE_180( aNewAngle );

    m_orient = aNewAngle;

    for( PAD* pad : m_pads )
    {
        pad->SetOrientation( pad->GetOrientation() + angleChange );
        pad->SetDrawCoord();
    }

    for( ZONE* zone : m_fp_zones )
    {
        zone->Rotate( GetPosition(), angleChange );
    }

    // Update of the reference and value.
    m_reference->SetDrawCoord();
    m_value->SetDrawCoord();

    // Displace contours and text of the footprint.
    for( BOARD_ITEM* item : m_drawings )
    {
        if( item->Type() == PCB_FP_SHAPE_T )
        {
            static_cast<FP_SHAPE*>( item )->SetDrawCoord();
        }
        else if( item->Type() == PCB_FP_TEXT_T )
        {
            static_cast<FP_TEXT*>( item )->SetDrawCoord();
        }
    }

    m_boundingBoxCacheTimeStamp = 0;
    m_visibleBBoxCacheTimeStamp = 0;
    m_textExcludedBBoxCacheTimeStamp = 0;

    m_cachedHull.Rotate( -DECIDEG2RAD( angleChange ), GetPosition() );
}


BOARD_ITEM* FOOTPRINT::Duplicate() const
{
    FOOTPRINT* dupe = (FOOTPRINT*) Clone();
    const_cast<KIID&>( dupe->m_Uuid ) = KIID();

    dupe->RunOnChildren( [&]( BOARD_ITEM* child )
                         {
                             const_cast<KIID&>( child->m_Uuid ) = KIID();
                         });

    return static_cast<BOARD_ITEM*>( dupe );
}


BOARD_ITEM* FOOTPRINT::DuplicateItem( const BOARD_ITEM* aItem, bool aAddToFootprint )
{
    BOARD_ITEM* new_item = nullptr;
    FP_ZONE* new_zone = nullptr;

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

    case PCB_FP_ZONE_T:
    {
        new_zone = new FP_ZONE( *static_cast<const FP_ZONE*>( aItem ) );
        const_cast<KIID&>( new_zone->m_Uuid ) = KIID();

        if( aAddToFootprint )
            m_fp_zones.push_back( new_zone );

        new_item = new_zone;
        break;
    }

    case PCB_FP_TEXT_T:
    {
        FP_TEXT* new_text = new FP_TEXT( *static_cast<const FP_TEXT*>( aItem ) );
        const_cast<KIID&>( new_text->m_Uuid ) = KIID();

        if( new_text->GetType() == FP_TEXT::TEXT_is_REFERENCE )
        {
            new_text->SetText( wxT( "${REFERENCE}" ) );
            new_text->SetType( FP_TEXT::TEXT_is_DIVERS );
        }
        else if( new_text->GetType() == FP_TEXT::TEXT_is_VALUE )
        {
            new_text->SetText( wxT( "${VALUE}" ) );
            new_text->SetType( FP_TEXT::TEXT_is_DIVERS );
        }

        if( aAddToFootprint )
            Add( new_text );

        new_item = new_text;

        break;
    }

    case PCB_FP_SHAPE_T:
    {
        FP_SHAPE* new_shape = new FP_SHAPE( *static_cast<const FP_SHAPE*>( aItem ) );
        const_cast<KIID&>( new_shape->m_Uuid ) = KIID();

        if( aAddToFootprint )
            Add( new_shape );

        new_item = new_shape;
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
        wxFAIL_MSG( "Duplication not supported for items of class " + aItem->GetClass() );
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

    while( usedNumbers.count( wxString::Format( "%s%d", prefix, num ) ) )
        num++;

    return wxString::Format( "%s%d", prefix, num );
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
    else if( aItem->Type() == PCB_FP_TEXT_T )
    {
        const FP_TEXT* text = static_cast<const FP_TEXT*>( aItem );

        text->TransformTextShapeWithClearanceToPolygon( poly, UNDEFINED_LAYER, textMargin,
                                                        ARC_LOW_DEF, ERROR_OUTSIDE );
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
            aItem->TransformShapeWithClearanceToPolygon( poly, UNDEFINED_LAYER, 0,
                                                         ARC_LOW_DEF, ERROR_OUTSIDE );
        }
    }
    else if( aItem->Type() == PCB_TRACE_T || aItem->Type() == PCB_ARC_T )
    {
        double width = static_cast<const PCB_TRACK*>( aItem )->GetWidth();
        return width * width;
    }
    else
    {
        aItem->TransformShapeWithClearanceToPolygon( poly, UNDEFINED_LAYER, 0,
                                                     ARC_LOW_DEF, ERROR_OUTSIDE );
    }

    return polygonArea( poly );
}


double FOOTPRINT::CoverageRatio( const GENERAL_COLLECTOR& aCollector ) const
{
    int textMargin = KiROUND( 5 * aCollector.GetGuide()->OnePixelInIU() );

    SHAPE_POLY_SET footprintRegion( GetBoundingHull() );
    SHAPE_POLY_SET coveredRegion;

    TransformPadsWithClearanceToPolygon( coveredRegion, UNDEFINED_LAYER, 0, ARC_LOW_DEF,
                                         ERROR_OUTSIDE );

    TransformFPShapesWithClearanceToPolygon( coveredRegion, UNDEFINED_LAYER, textMargin,
                                             ARC_LOW_DEF, ERROR_OUTSIDE,
                                             true, /* include text */
                                             false /* include shapes */ );

    for( int i = 0; i < aCollector.GetCount(); ++i )
    {
        const BOARD_ITEM* item = aCollector[i];

        switch( item->Type() )
        {
        case PCB_FP_TEXT_T:
        case PCB_FP_SHAPE_T:
            if( item->GetParent() != this )
            {
                item->TransformShapeWithClearanceToPolygon( coveredRegion, UNDEFINED_LAYER, 0,
                                                            ARC_LOW_DEF, ERROR_OUTSIDE );
            }
            break;

        case PCB_TEXT_T:
        case PCB_SHAPE_T:
        case PCB_TRACE_T:
        case PCB_ARC_T:
        case PCB_VIA_T:
            item->TransformShapeWithClearanceToPolygon( coveredRegion, UNDEFINED_LAYER, 0,
                                                        ARC_LOW_DEF, ERROR_OUTSIDE );
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


std::shared_ptr<SHAPE> FOOTPRINT::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = std::make_shared<SHAPE_COMPOUND>();

    // There are several possible interpretations here:
    // 1) the bounding box (without or without invisible items)
    // 2) just the pads and "edges" (ie: non-text graphic items)
    // 3) the courtyard

    // We'll go with (2) for now....

    for( PAD* pad : Pads() )
        shape->AddShape( pad->GetEffectiveShape( aLayer )->Clone() );

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( item->Type() == PCB_FP_SHAPE_T )
            shape->AddShape( item->GetEffectiveShape( aLayer )->Clone() );
    }

    return shape;
}


void FOOTPRINT::BuildPolyCourtyards( OUTLINE_ERROR_HANDLER* aErrorHandler )
{
    m_poly_courtyard_front.RemoveAllContours();
    m_poly_courtyard_back.RemoveAllContours();
    ClearFlags( MALFORMED_COURTYARDS );

    // Build the courtyard area from graphic items on the courtyard.
    // Only PCB_FP_SHAPE_T have meaning, graphic texts are ignored.
    // Collect items:
    std::vector<PCB_SHAPE*> list_front;
    std::vector<PCB_SHAPE*> list_back;

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( item->GetLayer() == B_CrtYd && item->Type() == PCB_FP_SHAPE_T )
            list_back.push_back( static_cast<PCB_SHAPE*>( item ) );

        if( item->GetLayer() == F_CrtYd && item->Type() == PCB_FP_SHAPE_T )
            list_front.push_back( static_cast<PCB_SHAPE*>( item ) );
    }

    if( !list_front.size() && !list_back.size() )
        return;

    int errorMax = Millimeter2iu( 0.02 );         // max error for polygonization
    int chainingEpsilon = Millimeter2iu( 0.02 );  // max dist from one endPt to next startPt

    if( ConvertOutlineToPolygon( list_front, m_poly_courtyard_front, errorMax, chainingEpsilon,
                                 aErrorHandler ) )
    {
        // Touching courtyards, or courtyards -at- the clearance distance are legal.
        m_poly_courtyard_front.Inflate( -1, SHAPE_POLY_SET::CHAMFER_ACUTE_CORNERS );

        m_poly_courtyard_front.CacheTriangulation( false );
    }
    else
    {
        SetFlags( MALFORMED_F_COURTYARD );
    }

    if( ConvertOutlineToPolygon( list_back, m_poly_courtyard_back, errorMax, chainingEpsilon,
                                 aErrorHandler ) )
    {
        // Touching courtyards, or courtyards -at- the clearance distance are legal.
        m_poly_courtyard_back.Inflate( -1, SHAPE_POLY_SET::CHAMFER_ACUTE_CORNERS );

        m_poly_courtyard_back.CacheTriangulation( false );
    }
    else
    {
        SetFlags( MALFORMED_B_COURTYARD );
    }
}


void FOOTPRINT::CheckFootprintAttributes( const std::function<void( const wxString& msg )>* aErrorHandler )
{

    int      likelyAttr = GetLikelyAttribute();
    int      setAttr = ( GetAttributes() & ( FP_SMD | FP_THROUGH_HOLE ) );

    // This is only valid if the footprint doesn't have FP_SMD and FP_THROUGH_HOLE set
    // Which is, unfortunately, possible in theory but not in the UI (I think)
    if( aErrorHandler && likelyAttr != setAttr )
    {
        wxString msg;

        if( likelyAttr == FP_THROUGH_HOLE )
        {
            msg.Printf( _( "Expected \"Through hole\" type but set to \"%s\"" ), GetTypeName() );
        }
        else if( likelyAttr == FP_SMD )
        {
            msg.Printf( _( "Expected \"SMD\" type but set to \"%s\"" ), GetTypeName() );
        }
        else
        {
            msg.Printf( _( "Expected \"Other\" type but set to \"%s\"" ), GetTypeName() );
        }

        msg = "(" + msg + ")";

        (*aErrorHandler)( msg );
    }
}

void FOOTPRINT::CheckFootprintTHPadNoHoles(
                const std::function<void( const wxString& msg, const wxPoint& position )>*
                aErrorHandler )
{
    if( aErrorHandler == nullptr )
        return;

    for( const PAD* pad: Pads() )
    {

        if( pad->GetAttribute() != PAD_ATTRIB::PTH
            && pad->GetAttribute() != PAD_ATTRIB::NPTH )
            continue;

        if( pad->GetDrillSizeX() < 1 || pad->GetDrillSizeX() < 1 )
        {
            wxString msg;
            msg.Printf( _( "(pad \"%s\")" ), pad->GetNumber() );

            (*aErrorHandler)( msg, pad->GetPosition() );
        }
    }
}


void FOOTPRINT::SwapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == PCB_FOOTPRINT_T );

    std::swap( *((FOOTPRINT*) this), *((FOOTPRINT*) aImage) );
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


bool FOOTPRINT::cmp_drawings::operator()( const BOARD_ITEM* aFirst,
                                          const BOARD_ITEM* aSecond ) const
{
    if( aFirst->Type() != aSecond->Type() )
        return aFirst->Type() < aSecond->Type();

    if( aFirst->GetLayer() != aSecond->GetLayer() )
        return aFirst->GetLayer() < aSecond->GetLayer();

    if( aFirst->Type() == PCB_FP_SHAPE_T )
    {
        const FP_SHAPE* dwgA = static_cast<const FP_SHAPE*>( aFirst );
        const FP_SHAPE* dwgB = static_cast<const FP_SHAPE*>( aSecond );

        if( dwgA->GetShape() != dwgB->GetShape() )
            return dwgA->GetShape() < dwgB->GetShape();
    }

    if( aFirst->m_Uuid != aSecond->m_Uuid ) // shopuld be always the case foer valid boards
        return aFirst->m_Uuid < aSecond->m_Uuid;

    return aFirst < aSecond;
}


bool FOOTPRINT::cmp_pads::operator()( const PAD* aFirst, const PAD* aSecond ) const
{
    if( aFirst->GetNumber() != aSecond->GetNumber() )
        return StrNumCmp( aFirst->GetNumber(), aSecond->GetNumber() ) < 0;

    if( aFirst->m_Uuid != aSecond->m_Uuid ) // shopuld be always the case foer valid boards
        return aFirst->m_Uuid < aSecond->m_Uuid;

    return aFirst < aSecond;
}


void FOOTPRINT::TransformPadsWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                     PCB_LAYER_ID aLayer, int aClearance,
                                                     int aMaxError, ERROR_LOC aErrorLoc,
                                                     bool aSkipNPTHPadsWihNoCopper,
                                                     bool aSkipPlatedPads,
                                                     bool aSkipNonPlatedPads ) const
{
    for( const PAD* pad : m_pads )
    {
        if( aLayer != UNDEFINED_LAYER && !pad->IsOnLayer(aLayer) )
            continue;

        if( !pad->FlashLayer( aLayer ) && IsCopperLayer( aLayer ) )
            continue;

        // NPTH pads are not drawn on layers if the shape size and pos is the same
        // as their hole:
        if( aSkipNPTHPadsWihNoCopper && pad->GetAttribute() == PAD_ATTRIB::NPTH )
        {
            if( pad->GetDrillSize() == pad->GetSize() && pad->GetOffset() == wxPoint( 0, 0 ) )
            {
                switch( pad->GetShape() )
                {
                case PAD_SHAPE::CIRCLE:
                    if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                        continue;

                    break;

                case PAD_SHAPE::OVAL:
                    if( pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE )
                        continue;

                    break;

                default:
                    break;
                }
            }
        }

        const bool isPlated = ( ( aLayer == F_Cu ) && pad->FlashLayer( F_Mask ) ) ||
                              ( ( aLayer == B_Cu ) && pad->FlashLayer( B_Mask ) );

        if( aSkipPlatedPads && isPlated )
            continue;

        if( aSkipNonPlatedPads && !isPlated )
            continue;

        wxSize clearance( aClearance, aClearance );

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            clearance.x += pad->GetSolderMaskMargin();
            clearance.y += pad->GetSolderMaskMargin();
            break;

        case F_Paste:
        case B_Paste:
            clearance += pad->GetSolderPasteMargin();
            break;

        default:
            break;
        }

        // Our standard TransformShapeWithClearanceToPolygon() routines can't handle differing
        // x:y clearance values (which get generated when a relative paste margin is used with
        // an oblong pad).  So we apply this huge hack and fake a larger pad to run the transform
        // on.
        // Of course being a hack it falls down when dealing with custom shape pads (where the
        // size is only the size of the anchor), so for those we punt and just use clearance.x.

        if( ( clearance.x < 0 || clearance.x != clearance.y )
                && pad->GetShape() != PAD_SHAPE::CUSTOM )
        {
            PAD dummy( *pad );
            dummy.SetSize( pad->GetSize() + clearance + clearance );
            dummy.TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, 0,
                                                        aMaxError, aErrorLoc );
        }
        else
        {
            pad->TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, clearance.x,
                                                       aMaxError, aErrorLoc );
        }
    }
}


void FOOTPRINT::TransformFPShapesWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                         PCB_LAYER_ID aLayer, int aClearance,
                                                         int aError, ERROR_LOC aErrorLoc,
                                                         bool aIncludeText,
                                                         bool aIncludeShapes ) const
{
    std::vector<FP_TEXT*> texts;  // List of FP_TEXT to convert

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( item->Type() == PCB_FP_TEXT_T && aIncludeText )
        {
            FP_TEXT* text = static_cast<FP_TEXT*>( item );

            if( aLayer != UNDEFINED_LAYER && text->GetLayer() == aLayer && text->IsVisible() )
                texts.push_back( text );
        }

        if( item->Type() == PCB_FP_SHAPE_T && aIncludeShapes )
        {
            const FP_SHAPE* outline = static_cast<FP_SHAPE*>( item );

            if( aLayer != UNDEFINED_LAYER && outline->GetLayer() == aLayer )
            {
                outline->TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, 0,
                                                               aError, aErrorLoc );
            }
        }
    }

    if( aIncludeText )
    {
        if( Reference().GetLayer() == aLayer && Reference().IsVisible() )
            texts.push_back( &Reference() );

        if( Value().GetLayer() == aLayer && Value().IsVisible() )
            texts.push_back( &Value() );
    }

    for( const FP_TEXT* text : texts )
    {
        text->TransformTextShapeWithClearanceToPolygon( aCornerBuffer, aLayer, aClearance,
                                                        aError, aErrorLoc );
    }
}


static struct FOOTPRINT_DESC
{
    FOOTPRINT_DESC()
    {
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

        auto layer = new PROPERTY_ENUM<FOOTPRINT, PCB_LAYER_ID, BOARD_ITEM>( _HKI( "Layer" ),
                    &FOOTPRINT::SetLayer, &FOOTPRINT::GetLayer );
        layer->SetChoices( fpLayers );
        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ), layer );

        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Reference" ),
                    &FOOTPRINT::SetReference, &FOOTPRINT::GetReference ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, wxString>( _HKI( "Value" ),
                    &FOOTPRINT::SetValue, &FOOTPRINT::GetValue ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, double>( _HKI( "Orientation" ),
                    &FOOTPRINT::SetOrientationDegrees, &FOOTPRINT::GetOrientationDegrees,
                    PROPERTY_DISPLAY::DEGREE ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, int>( _HKI( "Clearance Override" ),
                    &FOOTPRINT::SetLocalClearance, &FOOTPRINT::GetLocalClearance,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, int>( _HKI( "Solderpaste Margin Override" ),
                    &FOOTPRINT::SetLocalSolderPasteMargin, &FOOTPRINT::GetLocalSolderPasteMargin,
                    PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT,
                             double>( _HKI( "Solderpaste Margin Ratio Override" ),
                                      &FOOTPRINT::SetLocalSolderPasteMarginRatio,
                                      &FOOTPRINT::GetLocalSolderPasteMarginRatio ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, int>( _HKI( "Thermal Relief Width" ),
                                                           &FOOTPRINT::SetThermalWidth,
                                                           &FOOTPRINT::GetThermalWidth,
                                                           PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<FOOTPRINT, int>( _HKI( "Thermal Relief Gap" ),
                                                           &FOOTPRINT::SetThermalGap,
                                                           &FOOTPRINT::GetThermalGap,
                                                           PROPERTY_DISPLAY::DISTANCE ) );
        // TODO zone connection, FPID?
    }
} _FOOTPRINT_DESC;

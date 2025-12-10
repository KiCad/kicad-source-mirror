/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <pybind11/pybind11.h>

#include <wx/debug.h>
#include <wx/msgdlg.h>
#include <i18n_utility.h>
#include <macros.h>
#include <board.h>
#include <board_design_settings.h>
#include <lset.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <footprint.h>
#include <font/font.h>


bool BOARD_ITEM::IsGroupableType() const
{
    switch ( Type() )
    {
    case PCB_FOOTPRINT_T:
    case PCB_PAD_T:
    case PCB_SHAPE_T:
    case PCB_REFERENCE_IMAGE_T:
    case PCB_FIELD_T:
    case PCB_TEXT_T:
    case PCB_TEXTBOX_T:
    case PCB_TABLE_T:
    case PCB_GROUP_T:
    case PCB_GENERATOR_T:
    case PCB_TRACE_T:
    case PCB_VIA_T:
    case PCB_ARC_T:
    case PCB_DIMENSION_T:
    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_LEADER_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_ZONE_T:
    case PCB_BARCODE_T:
        return true;
    default:
        return false;
    }
}


void BOARD_ITEM::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther, /* void */ );
    *this = *aOther;
}


const BOARD* BOARD_ITEM::GetBoard() const
{
    if( Type() == PCB_T )
        return static_cast<const BOARD*>( this );

    return static_cast<const BOARD*>( findParent( PCB_T ) );
}


BOARD* BOARD_ITEM::GetBoard()
{
    if( Type() == PCB_T )
        return static_cast<BOARD*>( this );

    return static_cast<BOARD*>( findParent( PCB_T ) );
}


FOOTPRINT* BOARD_ITEM::GetParentFootprint() const
{
    return static_cast<FOOTPRINT*>( findParent( PCB_FOOTPRINT_T ) );
}


bool BOARD_ITEM::IsLocked() const
{
    if( EDA_GROUP* group = GetParentGroup() )
    {
        if( group->AsEdaItem()->IsLocked() )
            return true;
    }

    if( !GetBoard() || GetBoard()->GetBoardUse() == BOARD_USE::FPHOLDER )
        return false;

    return m_isLocked;
}


STROKE_PARAMS BOARD_ITEM::GetStroke() const
{
    wxFAIL_MSG( wxString( "GetStroke() not defined by " ) + GetClass() );

    return STROKE_PARAMS( pcbIUScale.mmToIU( DEFAULT_LINE_WIDTH ) );
}


void BOARD_ITEM::SetStroke( const STROKE_PARAMS& aStroke )
{
    wxFAIL_MSG( wxString( "SetStroke() not defined by " ) + GetClass() );
}


const KIFONT::METRICS& BOARD_ITEM::GetFontMetrics() const
{
    return KIFONT::METRICS::Default();
}


int BOARD_ITEM::GetMaxError() const
{
    if( const BOARD* board = GetBoard() )
        return board->GetDesignSettings().m_MaxError;

    return ARC_HIGH_DEF;
}


int BOARD_ITEM::BoardLayerCount() const
{
    const BOARD* board = GetBoard();

    if( board )
        return board->GetLayerSet().count();

    return 64;
}


int BOARD_ITEM::BoardCopperLayerCount() const
{
    const BOARD* board = GetBoard();

    if( board )
        return board->GetCopperLayerCount();

    return 32;
}


LSET BOARD_ITEM::BoardLayerSet() const
{
    const BOARD* board = GetBoard();

    if( board )
        return board->GetEnabledLayers();

    return LSET::AllLayersMask();
}


wxString BOARD_ITEM::GetLayerName() const
{
    if( const BOARD* board = GetBoard() )
        return board->GetLayerName( m_layer );

    // If no parent, return standard name
    return BOARD::GetStandardLayerName( m_layer );
}


bool BOARD_ITEM::IsSideSpecific() const
{
    if( ( GetLayerSet() & LSET::SideSpecificMask() ).any() )
        return true;

    if( const BOARD* board = GetBoard() )
    {
        LAYER_T principalLayerType = board->GetLayerType( m_layer );

        if( principalLayerType == LT_FRONT || principalLayerType == LT_BACK )
            return true;
    }

    return false;
}


wxString BOARD_ITEM::LayerMaskDescribe() const
{
    const BOARD* board = GetBoard();
    LSET         layers = GetLayerSet();

    if( board )
        layers &= board->GetEnabledLayers();

    LSET copperLayers = layers & LSET::AllCuMask();
    LSET techLayers = layers & LSET::AllTechMask();

    // Try to be smart and useful.  Check all copper first.
    if( (int) copperLayers.count() == board->GetCopperLayerCount() )
        return _( "all copper layers" );

    for( LSET testLayers : { copperLayers, techLayers, layers } )
    {
        for( int bit = PCBNEW_LAYER_ID_START; bit < PCB_LAYER_ID_COUNT; ++bit )
        {
            if( testLayers[ bit ] )
            {
                wxString layerInfo = board->GetLayerName( static_cast<PCB_LAYER_ID>( bit ) );

                if( testLayers.count() > 1 )
                    layerInfo << wxS( " " ) + _( "and others" );

                return layerInfo;
            }
        }
    }

    // No copper, no technicals: no layer
    return _( "no layers" );
}


std::vector<int> BOARD_ITEM::ViewGetLayers() const
{
    // Basic fallback
    if( IsLocked() )
        return { m_layer, LAYER_LOCKED_ITEM_SHADOW };

    return { m_layer };
}


void BOARD_ITEM::DeleteStructure()
{
    BOARD_ITEM_CONTAINER* parent = GetParent();

    if( parent )
        parent->Remove( this );

    delete this;
}


void BOARD_ITEM::swapData( BOARD_ITEM* aImage )
{
    UNIMPLEMENTED_FOR( GetClass() );
}


void BOARD_ITEM::SwapItemData( BOARD_ITEM* aImage )
{
    if( aImage == nullptr )
        return;

    EDA_ITEM* parent = GetParent();

    swapData( aImage );

    SetParent( parent );
}


BOARD_ITEM* BOARD_ITEM::Duplicate( bool addToParentGroup, BOARD_COMMIT* aCommit ) const
{
    BOARD_ITEM* dupe = static_cast<BOARD_ITEM*>( Clone() );
    const_cast<KIID&>( dupe->m_Uuid ) = KIID();

    if( addToParentGroup )
    {
        wxCHECK_MSG( aCommit, dupe, "Must supply a commit to update parent group" );

        if( EDA_GROUP* group = dupe->GetParentGroup() )
        {
            aCommit->Modify( group->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );
            group->AddItem( dupe );
        }
    }

    return dupe;
}


void BOARD_ITEM::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                          int aClearance, int aError, ERROR_LOC aErrorLoc,
                                          bool ignoreLineWidth ) const
{
    wxFAIL_MSG( wxString::Format( wxT( "%s doesn't implement TransformShapeToPolygon()" ), GetClass() ) );
};


bool BOARD_ITEM::ptr_cmp::operator() ( const BOARD_ITEM* a, const BOARD_ITEM* b ) const
{
    if( a->Type() != b->Type() )
        return a->Type() < b->Type();

    if( a->GetLayerSet() != b->GetLayerSet() )
        return a->GetLayerSet().Seq() < b->GetLayerSet().Seq();

    if( a->m_Uuid != b->m_Uuid )       // UUIDs *should* always be unique (for valid boards anyway)
        return a->m_Uuid < b->m_Uuid;

    return a < b;                      // But just in case; ptrs are guaranteed to be different
}


std::shared_ptr<SHAPE> BOARD_ITEM::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    static std::shared_ptr<SHAPE> shape;

    UNIMPLEMENTED_FOR( GetClass() );

    return shape;
}


std::shared_ptr<SHAPE_SEGMENT> BOARD_ITEM::GetEffectiveHoleShape() const
{
    static std::shared_ptr<SHAPE_SEGMENT> slot;

    UNIMPLEMENTED_FOR( GetClass() );

    return slot;
}


VECTOR2I BOARD_ITEM::GetFPRelativePosition() const
{
    VECTOR2I pos = GetPosition();

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        pos -= parentFP->GetPosition();
        RotatePoint( pos, -parentFP->GetOrientation() );
    }

    return pos;
}


void BOARD_ITEM::SetFPRelativePosition( const VECTOR2I& aPos )
{
    VECTOR2I pos( aPos );

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        RotatePoint( pos, parentFP->GetOrientation() );
        pos += parentFP->GetPosition();
    }

    SetPosition( pos );
}


void BOARD_ITEM::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    wxMessageBox( wxT( "virtual BOARD_ITEM::Rotate used, should not occur" ), GetClass() );
}


void BOARD_ITEM::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    wxMessageBox( wxT( "virtual BOARD_ITEM::Flip used, should not occur" ), GetClass() );
}


void BOARD_ITEM::Mirror( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    wxMessageBox( wxT( "virtual BOARD_ITEM::Mirror used, should not occur" ), GetClass() );
}


wxString BOARD_ITEM::GetParentAsString() const
{
    if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( m_parent ) )
        return fp->GetReference();

    return m_parent->m_Uuid.AsString();
}


const std::vector<wxString>* BOARD_ITEM::GetEmbeddedFonts()
{
    if( BOARD* board = GetBoard() )
        return board->GetFontFiles();

    return nullptr;
}


static struct BOARD_ITEM_DESC
{
    BOARD_ITEM_DESC()
    {
        ENUM_MAP<PCB_LAYER_ID>& layerEnum = ENUM_MAP<PCB_LAYER_ID>::Instance();

        if( layerEnum.Choices().GetCount() == 0 )
        {
            layerEnum.Undefined( UNDEFINED_LAYER );

            for( PCB_LAYER_ID layer : LSET::AllLayersMask() )
                layerEnum.Map( layer, LSET::Name( layer ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( BOARD_ITEM );
        propMgr.InheritsAfter( TYPE_HASH( BOARD_ITEM ), TYPE_HASH( EDA_ITEM ) );

        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, wxString>( _HKI( "Parent" ),
                     NO_SETTER( BOARD_ITEM, wxString ), &BOARD_ITEM::GetParentAsString ) )
                .SetIsHiddenFromLibraryEditors()
                .SetIsHiddenFromPropertiesManager();

        auto isNotFootprintHolder =
                []( INSPECTABLE* aItem ) -> bool
                {
                    BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( aItem );
                    return item && item->GetBoard() && !item->GetBoard()->IsFootprintHolder();
                };

        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, int>( _HKI( "Position X" ),
                    &BOARD_ITEM::SetX, &BOARD_ITEM::GetX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ) );
        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, int>( _HKI( "Position Y" ),
                    &BOARD_ITEM::SetY, &BOARD_ITEM::GetY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ) );
        propMgr.AddProperty( new PROPERTY_ENUM<BOARD_ITEM, PCB_LAYER_ID>( _HKI( "Layer" ),
                    &BOARD_ITEM::SetLayer, &BOARD_ITEM::GetLayer ) );
        propMgr.AddProperty( new PROPERTY<BOARD_ITEM, bool>( _HKI( "Locked" ),
                    &BOARD_ITEM::SetLocked, &BOARD_ITEM::IsLocked ) )
               .SetAvailableFunc( isNotFootprintHolder );
    }
} _BOARD_ITEM_DESC;

IMPLEMENT_ENUM_TO_WXANY( PCB_LAYER_ID )

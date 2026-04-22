/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "3d_placeholder_utils.h"

#include <footprint.h>
#include <pad.h>
#include <board_item.h>
#include <layer_ids.h>
#include <base_units.h>
#include <geometry/eda_angle.h>
#include <geometry/shape_poly_set.h>
#include <pcb_shape.h>
#include <convert_shape_list_to_polygon.h>
#include <convert_basic_shapes_to_polygon.h>
#include <wx/log.h>

BOX2I CalcPlaceholderLocalBox( const FOOTPRINT* aFootprint )
{
    BOX2I localBox;
    bool  hasLocalBounds = false;

    for( PAD* pad : aFootprint->Pads() )
    {
        VECTOR2I padPos = pad->GetFPRelativePosition();
        VECTOR2I padSize = pad->GetSize( PADSTACK::ALL_LAYERS );

        BOX2I padBox;
        padBox.SetOrigin( padPos - padSize / 2 );
        padBox.SetSize( padSize );

        if( !hasLocalBounds )
        {
            localBox = padBox;
            hasLocalBounds = true;
        }
        else
        {
            localBox.Merge( padBox );
        }
    }

    if( !hasLocalBounds )
    {
        VECTOR2I  fpPos = aFootprint->GetPosition();
        EDA_ANGLE fpAngle = aFootprint->GetOrientation();

        for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
        {
            PCB_LAYER_ID layer = item->GetLayer();

            if( layer == F_Fab || layer == B_Fab || layer == F_CrtYd || layer == B_CrtYd || layer == F_SilkS
                || layer == B_SilkS )
            {
                BOX2I itemBox = item->GetBoundingBox();

                VECTOR2I corners[4] = { itemBox.GetOrigin() - fpPos,
                                        VECTOR2I( itemBox.GetRight(), itemBox.GetTop() ) - fpPos,
                                        VECTOR2I( itemBox.GetRight(), itemBox.GetBottom() ) - fpPos,
                                        VECTOR2I( itemBox.GetLeft(), itemBox.GetBottom() ) - fpPos };

                BOX2I localItemBox;

                for( int ci = 0; ci < 4; ++ci )
                {
                    RotatePoint( corners[ci], -fpAngle );

                    if( ci == 0 )
                    {
                        localItemBox.SetOrigin( corners[ci] );
                        localItemBox.SetSize( VECTOR2I( 0, 0 ) );
                    }
                    else
                    {
                        localItemBox.Merge( BOX2I( corners[ci], VECTOR2I( 0, 0 ) ) );
                    }
                }

                if( !hasLocalBounds )
                {
                    localBox = localItemBox;
                    hasLocalBounds = true;
                }
                else
                {
                    localBox.Merge( localItemBox );
                }
            }
        }
    }

    if( !hasLocalBounds )
    {
        BOX2I fpBox = aFootprint->GetBoundingBox( false );
        int   size = std::min( fpBox.GetWidth(), fpBox.GetHeight() );
        localBox.SetOrigin( VECTOR2I( -size / 2, -size / 2 ) );
        localBox.SetSize( VECTOR2I( size, size ) );
    }

    return localBox;
}


/**
 * Build a filled polygon from shapes on the given layer.
 *
 * Separates shapes into self-closed and open shapes.
 * Self-closed shapes are converted to filled polygons.
 * Open shapes are chained into closed contours.
 */
static bool buildFilledPolygonFromShapes( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aOutline )
{
    std::vector<PCB_SHAPE*> closedShapes;
    std::vector<PCB_SHAPE*> openShapes;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->GetLayer() == aLayer && item->Type() == PCB_SHAPE_T )
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

            if( shape->GetShape() == SHAPE_T::CIRCLE || shape->GetShape() == SHAPE_T::RECTANGLE
                || shape->GetShape() == SHAPE_T::POLY )
            {
                closedShapes.push_back( shape );
            }
            else
            {
                openShapes.push_back( shape );
            }
        }
    }

    if( closedShapes.empty() && openShapes.empty() )
        return false;

    int maxError = pcbIUScale.mmToIU( 0.005 );

    if( !openShapes.empty() )
    {
        int chainingEpsilon = pcbIUScale.mmToIU( 0.02 );

        if( ConvertOutlineToPolygon( openShapes, aOutline, maxError, chainingEpsilon, true, nullptr )
            && aOutline.OutlineCount() > 0 )
        {
            aOutline.Simplify();
        }
        else
        {
            aOutline.RemoveAllContours();

            SHAPE_POLY_SET strokes;

            for( PCB_SHAPE* shape : openShapes )
                shape->TransformShapeToPolygon( strokes, aLayer, 0, maxError, ERROR_INSIDE );

            if( strokes.OutlineCount() > 0 )
            {
                strokes.Simplify();

                for( int i = 0; i < strokes.OutlineCount(); i++ )
                    aOutline.AddOutline( strokes.COutline( i ) );
            }
        }
    }

    for( const PCB_SHAPE* shape : closedShapes )
    {
        switch( shape->GetShape() )
        {
        case SHAPE_T::CIRCLE:
        {
            TransformCircleToPolygon( aOutline, shape->GetCenter(), shape->GetRadius(), ARC_HIGH_DEF, ERROR_INSIDE );
            break;
        }
        case SHAPE_T::RECTANGLE:
        {
            std::vector<VECTOR2I> corners = shape->GetRectCorners();

            if( corners.size() == 4 )
            {
                aOutline.NewOutline();

                for( const VECTOR2I& pt : corners )
                    aOutline.Append( pt );
            }

            break;
        }
        case SHAPE_T::POLY:
        {
            const SHAPE_POLY_SET& polyShape = shape->GetPolyShape();

            if( polyShape.OutlineCount() > 0 )
                aOutline.Append( polyShape );

            break;
        }
        default: break;
        }
    }

    if( aOutline.OutlineCount() > 0 )
    {
        aOutline.Simplify();
        return true;
    }

    return false;
}


static bool buildPadBoundingBox( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aOutline )
{
    BOX2I padBox = CalcPlaceholderLocalBox( aFootprint );

    if( padBox.GetWidth() <= 0 || padBox.GetHeight() <= 0 )
        return false;

    VECTOR2I  fpPos = aFootprint->GetPosition();
    EDA_ANGLE fpAngle = aFootprint->GetOrientation();

    VECTOR2I corners[4] = { padBox.GetOrigin(), VECTOR2I( padBox.GetRight(), padBox.GetTop() ),
                            VECTOR2I( padBox.GetRight(), padBox.GetBottom() ),
                            VECTOR2I( padBox.GetLeft(), padBox.GetBottom() ) };

    aOutline.NewOutline();

    for( int i = 0; i < 4; ++i )
    {
        RotatePoint( corners[i], fpAngle );
        corners[i] += fpPos;
        aOutline.Append( corners[i] );
    }

    return true;
}


bool GetExtrusionOutline( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aOutline, PCB_LAYER_ID aLayerOverride )
{
    aOutline.RemoveAllContours();

    const EXTRUDED_3D_BODY* body = aFootprint->GetExtrudedBody();
    PCB_LAYER_ID            extLayer =
            ( aLayerOverride != UNDEFINED_LAYER ) ? aLayerOverride : ( body ? body->m_layer : UNDEFINED_LAYER );

    // Explicit pin bounding box mode
    if( extLayer == UNSELECTED_LAYER )
        return buildPadBoundingBox( aFootprint, aOutline );

    bool isBack = aFootprint->IsFlipped();

    if( isBack && ( extLayer == F_CrtYd || extLayer == F_Fab || extLayer == F_SilkS ) )
        extLayer = FlipLayer( extLayer );
    else if( !isBack && ( extLayer == B_CrtYd || extLayer == B_Fab || extLayer == B_SilkS ) )
        extLayer = FlipLayer( extLayer );

    if( extLayer != UNDEFINED_LAYER )
    {
        if( extLayer == F_CrtYd || extLayer == B_CrtYd )
        {
            const SHAPE_POLY_SET& courtyard = aFootprint->GetCourtyard( extLayer );

            if( courtyard.OutlineCount() > 0 )
            {
                aOutline.Append( courtyard );
                aOutline.Simplify();
                return true;
            }
        }
        else if( extLayer == F_Fab || extLayer == B_Fab )
        {
            if( buildFilledPolygonFromShapes( aFootprint, extLayer, aOutline ) )
                return true;
        }
        else if( extLayer == F_SilkS || extLayer == B_SilkS )
        {
            if( buildFilledPolygonFromShapes( aFootprint, extLayer, aOutline ) )
                return true;
        }

        wxLogTrace( wxT( "KI_TRACE_3D_RENDER" ), wxT( "Extrusion: no shapes found on explicit layer for '%s'" ),
                    aFootprint->GetReference() );
        return false;
    }

    // Auto mode is courtyard -> fab -> pad bbox
    PCB_LAYER_ID courtyardLayer = isBack ? B_CrtYd : F_CrtYd;
    PCB_LAYER_ID fabLayer = isBack ? B_Fab : F_Fab;

    const SHAPE_POLY_SET& courtyard = aFootprint->GetCourtyard( courtyardLayer );

    if( courtyard.OutlineCount() > 0 )
    {
        aOutline.Append( courtyard );
        aOutline.Simplify();
        return true;
    }

    wxLogTrace( wxT( "KI_TRACE_3D_RENDER" ), wxT( "Extrusion: courtyard outline failed for '%s', trying fab layer" ),
                aFootprint->GetReference() );

    if( buildFilledPolygonFromShapes( aFootprint, fabLayer, aOutline ) )
        return true;

    wxLogTrace( wxT( "KI_TRACE_3D_RENDER" ),
                wxT( "Extrusion: fab layer outline failed for '%s', trying silkscreen" ),
                aFootprint->GetReference() );

    PCB_LAYER_ID silkLayer = isBack ? B_SilkS : F_SilkS;

    if( buildFilledPolygonFromShapes( aFootprint, silkLayer, aOutline ) )
        return true;

    wxLogTrace( wxT( "KI_TRACE_3D_RENDER" ),
                wxT( "Extrusion: silkscreen outline failed for '%s', trying pad bbox" ),
                aFootprint->GetReference() );

    if( buildPadBoundingBox( aFootprint, aOutline ) )
        return true;

    wxLogTrace( wxT( "KI_TRACE_3D_RENDER" ), wxT( "Extrusion: no outline could be generated for '%s'" ),
                aFootprint->GetReference() );
    return false;
}

bool GetExtrusionPinOutline( const FOOTPRINT* aFootprint, SHAPE_POLY_SET& aPinPoly )
{
    aPinPoly.RemoveAllContours();

    for( PAD* pad : aFootprint->Pads() )
    {
        if( pad->HasHole() )
        {
            int shrink = -pad->GetDrillSize().x / 20; // ~90% of hole diameter
            pad->TransformHoleToPolygon( aPinPoly, shrink, ARC_HIGH_DEF, ERROR_INSIDE );
        }
    }

    if( aPinPoly.OutlineCount() == 0 )
        return false;

    aPinPoly.Simplify();
    return true;
}


EXTRUSION_MATERIAL_PROPS GetMaterialProps( EXTRUSION_MATERIAL aMaterial, const SFVEC3F& aDiffuse )
{
    switch( aMaterial )
    {
    default:
    case EXTRUSION_MATERIAL::PLASTIC: return { aDiffuse * 0.1f, SFVEC3F( 0.4f, 0.4f, 0.4f ), 0.3f * 128.0f };
    case EXTRUSION_MATERIAL::MATTE: return { aDiffuse * 0.1f, SFVEC3F( 0.05f, 0.05f, 0.05f ), 0.05f * 128.0f };
    case EXTRUSION_MATERIAL::METAL:
        return { aDiffuse * 0.15f, aDiffuse * 0.5f + SFVEC3F( 0.4f, 0.4f, 0.4f ), 0.5f * 128.0f };
    case EXTRUSION_MATERIAL::COPPER:
        return { aDiffuse * 0.1f, aDiffuse * 0.75f + SFVEC3F( 0.25f, 0.25f, 0.25f ), 0.4f * 128.0f };
    }
}


void ApplyExtrusionTransform( SHAPE_POLY_SET& aOutline, const EXTRUDED_3D_BODY* aBody, const VECTOR2I& aFpPos )
{
    if( aBody->m_rotation.z != 0.0 )
        aOutline.Rotate( EDA_ANGLE( aBody->m_rotation.z, DEGREES_T ), aFpPos );

    if( aBody->m_scale.x != 1.0 || aBody->m_scale.y != 1.0 )
        aOutline.Scale( aBody->m_scale.x, aBody->m_scale.y, aFpPos );

    if( aBody->m_offset.x != 0.0 || aBody->m_offset.y != 0.0 )
        aOutline.Move( VECTOR2I( pcbIUScale.mmToIU( aBody->m_offset.x ), pcbIUScale.mmToIU( aBody->m_offset.y ) ) );
}

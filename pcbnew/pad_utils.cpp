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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pad_utils.h"

#include <algorithm>

#include <wx/intl.h>

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_segment.h>
#include <math/util.h>
#include <pad.h>
#include <pcb_shape.h>
#include <units_provider.h>


double PAD_UTILS::GetDefaultIpcRoundingRatio( const PAD& aPad, PCB_LAYER_ID aLayer )
{
    const double defaultProportion = 0.25;
    const double minimumSizeIU = pcbIUScale.mmToIU( 0.25 );

    const VECTOR2I& size = aPad.GetSize( aLayer );
    const int       padMinSizeIU = std::min( size.x, size.y );
    const double    defaultRadiusIU = std::min( minimumSizeIU, padMinSizeIU * defaultProportion );

    // Convert back to a ratio
    return defaultRadiusIU / padMinSizeIU;
}


bool PAD_UTILS::PadHasMeaningfulRoundingRadius( const PAD& aPad, PCB_LAYER_ID aLayer )
{
    const PAD_SHAPE shape = aPad.GetShape( aLayer );
    return shape == PAD_SHAPE::ROUNDRECT || shape == PAD_SHAPE::CHAMFERED_RECT;
}


namespace
{

void checkPadLayer( const PAD& aPad, PCB_LAYER_ID aLayer, const UNITS_PROVIDER& aUnitsProvider, bool aForPadProperties,
                    const PAD_UTILS::PAD_CHECK_HANDLER& aErrorHandler )
{
    wxString msg;

    VECTOR2I pad_size = aPad.GetSize( aLayer );

    if( aPad.GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
        pad_size = aPad.GetBoundingBox().GetSize();
    else if( pad_size.x <= 0 || ( pad_size.y <= 0 && aPad.GetShape( aLayer ) != PAD_SHAPE::CIRCLE ) )
        aErrorHandler( {
                PAD_UTILS::PAD_CHECK_CODE::PAD_SIZE_NOT_POSITIVE,
                _( "Pad must have a positive size" ),
        } );

    // Test hole against pad shape
    if( aPad.IsOnCopperLayer() && aPad.GetDrillSize().x > 0 )
    {
        // Ensure the drill size can be handled in next calculations.
        // Use min size = 4 IU to be able to build a polygon from a hole shape
        const int min_drill_size = 4;

        if( aPad.GetDrillSizeX() <= min_drill_size || aPad.GetDrillSizeY() <= min_drill_size )
        {
            msg.Printf( _( "PTH pad hole size must be larger than %s" ),
                        aUnitsProvider.StringFromValue( min_drill_size, true ) );
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::PTH_HOLE_TOO_SMALL,
                    msg,
            } );
        }

        SHAPE_POLY_SET padOutline;

        aPad.TransformShapeToPolygon( padOutline, aLayer, 0, aPad.GetMaxError(), ERROR_INSIDE );

        if( aPad.GetAttribute() == PAD_ATTRIB::PTH )
        {
            // Test if there is copper area outside hole
            std::shared_ptr<SHAPE_SEGMENT> hole = aPad.GetEffectiveHoleShape();
            SHAPE_POLY_SET                 holeOutline;

            TransformOvalToPolygon( holeOutline, hole->GetSeg().A, hole->GetSeg().B, hole->GetWidth(),
                                    aPad.GetMaxError(), ERROR_OUTSIDE );

            SHAPE_POLY_SET copper = padOutline;
            copper.BooleanSubtract( holeOutline );

            if( copper.IsEmpty() )
            {
                aErrorHandler( {
                        PAD_UTILS::PAD_CHECK_CODE::PTH_HOLE_LEAVES_NO_COPPER,
                        _( "PTH pad hole leaves no copper" ),
                } );
            }
            else if( aForPadProperties )
            {
                // Test if the pad hole is fully inside the copper area.  Note that we only run
                // this check for pad properties because we run the more complete annular ring
                // checker on the board (which handles multiple pads with the same name).
                holeOutline.BooleanSubtract( padOutline );

                if( !holeOutline.IsEmpty() )
                {
                    aErrorHandler( {
                            PAD_UTILS::PAD_CHECK_CODE::PTH_HOLE_NOT_INSIDE_COPPER,
                            _( "PTH pad hole not fully inside copper" ),
                    } );
                }
            }
        }
        else
        {
            // Test only if the pad hole's centre is inside the copper area
            if( !padOutline.Collide( aPad.GetPosition() ) )
            {
                aErrorHandler( {
                        PAD_UTILS::PAD_CHECK_CODE::HOLE_NOT_INSIDE_PAD,
                        _( "pad hole not inside pad shape" ),
                } );
            }
        }
    }

    if( aPad.GetLocalClearance().value_or( 0 ) < 0 )
    {
        aErrorHandler( {
                PAD_UTILS::PAD_CHECK_CODE::NEGATIVE_LOCAL_CLEARANCE,
                _( "negative local clearance values have no effect" ),
        } );
    }

    // Some pads need a negative solder mask clearance (mainly for BGA with small pads)
    // However the negative solder mask clearance must not create negative mask size
    // Therefore test for minimal acceptable negative value
    std::optional<int> solderMaskMargin = aPad.GetLocalSolderMaskMargin();

    if( solderMaskMargin.has_value() && solderMaskMargin.value() < 0 )
    {
        int absMargin = abs( solderMaskMargin.value() );

        if( aPad.GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
        {
            for( const std::shared_ptr<PCB_SHAPE>& shape : aPad.GetPrimitives( aLayer ) )
            {
                BOX2I shapeBBox = shape->GetBoundingBox();

                if( absMargin > shapeBBox.GetWidth() || absMargin > shapeBBox.GetHeight() )
                {
                    aErrorHandler( {
                            PAD_UTILS::PAD_CHECK_CODE::NEGATIVE_MASK_CLEARANCE_TOO_LARGE,
                            _( "negative solder mask clearance is larger than some shape primitives; "
                               "results may be surprising" ),
                    } );

                    break;
                }
            }
        }
        else if( absMargin > pad_size.x || absMargin > pad_size.y )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::NEGATIVE_MASK_CLEARANCE_TOO_LARGE,
                    _( "negative solder mask clearance is larger than pad; no solder mask will "
                       "be generated" ),
            } );
        }
    }

    // Some pads need a positive solder paste clearance (mainly for BGA with small pads)
    // However, a positive value can create issues if the resulting shape is too big.
    // (like a solder paste creating a solder paste area on a neighbor pad or on the solder mask)
    // So we could ask for user to confirm the choice
    // For now we just check for disappearing paste
    wxSize paste_size;
    int    paste_margin = aPad.GetLocalSolderPasteMargin().value_or( 0 );
    auto   mratio = aPad.GetLocalSolderPasteMarginRatio();

    paste_size.x = pad_size.x + paste_margin + KiROUND( pad_size.x * mratio.value_or( 0 ) );
    paste_size.y = pad_size.y + paste_margin + KiROUND( pad_size.y * mratio.value_or( 0 ) );

    if( paste_size.x <= 0 || paste_size.y <= 0 )
    {
        aErrorHandler( {
                PAD_UTILS::PAD_CHECK_CODE::NEGATIVE_PASTE_MARGIN_TOO_LARGE,
                _( "negative solder paste margin is larger than pad; no solder paste mask will "
                   "be generated" ),
        } );
    }

    if( aPad.GetShape( aLayer ) == PAD_SHAPE::ROUNDRECT )
    {
        if( aPad.GetRoundRectRadiusRatio( aLayer ) < 0.0 )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::ROUNDRECT_RADIUS_NEGATIVE,
                    _( "negative corner radius is not allowed" ),
            } );
        }
        else if( aPad.GetRoundRectRadiusRatio( aLayer ) > 50.0 )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::ROUNDRECT_RADIUS_TOO_LARGE,
                    _( "corner size will make pad circular" ),
            } );
        }
    }
    else if( aPad.GetShape( aLayer ) == PAD_SHAPE::CHAMFERED_RECT )
    {
        if( aPad.GetChamferRectRatio( aLayer ) < 0.0 )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::CHAMFER_NEGATIVE,
                    _( "negative corner chamfer is not allowed" ),
            } );
        }
        else if( aPad.GetChamferRectRatio( aLayer ) > 50.0 )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::CHAMFER_TOO_LARGE,
                    _( "corner chamfer is too large" ),
            } );
        }
    }
    else if( aPad.GetShape( aLayer ) == PAD_SHAPE::TRAPEZOID )
    {
        if(    ( aPad.GetDelta( aLayer ).x < 0 && aPad.GetDelta( aLayer ).x < -aPad.GetSize( aLayer ).y )
            || ( aPad.GetDelta( aLayer ).x > 0 && aPad.GetDelta( aLayer ).x > aPad.GetSize( aLayer ).y )
            || ( aPad.GetDelta( aLayer ).y < 0 && aPad.GetDelta( aLayer ).y < -aPad.GetSize( aLayer ).x )
            || ( aPad.GetDelta( aLayer ).y > 0 && aPad.GetDelta( aLayer ).y > aPad.GetSize( aLayer ).x ) )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::TRAPEZOID_DELTA_TOO_LARGE,
                    _( "trapezoid delta is too large" ),
            } );
        }
    }

    if( aPad.GetShape( aLayer ) == PAD_SHAPE::CUSTOM )
    {
        SHAPE_POLY_SET mergedPolygon;
        aPad.MergePrimitivesAsPolygon( aLayer, &mergedPolygon );

        if( mergedPolygon.OutlineCount() > 1 )
        {
            aErrorHandler( {
                    PAD_UTILS::PAD_CHECK_CODE::CUSTOM_SHAPE_NOT_SINGLE_POLYGON,
                    _( "custom pad shape must resolve to a single polygon" ),
            } );
        }
    }
}

} // namespace


void PAD_UTILS::CheckPad( const PAD& aPad, const UNITS_PROVIDER& aUnitsProvider, bool aForPadProperties,
                          const PAD_CHECK_HANDLER& aErrorHandler )
{
    aPad.Padstack().ForEachUniqueLayer(
            [&]( PCB_LAYER_ID aLayer )
            {
                checkPadLayer( aPad, aLayer, aUnitsProvider, aForPadProperties, aErrorHandler );
            } );

    const LSET     padlayers_mask = aPad.GetLayerSet();
    const VECTOR2I drill_size = aPad.GetDrillSize();

    if( !padlayers_mask[F_Cu] && !padlayers_mask[B_Cu] )
    {
        if( ( drill_size.x || drill_size.y ) && aPad.GetAttribute() != PAD_ATTRIB::NPTH )
        {
            aErrorHandler( {
                    PAD_CHECK_CODE::PTH_NO_OUTER_COPPER,
                    _( "plated through holes normally have a copper pad on at least one outer "
                       "layer" ),
            } );
        }
    }

    if( ( aPad.GetProperty() == PAD_PROP::FIDUCIAL_GLBL
          || aPad.GetProperty() == PAD_PROP::FIDUCIAL_LOCAL )
        && aPad.GetAttribute() == PAD_ATTRIB::NPTH )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::FIDUCIAL_NOT_PLATED,
                _( "'fiducial' pads are normally plated" ),
        } );
    }

    if( aPad.GetProperty() == PAD_PROP::TESTPOINT && aPad.GetAttribute() == PAD_ATTRIB::NPTH )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::TESTPOINT_NOT_PLATED,
                _( "'testpoint' pads are normally plated" ),
        } );
    }

    if( aPad.GetProperty() == PAD_PROP::HEATSINK && aPad.GetAttribute() == PAD_ATTRIB::NPTH )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::HEATSINK_NOT_PLATED,
                _( "'heatsink' pads are normally plated" ),
        } );
    }

    if( aPad.GetProperty() == PAD_PROP::CASTELLATED && aPad.GetAttribute() != PAD_ATTRIB::PTH )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::CASTELLATED_NOT_PTH,
                _( "'castellated' pads are normally PTH" ),
        } );
    }

    if( aPad.GetProperty() == PAD_PROP::BGA && aPad.GetAttribute() != PAD_ATTRIB::SMD )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::BGA_NOT_SMD,
                _( "'BGA' property is for SMD pads" ),
        } );
    }

    if( aPad.GetProperty() == PAD_PROP::MECHANICAL && aPad.GetAttribute() != PAD_ATTRIB::PTH )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::MECHANICAL_NOT_PTH,
                _( "'mechanical' pads are normally PTH" ),
        } );
    }

    if( aPad.GetProperty() == PAD_PROP::PRESSFIT
            && ( aPad.GetAttribute() != PAD_ATTRIB::PTH || !aPad.HasDrilledHole() ) )
    {
        aErrorHandler( {
                PAD_CHECK_CODE::PRESSFIT_NOT_PTH,
                _( "'press-fit' pads are normally PTH with round holes" ),
        } );
    }

    switch( aPad.GetAttribute() )
    {
    case PAD_ATTRIB::NPTH:   // Not plated, but through hole, a hole is expected
    case PAD_ATTRIB::PTH:    // Pad through hole, a hole is also expected
        if( drill_size.x <= 0
            || ( drill_size.y <= 0 && aPad.GetDrillShape() == PAD_DRILL_SHAPE::OBLONG ) )
        {
            aErrorHandler( {
                    PAD_CHECK_CODE::TH_NO_HOLE,
                    wxEmptyString,
            } );
        }
        break;

    case PAD_ATTRIB::CONN:      // Connector pads are smd pads, just they do not have solder paste.
        if( padlayers_mask[B_Paste] || padlayers_mask[F_Paste] )
        {
            aErrorHandler( {
                    PAD_CHECK_CODE::CONN_HAS_PASTE,
                    _( "connector pads normally have no solder paste; use a SMD pad instead" ),
            } );
        }
        KI_FALLTHROUGH;

    case PAD_ATTRIB::SMD:       // SMD and Connector pads (One external copper layer only)
    {
        if( drill_size.x > 0 || drill_size.y > 0 )
        {
            aErrorHandler( {
                    PAD_CHECK_CODE::SMD_HAS_HOLE,
                    _( "SMD pad has a hole" ),
            } );
        }

        LSET innerlayers_mask = padlayers_mask & LSET::InternalCuMask();

        if( aPad.IsOnLayer( F_Cu ) && aPad.IsOnLayer( B_Cu ) )
        {
            aErrorHandler( {
                    PAD_CHECK_CODE::SMD_BOTH_OUTER_SIDES,
                    _( "SMD pad has copper on both sides of the board" ),
            } );
        }
        else if( aPad.IsOnLayer( F_Cu ) )
        {
            if( aPad.IsOnLayer( B_Mask ) )
            {
                aErrorHandler( {
                        PAD_CHECK_CODE::SMD_MASK_WRONG_SIDE,
                        _( "SMD pad has copper and mask layers on different sides of the board" ),
                } );
            }
            else if( aPad.IsOnLayer( B_Paste ) )
            {
                aErrorHandler( {
                        PAD_CHECK_CODE::SMD_PASTE_WRONG_SIDE,
                        _( "SMD pad has copper and paste layers on different sides of the board" ),
                } );
            }
        }
        else if( aPad.IsOnLayer( B_Cu ) )
        {
            if( aPad.IsOnLayer( F_Mask ) )
            {
                aErrorHandler( {
                        PAD_CHECK_CODE::SMD_MASK_WRONG_SIDE,
                        _( "SMD pad has copper and mask layers on different sides of the board" ),
                } );
            }
            else if( aPad.IsOnLayer( F_Paste ) )
            {
                aErrorHandler( {
                        PAD_CHECK_CODE::SMD_PASTE_WRONG_SIDE,
                        _( "SMD pad has copper and paste layers on different sides of the board" ),
                } );
            }
        }
        else if( innerlayers_mask.count() != 0 )
        {
            aErrorHandler( {
                    PAD_CHECK_CODE::PAD_NO_OUTER_COPPER,
                    _( "SMD pad has no outer layers" ),
            } );
        }

        break;
    }
    }
}

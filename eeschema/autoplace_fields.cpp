/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
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

/******************************************************************************
 * Field autoplacer: Tries to find an optimal place for symbol fields, and places them there.
 * There are two modes: "auto"-autoplace, and "manual" autoplace.
 * Auto mode is for when the process is run automatically, like when rotating parts, and it
 * avoids doing things that would be helpful for the final positioning but annoying if they
 * happened without permission.
 * Short description of the process:
 *
 * 1. Compute the dimensions of the fields' bounding box    ::computeFBoxSize
 * 2. Determine which side the fields will go on.           ::chooseSideForFields
 *      1. Sort the four sides in preference order,
 *          depending on the symbol's shape and
 *          orientation                                     ::getPreferredSides
 *      2. If in manual mode, sift out the sides that would
 *          cause fields to overlap other items             ::getCollidingSides
 *      3. If any remaining sides have zero pins there,
 *          choose the highest zero-pin side according to
 *          preference order.
 *      4. If all sides have pins, choose the side with the
 *          fewest pins.
 * 3. Compute the position of the fields' bounding box      ::fieldBoxPlacement
 * 4. In manual mode, shift the box vertically if possible
 *      to fit fields between adjacent wires                ::fitFieldsBetweenWires
 * 5. Move all fields to their final positions
 *      1. Re-justify fields if options allow that          ::justifyField
 *      2. Round to a 50-mil grid coordinate if desired
 */

#include <boost/range/adaptor/reversed.hpp>

#include <drawing_sheet/ds_data_model.h>
#include <sch_edit_frame.h>
#include <sch_line.h>
#include <kiface_base.h>
#include <algorithm>
#include <tool/tool_manager.h>
#include <core/arraydim.h>

#define FIELD_PADDING schIUScale.MilsToIU( 15 )       // arbitrarily chosen for aesthetics
#define WIRE_V_SPACING schIUScale.MilsToIU( 100 )
#define HPADDING schIUScale.MilsToIU( 25 )            // arbitrarily chosen for aesthetics
#define VPADDING schIUScale.MilsToIU( 15 )            // arbitrarily chosen for aesthetics

/**
 * Round up/down to the nearest multiple of n
 */
template<typename T> T round_n( const T& value, const T& n, bool aRoundUp )
{
    if( value % n )
        return n * (value / n + (aRoundUp ? 1 : 0));
    else
        return value;
}


class AUTOPLACER
{
public:
    typedef VECTOR2I  SIDE;
    static const SIDE SIDE_TOP, SIDE_BOTTOM, SIDE_LEFT, SIDE_RIGHT;
    enum COLLISION { COLLIDE_NONE, COLLIDE_OBJECTS, COLLIDE_H_WIRES };

    struct SIDE_AND_NPINS
    {
        SIDE     side;
        unsigned pins;
    };

    struct SIDE_AND_COLL
    {
        SIDE      side;
        COLLISION collision;
    };

    AUTOPLACER( SYMBOL* aSymbol, SCH_SCREEN* aScreen ) :
            m_screen( aScreen ),
            m_symbol( aSymbol ),
            m_is_power_symbol( false )
    {
        m_symbol->GetFields( m_fields, /* aVisibleOnly */ true );

        auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );
        wxASSERT( cfg );

        m_allow_rejustify = false;
        m_align_to_grid = true;

        if( cfg )
        {
            m_allow_rejustify = cfg->m_AutoplaceFields.allow_rejustify;
            m_align_to_grid = cfg->m_AutoplaceFields.align_to_grid;
        }

        m_symbol_bbox = m_symbol->GetBodyBoundingBox();
        m_fbox_size = computeFBoxSize( /* aDynamic */ true );

        if( SCH_SYMBOL* schSymbol = dynamic_cast<SCH_SYMBOL*>( m_symbol ) )
            m_is_power_symbol = !schSymbol->IsInNetlist();

        if( aScreen )
            getPossibleCollisions( m_colliders );
    }

    /**
     * Do the actual autoplacement.
     * @param aManual - if true, use extra heuristics for smarter placement when manually
     * called up.
     */
    void DoAutoplace( AUTOPLACE_ALGO aAlgo )
    {
        bool            forceWireSpacing = false;
        SIDE_AND_NPINS  sideandpins = chooseSideForFields( aAlgo == AUTOPLACE_MANUAL );
        SIDE            field_side = sideandpins.side;
        VECTOR2I        fbox_pos = fieldBoxPlacement( sideandpins );
        BOX2I           field_box( fbox_pos, m_fbox_size );

        if( aAlgo == AUTOPLACE_MANUAL )
            forceWireSpacing = fitFieldsBetweenWires( &field_box, field_side );

        // Move the fields
        int last_y_coord = field_box.GetTop();

        for( SCH_FIELD* field : m_fields )
        {
            if( !field->IsVisible() || !field->CanAutoplace() )
                continue;

            if( m_allow_rejustify )
            {
                if( sideandpins.pins > 0 )
                {
                    if( field_side == SIDE_TOP || field_side == SIDE_BOTTOM )
                        justifyField( field, SIDE_RIGHT );
                    else
                        justifyField( field, SIDE_TOP );
                }
                else
                {
                    justifyField( field, field_side );
                }
            }

            VECTOR2I pos( fieldHPlacement( field, field_box ),
                          fieldVPlacement( field, field_box, &last_y_coord, !forceWireSpacing ) );

            if( m_align_to_grid )
            {
                if( abs( field_side.x ) > 0 )
                    pos.x = round_n( pos.x, schIUScale.MilsToIU( 50 ), field_side.x >= 0 );

                if( abs( field_side.y ) > 0 )
                    pos.y = round_n( pos.y, schIUScale.MilsToIU( 50 ), field_side.y >= 0 );
            }

            field->SetPosition( pos );
        }
    }

protected:
    /**
     * Compute and return the size of the fields' bounding box.
     * @param aDynamic - if true, use dynamic spacing
     */
    VECTOR2I computeFBoxSize( bool aDynamic )
    {
        int max_field_width = 0;
        int total_height = 0;

        for( SCH_FIELD* field : m_fields )
        {
            if( !field->IsVisible() || !field->CanAutoplace() )
            {
                continue;
            }

            if( m_symbol->GetTransform().y1 )
                field->SetTextAngle( ANGLE_VERTICAL );
            else
                field->SetTextAngle( ANGLE_HORIZONTAL );

            BOX2I bbox = field->GetBoundingBox();
            int   field_width = bbox.GetWidth();
            int   field_height = bbox.GetHeight();

            max_field_width = std::max( max_field_width, field_width );

            if( !aDynamic )
                total_height += WIRE_V_SPACING;
            else if( m_align_to_grid )
                total_height += round_n( field_height, schIUScale.MilsToIU( 50 ), true );
            else
                total_height += field_height + FIELD_PADDING;
        }

        return VECTOR2I( max_field_width, total_height );
    }

    /**
     * Return the side that a pin is on.
     */
    SIDE getPinSide( SCH_PIN* aPin )
    {
        PIN_ORIENTATION pin_orient = aPin->PinDrawOrient( m_symbol->GetTransform() );

        switch( pin_orient )
        {
            case PIN_ORIENTATION::PIN_RIGHT: return SIDE_LEFT;
            case PIN_ORIENTATION::PIN_LEFT:  return SIDE_RIGHT;
            case PIN_ORIENTATION::PIN_UP:    return SIDE_BOTTOM;
            case PIN_ORIENTATION::PIN_DOWN:  return SIDE_TOP;
            default:
                wxFAIL_MSG( wxS( "Invalid pin orientation" ) );
                return SIDE_LEFT;
        }
    }

    /**
     * Count the number of pins on a side of the symbol.
     */
    unsigned pinsOnSide( SIDE aSide )
    {
        unsigned pin_count = 0;

        for( SCH_PIN* each_pin : m_symbol->GetPins() )
        {
            if( !each_pin->IsVisible() && !m_is_power_symbol )
                continue;

            if( getPinSide( each_pin ) == aSide )
                ++pin_count;
        }

        return pin_count;
    }

    /**
     * Populate a list of all drawing items that *may* collide with the fields. That is, all
     * drawing items, including other fields, that are not the current symbol or its own fields.
     */
    void getPossibleCollisions( std::vector<SCH_ITEM*>& aItems )
    {
        wxCHECK_RET( m_screen, wxS( "getPossibleCollisions() with null m_screen" ) );

        BOX2I symbolBox = m_symbol->GetBodyAndPinsBoundingBox();
        std::vector<SIDE_AND_NPINS> sides = getPreferredSides();

        for( SIDE_AND_NPINS& side : sides )
        {
            BOX2I box( fieldBoxPlacement( side ), m_fbox_size );
            box.Merge( symbolBox );

            for( SCH_ITEM* item : m_screen->Items().Overlapping( box ) )
            {
                if( SCH_SYMBOL* candidate = dynamic_cast<SCH_SYMBOL*>( item ) )
                {
                    if( candidate == m_symbol )
                        continue;

                    std::vector<SCH_FIELD*> fields;
                    candidate->GetFields( fields, /* aVisibleOnly */ true );

                    for( SCH_FIELD* field : fields )
                        aItems.push_back( field );
                }

                aItems.push_back( item );
            }
        }
    }

    /**
     * Filter a list of possible colliders to include only those that actually collide
     * with a given rectangle. Returns the new vector.
     */
    std::vector<SCH_ITEM*> filterCollisions( const BOX2I& aRect )
    {
        std::vector<SCH_ITEM*> filtered;

        for( SCH_ITEM* item : m_colliders )
        {
            BOX2I item_box;

            if( SCH_SYMBOL* item_comp = dynamic_cast<SCH_SYMBOL*>( item ) )
                item_box = item_comp->GetBodyAndPinsBoundingBox();
            else
                item_box = item->GetBoundingBox();

            if( item_box.Intersects( aRect ) )
                filtered.push_back( item );
        }

        return filtered;
    }

    /**
     * Return a list with the preferred field sides for the symbol, in decreasing order of
     * preference.
     */
    std::vector<SIDE_AND_NPINS> getPreferredSides()
    {
        SIDE_AND_NPINS sides_init[] = {
            { SIDE_RIGHT, pinsOnSide( SIDE_RIGHT ) },
            { SIDE_TOP, pinsOnSide( SIDE_TOP ) },
            { SIDE_LEFT, pinsOnSide( SIDE_LEFT ) },
            { SIDE_BOTTOM, pinsOnSide( SIDE_BOTTOM ) },
        };
        std::vector<SIDE_AND_NPINS> sides( sides_init, sides_init + arrayDim( sides_init ) );

        int    orient = m_symbol->GetOrientation();
        int    orient_angle = orient & 0xff; // enum is a bitmask
        bool   h_mirrored = ( ( orient & SYM_MIRROR_X )
                             && ( orient_angle == SYM_ORIENT_0 || orient_angle == SYM_ORIENT_180 ) );
        double w = double( m_symbol_bbox.GetWidth() );
        double h = double( m_symbol_bbox.GetHeight() );

        // The preferred-sides heuristics are a bit magical. These were determined mostly
        // by trial and error.

        if( m_is_power_symbol )
        {
            // For power symbols, we generally want the label at the top first.
            switch( orient_angle )
            {
            case SYM_ORIENT_0:
                std::swap( sides[0], sides[1] );
                std::swap( sides[1], sides[3] );
                // TOP, BOTTOM, RIGHT, LEFT
                break;
            case SYM_ORIENT_90:
                std::swap( sides[0], sides[2] );
                std::swap( sides[1], sides[2] );
                // LEFT, RIGHT, TOP, BOTTOM
                break;
            case SYM_ORIENT_180:
                std::swap( sides[0], sides[3] );
                // BOTTOM, TOP, LEFT, RIGHT
                break;
            case SYM_ORIENT_270:
                std::swap( sides[1], sides[2] );
                // RIGHT, LEFT, TOP, BOTTOM
                break;
            }
        }
        else
        {
            // If the symbol is horizontally mirrored, swap left and right
            if( h_mirrored )
            {
                std::swap( sides[0], sides[2] );
            }

            // If the symbol is very long or is a power symbol, swap H and V
            if( w/h > 3.0 )
            {
                std::swap( sides[0], sides[1] );
                std::swap( sides[1], sides[3] );
            }
        }

        return sides;
    }

    /**
     * Compute the drawable area (inside the drawing sheet border) for collision detection.
     */
    BOX2I getDrawableArea()
    {
        if( !m_screen )
            return BOX2I();

        const PAGE_INFO& pageInfo = m_screen->GetPageSettings();
        DS_DATA_MODEL&   dsModel = DS_DATA_MODEL::GetTheInstance();

        int pageWidth = pageInfo.GetWidthIU( schIUScale.IU_PER_MILS );
        int pageHeight = pageInfo.GetHeightIU( schIUScale.IU_PER_MILS );

        int leftMargin = schIUScale.mmToIU( dsModel.GetLeftMargin() );
        int rightMargin = schIUScale.mmToIU( dsModel.GetRightMargin() );
        int topMargin = schIUScale.mmToIU( dsModel.GetTopMargin() );
        int bottomMargin = schIUScale.mmToIU( dsModel.GetBottomMargin() );

        BOX2I drawableArea;
        drawableArea.SetOrigin( leftMargin, topMargin );
        drawableArea.SetEnd( pageWidth - rightMargin, pageHeight - bottomMargin );

        return drawableArea;
    }

    /**
     * Return a list of the sides where a field set would collide with another item.
     */
    std::vector<SIDE_AND_COLL> getCollidingSides()
    {
        SIDE                       sides_init[] = { SIDE_RIGHT, SIDE_TOP, SIDE_LEFT, SIDE_BOTTOM };
        std::vector<SIDE>          sides( sides_init, sides_init + arrayDim( sides_init ) );
        std::vector<SIDE_AND_COLL> colliding;

        BOX2I drawableArea = getDrawableArea();
        bool  checkDrawableArea = ( drawableArea.GetWidth() > 0 && drawableArea.GetHeight() > 0 );

        // Iterate over all sides and find the ones that collide
        for( SIDE side : sides )
        {
            SIDE_AND_NPINS sideandpins;
            sideandpins.side = side;
            sideandpins.pins = pinsOnSide( side );

            BOX2I box( fieldBoxPlacement( sideandpins ), m_fbox_size );

            COLLISION collision = COLLIDE_NONE;

            // Check collision with drawing sheet boundary
            if( checkDrawableArea && !drawableArea.Contains( box ) )
                collision = COLLIDE_OBJECTS;

            for( SCH_ITEM* collider : filterCollisions( box ) )
            {
                SCH_LINE* line = dynamic_cast<SCH_LINE*>( collider );

                if( line && !side.x )
                {
                    VECTOR2I start = line->GetStartPoint(), end = line->GetEndPoint();

                    if( start.y == end.y && collision != COLLIDE_OBJECTS )
                        collision = COLLIDE_H_WIRES;
                    else
                        collision = COLLIDE_OBJECTS;
                }
                else
                {
                    collision = COLLIDE_OBJECTS;
                }
            }

            if( collision != COLLIDE_NONE )
                colliding.push_back( { side, collision } );
        }

        return colliding;
    }

    /**
     * Choose a side for the fields, filtered on only one side collision type.
     * Removes the sides matching the filter from the list.
     */
    SIDE_AND_NPINS chooseSideFiltered( std::vector<SIDE_AND_NPINS>& aSides,
                                       const std::vector<SIDE_AND_COLL>& aCollidingSides,
                                       COLLISION aCollision,
                                       SIDE_AND_NPINS aLastSelection)
    {
        SIDE_AND_NPINS sel = aLastSelection;

        std::vector<SIDE_AND_NPINS>::iterator it = aSides.begin();

        while( it != aSides.end() )
        {
            bool collide = false;

            for( SIDE_AND_COLL collision : aCollidingSides )
            {
                if( collision.side == it->side && collision.collision == aCollision )
                    collide = true;
            }

            if( !collide )
            {
                ++it;
            }
            else
            {
                if( it->pins <= sel.pins )
                {
                    sel.pins = it->pins;
                    sel.side = it->side;
                }

                it = aSides.erase( it );
            }
        }

        return sel;
    }

    /**
     * Look where a symbol's pins are to pick a side to put the fields on
     * @param aAvoidCollisions - if true, pick last the sides where the label will collide
     *      with other items.
     */
    SIDE_AND_NPINS chooseSideForFields( bool aAvoidCollisions )
    {
        std::vector<SIDE_AND_NPINS> sides = getPreferredSides();

        std::reverse( sides.begin(), sides.end() );
        SIDE_AND_NPINS side = { VECTOR2I( 1, 0 ), UINT_MAX };

        if( aAvoidCollisions )
        {
            std::vector<SIDE_AND_COLL> colliding_sides = getCollidingSides();
            side = chooseSideFiltered( sides, colliding_sides, COLLIDE_OBJECTS, side );
            side = chooseSideFiltered( sides, colliding_sides, COLLIDE_H_WIRES, side );
        }

        for( SIDE_AND_NPINS& each_side : sides | boost::adaptors::reversed )
        {
            if( !each_side.pins ) return each_side;
        }

        for( SIDE_AND_NPINS& each_side : sides )
        {
            if( each_side.pins <= side.pins )
            {
                side.pins = each_side.pins;
                side.side = each_side.side;
            }
        }

        return side;
    }

    /**
     * Set the justification of a field based on the side it's supposed to be on, taking
     * into account whether the field will be displayed with flipped justification due to
     * mirroring.
     */
    void justifyField( SCH_FIELD* aField, SIDE aFieldSide )
    {
        // Justification is set twice to allow IsHorizJustifyFlipped() to work correctly.
        aField->SetHorizJustify( ToHAlignment( -aFieldSide.x ) );
        if( aField->IsHorizJustifyFlipped() )
            aField->SetHorizJustify( GetFlippedAlignment( aField->GetHorizJustify() ) );

        aField->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    }

    /**
     * Return the position of the field bounding box.
     */
    VECTOR2I fieldBoxPlacement( SIDE_AND_NPINS aFieldSideAndPins )
    {
        VECTOR2I fbox_center = m_symbol_bbox.Centre();
        int      offs_x = ( m_symbol_bbox.GetWidth() + m_fbox_size.x ) / 2;
        int      offs_y = ( m_symbol_bbox.GetHeight() + m_fbox_size.y ) / 2;

        if( aFieldSideAndPins.side.x != 0 )
            offs_x += HPADDING;
        else if( aFieldSideAndPins.side.y != 0 )
            offs_y += VPADDING;

        fbox_center.x += aFieldSideAndPins.side.x * offs_x;
        fbox_center.y += aFieldSideAndPins.side.y * offs_y;

        int     x = fbox_center.x - ( m_fbox_size.x / 2 );
        int     y = fbox_center.y - ( m_fbox_size.y / 2 );

        auto getPinsBox =
                [&]( const VECTOR2I& aSide )
                {
                    BOX2I pinsBox;

                    for( SCH_PIN* each_pin : m_symbol->GetPins() )
                    {
                        if( !each_pin->IsVisible() && !m_is_power_symbol )
                            continue;

                        if( getPinSide( each_pin ) == aSide )
                            pinsBox.Merge( each_pin->GetBoundingBox() );
                    }

                    return pinsBox;
                };

        if( aFieldSideAndPins.pins > 0 )
        {
            BOX2I pinsBox = getPinsBox( aFieldSideAndPins.side );

            if( aFieldSideAndPins.side == SIDE_TOP || aFieldSideAndPins.side == SIDE_BOTTOM )
            {
                x = pinsBox.GetRight() + ( HPADDING * 2 );
            }
            else if( aFieldSideAndPins.side == SIDE_RIGHT || aFieldSideAndPins.side == SIDE_LEFT )
            {
                y = pinsBox.GetTop() - ( m_fbox_size.y + ( VPADDING * 2 ) );
            }
        }

        return VECTOR2I( x, y );
    }

    /**
     * Shift a field box up or down a bit to make the fields fit between some wires.
     * Returns true if a shift was made.
     */
    bool fitFieldsBetweenWires( BOX2I* aBox, SIDE aSide )
    {
        if( aSide != SIDE_TOP && aSide != SIDE_BOTTOM )
            return false;

        std::vector<SCH_ITEM*> colliders = filterCollisions( *aBox );

        if( colliders.empty() )
            return false;

        // Find the offset of the wires for proper positioning
        int offset = 0;

        for( SCH_ITEM* item : colliders )
        {
            SCH_LINE* line = dynamic_cast<SCH_LINE*>( item );

            if( !line )
                return false;

            VECTOR2I start = line->GetStartPoint(), end = line->GetEndPoint();

            if( start.y != end.y )
                return false;

            int this_offset = (3 * WIRE_V_SPACING / 2) - ( start.y % WIRE_V_SPACING );

            if( offset == 0 )
                offset = this_offset;
            else if( offset != this_offset )
                return false;
        }

        // At this point we are recomputing the field box size. Do not
        // return false after this point.
        m_fbox_size = computeFBoxSize( /* aDynamic */ false );

        VECTOR2I pos = aBox->GetPosition();

        pos.y = round_n( pos.y, WIRE_V_SPACING, aSide == SIDE_BOTTOM );

        aBox->SetOrigin( pos );
        return true;
    }

    /**
     * Place a field horizontally, taking into account the field width and justification.
     *
     * @param aField - the field to place.
     * @param aFieldBox - box in which fields will be placed
     *
     * @return Correct field horizontal position
     */
    int fieldHPlacement( SCH_FIELD* aField, const BOX2I& aFieldBox )
    {
        int field_hjust;
        int field_xcoord;

        if( aField->IsHorizJustifyFlipped() )
            field_hjust = -aField->GetHorizJustify();
        else
            field_hjust = aField->GetHorizJustify();

        switch( field_hjust )
        {
        case GR_TEXT_H_ALIGN_LEFT:
            field_xcoord = aFieldBox.GetLeft();
            break;
        case GR_TEXT_H_ALIGN_CENTER:
            field_xcoord = aFieldBox.Centre().x;
            break;
        case GR_TEXT_H_ALIGN_RIGHT:
            field_xcoord = aFieldBox.GetRight();
            break;
        default:
            wxFAIL_MSG( wxS( "Unexpected value for SCH_FIELD::GetHorizJustify()" ) );
            field_xcoord = aFieldBox.Centre().x; // Most are centered
        }

        return field_xcoord;
    }

    /**
     * Place a field vertically. Because field vertical placements accumulate,
     * this takes a pointer to a vertical position accumulator.
     *
     * @param aField - the field to place.
     * @param aFieldBox - box in which fields will be placed.
     * @param aAccumulatedPosition - pointer to a position accumulator
     * @param aDynamic - use dynamic spacing
     *
     * @return Correct field vertical position
     */
    int fieldVPlacement( SCH_FIELD* aField, const BOX2I& aFieldBox, int* aAccumulatedPosition,
                         bool aDynamic )
    {
        int field_height;
        int padding;

        if( !aDynamic )
        {
            field_height = WIRE_V_SPACING / 2;
            padding = WIRE_V_SPACING / 2;
        }
        else if( m_align_to_grid )
        {
            field_height = aField->GetBoundingBox().GetHeight();
            padding = round_n( field_height, schIUScale.MilsToIU( 50 ), true ) - field_height;
        }
        else
        {
            field_height = aField->GetBoundingBox().GetHeight();
            padding = FIELD_PADDING;
        }

        int placement = *aAccumulatedPosition + padding / 2 + field_height / 2;

        *aAccumulatedPosition += padding + field_height;

        return placement;
    }

private:
    SCH_SCREEN*             m_screen;
    SYMBOL*                 m_symbol;
    std::vector<SCH_FIELD*> m_fields;
    std::vector<SCH_ITEM*>  m_colliders;
    BOX2I                   m_symbol_bbox;
    VECTOR2I                m_fbox_size;
    bool                    m_allow_rejustify;
    bool                    m_align_to_grid;
    bool                    m_is_power_symbol;
};


const AUTOPLACER::SIDE AUTOPLACER::SIDE_TOP( 0, -1 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_BOTTOM( 0, 1 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_LEFT( -1, 0 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_RIGHT( 1, 0 );


void SCH_SYMBOL::AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo )
{
    if( aAlgo == AUTOPLACE_MANUAL )
        wxASSERT_MSG( aScreen, wxS( "A SCH_SCREEN ptr must be given for manual autoplacement" ) );

    AUTOPLACER autoplacer( this, aScreen );
    autoplacer.DoAutoplace( aAlgo );

    switch( aAlgo )
    {
    case AUTOPLACE_AUTO:    m_fieldsAutoplaced = AUTOPLACE_AUTO;          break;
    case AUTOPLACE_MANUAL:  m_fieldsAutoplaced = AUTOPLACE_MANUAL;        break;
    default:                wxFAIL_MSG( "Unknown autoplace algorithm" );  break;
    }
}


void LIB_SYMBOL::AutoplaceFields( SCH_SCREEN* aScreen, AUTOPLACE_ALGO aAlgo )
{
    if( aAlgo == AUTOPLACE_MANUAL )
        wxFAIL_MSG( wxS( "Manual autoplacement not supported for LIB_SYMBOLs" ) );

    AUTOPLACER autoplacer( this, aScreen );
    autoplacer.DoAutoplace( aAlgo );

    switch( aAlgo )
    {
    case AUTOPLACE_AUTO:    m_fieldsAutoplaced = AUTOPLACE_AUTO;          break;
    case AUTOPLACE_MANUAL:  m_fieldsAutoplaced = AUTOPLACE_MANUAL;        break;
    default:                wxFAIL_MSG( "Unknown autoplace algorithm" );  break;
    }
}

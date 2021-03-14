/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015, 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Field autoplacer: Tries to find an optimal place for component fields, and
 * places them there. There are two modes: "auto"-autoplace, and "manual" autoplace.
 * Auto mode is for when the process is run automatically, like when rotating parts,
 * and it avoids doing things that would be helpful for the final positioning but
 * annoying if they happened without permission.
 * Short description of the process:
 *
 * 1. Compute the dimensions of the fields' bounding box    ::computeFBoxSize
 * 2. Determine which side the fields will go on.           ::chooseSideForFields
 *      1. Sort the four sides in preference order,
 *          depending on the component's shape and
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

#include <sch_edit_frame.h>
#include <hotkeys_basic.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <lib_pin.h>
#include <sch_draw_panel.h>
#include <kiface_i.h>
#include <vector>
#include <algorithm>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <eeschema_settings.h>
#include <core/arraydim.h>

#define FIELD_PADDING Mils2iu( 10 )            // arbitrarily chosen for aesthetics
#define WIRE_V_SPACING Mils2iu( 100 )
#define HPADDING Mils2iu( 25 )
#define VPADDING Mils2iu( 25 )

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


/**
 * Convert an integer to a horizontal justification; neg=L zero=C pos=R
 */
EDA_TEXT_HJUSTIFY_T TO_HJUSTIFY( int x )
{
    return static_cast<EDA_TEXT_HJUSTIFY_T>( x );
}


class AUTOPLACER
{
public:
    typedef wxPoint SIDE;
    static const SIDE SIDE_TOP, SIDE_BOTTOM, SIDE_LEFT, SIDE_RIGHT;
    enum COLLISION { COLLIDE_NONE, COLLIDE_OBJECTS, COLLIDE_H_WIRES };

    struct SIDE_AND_NPINS
    {
        SIDE side;
        unsigned pins;
    };

    struct SIDE_AND_COLL
    {
        SIDE side;
        COLLISION collision;
    };

    AUTOPLACER( SCH_COMPONENT* aSymbol, SCH_SCREEN* aScreen ) :
            m_screen( aScreen ),
            m_symbol( aSymbol )
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

        m_is_power_symbol = !m_symbol->IsInNetlist();

        if( aScreen )
            getPossibleCollisions( m_colliders );
    }

    /**
     * Do the actual autoplacement.
     * @param aManual - if true, use extra heuristics for smarter placement when manually
     * called up.
     */
    void DoAutoplace( bool aManual )
    {
        bool     force_wire_spacing = false;
        SIDE     field_side = chooseSideForFields( aManual );
        wxPoint  fbox_pos = fieldBoxPlacement( field_side );
        EDA_RECT field_box( fbox_pos, m_fbox_size );

        if( aManual )
            force_wire_spacing = fitFieldsBetweenWires( &field_box, field_side );

        // Move the fields
        int last_y_coord = field_box.GetTop();

        for( unsigned field_idx = 0; field_idx < m_fields.size(); ++field_idx )
        {
            SCH_FIELD* field = m_fields[field_idx];

            if( m_allow_rejustify )
                justifyField( field, field_side );

            wxPoint pos( fieldHorizPlacement( field, field_box ),
                         fieldVertPlacement( field, field_box, &last_y_coord, !force_wire_spacing ) );

            if( m_align_to_grid )
            {
                if( abs( field_side.x ) > 0 )
                    pos.x = round_n( pos.x, Mils2iu( 50 ), field_side.x >= 0 );

                if( abs( field_side.y ) > 0 )
                    pos.y = round_n( pos.y, Mils2iu( 50 ), field_side.y >= 0 );
            }

            field->SetPosition( pos );
        }
    }

protected:
    /**
     * Compute and return the size of the fields' bounding box.
     * @param aDynamic - if true, use dynamic spacing
     */
    wxSize computeFBoxSize( bool aDynamic )
    {
        int max_field_width = 0;
        int total_height = 0;

        for( SCH_FIELD* field : m_fields )
        {
            if( m_symbol->GetTransform().y1 )
                field->SetTextAngle( TEXT_ANGLE_VERT );
            else
                field->SetTextAngle( TEXT_ANGLE_HORIZ );

            EDA_RECT bbox = field->GetBoundingBox();
            int      field_width = bbox.GetWidth();
            int      field_height = bbox.GetHeight();

            max_field_width = std::max( max_field_width, field_width );

            // Remove interline spacing from field_height for last line.
            if( field == m_fields[ m_fields.size() - 1 ] )
                field_height *= 0.62;

            if( !aDynamic )
                total_height += WIRE_V_SPACING;
            else if( m_align_to_grid )
                total_height += round_n( field_height, Mils2iu( 50 ), true );
            else
                total_height += field_height + FIELD_PADDING;
        }

        return wxSize( max_field_width, total_height );
    }

    /**
     * Return the side that a pin is on.
     */
    SIDE getPinSide( SCH_PIN* aPin )
    {
        int pin_orient = aPin->GetLibPin()->PinDrawOrient( m_symbol->GetTransform() );

        switch( pin_orient )
        {
            case PIN_RIGHT: return SIDE_LEFT;
            case PIN_LEFT:  return SIDE_RIGHT;
            case PIN_UP:    return SIDE_BOTTOM;
            case PIN_DOWN:  return SIDE_TOP;
            default:
                wxFAIL_MSG( "Invalid pin orientation" );
                return SIDE_LEFT;
        }
    }

    /**
     * Count the number of pins on a side of the component.
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
     * Populate a list of all drawing items that *may* collide with the fields. That is,
     * all drawing items, including other fields, that are not the current component or
     * its own fields.
     */
    void getPossibleCollisions( std::vector<SCH_ITEM*>& aItems )
    {
        wxCHECK_RET( m_screen, "getPossibleCollisions() with null m_screen" );

        for( SCH_ITEM* item : m_screen->Items().Overlapping( m_symbol->GetBoundingBox() ) )
        {
            if( SCH_COMPONENT* candidate = dynamic_cast<SCH_COMPONENT*>( item ) )
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

    /**
     * Filter a list of possible colliders to include only those that actually collide
     * with a given rectangle. Returns the new vector.
     */
    std::vector<SCH_ITEM*> filterCollisions( const EDA_RECT& aRect )
    {
        std::vector<SCH_ITEM*> filtered;

        for( SCH_ITEM* item : m_colliders )
        {
            EDA_RECT item_box;

            if( SCH_COMPONENT* item_comp = dynamic_cast<SCH_COMPONENT*>( item ) )
                item_box = item_comp->GetBodyBoundingBox();
            else
                item_box = item->GetBoundingBox();

            if( item_box.Intersects( aRect ) )
                filtered.push_back( item );
        }
        return filtered;
    }

    /**
     * Return a list with the preferred field sides for the component, in
     * decreasing order of preference.
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
        bool   h_mirrored = ( ( orient & CMP_MIRROR_X )
                             && ( orient_angle == CMP_ORIENT_0 || orient_angle == CMP_ORIENT_180 ) );
        double w = double( m_symbol_bbox.GetWidth() );
        double h = double( m_symbol_bbox.GetHeight() );

        // The preferred-sides heuristics are a bit magical. These were determined mostly
        // by trial and error.

        if( m_is_power_symbol )
        {
            // For power symbols, we generally want the label at the top first.
            switch( orient_angle )
            {
            case CMP_ORIENT_0:
                std::swap( sides[0], sides[1] );
                std::swap( sides[1], sides[3] );
                // TOP, BOTTOM, RIGHT, LEFT
                break;
            case CMP_ORIENT_90:
                std::swap( sides[0], sides[2] );
                std::swap( sides[1], sides[2] );
                // LEFT, RIGHT, TOP, BOTTOM
                break;
            case CMP_ORIENT_180:
                std::swap( sides[0], sides[3] );
                // BOTTOM, TOP, LEFT, RIGHT
                break;
            case CMP_ORIENT_270:
                std::swap( sides[1], sides[2] );
                // RIGHT, LEFT, TOP, BOTTOM
                break;
            }
        }
        else
        {
            // If the component is horizontally mirrored, swap left and right
            if( h_mirrored )
            {
                std::swap( sides[0], sides[2] );
            }

            // If the component is very long or is a power symbol, swap H and V
            if( w/h > 3.0 )
            {
                std::swap( sides[0], sides[1] );
                std::swap( sides[1], sides[3] );
            }
        }

        return sides;
    }

    /**
     * Return a list of the sides where a field set would collide with another item.
     */
    std::vector<SIDE_AND_COLL> getCollidingSides()
    {
        SIDE                       sides_init[] = { SIDE_RIGHT, SIDE_TOP, SIDE_LEFT, SIDE_BOTTOM };
        std::vector<SIDE>          sides( sides_init, sides_init + arrayDim( sides_init ) );
        std::vector<SIDE_AND_COLL> colliding;

        // Iterate over all sides and find the ones that collide
        for( SIDE side : sides )
        {
            EDA_RECT box( fieldBoxPlacement( side ), m_fbox_size );

            COLLISION collision = COLLIDE_NONE;

            for( SCH_ITEM* collider : filterCollisions( box ) )
            {
                SCH_LINE* line = dynamic_cast<SCH_LINE*>( collider );

                if( line && !side.x )
                {
                    wxPoint start = line->GetStartPoint(), end = line->GetEndPoint();

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
     * Look where a component's pins are to pick a side to put the fields on
     * @param aAvoidCollisions - if true, pick last the sides where the label will collide
     *      with other items.
     */
    SIDE chooseSideForFields( bool aAvoidCollisions )
    {
        std::vector<SIDE_AND_NPINS> sides = getPreferredSides();

        std::reverse( sides.begin(), sides.end() );
        SIDE_AND_NPINS side = { wxPoint( 1, 0 ), UINT_MAX };

        if( aAvoidCollisions )
        {
            std::vector<SIDE_AND_COLL> colliding_sides = getCollidingSides();
            side = chooseSideFiltered( sides, colliding_sides, COLLIDE_OBJECTS, side );
            side = chooseSideFiltered( sides, colliding_sides, COLLIDE_H_WIRES, side );
        }

        for( SIDE_AND_NPINS& each_side : sides | boost::adaptors::reversed )
        {
            if( !each_side.pins ) return each_side.side;
        }

        for( SIDE_AND_NPINS& each_side : sides )
        {
            if( each_side.pins <= side.pins )
            {
                side.pins = each_side.pins;
                side.side = each_side.side;
            }
        }

        return side.side;
    }

    /**
     * Set the justification of a field based on the side it's supposed to be on, taking
     * into account whether the field will be displayed with flipped justification due to
     * mirroring.
     */
    void justifyField( SCH_FIELD* aField, SIDE aFieldSide )
    {
        // Justification is set twice to allow IsHorizJustifyFlipped() to work correctly.
        aField->SetHorizJustify( TO_HJUSTIFY( -aFieldSide.x ) );
        aField->SetHorizJustify( TO_HJUSTIFY( -aFieldSide.x
                                                 * ( aField->IsHorizJustifyFlipped() ? -1 : 1 ) ) );
        aField->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
    }

    /**
     * Return the position of the field bounding box.
     */
    wxPoint fieldBoxPlacement( SIDE aFieldSide )
    {
        wxPoint fbox_center = m_symbol_bbox.Centre();
        int     offs_x = ( m_symbol_bbox.GetWidth() + m_fbox_size.GetWidth() ) / 2;
        int     offs_y = ( m_symbol_bbox.GetHeight() + m_fbox_size.GetHeight() ) / 2;

        if( aFieldSide.x != 0 )
            offs_x += HPADDING;
        else if( aFieldSide.y != 0 )
            offs_y += VPADDING;

        fbox_center.x += aFieldSide.x * offs_x;
        fbox_center.y += aFieldSide.y * offs_y;

        wxPoint fbox_pos( fbox_center.x - m_fbox_size.GetWidth() / 2,
                          fbox_center.y - m_fbox_size.GetHeight() / 2 );

        return fbox_pos;
    }

    /**
     * Shift a field box up or down a bit to make the fields fit between some wires.
     * Returns true if a shift was made.
     */
    bool fitFieldsBetweenWires( EDA_RECT* aBox, SIDE aSide )
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

            wxPoint start = line->GetStartPoint(), end = line->GetEndPoint();

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

        wxPoint pos = aBox->GetPosition();

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
    int fieldHorizPlacement( SCH_FIELD *aField, const EDA_RECT &aFieldBox )
    {
        int field_hjust;
        int field_xcoord;

        if( aField->IsHorizJustifyFlipped() )
            field_hjust = -aField->GetHorizJustify();
        else
            field_hjust = aField->GetHorizJustify();

        switch( field_hjust )
        {
        case GR_TEXT_HJUSTIFY_LEFT:
            field_xcoord = aFieldBox.GetLeft();
            break;
        case GR_TEXT_HJUSTIFY_CENTER:
            field_xcoord = aFieldBox.Centre().x;
            break;
        case GR_TEXT_HJUSTIFY_RIGHT:
            field_xcoord = aFieldBox.GetRight();
            break;
        default:
            wxFAIL_MSG( "Unexpected value for SCH_FIELD::GetHorizJustify()" );
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
     * @param aPosAccum - pointer to a position accumulator
     * @param aDynamic - use dynamic spacing
     *
     * @return Correct field vertical position
     */
    int fieldVertPlacement( SCH_FIELD *aField, const EDA_RECT &aFieldBox, int *aPosAccum,
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
            padding = round_n( field_height, Mils2iu( 50 ), true ) - field_height;
        }
        else
        {
            field_height = aField->GetBoundingBox().GetHeight();
            padding = FIELD_PADDING;
        }

        int placement = *aPosAccum + padding / 2 + field_height / 2;

        *aPosAccum += padding + field_height;

        return placement;
    }

private:
    SCH_SCREEN*             m_screen;
    SCH_COMPONENT*          m_symbol;
    std::vector<SCH_FIELD*> m_fields;
    std::vector<SCH_ITEM*>  m_colliders;
    EDA_RECT                m_symbol_bbox;
    wxSize                  m_fbox_size;
    bool                    m_allow_rejustify;
    bool                    m_align_to_grid;
    bool                    m_is_power_symbol;
};


const AUTOPLACER::SIDE AUTOPLACER::SIDE_TOP( 0, -1 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_BOTTOM( 0, 1 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_LEFT( -1, 0 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_RIGHT( 1, 0 );


void SCH_COMPONENT::AutoplaceFields( SCH_SCREEN* aScreen, bool aManual )
{
    if( aManual )
        wxASSERT_MSG( aScreen, "A SCH_SCREEN pointer must be given for manual autoplacement" );

    AUTOPLACER autoplacer( this, aScreen );
    autoplacer.DoAutoplace( aManual );
    m_fieldsAutoplaced = ( aManual ? FIELDS_AUTOPLACED_MANUAL : FIELDS_AUTOPLACED_AUTO );
}

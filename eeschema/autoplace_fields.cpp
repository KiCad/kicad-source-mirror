/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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
 * 1. Compute the dimensions of the fields' bounding box    ::ComputeFBoxSize
 * 2. Determine which side the fields will go on.           ::choose_side_for_fields
 *      1. Sort the four sides in preference order,
 *          depending on the component's shape and
 *          orientation                                     ::get_preferred_sides
 *      2. If in manual mode, sift out the sides that would
 *          cause fields to overlap other items             ::get_colliding_sides
 *      3. If any remaining sides have zero pins there,
 *          choose the highest zero-pin side according to
 *          preference order.
 *      4. If all sides have pins, choose the side with the
 *          fewest pins.
 * 3. Compute the position of the fields' bounding box      ::field_box_placement
 * 4. In manual mode, shift the box vertically if possible
 *      to fit fields between adjacent wires                ::fit_fields_between_wires
 * 5. Move all fields to their final positions
 *      1. Re-justify fields if options allow that          ::justify_field
 *      2. Round to a 50-mil grid coordinate if desired
 */

#include <schframe.h>
#include <hotkeys_basic.h>
#include <sch_component.h>
#include <sch_line.h>
#include <lib_pin.h>
#include <class_drawpanel.h>
#include <class_libentry.h>
#include <eeschema_config.h>
#include <kiface_i.h>
#include <boost/foreach.hpp>
#include <vector>
#include <algorithm>

#define FIELD_PADDING 10            // arbitrarily chosen for aesthetics
#define FIELD_PADDING_ALIGNED 18    // aligns 50 mil text to a 100 mil grid
#define WIRE_V_SPACING 100
#define HPADDING 25
#define VPADDING 25

/**
 * Function round_n
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
 * Function TO_HJUSTIFY
 * Converts an integer to a horizontal justification; neg=L zero=C pos=R
 */
EDA_TEXT_HJUSTIFY_T TO_HJUSTIFY( int x )
{
    return static_cast<EDA_TEXT_HJUSTIFY_T>( x );
}


class AUTOPLACER
{
    SCH_SCREEN* m_screen;
    SCH_COMPONENT* m_component;
    std::vector<SCH_FIELD*> m_fields;
    std::vector<SCH_ITEM*> m_colliders;
    EDA_RECT m_comp_bbox;
    wxSize m_fbox_size;
    bool m_allow_rejustify, m_align_to_grid;
    bool m_power_symbol;

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


    AUTOPLACER( SCH_COMPONENT* aComponent, SCH_SCREEN* aScreen )
        :m_screen( aScreen ), m_component( aComponent )
    {
        m_component->GetFields( m_fields, /* aVisibleOnly */ true );
        Kiface().KifaceSettings()->Read( AUTOPLACE_JUSTIFY_KEY, &m_allow_rejustify, true );
        Kiface().KifaceSettings()->Read( AUTOPLACE_ALIGN_KEY, &m_align_to_grid, false );

        m_comp_bbox = m_component->GetBodyBoundingBox();
        m_fbox_size = ComputeFBoxSize( /* aDynamic */ true );

        m_power_symbol = ! m_component->IsInNetlist();

        if( aScreen )
            get_possible_colliders( m_colliders );
    }


    /**
     * Do the actual autoplacement.
     * @param aManual - if true, use extra heuristics for smarter placement when manually
     * called up.
     */
    void DoAutoplace( bool aManual )
    {
        bool force_wire_spacing = false;
        SIDE field_side = choose_side_for_fields( aManual );
        wxPoint fbox_pos = field_box_placement( field_side );
        EDA_RECT field_box( fbox_pos, m_fbox_size );

        if( aManual )
            force_wire_spacing = fit_fields_between_wires( &field_box, field_side );

        // Move the fields
        int last_y_coord = field_box.GetTop();
        for( unsigned field_idx = 0; field_idx < m_fields.size(); ++field_idx )
        {
            SCH_FIELD* field = m_fields[field_idx];

            if( m_allow_rejustify )
                justify_field( field, field_side );

            wxPoint pos(
                field_horiz_placement( field, field_box ),
                field_vert_placement( field, field_box, &last_y_coord, !force_wire_spacing ) );

            if( m_align_to_grid )
            {
                pos.x = round_n( pos.x, 50, field_side.x >= 0 );
                pos.y = round_n( pos.y, 50, field_side.y == 1 );
            }

            field->SetPosition( pos );
        }
    }


protected:
    /**
     * Compute and return the size of the fields' bounding box.
     * @param aDynamic - if true, use dynamic spacing
     */
    wxSize ComputeFBoxSize( bool aDynamic )
    {
        int max_field_width = 0;
        int total_height = 0;

        BOOST_FOREACH( SCH_FIELD* field, m_fields )
        {
            int field_width;
            int field_height;

            if( m_component->GetTransform().y1 )
            {
                field->SetOrientation( TEXT_ORIENT_VERT );
            }
            else
            {
                field->SetOrientation( TEXT_ORIENT_HORIZ );
            }

            field_width = field->GetBoundingBox().GetWidth();
            field_height = field->GetBoundingBox().GetHeight();

            max_field_width = std::max( max_field_width, field_width );

            if( aDynamic )
                total_height += field_height + get_field_padding();
            else
                total_height += WIRE_V_SPACING;

        }

        return wxSize( max_field_width, total_height );
    }


    /**
     * Function get_pin_side
     * Return the side that a pin is on.
     */
    SIDE get_pin_side( LIB_PIN* aPin )
    {
        int pin_orient = aPin->PinDrawOrient( m_component->GetTransform() );
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
     * Function pins_on_side
     * Count the number of pins on a side of the component.
     */
    unsigned pins_on_side( SIDE aSide )
    {
        unsigned pin_count = 0;

        std::vector<LIB_PIN*> pins;
        m_component->GetPins( pins );

        BOOST_FOREACH( LIB_PIN* each_pin, pins )
        {
            if( !each_pin->IsVisible() && !m_power_symbol )
                continue;
            if( get_pin_side( each_pin ) == aSide )
                ++pin_count;
        }

        return pin_count;
    }


    /**
     * Function get_possible_colliders
     * Populate a list of all drawing items that *may* collide with the fields. That is,
     * all drawing items, including other fields, that are not the current component or
     * its own fields.
     */
    void get_possible_colliders( std::vector<SCH_ITEM*>& aItems )
    {
        wxASSERT_MSG( m_screen, "get_possible_colliders() with null m_screen" );
        for( SCH_ITEM* item = m_screen->GetDrawItems(); item; item = item->Next() )
        {
            if( SCH_COMPONENT* comp = dynamic_cast<SCH_COMPONENT*>( item ) )
            {
                if( comp == m_component ) continue;

                std::vector<SCH_FIELD*> fields;
                comp->GetFields( fields, /* aVisibleOnly */ true );
                BOOST_FOREACH( SCH_FIELD* field, fields )
                    aItems.push_back( field );
            }
            aItems.push_back( item );
        }
    }


    /**
     * Function filtered_colliders
     * Filter a list of possible colliders to include only those that actually collide
     * with a given rectangle. Returns the new vector.
     */
    std::vector<SCH_ITEM*> filtered_colliders( const EDA_RECT& aRect )
    {
        std::vector<SCH_ITEM*> filtered;
        BOOST_FOREACH( SCH_ITEM* item, m_colliders )
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
     * Function get_preferred_sides
     * Return a list with the preferred field sides for the component, in
     * decreasing order of preference.
     */
    std::vector<SIDE_AND_NPINS> get_preferred_sides()
    {
        SIDE_AND_NPINS sides_init[] = {
            { SIDE_RIGHT,   pins_on_side( SIDE_RIGHT ) },
            { SIDE_TOP,     pins_on_side( SIDE_TOP ) },
            { SIDE_LEFT,    pins_on_side( SIDE_LEFT ) },
            { SIDE_BOTTOM,  pins_on_side( SIDE_BOTTOM ) },
        };
        std::vector<SIDE_AND_NPINS> sides( sides_init, sides_init + DIM( sides_init ) );

        int orient = m_component->GetOrientation();
        int orient_angle = orient & 0xff; // enum is a bitmask
        bool h_mirrored = ( ( orient & CMP_MIRROR_X )
                && ( orient_angle == CMP_ORIENT_0 || orient_angle == CMP_ORIENT_180 ) );
        double w = double( m_comp_bbox.GetWidth() );
        double h = double( m_comp_bbox.GetHeight() );

        // The preferred-sides heuristics are a bit magical. These were determined mostly
        // by trial and error.

        if( m_power_symbol )
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
     * Function get_colliding_sides
     * Return a list of the sides where a field set would collide with another item.
     */
    std::vector<SIDE_AND_COLL> get_colliding_sides()
    {
        SIDE sides_init[] = { SIDE_RIGHT, SIDE_TOP, SIDE_LEFT, SIDE_BOTTOM };
        std::vector<SIDE> sides( sides_init, sides_init + DIM( sides_init ) );
        std::vector<SIDE_AND_COLL> colliding;

        // Iterate over all sides and find the ones that collide
        BOOST_FOREACH( SIDE side, sides )
        {
            EDA_RECT box( field_box_placement( side ), m_fbox_size );

            COLLISION collision = COLLIDE_NONE;
            BOOST_FOREACH( SCH_ITEM* collider, filtered_colliders( box ) )
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
                    collision = COLLIDE_OBJECTS;
            }

            if( collision != COLLIDE_NONE )
                colliding.push_back( (SIDE_AND_COLL){ side, collision } );
        }

        return colliding;
    }


    /**
     * Function choose_side_filtered
     * Choose a side for the fields, filtered on only one side collision type.
     * Removes the sides matching the filter from the list.
     */
    SIDE_AND_NPINS choose_side_filtered( std::vector<SIDE_AND_NPINS>& aSides,
            const std::vector<SIDE_AND_COLL>& aCollidingSides, COLLISION aCollision,
            SIDE_AND_NPINS aLastSelection)
    {
        SIDE_AND_NPINS sel = aLastSelection;

        std::vector<SIDE_AND_NPINS>::iterator it = aSides.begin();
        while( it != aSides.end() )
        {
            bool collide = false;
            BOOST_FOREACH( SIDE_AND_COLL collision, aCollidingSides )
            {
                if( collision.side == it->side && collision.collision == aCollision )
                    collide = true;
            }
            if( !collide )
                ++it;
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
     * Function choose_side_for_fields
     * Look where a component's pins are to pick a side to put the fields on
     * @param aAvoidCollisions - if true, pick last the sides where the label will collide
     *      with other items.
     */
    SIDE choose_side_for_fields( bool aAvoidCollisions )
    {
        std::vector<SIDE_AND_NPINS> sides = get_preferred_sides();

        std::reverse( sides.begin(), sides.end() );
        SIDE_AND_NPINS side = { wxPoint( 1, 0 ), UINT_MAX };

        if( aAvoidCollisions )
        {
            std::vector<SIDE_AND_COLL> colliding_sides = get_colliding_sides();
            side = choose_side_filtered( sides, colliding_sides, COLLIDE_OBJECTS, side );
            side = choose_side_filtered( sides, colliding_sides, COLLIDE_H_WIRES, side );
        }

        BOOST_REVERSE_FOREACH( SIDE_AND_NPINS& each_side, sides )
        {
            if( !each_side.pins ) return each_side.side;
        }

        BOOST_FOREACH( SIDE_AND_NPINS& each_side, sides )
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
     * Function justify_field
     * Set the justification of a field based on the side it's supposed to be on, taking
     * into account whether the field will be displayed with flipped justification due to
     * mirroring.
     */
    void justify_field( SCH_FIELD* aField, SIDE aFieldSide )
    {
        // Justification is set twice to allow IsHorizJustifyFlipped() to work correctly.
        aField->SetHorizJustify( TO_HJUSTIFY( -aFieldSide.x ) );
        aField->SetHorizJustify( TO_HJUSTIFY( -aFieldSide.x *
                    ( aField->IsHorizJustifyFlipped() ? -1 : 1 ) ) );
        aField->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
    }


    /**
     * Function field_box_placement
     * Returns the position of the field bounding box.
     */
    wxPoint field_box_placement( SIDE aFieldSide )
    {
        wxPoint fbox_center = m_comp_bbox.Centre();
        int offs_x = ( m_comp_bbox.GetWidth() + m_fbox_size.GetWidth() ) / 2 + HPADDING;
        int offs_y = ( m_comp_bbox.GetHeight() + m_fbox_size.GetHeight() ) / 2 + VPADDING;

        fbox_center.x += aFieldSide.x * offs_x;
        fbox_center.y += aFieldSide.y * offs_y;

        wxPoint fbox_pos(
                fbox_center.x - m_fbox_size.GetWidth() / 2,
                fbox_center.y - m_fbox_size.GetHeight() / 2 );

        return fbox_pos;
    }


    /**
     * Function fit_fields_between_wires
     * Shift a field box up or down a bit to make the fields fit between some wires.
     * Returns true if a shift was made.
     */
    bool fit_fields_between_wires( EDA_RECT* aBox, SIDE aSide )
    {
        if( aSide != SIDE_TOP && aSide != SIDE_BOTTOM )
            return false;

        std::vector<SCH_ITEM*> colliders = filtered_colliders( *aBox );
        if( colliders.empty() )
            return false;

        // Find the offset of the wires for proper positioning
        int offset = 0;

        BOOST_FOREACH( SCH_ITEM* item, colliders )
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
        m_fbox_size = ComputeFBoxSize( /* aDynamic */ false );

        wxPoint pos = aBox->GetPosition();

        // Remove the existing padding to get a bit more space to work with
        if( aSide == SIDE_BOTTOM )
        {
            pos.y = m_comp_bbox.GetBottom() - get_field_padding();
        }
        else
        {
            pos.y = m_comp_bbox.GetTop() - m_fbox_size.y + get_field_padding();
        }

        pos.y = round_n( pos.y, WIRE_V_SPACING, aSide == SIDE_BOTTOM );

        aBox->SetOrigin( pos );
        return true;
    }


    /**
     * Function field_horiz_placement
     * Place a field horizontally, taking into account the field width and
     * justification.
     *
     * @param aField - the field to place.
     * @param aFieldBox - box in which fields will be placed
     *
     * @return Correct field horizontal position
     */
    int field_horiz_placement( SCH_FIELD *aField, const EDA_RECT &aFieldBox )
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
     * Function field_vert_placement
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
    int field_vert_placement( SCH_FIELD *aField, const EDA_RECT &aFieldBox, int *aPosAccum,
            bool aDynamic )
    {
        int field_height;
        int padding;

        if( aDynamic )
        {
            if( m_component->GetTransform().y1 )
                field_height = aField->GetBoundingBox().GetWidth();
            else
                field_height = aField->GetBoundingBox().GetHeight();
            field_height = aField->GetBoundingBox().GetHeight();

            padding = get_field_padding();
        }
        else
        {
            field_height = WIRE_V_SPACING / 2;
            padding = WIRE_V_SPACING / 2;
        }

        int placement = *aPosAccum + padding / 2 + field_height / 2;

        *aPosAccum += padding + field_height;

        return placement;
    }

    /**
     * Function get_field_padding
     * Return the desired padding between fields.
     */
    int get_field_padding()
    {
        if( m_align_to_grid )
            return FIELD_PADDING_ALIGNED;
        else
            return FIELD_PADDING;
    }

};

const AUTOPLACER::SIDE AUTOPLACER::SIDE_TOP( 0, -1 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_BOTTOM( 0, 1 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_LEFT( -1, 0 );
const AUTOPLACER::SIDE AUTOPLACER::SIDE_RIGHT( 1, 0 );


void SCH_EDIT_FRAME::OnAutoplaceFields( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_ITEM* item = screen->GetCurItem();

    // Get the item under cursor if we're not currently moving something
    if( !item )
    {
        if( aEvent.GetInt() == 0 )
            return;

        EDA_HOTKEY_CLIENT_DATA& data = dynamic_cast<EDA_HOTKEY_CLIENT_DATA&>(
                *aEvent.GetClientObject() );
        item = LocateItem( data.GetPosition(), SCH_COLLECTOR::MovableItems, aEvent.GetInt() );
        screen->SetCurItem( NULL );
        if( !item || item->GetFlags() )
            return;
    }

    SCH_COMPONENT* component = dynamic_cast<SCH_COMPONENT*>( item );
    if( !component )
        return;

    if( !component->IsNew() )
        SaveCopyInUndoList( component, UR_CHANGED );

    component->AutoplaceFields( screen, /* aManual */ true );

    GetCanvas()->Refresh();
    OnModify();
}


void SCH_COMPONENT::AutoplaceFields( SCH_SCREEN* aScreen, bool aManual )
{
    if( aManual )
        wxASSERT_MSG( aScreen, "A SCH_SCREEN pointer must be given for manual autoplacement" );
    AUTOPLACER autoplacer( this, aScreen );
    autoplacer.DoAutoplace( aManual );
    m_fieldsAutoplaced = ( aManual? AUTOPLACED_MANUAL : AUTOPLACED_AUTO );
}

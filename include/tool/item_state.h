/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

/**
 * Class ITEM_STATE
 *
 * Provides means for modifying properties of groups of items and gives possibility of rolling back
 * the introduced changes. Does not take ownership of modified items, neither takes care of
 * refreshing.
 */

#ifndef ITEM_STATE_H_
#define ITEM_STATE_H_

#include <deque>
#include <class_board_item.h>

class ITEM_STATE
{
public:
    ITEM_STATE() :
        m_movement( 0.0, 0.0 ), m_flips( 0 ), m_rotation( 0.0 )
    {
#ifdef __WXDEBUG__
        m_canSave = true;
#endif
    }

    /**
     * Function Save()
     *
     * Adds an item and saves it's state.
     * @param aItem is the item to be added.
     */
    void Save( BOARD_ITEM* aItem )
    {
#ifdef __WXDEBUG__
    wxASSERT_MSG( m_canSave, "You cannot save items after issuing commands. "
                             "You have either RestoreAll() or Apply() before adding items!" );
#endif
        m_items.push_back( aItem );
    }

    /**
     * Function RestoreAll()
     *
     * Rollbacks all the changes to the initial state.
     */
    void RestoreAll()
    {
        // Check if there is a not saved movement command
        saveMovement();

        std::deque<BOARD_ITEM*>::iterator it, it_end;
        std::deque<COMMAND>::iterator cmd, cmd_end;
        for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
        {
            for( cmd = m_commands.begin(), cmd_end = m_commands.end(); cmd != cmd_end; ++cmd )
                cmd->Revert( *it );
        }

        reset();
    }

    /**
     * Function Apply()
     *
     * Resets the state, clears the list of items & changes, so the object can be reused for
     * other items.
     */
    void Apply()
    {
        reset();
    }

    /**
     * Function Move()
     *
     * Moves stored items by a given vector.
     * @param aMovement is the movement vector.
     */
    void Move( const VECTOR2D& aMovement )
    {
#ifdef __WXDEBUG__
        m_canSave = false;
#endif
        std::deque<BOARD_ITEM*>::iterator it, it_end;
        for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
            (*it)->Move( wxPoint( aMovement.x, aMovement.y ) );

        m_movement += aMovement;
    }

    /**
     * Function Rotate()
     *
     * Rotates stored items by a given angle.
     * @param aAngle is the angle (in decidegrees).
     */
    void Rotate( const VECTOR2D& aPoint, double aAngle )
    {
#ifdef __WXDEBUG__
        m_canSave = false;
#endif
        saveMovement();
        m_commands.push_front( COMMAND( COMMAND::ROTATE, aPoint, aAngle ) );

        std::deque<BOARD_ITEM*>::iterator it, it_end;
        for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
            (*it)->Rotate( wxPoint( aPoint.x, aPoint.y ), aAngle );

        m_rotation += aAngle;
    }

    /**
     * Function Flip()
     *
     * Changes the board side for stored items.
     * @param aPoint is the rotation point.
     */
    void Flip( const VECTOR2D& aPoint )
    {
#ifdef __WXDEBUG__
        m_canSave = false;
#endif
        saveMovement();
        m_commands.push_front( COMMAND( COMMAND::FLIP, aPoint ) );

        std::deque<BOARD_ITEM*>::iterator it, it_end;
        for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
            (*it)->Flip( wxPoint( aPoint.x, aPoint.y ) );

        m_flips++;
    }

    /**
     * Function ToggleVisibility()
     *
     * Switches the visibility property of stored items.
     */
    void ToggleVisibility()
    {
#ifdef __WXDEBUG__
        m_canSave = false;
#endif
        m_commands.push_front( COMMAND( COMMAND::VISIBILITY ) );

        std::deque<BOARD_ITEM*>::iterator it, it_end;
        for( it = m_items.begin(), it_end = m_items.end(); it != it_end; ++it )
            (*it)->ViewSetVisible( !(*it)->ViewIsVisible() );
    }

    /**
     * Function GetUpdateFlag()
     *
     * Returns information on what kind of update should be applied to items in order to display
     * them properly.
     * @return Flag required to refresh items.
     */
    KiGfx::VIEW_ITEM::ViewUpdateFlags GetUpdateFlag() const
    {
        if( m_flips % 2 == 1 )  // If number of flips is odd, then we need to change layers
            return KiGfx::VIEW_ITEM::LAYERS;
        else if( m_movement.x != 0.0 || m_movement.y != 0.0 || m_rotation != 0.0 )
            return KiGfx::VIEW_ITEM::GEOMETRY;

        return KiGfx::VIEW_ITEM::APPEARANCE;
    }

private:
    /// COMMAND stores modifications that were done to items
    struct COMMAND
    {
        /// Type of command
        enum TYPE { MOVE, ROTATE, FLIP, VISIBILITY };
        TYPE m_type;

        /// Point where flip/rotation occurred or movement vector
        VECTOR2D m_point;

        /// Used only for rotation
        double m_angle;

        COMMAND( TYPE aType, VECTOR2D aPoint = VECTOR2D( 0.0, 0.0 ), double aAngle = 0.0 ) :
            m_type( aType ), m_point( aPoint ), m_angle( aAngle ) {};

        void Revert( BOARD_ITEM* aItem )
        {
            switch( m_type )
            {
            case MOVE:
                aItem->Move( wxPoint( -m_point.x, -m_point.y ) );
                break;

            case ROTATE:
                aItem->Rotate( wxPoint( m_point.x, m_point.y ), -m_angle );
                break;

            case FLIP:
                aItem->Flip( wxPoint( m_point.x, m_point.y ) );
                break;

            case VISIBILITY:
                aItem->ViewSetVisible( !aItem->ViewIsVisible() );
                break;
            }
        }
    };

    /// Adds a MOVEMENT command basing on the current movement vector
    void saveMovement()
    {
        if( m_movement.x != 0.0 || m_movement.y != 0.0 )
        {
            m_commands.push_front( COMMAND( COMMAND::MOVE, m_movement ) );

            m_movement.x = 0.0;
            m_movement.y = 0.0;
        }
    }

    /// Restores the initial state
    void reset()
    {
        m_movement.x = 0.0;
        m_movement.y = 0.0;
        m_flips = 0;
        m_rotation = 0.0;

        m_items.clear();
        m_commands.clear();

#ifdef __WXDEBUG__
        m_canSave = true;
#endif
    }

    /// List of issued commands
    std::deque<BOARD_ITEM*> m_items;

    /// List of items that are affected by commands
    std::deque<COMMAND> m_commands;

    /// Current movement vector (updated by Move() command)
    VECTOR2D m_movement;

    /// Number of flips applied to items
    unsigned int m_flips;

    /// Total rotation applied to items
    double m_rotation;

#ifdef __WXDEBUG__
    /// Debug flag assuring that functions are called in proper order
    bool m_canSave;
#endif
};

#endif /* ITEM_STATE_H_ */


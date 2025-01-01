/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef CONDITIONAL_MENU_H
#define CONDITIONAL_MENU_H

#include <tool/selection_conditions.h>
#include <tool/action_menu.h>
#include <list>

class PCB_SELECTION_TOOL;
class TOOL_ACTION;
class TOOL_INTERACTIVE;

enum class BITMAPS : unsigned int;


class CONDITIONAL_MENU : public ACTION_MENU
{
public:
    ///< Constant to indicate that we do not care about an #ENTRY location in the menu.
    static const int ANY_ORDER = -1;

    CONDITIONAL_MENU( TOOL_INTERACTIVE* aTool );

    ACTION_MENU* create() const override;

    /**
     * Add a menu entry to run a #TOOL_ACTION on selected items.
     *
     * @param aAction is a menu entry to be added.
     * @param aCondition is a condition that has to be fulfilled to show the menu entry in the menu.
     * @param aOrder determines location of the added item, higher numbers are put on the bottom.
     *               You may use ANY_ORDER here if you think it does not matter.
     */
    void AddItem( const TOOL_ACTION& aAction, const SELECTION_CONDITION& aCondition,
                  int aOrder = ANY_ORDER );

    void AddItem( int aId, const wxString& aText, const wxString& aTooltip, BITMAPS aIcon,
                  const SELECTION_CONDITION& aCondition, int aOrder = ANY_ORDER );

    /**
     * Add a checked menu entry to run a TOOL_ACTION on selected items.
     *
     * The condition for checking the menu entry should be supplied through a #ACTION_CONDITION
     * registered with the #ACTION_MANAGER.
     *
     * @param aAction is a menu entry to be added.
     * @param aCondition is a condition that has to be fulfilled to show the menu entry in the menu.
     * @param aOrder determines location of the added item, higher numbers are put on the bottom.
     *               You may use #ANY_ORDER here if you think it does not matter.
     */
    void AddCheckItem( const TOOL_ACTION& aAction, const SELECTION_CONDITION& aCondition,
                       int aOrder = ANY_ORDER );

    void AddCheckItem( int aId, const wxString& aText, const wxString& aTooltip, BITMAPS aIcon,
                       const SELECTION_CONDITION& aCondition, int aOrder = ANY_ORDER );

    /**
     * Add a submenu to the menu.
     * CONDITIONAL_MENU takes ownership of the added menu, so it will be freed when the
     * CONDITIONAL_MENU object is destroyed.
     *
     * @param aMenu is the submenu to be added.
     * @param aExpand determines if the added submenu items should be added as individual items
     *                or as a submenu.
     * @param aCondition is a condition that has to be fulfilled to show the submenu entry in
     *                   the menu.
     * @param aOrder determines location of the added menu, higher numbers are put on the bottom.
     *               You may use ANY_ORDER here if you think it does not matter.
     */
    void AddMenu( ACTION_MENU* aMenu,
                  const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                  int aOrder = ANY_ORDER );

    /**
     * Add a separator to the menu.
     *
     * @param aOrder determines location of the separator, higher numbers are put on the bottom.
     */
    void AddSeparator( int aOrder = ANY_ORDER );

    void AddSeparator( const SELECTION_CONDITION& aCondition, int aOrder = ANY_ORDER );

    /**
     * Update the contents of the menu based on the supplied conditions.
     */
    void Evaluate( const SELECTION& aSelection );

    /**
     * Update the initial contents so that wxWidgets doesn't get its knickers tied in a knot
     * over the menu being empty (mainly an issue on GTK, but also on OSX with the preferences
     * and quit menu items).
     */
     void Resolve();

private:
    ///< Helper class to organize menu entries.
    class ENTRY
    {
    public:
        ENTRY( const TOOL_ACTION* aAction, SELECTION_CONDITION aCondition, int aOrder,
               bool aCheckmark ) :
            m_type( ACTION ),
            m_icon( static_cast<BITMAPS>( 0 ) ),
            m_condition( std::move( aCondition ) ),
            m_order( aOrder ),
            m_isCheckmarkEntry( aCheckmark )
        {
            m_data.action = aAction;
        }

        ENTRY( ACTION_MENU* aMenu, SELECTION_CONDITION aCondition, int aOrder ) :
            m_type( MENU ),
            m_icon( static_cast<BITMAPS>( 0 ) ),
            m_condition( std::move( aCondition ) ),
            m_order( aOrder ),
            m_isCheckmarkEntry( false )
        {
            m_data.menu = aMenu;
        }

        ENTRY( const wxMenuItem& aItem, BITMAPS aBitmap,
               SELECTION_CONDITION aCondition, int aOrder, bool aCheckmark ) :
            m_type( WXITEM ),
            m_icon( aBitmap ),
            m_condition( std::move( aCondition ) ),
            m_order( aOrder ),
            m_isCheckmarkEntry( aCheckmark )
        {
            m_data.wxItem = new wxMenuItem( nullptr, aItem.GetId(), aItem.GetItemLabel(),
                                            aItem.GetHelp(), aItem.GetKind() );
        }

        // Separator
        ENTRY( SELECTION_CONDITION aCondition, int aOrder ) :
            m_type( SEPARATOR ),
            m_icon( static_cast<BITMAPS>( 0 ) ),
            m_data(),
            m_condition( std::move( aCondition ) ),
            m_order( aOrder ),
            m_isCheckmarkEntry( false )
        {
        }

        ENTRY( const ENTRY& aEntry );

        ~ENTRY();

        ///< Possible entry types.
        enum ENTRY_TYPE {
            ACTION,
            MENU,
            WXITEM,
            SEPARATOR
        };

        inline ENTRY_TYPE Type() const
        {
            return m_type;
        }

        inline BITMAPS GetIcon() const
        {
            return m_icon;
        }

        inline const TOOL_ACTION* Action() const
        {
            assert( m_type == ACTION );
            return m_data.action;
        }

        inline ACTION_MENU* Menu() const
        {
            assert( m_type == MENU );
            return m_data.menu;
        }

        inline wxMenuItem* wxItem() const
        {
            assert( m_type == WXITEM );
            return m_data.wxItem;
        }

        inline bool IsCheckmarkEntry() const
        {
            return m_isCheckmarkEntry;
        }

        inline const SELECTION_CONDITION& Condition() const
        {
            return m_condition;
        }

        inline int Order() const
        {
            return m_order;
        }

        inline void SetOrder( int aOrder )
        {
            m_order = aOrder;
        }

    private:
        ENTRY_TYPE m_type;
        BITMAPS m_icon;

        // This class owns the wxItem object and needs to create, copy and delete it accordingly
        // But it does not own the action nor menu item
        union {
            const TOOL_ACTION* action;
            ACTION_MENU*       menu;
            wxMenuItem*        wxItem;
        } m_data;

        ///< Condition to be fulfilled to show the entry in menu.
        SELECTION_CONDITION m_condition;

        ///< Order number, the higher the number the lower position it takes it is in the menu.
        int m_order;

        bool m_isCheckmarkEntry;
    };

    ///< Inserts the entry, preserving the requested order.
    void addEntry( ENTRY aEntry );

    ///< List of all menu entries.
    std::list<ENTRY> m_entries;
};

#endif /* CONDITIONAL_MENU_H */

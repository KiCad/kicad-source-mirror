/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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

#include <tool/context_menu.h>
#include "selection_conditions.h"

#include <boost/unordered_map.hpp>

class SELECTION_TOOL;

class CONDITIONAL_MENU
{
public:
    CONDITIONAL_MENU() {}
    ~CONDITIONAL_MENU();

    ///> Constant to indicate that we do not care about an ENTRY location in the menu.
    static const int ANY_ORDER = -1;

    /**
     * Function AddItem()
     *
     * Adds a menu entry to run a TOOL_ACTION on selected items.
     * @param aAction is a menu entry to be added.
     * @param aCondition is a condition that has to be fulfilled to enable the menu entry.
     * @param aOrder determines location of the added item, higher numbers are put on the bottom.
     * You may use ANY_ORDER here if you think it does not matter.
     */
    void AddItem( const TOOL_ACTION& aAction,
                  const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                  int aOrder = ANY_ORDER );

    /**
     * Function AddMenu()
     *
     * Adds a submenu to the menu. CONDITIONAL_MENU takes ownership of the added menu, so it will
     * be freed when the CONDITIONAL_MENU object is destroyed.
     * @param aMenu is the submenu to be added.
     * @param aLabel is the label of added submenu.
     * @param aExpand determines if the added submenu items should be added as individual items
     * or as a submenu.
     * @param aCondition is a condition that has to be fulfilled to enable the submenu entry.
     * @param aOrder determines location of the added menu, higher numbers are put on the bottom.
     * You may use ANY_ORDER here if you think it does not matter.
     */
    void AddMenu( CONTEXT_MENU* aMenu, const wxString& aLabel, bool aExpand = false,
                  const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                  int aOrder = ANY_ORDER );

    /**
     * Function AddSeparator()
     *
     * Adds a separator to the menu.
     * @param aCondition is a condition that has to be fulfilled to enable the submenu entry.
     * @param aOrder determines location of the added menu, higher numbers are put on the bottom.
     * You may use ANY_ORDER here if you think it does not matter.
     */
    void AddSeparator( const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                       int aOrder = ANY_ORDER );

    /**
     * Function Generate()
     *
     * Generates a context menu that contains only entries that are satisfying assigned conditions.
     * @param aSelection is selection for which the conditions are checked against.
     * @return Menu filtered by the entry conditions.
     */
    CONTEXT_MENU& Generate( SELECTION& aSelection );

private:
    ///> Returned menu instance, prepared by Generate() function.
    CONTEXT_MENU m_menu;

    ///> Helper class to organize menu entries.
    class ENTRY
    {
    public:
        ENTRY( const TOOL_ACTION* aAction,
                            const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                            int aOrder = ANY_ORDER ) :
            m_type( ACTION ), m_condition( aCondition ), m_order( aOrder ), m_expand( false )
        {
            m_data.action = aAction;
        }

        ENTRY( CONTEXT_MENU* aMenu, const wxString aLabel, bool aExpand = false,
                            const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                            int aOrder = ANY_ORDER ) :
            m_type( MENU ), m_condition( aCondition ), m_order( aOrder ), m_label( aLabel ), m_expand( aExpand )
        {
            m_data.menu = aMenu;
        }

        ENTRY( wxMenuItem* aItem, const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                            int aOrder = ANY_ORDER ) :
            m_type( WXITEM ), m_condition( aCondition ), m_order( aOrder ), m_expand( false )
        {
            m_data.wxItem = aItem;
        }

        // Separator
        ENTRY( const SELECTION_CONDITION& aCondition = SELECTION_CONDITIONS::ShowAlways,
                            int aOrder = ANY_ORDER ) :
            m_type( SEPARATOR ), m_condition( aCondition ), m_order( aOrder ), m_expand( false )
        {
            m_data.wxItem = NULL;
        }

        ///> Possible entry types.
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

        inline const TOOL_ACTION* Action() const
        {
            assert( m_type == ACTION );
            return m_data.action;
        }

        inline CONTEXT_MENU* Menu() const
        {
            assert( m_type == MENU );
            return m_data.menu;
        }

        inline wxMenuItem* wxItem() const
        {
            assert( m_type == WXITEM );
            return m_data.wxItem;
        }

        inline const wxString& Label() const
        {
            assert( m_type == MENU );
            return m_label;
        }

        inline bool Expand() const
        {
            assert( m_type == MENU );
            return m_expand;
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

        union {
            const TOOL_ACTION* action;
            CONTEXT_MENU* menu;
            wxMenuItem* wxItem;
        } m_data;

        ///> Condition to be fulfilled to show the entry in menu.
        SELECTION_CONDITION m_condition;

        ///> Order number, the higher the number the lower position it takes it is in the menu.
        int m_order;

        /// CONTEXT_MENU specific fields.
        const wxString m_label;
        bool m_expand;
    };

    ///> Inserts the entry, preserving the requested order.
    void addEntry( ENTRY aEntry );

    ///> List of all menu entries.
    std::list<ENTRY> m_entries;
};

#endif /* CONDITIONAL_MENU_H */

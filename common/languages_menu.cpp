/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file languages_menu.cpp
 *
 * @brief build the standard menu to show the list of available translations
 */

#include <pgm_base.h>
#include <id.h>
#include <menus_helpers.h>
#include <tool/tool_interactive.h>
#include <tool/action_menu.h>
#include <tool/conditional_menu.h>
#include <bitmaps.h>

/**
 * Function AddMenuLanguageList
 * creates a menu list for language choice, and add it as submenu to \a MasterMenu.
 *
 * @param aMasterMenu is the main menu.
 * @param aControlTool is the tool to associate with the menu
 */
void AddMenuLanguageList( ACTION_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool )
{
    ACTION_MENU* langsMenu = new ACTION_MENU( false, aControlTool );
    langsMenu->SetTitle( _( "Set Language" ) );
    langsMenu->SetIcon( BITMAPS::language );

    wxString tooltip;

    for( unsigned ii = 0;  LanguagesList[ii].m_KI_Lang_Identifier != 0; ii++ )
    {
        wxString label;

        if( LanguagesList[ii].m_DoNotTranslate )
            label = LanguagesList[ii].m_Lang_Label;
        else
            label = wxGetTranslation( LanguagesList[ii].m_Lang_Label );

        wxMenuItem* item = new wxMenuItem( langsMenu,
                                           LanguagesList[ii].m_KI_Lang_Identifier,    // wxMenuItem wxID
                                           label,
                                           tooltip,
                                           wxITEM_CHECK );

        langsMenu->Append( item );
    }

    // This must be done after the items are added
    aMasterMenu->Add( langsMenu );
}

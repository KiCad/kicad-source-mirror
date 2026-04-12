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

/**
 * @file  sch_action_plugin.h
 * @brief Action plugin support for Eeschema
 */

#ifndef SCH_ACTION_PLUGIN_H
#define SCH_ACTION_PLUGIN_H

#include <vector>
#include <wx/string.h>
#include <wx/bitmap.h>


/**
 * Base class for eeschema action plugins.
 */
class SCH_ACTION_PLUGIN
{
public:
    SCH_ACTION_PLUGIN() : m_actionMenuId( 0 ), m_actionButtonId( 0 ),
                          show_on_toolbar( false ) {}
    virtual ~SCH_ACTION_PLUGIN();

    virtual wxString GetCategoryName() = 0;
    virtual wxString GetName() = 0;
    virtual wxString GetClassName() = 0;
    virtual wxString GetDescription() = 0;
    virtual bool     GetShowToolbarButton() = 0;
    virtual wxString GetIconFileName( bool aDark ) = 0;
    virtual wxString GetPluginPath() = 0;
    virtual void*    GetObject() = 0;
    virtual void     Run() = 0;

    void register_action();

    int      m_actionMenuId;
    int      m_actionButtonId;
    wxBitmap iconBitmap;
    bool     show_on_toolbar;
};


/**
 * Static registry for eeschema action plugins.
 */
class SCH_ACTION_PLUGINS
{
public:
    static void               register_action( SCH_ACTION_PLUGIN* aAction );
    static bool               deregister_object( void* aObject );
    static SCH_ACTION_PLUGIN* GetAction( const wxString& aName );
    static void               SetActionMenu( int aIndex, int idMenu );
    static SCH_ACTION_PLUGIN* GetActionByMenu( int aMenu );
    static void               SetActionButton( SCH_ACTION_PLUGIN* aAction, int idButton );
    static SCH_ACTION_PLUGIN* GetActionByButton( int aButton );
    static SCH_ACTION_PLUGIN* GetActionByPath( const wxString& aPath );
    static SCH_ACTION_PLUGIN* GetAction( int aIndex );
    static int                GetActionsCount();
    static bool               IsActionRunning();
    static void               SetActionRunning( bool aRunning );
    static void               UnloadAll();

private:
    static std::vector<SCH_ACTION_PLUGIN*> m_actionsList;
    static bool                            m_actionRunning;
};


#endif /* SCH_ACTION_PLUGIN_H */

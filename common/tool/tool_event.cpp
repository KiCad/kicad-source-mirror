/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <cstring>
#include <string>

#include <tool/tool_event.h>
#include <tool/tool_action.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>


#include <wx/debug.h>

struct FlagString
{
    int flag;
    std::string str;
};


static const std::string flag2string( int aFlag, const FlagString* aExps )
{
    std::string rv;

    for( int i = 0; aExps[i].str.length(); i++ )
    {
        if( aExps[i].flag & aFlag )
            rv += aExps[i].str + " ";
    }

    return rv;
}


void TOOL_EVENT::init()
{
    // By default only MESSAGEs and Cancels are passed to multiple recipients
    m_passEvent = m_category == TC_MESSAGE || IsCancelInteractive() || IsActivate();

    m_hasPosition = ( m_category == TC_MOUSE || m_category == TC_COMMAND );

    // Cancel tool doesn't contain a position
    if( IsCancel() )
        m_hasPosition = false;

    m_forceImmediate = false;
    m_reactivate = false;
}


VECTOR2D TOOL_EVENT::returnCheckedPosition( const VECTOR2D& aPos ) const
{
    wxCHECK_MSG( HasPosition(), VECTOR2D(), "Attempted to get position from non-position event" );

    return aPos;
}


bool TOOL_EVENT::IsAction( const TOOL_ACTION* aAction ) const
{
    return Matches( aAction->MakeEvent() );
}


bool TOOL_EVENT::IsActionInGroup( const TOOL_ACTION_GROUP& aGroup ) const
{
    if( m_actionGroup.has_value() )
        return m_actionGroup.value() == aGroup;

    return false;
}


const std::string TOOL_EVENT::Format() const
{
    std::string ev;

    const FlagString categories[] =
    {
        { TC_MOUSE,    "mouse"    },
        { TC_KEYBOARD, "keyboard" },
        { TC_COMMAND,  "command"  },
        { TC_MESSAGE,  "message"  },
        { TC_VIEW,     "view"     },
        { 0,           ""         }
    };

    const FlagString actions[] =
    {
        { TA_MOUSE_CLICK,        "click"               },
        { TA_MOUSE_DBLCLICK,     "double click"        },
        { TA_MOUSE_UP,           "button-up"           },
        { TA_MOUSE_DOWN,         "button-down"         },
        { TA_MOUSE_DRAG,         "drag"                },
        { TA_MOUSE_MOTION,       "motion"              },
        { TA_MOUSE_WHEEL,        "wheel"               },
        { TA_KEY_PRESSED,        "key-pressed"         },
        { TA_VIEW_REFRESH,       "view-refresh"        },
        { TA_VIEW_ZOOM,          "view-zoom"           },
        { TA_VIEW_PAN,           "view-pan"            },
        { TA_VIEW_DIRTY,         "view-dirty"          },
        { TA_CHANGE_LAYER,       "change-layer"        },
        { TA_CANCEL_TOOL,        "cancel-tool"         },
        { TA_CHOICE_MENU_UPDATE, "choice-menu-update"  },
        { TA_CHOICE_MENU_CHOICE, "choice-menu-choice"  },
        { TA_UNDO_REDO_PRE,      "undo-redo-pre"       },
        { TA_UNDO_REDO_POST,     "undo-redo-post"      },
        { TA_ACTION,             "action"              },
        { TA_ACTIVATE,           "activate"            },
        { 0,                     ""                    }
    };

    const FlagString buttons[] =
    {
        { BUT_NONE,   "none"   },
        { BUT_LEFT,   "left"   },
        { BUT_RIGHT,  "right"  },
        { BUT_MIDDLE, "middle" },
        { BUT_AUX1,   "aux1"   },
        { BUT_AUX2,   "aux2"   },
        { 0,          ""       }
    };

    const FlagString modifiers[] =
    {
        { MD_SHIFT, "shift" },
        { MD_CTRL,  "ctrl"  },
        { MD_ALT,   "alt"   },
        { MD_SUPER, "super" },
        { MD_META,  "meta"  },
        { MD_ALTGR, "altgr" },
        { 0,        ""      }
    };

    ev = "category: " + flag2string( m_category, categories ) + " ";
    ev += "action: " + flag2string( m_actions, actions ) + " ";
    ev += "action-group: ";

    if( m_actionGroup.has_value() )
    {
        ev += m_actionGroup.value().GetName() +
              "(" + std::to_string( m_actionGroup.value().GetGroupID() ) + ")" + " ";
    }
    else
    {
        ev += "none ";
    }

    if( m_actions & TA_MOUSE )
        ev += "btns: " + flag2string( m_mouseButtons, buttons ) + " ";

    if( m_actions & TA_KEYBOARD )
        ev += "key: " + std::to_string( m_keyCode ) + " ";

    if( m_actions & ( TA_MOUSE | TA_KEYBOARD ) )
        ev += "mods: " + flag2string( m_modifiers, modifiers ) + " ";

    if( m_commandId )
        ev += "cmd-id: " + std::to_string( *m_commandId ) + " ";

    ev += "cmd-str: " + m_commandStr;

    return ev;
}


const std::string TOOL_EVENT_LIST::Format() const
{
    std::string s;

    for( const TOOL_EVENT& e : m_events )
        s += e.Format() + " ";

    return s;
}


const std::string TOOL_EVENT_LIST::Names() const
{
    std::string s;

    for( const TOOL_EVENT& e : m_events )
        s += e.m_commandStr + " ";

    return s;
}


bool TOOL_EVENT::IsClick( int aButtonMask ) const
{
    return ( m_actions & TA_MOUSE_CLICK ) && ( m_mouseButtons & aButtonMask ) == m_mouseButtons;
}


bool TOOL_EVENT::IsDblClick( int aButtonMask ) const
{
    return m_actions == TA_MOUSE_DBLCLICK && ( m_mouseButtons & aButtonMask ) == m_mouseButtons;
}


bool TOOL_EVENT::IsCancelInteractive() const
{
    return ( ( m_commandStr == ACTIONS::cancelInteractive.GetName() )
             || ( m_commandId && *m_commandId == ACTIONS::cancelInteractive.GetId() )
             || ( m_actions == TA_CANCEL_TOOL ) );
}


bool TOOL_EVENT::IsSelectionEvent() const
{
    return Matches( EVENTS::ClearedEvent )
        || Matches( EVENTS::UnselectedEvent )
        || Matches( EVENTS::SelectedEvent )
        || Matches( EVENTS::PointSelectedEvent );
}


bool TOOL_EVENT::IsPointEditor() const
{
    return ( ( m_commandStr.find( "PointEditor" ) != getCommandStr().npos )
             || ( m_commandId && *m_commandId == ACTIONS::activatePointEditor.GetId() ) );
}


bool TOOL_EVENT::IsMoveTool() const
{
    return ( m_commandStr.find( "InteractiveMove" ) != getCommandStr().npos );
}


bool TOOL_EVENT::IsEditorTool() const
{
    return ( m_commandStr.find( "InteractiveEdit" ) != getCommandStr().npos );
}


bool TOOL_EVENT::IsSimulator() const
{
    return ( m_commandStr.find( "Simulation" ) != getCommandStr().npos );
}

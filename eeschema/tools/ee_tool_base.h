/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EE_TOOL_BASE_H
#define EE_TOOL_BASE_H

#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tool/actions.h>
#include <tools/ee_selection_tool.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <lib_edit_frame.h>
#include <undo_redo_container.h>


class EE_SELECTION;

/**
 * Class EE_TOOL_BASE
 *
 * A foundation class for a tool operating on a schematic or symbol
**/


template <class T>
class EE_TOOL_BASE : public TOOL_INTERACTIVE
{
public:
    /**
     * Constructor
     *
     * Creates a tool with given name. The name must be unique. 
     */
    EE_TOOL_BASE( const std::string& aName ) :
        TOOL_INTERACTIVE ( aName ),
        m_frame( nullptr ),
        m_view( nullptr ),
        m_selectionTool( nullptr ),
        m_isLibEdit( false )
    {};

    ~EE_TOOL_BASE() override {};

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override
    {
        m_frame = getEditFrame<T>();
        m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
        m_isLibEdit = m_frame->IsType( FRAME_SCH_LIB_EDITOR );

        // A basic context manu.  Many (but not all) tools will choose to override this.

        auto& ctxMenu = m_menu.GetMenu();

        // cancel current tool goes in main context menu at the top if present
        ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
        ctxMenu.AddSeparator( 1 );

        // Finally, add the standard zoom/grid items
        m_frame->AddStandardSubMenus( m_menu );

        return true;
    }

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override
    {
        if( aReason == MODEL_RELOAD )
        {
            // Init variables used by every drawing tool
            m_frame = getEditFrame<T>();
            m_isLibEdit = dynamic_cast<LIB_EDIT_FRAME*>( m_frame ) != nullptr;
        }

        m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    }

protected:
    ///> Similar to getView()->Update(), but handles items that are redrawn by their parents.
    void updateView( EDA_ITEM* aItem ) const
    {
        KICAD_T itemType = aItem->Type();

        if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
            getView()->Update( aItem->GetParent() );

        getView()->Update( aItem );
    }


    ///> Similar to m_frame->SaveCopyInUndoList(), but handles items that are owned by their
    ///> parents.
    void saveCopyInUndoList( EDA_ITEM* aItem, UNDO_REDO_T aType, bool aAppend = false )
    {
        KICAD_T itemType = aItem->Type();

        if( m_isLibEdit )
        {
            LIB_EDIT_FRAME* editFrame = dynamic_cast<LIB_EDIT_FRAME*>( m_frame );
            editFrame->SaveCopyInUndoList( (LIB_ITEM*) aItem, aType, aAppend );
        }
        else
        {
            SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

            if( itemType == SCH_PIN_T || itemType == SCH_FIELD_T || itemType == SCH_SHEET_PIN_T )
                editFrame->SaveCopyInUndoList( (SCH_ITEM*) aItem->GetParent(), UR_CHANGED, aAppend );
            else
                editFrame->SaveCopyInUndoList( (SCH_ITEM*) aItem, aType, aAppend );
        }
    }

protected:
    T*                 m_frame;
    KIGFX::SCH_VIEW*   m_view;
    EE_SELECTION_TOOL* m_selectionTool;
    bool               m_isLibEdit;
};

#endif

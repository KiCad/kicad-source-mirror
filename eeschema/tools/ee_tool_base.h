/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <math/vector2d.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tool/actions.h>
#include <tools/ee_selection_tool.h>
#include <sch_edit_frame.h>
#include <sch_view.h>
#include <symbol_edit_frame.h>

class EE_SELECTION;

/**
 * A foundation class for a tool operating on a schematic or symbol.
 */


template <class T>
class EE_TOOL_BASE : public TOOL_INTERACTIVE
{
public:
    /**
     * Create a tool with given name. The name must be unique.
     */
    EE_TOOL_BASE( const std::string& aName ) :
            TOOL_INTERACTIVE ( aName ),
            m_frame( nullptr ),
            m_view( nullptr ),
            m_selectionTool( nullptr ),
            m_isSymbolEditor( false )
    {};

    ~EE_TOOL_BASE() override {};

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override
    {
        m_frame = getEditFrame<T>();
        m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
        m_isSymbolEditor = m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

        // A basic context menu.  Many (but not all) tools will choose to override this.
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
        if( aReason == MODEL_RELOAD || aReason == SUPERMODEL_RELOAD )
        {
            // Init variables used by every drawing tool
            m_frame = getEditFrame<T>();
            m_isSymbolEditor = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) != nullptr;
        }

        m_view = static_cast<KIGFX::SCH_VIEW*>( getView() );
    }

    /**
     * Returns true if the tool is running in the symbol editor
     */
    bool IsSymbolEditor() const
    {
        return m_isSymbolEditor;
    }

protected:
    /**
     * Similar to getView()->Update(), but handles items that are redrawn by their parents
     * and updating the SCH_SCREEN's RTree.
     */
    void updateItem( EDA_ITEM* aItem, bool aUpdateRTree ) const
    {
        switch( aItem->Type() )
        {
        case SCH_SHEET_PIN_T:
            getView()->Update( aItem );
            getView()->Update( aItem->GetParent() );

            // Moving sheet pins does not change the BBox.
            break;

        case SCH_PIN_T:
        case SCH_FIELD_T:
            getView()->Update( aItem );
            getView()->Update( aItem->GetParent() );

            if( aUpdateRTree )
                m_frame->GetScreen()->Update( static_cast<SCH_ITEM*>( aItem->GetParent() ) );

            break;

        default:
            getView()->Update( aItem );

            if( aUpdateRTree && dynamic_cast<SCH_ITEM*>( aItem ) )
                m_frame->GetScreen()->Update( static_cast<SCH_ITEM*>( aItem ) );
        }
    }

    ///< Similar to m_frame->SaveCopyInUndoList(), but handles items that are owned by their
    ///< parents.
    void saveCopyInUndoList( EDA_ITEM* aItem, UNDO_REDO aType, bool aAppend = false,
                             bool aDirtyConnectivity = true )
    {
        wxASSERT( aItem );

        KICAD_T itemType = aItem->Type();
        bool    selected = aItem->IsSelected();

        // IS_SELECTED flag should not be set on undo items which were added for
        // a drag operation.
        if( selected && aItem->HasFlag( SELECTED_BY_DRAG ) )
            aItem->ClearSelected();

        if( m_isSymbolEditor )
        {
            SYMBOL_EDIT_FRAME* editFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );
            wxCHECK_RET( editFrame, wxT( "editFrame is null" ) );

            editFrame->SaveCopyInUndoList( wxEmptyString, static_cast<LIB_ITEM*>( aItem ) );
        }
        else
        {
            SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
            wxASSERT( editFrame );

            if( editFrame )
            {
                if( itemType == SCH_FIELD_T )
                {
                    editFrame->SaveCopyInUndoList( editFrame->GetScreen(),
                                                   static_cast<SCH_ITEM*>( aItem->GetParent() ),
                                                   UNDO_REDO::CHANGED, aAppend,
                                                   false );
                }
                else if( itemType == SCH_PIN_T || itemType == SCH_SHEET_PIN_T )
                {
                    editFrame->SaveCopyInUndoList( editFrame->GetScreen(),
                                                   static_cast<SCH_ITEM*>( aItem->GetParent() ),
                                                   UNDO_REDO::CHANGED, aAppend,
                                                   aDirtyConnectivity );
                }
                else
                {
                    editFrame->SaveCopyInUndoList( editFrame->GetScreen(),
                                                   static_cast<SCH_ITEM*>( aItem ), aType,
                                                   aAppend, aDirtyConnectivity );
                }
            }
        }

        if( selected && aItem->HasFlag( SELECTED_BY_DRAG ) )
            aItem->SetSelected();
    }

protected:
    T*                 m_frame;
    KIGFX::SCH_VIEW*   m_view;
    EE_SELECTION_TOOL* m_selectionTool;
    bool               m_isSymbolEditor;
};


// For LibEdit
inline VECTOR2I mapCoords( const wxPoint& aCoord )
{
    return VECTOR2I( aCoord.x, -aCoord.y );
}

inline VECTOR2I mapCoords( const VECTOR2I& aCoord )
{
    return VECTOR2I( aCoord.x, -aCoord.y );
}

inline VECTOR2I mapCoords( const int x, const int y )
{
    return VECTOR2I( x, -y );
}

#endif

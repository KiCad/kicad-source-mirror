/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <ws_data_model.h>
#include <ws_draw_item.h>

#include <pl_editor_frame.h>
#include <tool/tool_manager.h>
#include <tools/pl_selection_tool.h>

/* Note: the Undo/redo commands use a "brute" method:
 * the full page layout is converted to a S expression, and saved as string.
 * When a previous version is needed, the old string is parsed,
 * and the description replaces the current desc, just like reading a new file
 *
 * This is not optimal from the memory point of view, but:
 * - the descriptions are never very long (max few thousand of bytes)
 * - this is very easy to code
 */

// A helper class used in undo/redo commad:
class PL_ITEM_LAYOUT: public EDA_ITEM
{
    wxString m_serialization;
    int      m_selectedDataItem;
    int      m_selectedDrawItem;

public:
    PL_ITEM_LAYOUT() :
            EDA_ITEM( TYPE_PL_EDITOR_LAYOUT ),
            m_selectedDataItem( INT_MAX ),
            m_selectedDrawItem( INT_MAX )
    {
        WS_DATA_MODEL& pglayout = WS_DATA_MODEL::GetTheInstance();
        pglayout.SaveInString( m_serialization );

        for( size_t ii = 0; ii < pglayout.GetItems().size(); ++ii )
        {
            WS_DATA_ITEM* dataItem = pglayout.GetItem( ii );

            for( size_t jj = 0; jj < dataItem->GetDrawItems().size(); ++jj )
            {
                WS_DRAW_ITEM_BASE* drawItem = dataItem->GetDrawItems()[ jj ];

                if( drawItem->IsSelected() )
                {
                    m_selectedDataItem = ii;
                    m_selectedDrawItem = jj;
                    break;
                }
            }
        }
    }

    void RestoreLayout( PL_EDITOR_FRAME* aFrame )
    {
        WS_DATA_MODEL&     pglayout = WS_DATA_MODEL::GetTheInstance();
        PL_SELECTION_TOOL* selTool = aFrame->GetToolManager()->GetTool<PL_SELECTION_TOOL>();
        KIGFX::VIEW*       view = aFrame->GetGalCanvas()->GetView();

        pglayout.SetPageLayout( TO_UTF8( m_serialization ) );

        selTool->ClearSelection();
        view->Clear();

        for( size_t ii = 0; ii < pglayout.GetItems().size(); ++ii )
        {
            WS_DATA_ITEM* dataItem = pglayout.GetItem( ii );

            dataItem->SyncDrawItems( nullptr, view );

            if( ii == m_selectedDataItem && m_selectedDrawItem < dataItem->GetDrawItems().size() )
            {
                WS_DRAW_ITEM_BASE* drawItem = dataItem->GetDrawItems()[ m_selectedDrawItem ];
                drawItem->SetSelected();
            }
        }

        selTool->RebuildSelection();
    }

    // Required to keep compiler happy on debug builds.
#if defined(DEBUG)
    virtual void Show( int nestLevel, std::ostream& os ) const override {}
#endif

    /** Get class name
     * @return  string "PL_ITEM_LAYOUT"
     */
    virtual wxString GetClass() const override
    {
        return wxT( "PL_ITEM_LAYOUT" );
    }
};

void PL_EDITOR_FRAME::SaveCopyInUndoList()
{
    PL_ITEM_LAYOUT* copyItem = new PL_ITEM_LAYOUT;  // constructor stores current layout

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    ITEM_PICKER wrapper( copyItem, UR_LIBEDIT );
    lastcmd->PushItem(wrapper);
    GetScreen()->PushCommandToUndoList( lastcmd );

    // Clear redo list, because after new save there is no redo to do.
    GetScreen()->ClearUndoORRedoList( GetScreen()->m_RedoList );
}


/* Redo the last edit:
 * - Place the current edited layout in undo list
 * - Get previous version of the current edited layput
 */
void PL_EDITOR_FRAME::GetLayoutFromRedoList()
{
    if ( GetScreen()->GetRedoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    PL_ITEM_LAYOUT* copyItem = new PL_ITEM_LAYOUT;  // constructor stores current layout

    ITEM_PICKER wrapper( copyItem, UR_LIBEDIT );

    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToUndoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromRedoList();

    wrapper = lastcmd->PopItem();
    copyItem = static_cast<PL_ITEM_LAYOUT*>( wrapper.GetItem() );
    copyItem->RestoreLayout( this );
    delete copyItem;

    GetCanvas()->Refresh();
    OnModify();
}


/* Undo the last edit:
 * - Place the current layout in Redo list
 * - Get previous version of the current edited layout
 */
void PL_EDITOR_FRAME::GetLayoutFromUndoList()
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = new PICKED_ITEMS_LIST();
    PL_ITEM_LAYOUT* copyItem = new PL_ITEM_LAYOUT;  // constructor stores current layout

    ITEM_PICKER wrapper( copyItem, UR_LIBEDIT );
    lastcmd->PushItem( wrapper );
    GetScreen()->PushCommandToRedoList( lastcmd );

    lastcmd = GetScreen()->PopCommandFromUndoList();

    wrapper = lastcmd->PopItem();
    copyItem = static_cast<PL_ITEM_LAYOUT*>( wrapper.GetItem() );
    copyItem->RestoreLayout( this );
    delete copyItem;

    GetCanvas()->Refresh();
    OnModify();
}

/* Remove the last command in Undo List.
 * Used to clean the uUndo stack after a cancel command
 */
void PL_EDITOR_FRAME::RollbackFromUndo()
{
    if ( GetScreen()->GetUndoCommandCount() <= 0 )
        return;

    PICKED_ITEMS_LIST* lastcmd = GetScreen()->PopCommandFromUndoList();

    ITEM_PICKER wrapper = lastcmd->PopItem();
    PL_ITEM_LAYOUT* copyItem = static_cast<PL_ITEM_LAYOUT*>( wrapper.GetItem() );
    copyItem->RestoreLayout( this );
    delete copyItem;

    GetCanvas()->Refresh();
}

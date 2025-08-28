/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#pragma once

#include "increment.h"
#include <math/vector2d.h>
#include <tool/tool_event.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tool/actions.h>
#include <tools/sch_selection_tool.h>
#include <sch_edit_frame.h>
#include <sch_view.h>
#include <symbol_edit_frame.h>
#include <sch_shape.h>
#include <pin_layout_cache.h>
#include <sch_commit.h>
#include <tool/picker_tool.h>
#include <view/view_controls.h>

class SCH_SELECTION;

/**
 * A foundation class for a tool operating on a schematic or symbol.
 */


template <class T>
class SCH_TOOL_BASE : public TOOL_INTERACTIVE
{
public:
    /**
     * Create a tool with given name. The name must be unique.
     */
    SCH_TOOL_BASE( const std::string& aName ) :
            TOOL_INTERACTIVE ( aName ),
            m_frame( nullptr ),
            m_view( nullptr ),
            m_selectionTool( nullptr ),
            m_isSymbolEditor( false ),
            m_pickerItem( nullptr )
    {};

    ~SCH_TOOL_BASE() override {};

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override
    {
        m_frame = getEditFrame<T>();
        m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
        m_isSymbolEditor = m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

        // A basic context menu.  Many (but not all) tools will choose to override this.
        auto& ctxMenu = m_menu->GetMenu();

        // cancel current tool goes in main context menu at the top if present
        ctxMenu.AddItem( ACTIONS::cancelInteractive, SELECTION_CONDITIONS::ShowAlways, 1 );
        ctxMenu.AddSeparator( 1 );

        // Finally, add the standard zoom/grid items
        m_frame->AddStandardSubMenus( *m_menu.get() );

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

    int Increment( const TOOL_EVENT& aEvent )
    {
        static const std::vector<KICAD_T> incrementable = { SCH_LABEL_T,
                                                            SCH_GLOBAL_LABEL_T,
                                                            SCH_HIER_LABEL_T,
                                                            SCH_PIN_T,
                                                            SCH_TEXT_T };

        const ACTIONS::INCREMENT param = { 1, 0 };

        if( aEvent.HasParameter() )
            aEvent.Parameter<ACTIONS::INCREMENT>();

        SCH_SELECTION& selection = m_selectionTool->RequestSelection( incrementable );

        if( selection.Empty() )
            return 0;

        KICAD_T type = selection.Front()->Type();
        bool    allSameType = true;

        for( EDA_ITEM* item : selection )
        {
            if( item->Type() != type )
            {
                allSameType = false;
                break;
            }
        }

        // Incrementing multiple types at once seems confusing though it would work.
        if( !allSameType )
            return 0;

        const VECTOR2I mousePosition = getViewControls()->GetMousePosition();

        STRING_INCREMENTER incrementer;
        // In schematics, it's probably less common to be operating
        // on pin numbers which are usually IOSQXZ-skippy.
        incrementer.SetSkipIOSQXZ( m_isSymbolEditor );

        // If we're coming via another action like 'Move', use that commit
        SCH_COMMIT  localCommit( m_toolMgr );
        SCH_COMMIT* commit = dynamic_cast<SCH_COMMIT*>( aEvent.Commit() );

        if( !commit )
            commit = &localCommit;

        const auto modifyItem =
                [&]( EDA_ITEM& aItem )
                {
                    if( aItem.IsNew() )
                        m_toolMgr->PostAction( ACTIONS::refreshPreview );

                    commit->Modify( &aItem, m_frame->GetScreen() );
                };

        for( EDA_ITEM* item : selection )
        {
            switch( item->Type() )
            {
            case SCH_PIN_T:
            {
                SCH_PIN&          pin = static_cast<SCH_PIN&>( *item );
                PIN_LAYOUT_CACHE& layout = pin.GetLayoutCache();

                bool      found = false;
                OPT_BOX2I bbox = layout.GetPinNumberBBox();

                if( bbox && bbox->Contains( mousePosition ) )
                {
                    std::optional<wxString> nextNumber = incrementer.Increment( pin.GetNumber(), param.Delta,
                                                                                param.Index );

                    if( nextNumber )
                    {
                        modifyItem( pin );
                        pin.SetNumber( *nextNumber );
                    }

                    found = true;
                }

                if( !found )
                {
                    bbox = layout.GetPinNameBBox();

                    if( bbox && bbox->Contains( mousePosition ) )
                    {
                        std::optional<wxString> nextName = incrementer.Increment( pin.GetName(), param.Delta,
                                                                                  param.Index );

                        if( nextName )
                        {
                            modifyItem( pin );
                            pin.SetName( *nextName );
                        }

                        found = true;
                    }
                }
                break;
            }

            case SCH_LABEL_T:
            case SCH_GLOBAL_LABEL_T:
            case SCH_HIER_LABEL_T:
            case SCH_TEXT_T:
            {
                SCH_TEXT& label = static_cast<SCH_TEXT&>( *item );

                std::optional<wxString> newLabel = incrementer.Increment( label.GetText(), param.Delta,
                                                                          param.Index );

                if( newLabel )
                {
                    modifyItem( label );
                    label.SetText( *newLabel );
                }

                break;
            }

            default:
                // No increment for other items
                break;
            }
        }

        commit->Push( _( "Increment" ) );

        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        return 0;
    }

    int InteractiveDelete( const TOOL_EVENT& aEvent )
    {
        PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();

        m_toolMgr->RunAction( ACTIONS::selectionClear );
        m_pickerItem = nullptr;

        // Deactivate other tools; particularly important if another PICKER is currently running
        Activate();

        picker->SetCursor( KICURSOR::REMOVE );
        picker->SetSnapping( false );
        picker->ClearHandlers();

        picker->SetClickHandler(
                [this]( const VECTOR2D& aPosition ) -> bool
                {
                    if( m_pickerItem )
                    {
                        SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                        selectionTool->UnbrightenItem( m_pickerItem );
                        selectionTool->AddItemToSel( m_pickerItem, true /*quiet mode*/ );
                        m_toolMgr->RunAction( ACTIONS::doDelete );
                        m_pickerItem = nullptr;
                    }

                    return true;
                } );

        picker->SetMotionHandler(
                [this]( const VECTOR2D& aPos )
                {
                    SCH_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
                    SCH_COLLECTOR       collector;

                    selectionTool->CollectHits( collector, aPos, SCH_COLLECTOR::DeletableItems );

                    // Remove unselectable items
                    for( int i = collector.GetCount() - 1; i >= 0; --i )
                    {
                        if( !selectionTool->Selectable( collector[ i ] ) )
                            collector.Remove( i );
                    }

                    if( collector.GetCount() > 1 )
                        selectionTool->GuessSelectionCandidates( collector, aPos );

                    EDA_ITEM* item = collector.GetCount() == 1 ? collector[ 0 ] : nullptr;

                    if( m_pickerItem != item )
                    {
                        if( m_pickerItem )
                            selectionTool->UnbrightenItem( m_pickerItem );

                        m_pickerItem = item;

                        if( m_pickerItem )
                            selectionTool->BrightenItem( m_pickerItem );
                    }
                } );

        picker->SetFinalizeHandler(
                [this]( const int& aFinalState )
                {
                    if( m_pickerItem )
                        m_toolMgr->GetTool<SCH_SELECTION_TOOL>()->UnbrightenItem( m_pickerItem );

                    // Wake the selection tool after exiting to ensure the cursor gets updated
                    m_toolMgr->PostAction( ACTIONS::selectionActivate );
                } );

        m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

        return 0;
    }

protected:
    /**
     * Similar to getView()->Update(), but also updates the SCH_SCREEN's RTree.
     */
    void updateItem( EDA_ITEM* aItem, bool aUpdateRTree ) const
    {
        m_frame->UpdateItem( aItem, false, aUpdateRTree );
    }

    ///< Similar to m_frame->SaveCopyInUndoList(), but also handles connectivity.
    void saveCopyInUndoList( EDA_ITEM* aItem, UNDO_REDO aType, bool aAppend = false, bool aDirtyConnectivity = true )
    {
        if( !aItem->IsSCH_ITEM() )
            return;

        SCH_ITEM* item = static_cast<SCH_ITEM*>( aItem );
        bool      selected = item->IsSelected();

        // IS_SELECTED flag should not be set on undo items which were added for
        // a drag operation.
        if( selected && item->HasFlag( SELECTED_BY_DRAG ) )
            item->ClearSelected();

        if( SYMBOL_EDIT_FRAME* symbolEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame ) )
        {
            symbolEditFrame->SaveCopyInUndoList( wxEmptyString, dynamic_cast<LIB_SYMBOL*>( item ) );
        }
        else if( SCH_EDIT_FRAME* schematicFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
        {
            schematicFrame->SaveCopyInUndoList( schematicFrame->GetScreen(), item, UNDO_REDO::CHANGED, aAppend );

            if( aDirtyConnectivity )
            {
                if( !item->IsConnectivityDirty()
                    && item->Connection()
                    && ( item->Connection()->Name() == schematicFrame->GetHighlightedConnection()
                             || item->Connection()->HasDriverChanged() ) )
                {
                    schematicFrame->DirtyHighlightedConnection();
                }

                item->SetConnectivityDirty();
            }
        }

        if( selected && aItem->HasFlag( SELECTED_BY_DRAG ) )
            aItem->SetSelected();
    }

protected:
    T*                  m_frame;
    KIGFX::SCH_VIEW*    m_view;
    SCH_SELECTION_TOOL* m_selectionTool;
    bool                m_isSymbolEditor;
    EDA_ITEM*           m_pickerItem;
};

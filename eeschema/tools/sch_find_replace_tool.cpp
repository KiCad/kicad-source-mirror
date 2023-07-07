/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_sheet_pin.h>
#include <schematic.h>
#include <tools/ee_actions.h>
#include <tools/sch_find_replace_tool.h>
#include <sch_sheet_path.h>


int SCH_FIND_REPLACE_TOOL::FindAndReplace( const TOOL_EVENT& aEvent )
{
    m_frame->ShowFindReplaceDialog( aEvent.IsAction( &ACTIONS::findAndReplace ) );
    return UpdateFind( aEvent );
}


int SCH_FIND_REPLACE_TOOL::UpdateFind( const TOOL_EVENT& aEvent )
{
    EDA_SEARCH_DATA& data = m_frame->GetFindReplaceData();
    SCH_SEARCH_DATA* schSearchData = dynamic_cast<SCH_SEARCH_DATA*>( &data );
    bool             selectedOnly = schSearchData ? schSearchData->searchSelectedOnly : false;

    auto visit =
            [&]( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheet )
            {
                // We may get triggered when the dialog is not opened due to binding
                // SelectedItemsModified we also get triggered when the find dialog is
                // closed....so we need to double check the dialog is open.
                if( m_frame->m_findReplaceDialog != nullptr
                    && !data.findString.IsEmpty()
                    && aItem->Matches( data, aSheet )
                    && ( !selectedOnly || aItem->IsSelected() ) )
                {
                    aItem->SetForceVisible( true );
                    m_selectionTool->BrightenItem( aItem );
                    m_foundItemHighlighted = true;
                }
                else if( aItem->IsBrightened() )
                {
                    aItem->SetForceVisible( false );
                    m_selectionTool->UnbrightenItem( aItem );
                }
            };

    auto visitAll =
            [&]()
            {
                for( SCH_ITEM* item : m_frame->GetScreen()->Items() )
                {
                    visit( item, &m_frame->GetCurrentSheet() );

                    item->RunOnChildren(
                            [&]( SCH_ITEM* aChild )
                            {
                                visit( aChild, &m_frame->GetCurrentSheet() );
                            } );
                }
            };

    if( aEvent.IsAction( &ACTIONS::find ) || aEvent.IsAction( &ACTIONS::findAndReplace )
        || aEvent.IsAction( &ACTIONS::updateFind ) )
    {
        m_foundItemHighlighted = false;
        visitAll();
    }
    else if( aEvent.Matches( EVENTS::SelectedItemsModified ) )
    {
        for( EDA_ITEM* item : m_selectionTool->GetSelection() )
            visit( item, &m_frame->GetCurrentSheet() );
    }
    else if( aEvent.Matches( EVENTS::PointSelectedEvent )
             || aEvent.Matches( EVENTS::SelectedEvent )
             || aEvent.Matches( EVENTS::UnselectedEvent )
             || aEvent.Matches( EVENTS::ClearedEvent ) )
    {
        // Normal find modifies the selection, but selection-based find does
        // not so we want to start over in the items we are searching through when
        // the selection changes
        if( selectedOnly )
        {
            m_afterItem = nullptr;
            visitAll();
        }
    }
    else if( m_foundItemHighlighted )
    {
        m_foundItemHighlighted = false;
        visitAll();
    }

    getView()->UpdateItems();
    m_frame->GetCanvas()->Refresh();
    m_frame->updateTitle();

    return 0;
}


SCH_ITEM* SCH_FIND_REPLACE_TOOL::nextMatch( SCH_SCREEN* aScreen, SCH_SHEET_PATH* aSheet,
                                            SCH_ITEM* aAfter, EDA_SEARCH_DATA& aData,
                                            bool reversed )
{
    SCH_SEARCH_DATA*       schSearchData = dynamic_cast<SCH_SEARCH_DATA*>( &aData );
    bool                   selectedOnly = schSearchData ? schSearchData->searchSelectedOnly : false;
    bool                   past_item = !aAfter;
    std::vector<SCH_ITEM*> sorted_items;

    auto addItem =
            [&](SCH_ITEM* item)
            {
                sorted_items.push_back( item );

                if( item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* cmp = static_cast<SCH_SYMBOL*>( item );

                    for( SCH_FIELD& field : cmp->GetFields() )
                        sorted_items.push_back( &field );

                    for( SCH_PIN* pin : cmp->GetPins() )
                        sorted_items.push_back( pin );
                }
                else if( item->Type() == SCH_SHEET_T )
                {
                    SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

                    for( SCH_FIELD& field : sheet->GetFields() )
                        sorted_items.push_back( &field );

                    for( SCH_SHEET_PIN* pin : sheet->GetPins() )
                        sorted_items.push_back( pin );
                }
                else if( item->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
                {
                    SCH_LABEL_BASE* label = static_cast<SCH_LABEL_BASE*>( item );

                    for( SCH_FIELD& field : label->GetFields() )
                        sorted_items.push_back( &field );
                }
            };

    if( selectedOnly )
        for( EDA_ITEM* item : m_selectionTool->GetSelection() )
            addItem( static_cast<SCH_ITEM*>( item ) );
    else
        for( SCH_ITEM* item : aScreen->Items() )
            addItem( item );

    std::sort( sorted_items.begin(), sorted_items.end(),
            [&]( SCH_ITEM* a, SCH_ITEM* b )
            {
                if( a->GetPosition().x == b->GetPosition().x )
                {
                    // Ensure deterministic sort
                    if( a->GetPosition().y == b->GetPosition().y )
                        return a->m_Uuid < b->m_Uuid;

                    return a->GetPosition().y < b->GetPosition().y;
                }
                else
                    return a->GetPosition().x < b->GetPosition().x;
            } );

    if( reversed )
        std::reverse( sorted_items.begin(), sorted_items.end() );

    for( SCH_ITEM* item : sorted_items )
    {
        if( item == aAfter )
        {
            past_item = true;
        }
        else if( past_item )
        {
            if( aData.markersOnly && item->Type() == SCH_MARKER_T )
                return item;

            if( item->Matches( aData, aSheet ) )
                return item;
        }
    }

    return nullptr;
}


int SCH_FIND_REPLACE_TOOL::FindNext( const TOOL_EVENT& aEvent )
{
    EDA_SEARCH_DATA& data            = m_frame->GetFindReplaceData();
    bool             searchAllSheets = false;
    bool             selectedOnly    = false;
    bool             isReversed      = aEvent.IsAction( &ACTIONS::findPrevious );
    SCH_ITEM*        item            = nullptr;
    SCH_SHEET_PATH*  afterSheet      = &m_frame->GetCurrentSheet();

    try
    {
        const SCH_SEARCH_DATA& schSearchData = dynamic_cast<const SCH_SEARCH_DATA&>( data );
        searchAllSheets = !( schSearchData.searchCurrentSheetOnly );
        selectedOnly = schSearchData.searchSelectedOnly;
    }
    catch( const std::bad_cast& )
    {
    }

    if( aEvent.IsAction( &ACTIONS::findNextMarker ) )
        data.markersOnly = true;
    else if( data.findString.IsEmpty() )
        return FindAndReplace( ACTIONS::find.MakeEvent() );

    if( m_wrapAroundTimer.IsRunning() )
    {
        afterSheet = nullptr;
        m_afterItem = nullptr;
        m_wrapAroundTimer.Stop();
        m_frame->ClearFindReplaceStatus();
    }

    if( afterSheet || !searchAllSheets )
        item = nextMatch( m_frame->GetScreen(), &m_frame->GetCurrentSheet(), m_afterItem, data,
                          isReversed );

    if( !item && searchAllSheets )
    {
        SCH_SCREENS                  screens( m_frame->Schematic().Root() );
        std::vector<SCH_SHEET_PATH*> paths;

        screens.BuildClientSheetPathList();

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        {
            for( SCH_SHEET_PATH& sheet : screen->GetClientSheetPaths() )
                paths.push_back( &sheet );
        }

        std::sort( paths.begin(), paths.end(), [] ( const SCH_SHEET_PATH* lhs,
                                                    const SCH_SHEET_PATH* rhs ) -> bool
                {
                    int retval = lhs->ComparePageNum( *rhs );

                    if( retval < 0 )
                        return true;
                    else if( retval > 0 )
                        return false;
                    else /// Enforce strict ordering.  If the page numbers are the same, use UUIDs
                        return lhs->GetCurrentHash() < rhs->GetCurrentHash();
                } );

        if( isReversed )
            std::reverse( paths.begin(), paths.end() );

        for( SCH_SHEET_PATH* sheet : paths )
        {
            if( afterSheet )
            {
                if( afterSheet->GetCurrentHash() == sheet->GetCurrentHash() )
                    afterSheet = nullptr;

                continue;
            }

            item = nextMatch( sheet->LastScreen(), sheet, nullptr, data, isReversed );

            if( item )
            {
                if( m_frame->Schematic().CurrentSheet() != *sheet )
                {
                    m_frame->Schematic().SetCurrentSheet( *sheet );
                    m_frame->DisplayCurrentSheet();
                }

                break;
            }
        }
    }

    if( item )
    {
        m_afterItem = item;

        if( !item->IsBrightened() )
        {
            // Clear any previous brightening
            UpdateFind( aEvent );

            // Brighten (and show) found object
            item->SetForceVisible( true );
            m_selectionTool->BrightenItem( item );
            m_foundItemHighlighted = true;
        }

        if( !selectedOnly )
        {
            m_selectionTool->ClearSelection();
            m_selectionTool->AddItemToSel( item );
        }

        m_frame->FocusOnLocation( item->GetBoundingBox().GetCenter() );
        m_frame->GetCanvas()->Refresh();
    }
    else
    {
        wxString msg = searchAllSheets ? _( "Reached end of schematic." )
                                       : _( "Reached end of sheet." );

       // Show the popup during the time period the user can wrap the search
        m_frame->ShowFindReplaceStatus( msg + wxS( " " ) +
                                        _( "Find again to wrap around to the start." ), 4000 );
        m_wrapAroundTimer.StartOnce( 4000 );
    }

    return 0;
}

EDA_ITEM* SCH_FIND_REPLACE_TOOL::getCurrentMatch()
{
    EDA_SEARCH_DATA& data = m_frame->GetFindReplaceData();
    SCH_SEARCH_DATA* schSearchData = dynamic_cast<SCH_SEARCH_DATA*>( &data );
    bool             selectedOnly = schSearchData ? schSearchData->searchSelectedOnly : false;

    return selectedOnly ? m_afterItem : m_selectionTool->GetSelection().Front();
}

bool SCH_FIND_REPLACE_TOOL::HasMatch()
{
    EDA_SEARCH_DATA& data = m_frame->GetFindReplaceData();
    EDA_ITEM*        match = getCurrentMatch();

    return match && match->Matches( data, &m_frame->GetCurrentSheet() );
}


int SCH_FIND_REPLACE_TOOL::ReplaceAndFindNext( const TOOL_EVENT& aEvent )
{
    EDA_SEARCH_DATA& data = m_frame->GetFindReplaceData();
    EDA_ITEM*        item = getCurrentMatch();
    SCH_SHEET_PATH*  sheet = &m_frame->GetCurrentSheet();

    if( data.findString.IsEmpty() )
        return FindAndReplace( ACTIONS::find.MakeEvent() );

    // TODO: move to SCH_COMMIT....

    if( item && HasMatch() )
    {
        SCH_ITEM* sch_item = static_cast<SCH_ITEM*>( item );

        m_frame->SaveCopyInUndoList( sheet->LastScreen(), sch_item, UNDO_REDO::CHANGED, false );

        if( item->Replace( data, sheet ) )
        {
            m_frame->UpdateItem( item, false, true );
            m_frame->GetCurrentSheet().UpdateAllScreenReferences();
            m_frame->OnModify();
        }

        FindNext( ACTIONS::findNext.MakeEvent() );
    }

    return 0;
}


int SCH_FIND_REPLACE_TOOL::ReplaceAll( const TOOL_EVENT& aEvent )
{
    EDA_SEARCH_DATA& data = m_frame->GetFindReplaceData();
    bool             currentSheetOnly = false;
    bool             selectedOnly = false;

    try
    {
        const SCH_SEARCH_DATA& schSearchData = dynamic_cast<const SCH_SEARCH_DATA&>( data );
        currentSheetOnly = schSearchData.searchCurrentSheetOnly;
        selectedOnly     = schSearchData.searchSelectedOnly;
    }
    catch( const std::bad_cast& )
    {
    }

    bool modified = false;      // TODO: move to SCH_COMMIT....

    if( data.findString.IsEmpty() )
        return FindAndReplace( ACTIONS::find.MakeEvent() );

    auto doReplace =
            [&]( SCH_ITEM* aItem, SCH_SHEET_PATH* aSheet, EDA_SEARCH_DATA& aData )
            {
                m_frame->SaveCopyInUndoList( aSheet->LastScreen(), aItem, UNDO_REDO::CHANGED,
                                             modified );

                if( aItem->Replace( aData, aSheet ) )
                {
                    m_frame->UpdateItem( aItem, false, true );
                    modified = true;
                }
            };

    if( currentSheetOnly || selectedOnly )
    {
        SCH_SHEET_PATH* currentSheet = &m_frame->GetCurrentSheet();

        SCH_ITEM* item = nextMatch( m_frame->GetScreen(), currentSheet, nullptr, data, false );

        while( item )
        {
            if( !selectedOnly || item->IsSelected() )
                doReplace( item, currentSheet, data );

            item = nextMatch( m_frame->GetScreen(), currentSheet, item, data, false );
        }
    }
    else
    {
        SCH_SHEET_LIST allSheets = m_frame->Schematic().GetSheets();
        SCH_SCREENS    screens( m_frame->Schematic().Root() );

        for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
        {
            SCH_SHEET_LIST sheets = allSheets.FindAllSheetsForScreen( screen );

            for( unsigned ii = 0; ii < sheets.size(); ++ii )
            {
                SCH_ITEM* item = nextMatch( screen, &sheets[ii], nullptr, data, false );

                while( item )
                {
                    if( ii == 0 )
                    {
                        doReplace( item, &sheets[0], data );
                    }
                    else if( item->Type() == SCH_FIELD_T )
                    {
                        SCH_FIELD* field = static_cast<SCH_FIELD*>( item );

                        if( field->GetParent() && field->GetParent()->Type() == SCH_SYMBOL_T )
                        {
                            switch( field->GetId() )
                            {
                            case REFERENCE_FIELD:
                            case VALUE_FIELD:
                            case FOOTPRINT_FIELD:
                                // must be handled for each distinct sheet
                                doReplace( field, &sheets[ii], data );
                                break;

                            default:
                                // handled in first iteration
                                break;
                            }
                        }
                    }

                    item = nextMatch( screen, &sheets[ii], item, data, false );
                }
            }
        }
    }

    if( modified )
    {
        m_frame->GetCurrentSheet().UpdateAllScreenReferences();
        m_frame->OnModify();
    }

    return 0;
}


void SCH_FIND_REPLACE_TOOL::setTransitions()
{
    Go( &SCH_FIND_REPLACE_TOOL::FindAndReplace,        ACTIONS::find.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::FindAndReplace,        ACTIONS::findAndReplace.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::FindNext,              ACTIONS::findNext.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::FindNext,              ACTIONS::findPrevious.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::FindNext,              ACTIONS::findNextMarker.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::ReplaceAndFindNext,    ACTIONS::replaceAndFindNext.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::ReplaceAll,            ACTIONS::replaceAll.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::UpdateFind,            ACTIONS::updateFind.MakeEvent() );
    Go( &SCH_FIND_REPLACE_TOOL::UpdateFind,            EVENTS::SelectedItemsModified );
    Go( &SCH_FIND_REPLACE_TOOL::UpdateFind,            EVENTS::PointSelectedEvent );
    Go( &SCH_FIND_REPLACE_TOOL::UpdateFind,            EVENTS::SelectedEvent );
    Go( &SCH_FIND_REPLACE_TOOL::UpdateFind,            EVENTS::UnselectedEvent );
    Go( &SCH_FIND_REPLACE_TOOL::UpdateFind,            EVENTS::ClearedEvent );
}

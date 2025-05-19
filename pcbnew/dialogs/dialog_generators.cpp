/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "dialog_generators.h"

#include <pcb_edit_frame.h>
#include <board.h>
#include <pcb_generator.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/generator_tool.h>


void DIALOG_GENERATORS::clearModels()
{
    for( auto& [name, ptr] : m_dataModels )
    {
        if( ptr )
            ptr->DecRef();
    }

    m_dataModels.clear();
    m_columnNameTypes.clear();
    m_dataViews.clear();
}


void DIALOG_GENERATORS::clearModel( const wxString& aName )
{
    if( m_dataModels[aName] )
    {
        m_dataModels[aName]->DeleteAllItems();
        m_dataModels[aName]->ClearColumns();
    }

    m_columnNameTypes[aName].clear();
}


void DIALOG_GENERATORS::deleteModel( const wxString& aName )
{
    if( m_dataModels[aName] )
        m_dataModels[aName]->DecRef();

    for( size_t i = 0; i < m_Notebook->GetPageCount(); i++ )
    {
        wxWindow* page = m_Notebook->GetPage( i );
        if( page->GetName() == aName )
        {
            m_Notebook->DeletePage( i );
            break;
        }
    }

    m_dataViews.erase( aName );
    m_dataModels.erase( aName );
    m_columnNameTypes.erase( aName );
}


void DIALOG_GENERATORS::RebuildModels()
{
    wxString           lastPageName;
    std::set<wxString> lastUUIDs;

    if( auto page = m_Notebook->GetCurrentPage() )
    {
        lastPageName = page->GetName();
        wxDataViewCtrl* dataView = m_dataViews[lastPageName];

        int uuidCol = dataView->GetColumnCount() - 1;

        wxDataViewItemArray selections;
        dataView->GetSelections( selections );

        for( wxDataViewItem& item : selections )
        {
            wxVariant var;
            dataView->GetModel()->GetValue( var, item, uuidCol );
            lastUUIDs.emplace( var.GetString() );
        }
    }

    int newPageId = -1;

    std::map<wxString, std::map<KIID, std::vector<std::pair<wxString, wxVariant>>>> dataMap;

    for( PCB_GENERATOR* gen : m_currentBoard->Generators() )
    {
        std::vector<std::pair<wxString, wxVariant>> rowData = gen->GetRowData();

        const KIID uuid = gen->m_Uuid;
        rowData.emplace_back( wxS( "UUID" ), uuid.AsString() );

        dataMap[gen->GetName()][uuid] = rowData;
    }

    std::vector<wxString> toDelete;
    for( size_t i = 0; i < m_Notebook->GetPageCount(); i++ )
    {
        wxWindow* page = m_Notebook->GetPage( i );

        if( dataMap.find(page->GetName()) == dataMap.end() )
        {
            toDelete.emplace_back( page->GetName() );
        }
    }

    for( const wxString& name : toDelete )
        deleteModel( name );

    for( auto& [typeName, uuidToRowMap] : dataMap )
    {
        bool exists = false;
        for( size_t i = 0; i < m_Notebook->GetPageCount(); i++ )
        {
            if( m_Notebook->GetPage( i )->GetName() == typeName )
            {
                exists = true;
                break;
            }
        }

        if( exists )
        {
            clearModel( typeName );
        }
        else
        {
            wxString title = typeName + wxString::Format( " (%d)", int( uuidToRowMap.size() ) );
            addPage( typeName, title );
        }
    }

    for( auto& [typeName, uuidToRowMap] : dataMap )
    {
        std::vector<std::pair<wxString, wxString>>& thisColNameTypes = m_columnNameTypes[typeName];
        std::map<wxString, int>                     nameToColIdMap;
        std::set<wxString>                          columnsSet;

        for( auto& [uuid, rowMap] : uuidToRowMap )
        {
            for( auto& [colName, value] : rowMap )
            {
                if( columnsSet.find( colName ) == columnsSet.end() )
                {
                    int colId = columnsSet.size();
                    columnsSet.emplace( colName );

                    nameToColIdMap[colName] = colId;
                    thisColNameTypes.emplace_back( colName, "string" );
                }
            }
        }

        wxDataViewListStore* store = new wxDataViewListStore();

        for( auto& [name, type] : thisColNameTypes )
            store->AppendColumn( type );

        int colCount = thisColNameTypes.size();

        for( auto& [uuid, rowMap] : uuidToRowMap )
        {
            wxVector<wxVariant> values( colCount );

            for( auto& [dataName, value] : rowMap )
            {
                values[nameToColIdMap[dataName]] = value;
            }

            store->AppendItem( values );
        }

        m_dataModels[typeName] = store;
    }

    m_Notebook->DeleteAllPages();

    int pageId = 0;
    for( auto& [typeName, model] : m_dataModels )
    {
        wxString        title = typeName + wxString::Format( " (%d)", model->GetItemCount() );
        wxDataViewCtrl* dataView = addPage( typeName, title );

        if( typeName == lastPageName )
            newPageId = pageId;

        dataView->AssociateModel( model );

        int colId = 0;
        for( auto& [name, type] : m_columnNameTypes[typeName] )
        {
            int flags = wxDATAVIEW_COL_RESIZABLE | wxDATAVIEW_COL_SORTABLE;

            if( name == wxS( "UUID" ) )
                flags |= wxDATAVIEW_COL_HIDDEN;

            dataView->AppendTextColumn( name, colId, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE,
                                        wxALIGN_LEFT, flags );

            colId++;
        }

        m_dataViews[typeName] = dataView;

        pageId++;
    }

    if( newPageId != -1 )
    {
        m_Notebook->SetSelection( newPageId );

        wxDataViewCtrl*      dataView = m_dataViews[lastPageName];
        int                  uuidCol = dataView->GetColumnCount() - 1;
        wxDataViewListStore* model = m_dataModels[lastPageName];
        size_t               itemCount = model->GetItemCount();
        wxDataViewItemArray  newSelections;

        for( size_t itemId = 0; itemId < itemCount; itemId++ )
        {
            wxVariant var;
            model->GetValueByRow( var, itemId, uuidCol );

            if( lastUUIDs.find( var.GetString() ) != lastUUIDs.end() )
                newSelections.push_back( model->GetItem( itemId ) );
        }

        dataView->SetSelections( newSelections );
    }
}


wxDataViewCtrl* DIALOG_GENERATORS::addPage( const wxString& aName, const wxString& aTitle )
{
    wxPanel* panelPage =
            new wxPanel( m_Notebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );

    panelPage->SetName( aName );

    wxBoxSizer* bSizerPage1;
    bSizerPage1 = new wxBoxSizer( wxVERTICAL );

    bSizerPage1->SetMinSize( wxSize( -1, 320 ) );
    wxDataViewCtrl* dataView = new wxDataViewCtrl( panelPage, wxID_ANY, wxDefaultPosition,
                                                   wxDefaultSize, wxDV_MULTIPLE | wxDV_ROW_LINES );

    dataView->Bind( wxEVT_DATAVIEW_SELECTION_CHANGED, &DIALOG_GENERATORS::OnItemSelected, this );

    bSizerPage1->Add( dataView, 1, wxEXPAND | wxALL, 5 );


    bSizerPage1->Add( 0, 8, 0, wxEXPAND, 5 );


    panelPage->SetSizer( bSizerPage1 );
    panelPage->Layout();
    bSizerPage1->Fit( panelPage );
    m_Notebook->AddPage( panelPage, aTitle, false );

    return dataView;
}


void DIALOG_GENERATORS::onUnitsChanged( wxCommandEvent& event )
{
    m_units = m_frame->GetUserUnits();

    RebuildModels();

    event.Skip();
}


void DIALOG_GENERATORS::onBoardChanged( wxCommandEvent& event )
{
    m_currentBoard = m_frame->GetBoard();

    if( m_currentBoard != nullptr )
        m_currentBoard->AddListener( this );

    RebuildModels();

    event.Skip();
}


void DIALOG_GENERATORS::OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnBoardCompositeUpdate( BOARD&                    aBoard,
                                                std::vector<BOARD_ITEM*>& aAddedItems,
                                                std::vector<BOARD_ITEM*>& aRemovedItems,
                                                std::vector<BOARD_ITEM*>& aChangedItems )
{
    RebuildModels();
}


DIALOG_GENERATORS::DIALOG_GENERATORS( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent ) :
        DIALOG_GENERATORS_BASE( aParent )
{
    SetName( DIALOG_GENERATORS_WINDOW_NAME );

    m_frame = aEditorFrame;
    m_currentBoard = m_frame->GetBoard();

    m_Notebook->DeleteAllPages();

    RebuildModels();

    Bind( EDA_EVT_UNITS_CHANGED, &DIALOG_GENERATORS::onUnitsChanged, this );
    Bind( EDA_EVT_BOARD_CHANGED, &DIALOG_GENERATORS::onBoardChanged, this );

    if( m_currentBoard != nullptr )
    {
        m_currentBoard->AddListener( this );
    }
}


DIALOG_GENERATORS::~DIALOG_GENERATORS()
{
    clearModels();

    if( m_currentBoard != nullptr )
        m_currentBoard->RemoveListener( this );
}


wxDataViewListStore* DIALOG_GENERATORS::getCurrentModel()
{
    wxString pageName = m_Notebook->GetCurrentPage()->GetName();
    return m_dataModels[pageName];
}


void DIALOG_GENERATORS::OnItemSelected( wxDataViewEvent& aEvent )
{
    wxDataViewListStore* model = getCurrentModel();
    wxString             pageName = m_Notebook->GetCurrentPage()->GetName();
    wxDataViewCtrl*      dataView = m_dataViews[pageName];

    if( !model )
        return;

    int                      uuidCol = dataView->GetColumnCount() - 1;
    std::vector<BOARD_ITEM*> boardItems;
    EDA_ITEMS                edaItems;

    wxDataViewItemArray selections;
    dataView->GetSelections( selections );

    for( wxDataViewItem& viewItem : selections )
    {
        wxVariant var;
        model->GetValue( var, viewItem, uuidCol );

        BOARD_ITEM* brdItem = m_currentBoard->ResolveItem( var.GetString() );

        if( !brdItem || brdItem->Type() != KICAD_T::PCB_GENERATOR_T )
            continue;

        boardItems.push_back( brdItem );
        edaItems.push_back( brdItem );
    }

    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );
    m_frame->GetToolManager()->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &edaItems );
    m_frame->FocusOnItems( boardItems );
}


void DIALOG_GENERATORS::OnRebuildSelectedClick( wxCommandEvent& event )
{
    RebuildModels();
}


void DIALOG_GENERATORS::OnRebuildTypeClick( wxCommandEvent& event )
{
    wxDataViewListStore* model = getCurrentModel();
    wxString             pageName = m_Notebook->GetCurrentPage()->GetName();

    if( !model )
        return;

    int       uuidCol = m_columnNameTypes[pageName].size() - 1;
    EDA_ITEMS items;

    for( size_t row = 0; row < model->GetItemCount(); row++ )
    {
        wxVariant var;
        model->GetValueByRow( var, row, uuidCol );

        BOARD_ITEM* item = m_currentBoard->ResolveItem( var.GetString() );

        if( !item || item->Type() != KICAD_T::PCB_GENERATOR_T )
            continue;

        items.push_back( item );
    }

    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );
    m_frame->GetToolManager()->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &items );
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::regenerateSelected );
    RebuildModels();
}


void DIALOG_GENERATORS::OnRebuildAllClick( wxCommandEvent& event )
{
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::regenerateAll );
    RebuildModels();
}


void DIALOG_GENERATORS::OnCancelClick( wxCommandEvent& event )
{
    GENERATOR_TOOL* genTool = m_frame->GetToolManager()->GetTool<GENERATOR_TOOL>();
    genTool->DestroyManagerDialog();
}

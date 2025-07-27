/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
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

#include "panel_sync_sheet_pins.h"
#include "sch_sheet_pin.h"
#include "sheet_synchronization_model.h"
#include "sheet_synchronization_item.h"
#include "sync_sheet_pin_preference.h"
#include "sheet_synchronization_agent.h"

#include <bitmaps.h>
#include <map>
#include <memory>
#include <sch_label.h>
#include <sch_sheet.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <wx/bookctrl.h>
#include <eda_item.h>
#include <wx/string.h>
#include <string_utils.h>

// sch_label.cpp
extern wxString getElectricalTypeLabel( LABEL_FLAG_SHAPE aType );

PANEL_SYNC_SHEET_PINS::PANEL_SYNC_SHEET_PINS( wxWindow* aParent, SCH_SHEET* aSheet, wxNotebook* aNoteBook,
                                              int aIndex, SHEET_SYNCHRONIZATION_AGENT& aAgent,
                                              const SCH_SHEET_PATH& aPath ) :
        PANEL_SYNC_SHEET_PINS_BASE( aParent ),
        m_sheet( aSheet ),
        m_noteBook( aNoteBook ),
        m_index( aIndex ),
        m_sheetFileName( aSheet->GetFileName() ),
        m_agent( aAgent ),
        m_path( aPath ),
        m_views( {
                { SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL, m_viewSheetLabels },
                { SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN, m_viewSheetPins },
                { SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED, m_viewAssociated },
        } )
{
    m_btnUsePinAsTemplate->SetBitmap( KiBitmapBundle( BITMAPS::add_hierar_pin ) );
    m_btnUseLabelAsTemplate->SetBitmap( KiBitmapBundle( BITMAPS::add_hierarchical_label ) );
    m_btnUndo->SetBitmap( KiBitmapBundle( BITMAPS::left ) );

    m_labelSheetName->SetLabel( aSheet->GetFileName() );
    m_labelSymName->SetLabel( aSheet->GetShownName( true ) );


    for( auto& [idx, view] : m_views )
    {
        auto model = wxObjectDataPtr<SHEET_SYNCHRONIZATION_MODEL>(
                new SHEET_SYNCHRONIZATION_MODEL( m_agent, m_sheet, m_path ) );
        view->AssociateModel( model.get() );
        m_models.try_emplace( idx, std::move( model ) );

        for( int col : { SHEET_SYNCHRONIZATION_MODEL::NAME, SHEET_SYNCHRONIZATION_MODEL::SHAPE } )
        {
            switch( col )
            {
            case SHEET_SYNCHRONIZATION_MODEL::NAME:
                view->AppendIconTextColumn( SHEET_SYNCHRONIZATION_MODEL::GetColName( col ), col,
                                            wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );
                break;
            case SHEET_SYNCHRONIZATION_MODEL::SHAPE:
                view->AppendTextColumn( SHEET_SYNCHRONIZATION_MODEL::GetColName( col ), col,
                                        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE );
                break;
            }
        }
    }

    for( auto& [idx, view] : m_views )
        PostProcessModelSelection( idx, {} );
}


void PANEL_SYNC_SHEET_PINS::UpdateForms()
{
    SHEET_SYNCHRONIZATION_ITEM_LIST labels_list, pins_list, associated_list;
    auto labels_ori = m_sheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T );
    std::vector<SCH_SHEET_PIN*> pins_ori = m_sheet->GetPins();

    // De-duplicate the hierarchical labels list
    std::set<wxString>          dedup_labels_ori_text;
    std::vector<SCH_HIERLABEL*> dedup_labels_ori;

    for( const auto& item : labels_ori )
    {
        SCH_HIERLABEL* label = static_cast<SCH_HIERLABEL*>( item );

        if( dedup_labels_ori_text.count( label->GetText() ) == 0 )
        {
            dedup_labels_ori_text.insert( label->GetText() );
            dedup_labels_ori.push_back( label );
        }
    }

    std::sort( dedup_labels_ori.begin(), dedup_labels_ori.end(),
               []( const SCH_HIERLABEL* label1, const SCH_HIERLABEL* label2 )
               {
                   return StrNumCmp( label1->GetText(), label2->GetText(), true ) < 0;
               } );

    auto check_matched = [&]( SCH_HIERLABEL* label )
    {
        for( size_t i = 0; i < pins_ori.size(); i++ )
        {
            SCH_SHEET_PIN* cur_pin = pins_ori[i];

            if( label->GetText() == cur_pin->GetText() && label->GetShape() == cur_pin->GetShape() )
            {
                associated_list.push_back(
                        std::make_shared<ASSOCIATED_SCH_LABEL_PIN>( label, cur_pin ) );
                pins_ori.erase( pins_ori.begin() + i );
                return;
            }
        }

        labels_list.push_back(
                std::make_shared<SCH_HIERLABEL_SYNCHRONIZATION_ITEM>( label, m_sheet ) );
    };

    for( const auto& item : dedup_labels_ori )
        check_matched( static_cast<SCH_HIERLABEL*>( item ) );

    for( const auto& pin : pins_ori )
        pins_list.push_back( std::make_shared<SCH_SHEET_PIN_SYNCHRONIZATION_ITEM>(
                static_cast<SCH_SHEET_PIN*>( pin ), m_sheet ) );

    m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->UpdateItems( std::move( labels_list ) );
    m_models[SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN]->UpdateItems( std::move( pins_list ) );
    m_models[SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED]->UpdateItems( std::move( associated_list ) );

    UpdatePageImage();
}


SHEET_SYNCHRONIZATION_MODEL_PTR PANEL_SYNC_SHEET_PINS::GetModel( int aKind ) const
{
    return m_models.at( aKind );
}


const wxString& PANEL_SYNC_SHEET_PINS::GetSheetFileName() const
{
    return m_sheetFileName;
}


PANEL_SYNC_SHEET_PINS::~PANEL_SYNC_SHEET_PINS()
{
}


bool PANEL_SYNC_SHEET_PINS::HasUndefinedSheetPing() const
{
    return !m_models.at( SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL )->GetCount()
           && !m_models.at( SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN )->GetCount();
}


void PANEL_SYNC_SHEET_PINS::OnBtnAddLabelsClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )

    wxDataViewItemArray selected_items;
    std::set<EDA_ITEM*> selected_items_set;
    m_viewSheetPins->GetSelections( selected_items );

    for( const auto& it : selected_items )
    {
        if( SHEET_SYNCHRONIZATION_ITE_PTR item =
                    m_models[SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN]->GetSynchronizationItem( it ) )
            selected_items_set.insert( item->GetItem() );
    }

    if( selected_items_set.empty() )
        return;

    m_agent.PlaceHieraLable( m_sheet, m_path, std::move( selected_items_set ) );
}


void PANEL_SYNC_SHEET_PINS::OnBtnAddSheetPinsClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )
    wxDataViewItemArray selected_items;
    std::set<EDA_ITEM*> selected_items_set;
    m_viewSheetLabels->GetSelections( selected_items );

    for( const auto& it : selected_items )
    {
        if( SHEET_SYNCHRONIZATION_ITE_PTR item =
                    m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->GetSynchronizationItem(
                            it ) )
            selected_items_set.insert( item->GetItem() );
    }

    if( selected_items_set.empty() )
        return;

    m_agent.PlaceSheetPin( m_sheet, m_path, std::move( selected_items_set ) );
}


void PANEL_SYNC_SHEET_PINS::GenericSync( SYNC_DIRECTION direction )
{
    wxDataViewItem labelIdx = m_viewSheetLabels->GetSelection();
    wxDataViewItem pinIdx = m_viewSheetPins->GetSelection();

    for( auto& idx : { labelIdx, pinIdx } )
    {
        if( !idx.IsOk() )
            return;
    }

    SHEET_SYNCHRONIZATION_ITE_PTR labelItem =
            m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->TakeItem( labelIdx );
    SHEET_SYNCHRONIZATION_ITE_PTR pinItem =
            m_models[SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN]->TakeItem( pinIdx );

    for( const auto& item : { labelItem, pinItem } )
    {
        if( !item )
            return;
    }

    auto label_ptr =
            std::static_pointer_cast<SCH_HIERLABEL_SYNCHRONIZATION_ITEM>( labelItem )->GetLabel();
    auto pin_ptr =
            std::static_pointer_cast<SCH_SHEET_PIN_SYNCHRONIZATION_ITEM>( pinItem )->GetPin();

    switch( direction )
    {
    case SYNC_DIRECTION::USE_LABEL_AS_TEMPLATE:
        m_agent.ModifyItem(
                *pinItem,
                [&]()
                {
                    pin_ptr->SetText( label_ptr->GetText() );
                    pin_ptr->SetShape( label_ptr->GetShape() );
                },
                m_path );
        break;
    case SYNC_DIRECTION::USE_PIN_AS_TEMPLATE:
        m_agent.ModifyItem(
                *labelItem,
                [&]()
                {
                    label_ptr->SetText( pin_ptr->GetText() );
                    label_ptr->SetShape( pin_ptr->GetShape() );
                },
                m_path );
        break;
    }

    m_models[SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED]->AppendItem(
            std::make_shared<ASSOCIATED_SCH_LABEL_PIN>( label_ptr, pin_ptr ) );
    UpdatePageImage();

    for( auto idx :
         { SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL, SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN } )
    {
        PostProcessModelSelection( idx, {} );
    }

    m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->DoNotify();
}


void PANEL_SYNC_SHEET_PINS::UpdatePageImage() const
{
    m_noteBook->SetPageImage( m_index, HasUndefinedSheetPing()
                                               ? SYNC_SHEET_PIN_PREFERENCE::HAS_UNMATCHED
                                               : SYNC_SHEET_PIN_PREFERENCE::ALL_MATCHED );
}


void PANEL_SYNC_SHEET_PINS::OnBtnUsePinAsTemplateClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )
    return GenericSync( SYNC_DIRECTION::USE_PIN_AS_TEMPLATE );
}


void PANEL_SYNC_SHEET_PINS::OnBtnUseLabelAsTemplateClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )
    return GenericSync( SYNC_DIRECTION::USE_LABEL_AS_TEMPLATE );
}


void PANEL_SYNC_SHEET_PINS::OnBtnRmPinsClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )
    wxDataViewItemArray array;
    m_viewSheetPins->GetSelections( array );
    m_models[SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN]->RemoveItems( array );
    PostProcessModelSelection( SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN, {} );
    UpdatePageImage();
}


void PANEL_SYNC_SHEET_PINS::OnBtnRmLabelsClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )
    wxDataViewItemArray array;
    m_viewSheetLabels->GetSelections( array );
    m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->RemoveItems( array );
    PostProcessModelSelection( SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL, {} );
    UpdatePageImage();
}


void PANEL_SYNC_SHEET_PINS::OnBtnUndoClicked( wxCommandEvent& aEvent )
{
    WXUNUSED( aEvent )
    wxDataViewItemArray indexes;
    m_viewAssociated->GetSelections( indexes );
    auto items = m_models[SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED]->TakeItems( indexes );

    if( !items.size() )
        return;

    for( auto& item : items )
    {
        auto associated = std::static_pointer_cast<ASSOCIATED_SCH_LABEL_PIN>( item );
        m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->AppendItem(
                std::make_shared<SCH_HIERLABEL_SYNCHRONIZATION_ITEM>( associated->GetLabel(),
                                                                      m_sheet ) );
        m_models[SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN]->AppendItem(
                std::make_shared<SCH_SHEET_PIN_SYNCHRONIZATION_ITEM>( associated->GetPin(),
                                                                      m_sheet ) );
    }

    PostProcessModelSelection( SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED, {} );
    UpdatePageImage();
}


void PANEL_SYNC_SHEET_PINS::PostProcessModelSelection( int aIdex, wxDataViewItem const& aItem )
{
    if( aItem.IsOk() )
        m_models[aIdex]->OnRowSelected( m_models[aIdex]->GetRow( aItem ) );
    else
        m_models[aIdex]->OnRowSelected( {} );

    const bool has_selected_row = m_views[aIdex]->GetSelectedItemsCount() > 0;

    switch( aIdex )
    {
    case SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN:
    {
        for( auto btn : { m_btnAddLabels, m_btnRmPins } )
            btn->Enable( has_selected_row );

        break;
    }
    case SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL:
    {
        for( auto btn : { m_btnAddSheetPins, m_btnRmLabels } )
            btn->Enable( has_selected_row );

        break;
    }
    case SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED:
    {
        m_btnUndo->Enable( has_selected_row );
        break;
    }
    default:
        break;
    }

    if( aIdex != SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED )
    {
        for( auto btn : { m_btnUsePinAsTemplate, m_btnUseLabelAsTemplate } )
        {
            btn->Enable( m_models[SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN]->HasSelectedIndex()
                         && m_models[SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL]->HasSelectedIndex() );
        }
    }
}


void PANEL_SYNC_SHEET_PINS::OnViewSheetLabelCellClicked( wxDataViewEvent& aEvent )
{
    PostProcessModelSelection( SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL, aEvent.GetItem() );
}


void PANEL_SYNC_SHEET_PINS::OnViewSheetPinCellClicked( wxDataViewEvent& aEvent )
{
    PostProcessModelSelection( SHEET_SYNCHRONIZATION_MODEL::SHEET_PIN, aEvent.GetItem() );
}


void PANEL_SYNC_SHEET_PINS::OnViewMatchedCellClicked( wxDataViewEvent& aEvent )
{
    PostProcessModelSelection( SHEET_SYNCHRONIZATION_MODEL::ASSOCIATED, aEvent.GetItem() );
}

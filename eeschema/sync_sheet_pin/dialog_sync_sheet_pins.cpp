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

#include "dialog_sync_sheet_pins.h"
#include "panel_sync_sheet_pins.h"
#include "sch_item.h"
#include "sch_label.h"
#include "sheet_synchronization_model.h"
#include "sync_sheet_pin_preference.h"
#include "sheet_synchronization_notifier.h"
#include "sheet_synchronization_agent.h"
#include "sheet_synchronization_item.h"

#include <memory>
#include <cstddef>
#include <sch_sheet_pin.h>
#include <sch_sheet.h>
#include <unordered_map>
#include <sch_drawing_tools.h>


DIALOG_SYNC_SHEET_PINS::DIALOG_SYNC_SHEET_PINS(
        wxWindow* aParent, std::list<SCH_SHEET_PATH> aSheetPath,
        std::shared_ptr<SHEET_SYNCHRONIZATION_AGENT> aAgent ) :
        DIALOG_SYNC_SHEET_PINS_BASE( aParent ), m_agent( std::move( aAgent ) ),
        m_lastEditSheet( nullptr ), m_placeItemKind( PlaceItemKind::UNDEFINED ),
        m_currentTemplate( nullptr )
{
    wxImageList* imageList = new wxImageList( SYNC_SHEET_PIN_PREFERENCE::NORMAL_WIDTH,
                                              SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT );

    for( const auto& [icon_idx, bitmap] : SYNC_SHEET_PIN_PREFERENCE::GetBookctrlPageIcon() )
    {
        imageList->Add( KiBitmap( bitmap, SYNC_SHEET_PIN_PREFERENCE::NORMAL_HEIGHT ) );
    }

    m_notebook->AssignImageList( imageList );
    int                                                             count = -1;
    std::unordered_map<wxString, std::list<PANEL_SYNC_SHEET_PINS*>> sheet_instances;

    for( const auto& sheet_path : aSheetPath )
    {
        auto                   sheet = sheet_path.Last();
        wxString               fileName = sheet->GetFileName();
        PANEL_SYNC_SHEET_PINS* page = new PANEL_SYNC_SHEET_PINS( m_notebook, sheet, m_notebook,
                                                                 ++count, *m_agent, sheet_path );
        m_notebook->AddPage( page, sheet->GetShownName( true ), {}, page->HasUndefinedSheetPing() );
        page->UpdateForms();

        if( sheet_instances.find( fileName ) == sheet_instances.end() )
        {
            sheet_instances.try_emplace( fileName, std::list<PANEL_SYNC_SHEET_PINS*>{ page } );
        }
        else
        {
            sheet_instances[fileName].push_back( page );
        }

        m_panels.try_emplace( sheet, page );
    }

    for( auto& [sheet_name, panel_list] : sheet_instances )
    {
        if( panel_list.size() > 1 )
        {
            std::list<std::shared_ptr<SHEET_SYNCHRONIZATION_NOTIFIER>> sheet_change_notifiers;
            std::list<SHEET_SYNCHRONIZATION_MODEL*>                    sheet_sync_models;

            for( auto& panel : panel_list )
            {
                SHEET_SYNCHRONIZATION_MODEL* model =
                        panel->GetModel( SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL ).get();
                sheet_sync_models.push_back( model );
                sheet_change_notifiers.push_back(
                        std::make_shared<SHEET_FILE_CHANGE_NOTIFIER>( model, panel ) );
            }

            for( auto& notifier : sheet_change_notifiers )
            {
                for( auto& other : sheet_sync_models )
                {
                    if( notifier->GetOwner() != other )
                    {
                        other->AddNotifier( notifier );
                    }
                }
            }
        }
    }

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_SYNC_SHEET_PINS::OnClose, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_SYNC_SHEET_PINS::~DIALOG_SYNC_SHEET_PINS() = default;


void DIALOG_SYNC_SHEET_PINS::OnClose( wxCloseEvent& aEvent )
{
    aEvent.Skip();
}


void DIALOG_SYNC_SHEET_PINS::EndPlaceItem( EDA_ITEM* aNewItem )
{
    if( !aNewItem )
        return;

    auto post_end_place_item =
            std::shared_ptr<std::nullptr_t>( nullptr,
                                             [&]( std::nullptr_t )
                                             {
                                                 m_placementTemplateSet.erase( m_currentTemplate );

                                                 if( m_placementTemplateSet.empty() )
                                                 {
                                                     EndPlacement();
                                                 }
                                                 else
                                                 {
                                                     m_currentTemplate = *m_placementTemplateSet.begin();
                                                 }
                                             } );


    if( m_lastEditSheet && m_panels.find( m_lastEditSheet ) != m_panels.end() )
    {
        auto& panel = m_panels[m_lastEditSheet];
        auto  template_item = static_cast<SCH_HIERLABEL*>( m_currentTemplate );
        auto  new_item = static_cast<SCH_HIERLABEL*>( aNewItem );

        //Usr may edit the name or shape while placing the new item , do sync if either differs
        if( template_item->GetText() != new_item->GetText()
            || template_item->GetShape() != new_item->GetShape() )
        {
            m_agent->ModifyItem(
                    template_item,
                    [&]()
                    {
                        template_item->SetText( new_item->GetText() );
                        template_item->SetShape( new_item->GetShape() );
                    },
                    panel->GetSheetPath(),
                    PlaceItemKind::SHEET_PIN == m_placeItemKind
                            ? SHEET_SYNCHRONIZATION_ITEM_KIND::HIERLABEL
                            : SHEET_SYNCHRONIZATION_ITEM_KIND::SHEET_PIN );
        }

        panel->UpdateForms();

        if( PlaceItemKind::HIERLABEL == m_placeItemKind )
            panel->GetModel( SHEET_SYNCHRONIZATION_MODEL::HIRE_LABEL )->DoNotify();
    }
}


void DIALOG_SYNC_SHEET_PINS::PreparePlacementTemplate( SCH_SHEET* aSheet, PlaceItemKind aKind,
                                                       std::set<EDA_ITEM*> const& aPlacementTemplateSet )
{
    if( aPlacementTemplateSet.empty() )
        return;

    m_lastEditSheet = aSheet;
    m_placeItemKind = aKind;
    m_placementTemplateSet = aPlacementTemplateSet;
    m_currentTemplate = *m_placementTemplateSet.begin();
}


SCH_HIERLABEL* DIALOG_SYNC_SHEET_PINS::GetPlacementTemplate() const
{
    if( !m_currentTemplate )
        return {};

    return static_cast<SCH_HIERLABEL*>( m_currentTemplate );
}


bool DIALOG_SYNC_SHEET_PINS::CanPlaceMore() const
{
    return !m_placementTemplateSet.empty();
}


void DIALOG_SYNC_SHEET_PINS::EndPlacement()
{
    m_placementTemplateSet.clear();
    m_placeItemKind = PlaceItemKind::UNDEFINED;
    m_currentTemplate = nullptr;
}

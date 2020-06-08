/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <drc/drc_textvar_tester.h>

#include <class_module.h>
#include <class_pcb_text.h>
#include <ws_draw_item.h>
#include <ws_proxy_view_item.h>

DRC_TEXTVAR_TESTER::DRC_TEXTVAR_TESTER( MARKER_HANDLER aMarkerHandler,
                                        KIGFX::WS_PROXY_VIEW_ITEM* aWorksheet ) :
        DRC_TEST_PROVIDER( std::move( aMarkerHandler ) ),
        m_units( EDA_UNITS::MILLIMETRES ),
        m_board( nullptr ),
        m_worksheet( aWorksheet )
{
}


bool DRC_TEXTVAR_TESTER::RunDRC( EDA_UNITS aUnits, BOARD& aBoard )
{
    bool success = true;

    m_units = aUnits;
    m_board = &aBoard;

    for( MODULE* module : m_board->Modules() )
    {
        module->RunOnChildren(
            [&]( BOARD_ITEM* child )
            {
                if( child->Type() == PCB_MODULE_TEXT_T )
                {
                    TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( child );

                    if( text->GetShownText().Matches( wxT( "*${*}*" ) ) )
                    {
                        DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
                        drcItem->SetItems( text );

                        HandleMarker( new MARKER_PCB( drcItem, text->GetPosition() ) );
                        success = false;
                    }
                }
            } );
    }

    for( BOARD_ITEM* drawing : m_board->Drawings() )
    {
        if( drawing->Type() == PCB_TEXT_T )
        {
            TEXTE_PCB* text = static_cast<TEXTE_PCB*>( drawing );

            if( text->GetShownText().Matches( wxT( "*${*}*" ) ) )
            {
                DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
                drcItem->SetItems( text );

                HandleMarker( new MARKER_PCB( drcItem, text->GetPosition() ) );
                success = false;
            }
        }
    }

    WS_DRAW_ITEM_LIST wsItems;

    if( m_worksheet )
    {
        wsItems.SetMilsToIUfactor( IU_PER_MILS );
        wsItems.BuildWorkSheetGraphicList( m_worksheet->GetPageInfo(),
                                           m_worksheet->GetTitleBlock() );

        for( WS_DRAW_ITEM_BASE* item = wsItems.GetFirst(); item; item = wsItems.GetNext() )
        {
            if( WS_DRAW_ITEM_TEXT* text = dynamic_cast<WS_DRAW_ITEM_TEXT*>( item ) )
            {
                if( text->GetShownText().Matches( wxT( "*${*}*" ) ) )
                {
                    DRC_ITEM* drcItem = DRC_ITEM::Create( DRCE_UNRESOLVED_VARIABLE );
                    drcItem->SetErrorMessage( _( "Unresolved text variable in worksheet." ) );

                    HandleMarker( new MARKER_PCB( drcItem, text->GetPosition() ) );
                    success = false;
                }
            }
        }
    }

    // JEY TODO: Test text vars in worksheet...

    return success;
}



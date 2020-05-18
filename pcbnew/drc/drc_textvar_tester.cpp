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


DRC_TEXTVAR_TESTER::DRC_TEXTVAR_TESTER( MARKER_HANDLER aMarkerHandler ) :
        DRC_TEST_PROVIDER( std::move( aMarkerHandler ) ),
        m_units( EDA_UNITS::MILLIMETRES ),
        m_board( nullptr )
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
                        DRC_ITEM* drcItem = new DRC_ITEM( DRCE_UNRESOLVED_VARIABLE );
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
                DRC_ITEM* drcItem = new DRC_ITEM( DRCE_UNRESOLVED_VARIABLE );
                drcItem->SetItems( text );

                HandleMarker( new MARKER_PCB( drcItem, text->GetPosition() ) );
                success = false;
            }
        }
    }

    return success;
}



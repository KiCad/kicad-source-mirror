/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#include "dialog_footprint_associations.h"
#include <widgets/wx_grid.h>
#include <pcb_base_frame.h>
#include <kiface_base.h>
#include <footprint_library_adapter.h>
#include <board.h>
#include <footprint.h>
#include <project_pcb.h>


DIALOG_FOOTPRINT_ASSOCIATIONS::DIALOG_FOOTPRINT_ASSOCIATIONS( PCB_BASE_FRAME* aFrame,
                                                              FOOTPRINT* aFootprint ) :
        DIALOG_FOOTPRINT_ASSOCIATIONS_BASE( aFrame ),
        m_frame( aFrame ),
        m_footprint( aFootprint )
{
    // Remove wxgrid's selection boxes
    for( wxGrid* grid : { m_gridLibrary, m_gridSymbol } )
        grid->SetCellHighlightPenWidth( 0 );

    m_libraryAssociationLabel->SetFont( KIUI::GetStatusFont( this ) );
    m_symbolAssociationLabel->SetFont( KIUI::GetStatusFont( this ) );

    m_gridLibrary->SetCellValue( 0, 0, _( "Library: " ) );
    m_gridLibrary->SetCellValue( 1, 0, _( "Footprint: " ) );
}


bool DIALOG_FOOTPRINT_ASSOCIATIONS::TransferDataToWindow()
{
    LIB_ID   fpID = m_footprint->GetFPID();
    wxString libName = fpID.GetLibNickname();
    wxString fpName = fpID.GetLibItemName();
    wxString libDesc;
    wxString fpDesc;

    PROJECT* project = m_footprint->GetBoard()->GetProject();
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( project );

    if( std::optional<LIBRARY_TABLE_ROW*> row = adapter->GetRow( libName ); row )
        libDesc = ( *row )->Description();

    std::shared_ptr<FOOTPRINT> libFootprint;

    try
    {
        libFootprint.reset( adapter->LoadFootprint( libName, fpName, true ) );
        fpDesc = libFootprint->GetLibDescription();
    }
    catch( const IO_ERROR& )
    {
    }

    m_gridLibrary->SetCellValue( 0, 1, fpID.GetLibNickname() );
    m_gridLibrary->SetCellValue( 0, 2, libDesc );

    m_gridLibrary->SetCellValue( 1, 1, fpID.GetLibItemName() );
    m_gridLibrary->SetCellValue( 1, 2, fpDesc );

    KIID_PATH symbolPath = m_footprint->GetPath();

    m_gridSymbol->ClearRows();

    for( int ii = 0; ii < (int) symbolPath.size(); ++ii )
    {
        m_gridSymbol->AppendRows();
        m_gridSymbol->SetCellValue( ii, 0, ii == (int) symbolPath.size() - 1 ? _( "Symbol:" )
                                                                             : _( "Sheet: " ) );
        m_gridSymbol->SetCellValue( ii, 1, symbolPath[ii].AsString() );

        if( !Kiface().IsSingle() && m_frame->Kiway().Player( FRAME_SCH, false ) )
        {
            std::string item = symbolPath[ii].AsString().ToStdString();
            m_frame->Kiway().ExpressMail( FRAME_SCH, MAIL_SCH_GET_ITEM, item );
            m_gridSymbol->SetCellValue( ii, 2, item );
        }
    }

    finishDialogSettings();

    return true;
}



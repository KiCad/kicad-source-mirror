/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/sizer.h>
#include <wx/aui/auibook.h>
#include <wx/msgdlg.h>
#include <confirm.h>
#include <lib_table_grid_data_model.h>
#include <lib_table_notebook_panel.h>


LIB_TABLE_NOTEBOOK_PANEL::~LIB_TABLE_NOTEBOOK_PANEL()
{
    // Delete the GRID_TRICKS.
    GetGrid()->PopEventHandler( true );
}


void LIB_TABLE_NOTEBOOK_PANEL::AddTable( wxAuiNotebook* aNotebook, const wxString& aTitle, bool aClosable )
{
    LIB_TABLE_NOTEBOOK_PANEL* panel = new LIB_TABLE_NOTEBOOK_PANEL( aNotebook, wxID_ANY );
   	wxBoxSizer*               sizer = new wxBoxSizer( wxVERTICAL );
    WX_GRID*                  grid = new WX_GRID( panel, wxID_ANY );
   
   	// Grid
   	grid->CreateGrid( 1, 7 );
   	grid->EnableGridLines( true );
   	grid->SetMargins( 0, 0 );
    grid->SetSelectionMode( wxGrid::wxGridSelectRows );

   	// Columns
   	grid->SetColSize( 0, 30 );
   	grid->SetColSize( 1, 48 );
   	grid->SetColSize( 2, 48 );
   	grid->SetColSize( 3, 240 );
   	grid->SetColSize( 4, 100 );
   	grid->SetColSize( 5, 80 );
   	grid->SetColSize( 6, 240 );
   	grid->SetColLabelSize( 22 );
   	grid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
   
   	// Rows
   	grid->EnableDragRowSize( false );
   	grid->SetRowLabelSize( 0 );
   	grid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
   
   	// Cell Defaults
   	grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );

    grid->DisableColResize( COL_STATUS );
    grid->DisableColResize( COL_VISIBLE );
    grid->DisableColResize( COL_ENABLED );

    grid->AutoSizeColumn( COL_VISIBLE, true );
    grid->AutoSizeColumn( COL_ENABLED, true );

    sizer->Add( grid, 1, wxALL|wxEXPAND, 5 );

    panel->SetSizer( sizer );
    panel->SetClosable( aClosable );
   	panel->Layout();
   	sizer->Fit( panel );

   	aNotebook->AddPage( panel, aTitle, true );
}


bool LIB_TABLE_NOTEBOOK_PANEL::TableModified()
{
    wxFileName                     uri = GetModel()->Table().Path();
    std::unique_ptr<LIBRARY_TABLE> sourceTable = std::make_unique<LIBRARY_TABLE>( uri, LIBRARY_TABLE_SCOPE::GLOBAL );

    return GetModel()->Table() != *sourceTable;
}


bool LIB_TABLE_NOTEBOOK_PANEL::SaveTable()
{
    bool retVal = true;

    GetModel()->Table().Save().map_error(
            [&]( const LIBRARY_ERROR& aError )
            {
                wxMessageBox( _( "Error saving nested library table:\n\n" ) + aError.message,
                              _( "File Save Error" ), wxOK | wxICON_ERROR );

                retVal = false;
            } );

    return retVal;
}


bool LIB_TABLE_NOTEBOOK_PANEL::GetCanClose()
{
    if( !GetGrid()->CommitPendingChanges() )
        return false;

    if( TableModified() )
    {
        if( !HandleUnsavedChanges( this, _( "Save changes to nested library table?" ),
                                   [&]() -> bool
                                   {
                                       return SaveTable();
                                   } ) )
        {
            return false;
        }
    }

    return true;
}


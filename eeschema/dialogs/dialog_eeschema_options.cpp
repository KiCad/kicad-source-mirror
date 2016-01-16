/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file dialog_eeschema_options.cpp
 */

#include <fctsys.h>
#include <class_base_screen.h>

#include <dialog_eeschema_options.h>
#include <widgets/widget_hotkey_list.h>
#include <schframe.h>
#include <hotkeys.h>

#include <wx/settings.h>


DIALOG_EESCHEMA_OPTIONS::DIALOG_EESCHEMA_OPTIONS( SCH_EDIT_FRAME* parent ) :
    DIALOG_EESCHEMA_OPTIONS_BASE( parent )
{
    m_choiceUnits->SetFocus();
    m_sdbSizerOK->SetDefault();

    // Dialog should not shrink beyond it's minimal size.
    GetSizer()->SetSizeHints( this );

    // wxformbuilder doesn't seem to let us set minimal sizes. Copy the default
    // sizes into the minimal sizes, then, and autosize:
    for( int i = 0; i < m_fieldGrid->GetNumberCols(); ++i )
    {
        m_fieldGrid->SetColMinimalWidth( i, m_fieldGrid->GetColSize( i ) );
        m_fieldGrid->AutoSizeColLabelSize( i );
    }

    // Embed the hotkeys list
    HOTKEY_SECTIONS sections = WIDGET_HOTKEY_LIST::GenSections( g_Eeschema_Hokeys_Descr );
    m_hotkeyListCtrl = new WIDGET_HOTKEY_LIST( m_panelHotkeys, sections );
    m_hotkeyListCtrl->InstallOnPanel( m_panelHotkeys );

    // Make sure we select the first tab of the options tab page
    m_notebook->SetSelection( 0 );

    // Lay out all child pages
    // No, I don't know why this->Layout() doesn't propagate through to these,
    // but at least on MSW, it does not.
    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
    {
        m_notebook->GetPage( i )->Layout();
    }

    Layout();
}


SCH_EDIT_FRAME* DIALOG_EESCHEMA_OPTIONS::GetParent()
{
    return static_cast<SCH_EDIT_FRAME*>( DIALOG_EESCHEMA_OPTIONS_BASE::GetParent() );
}


void DIALOG_EESCHEMA_OPTIONS::SetUnits( const wxArrayString& units, int select )
{
    wxASSERT( units.GetCount() > 0
              && ( select >= 0 && (size_t) select < units.GetCount() ) );

    m_choiceUnits->Append( units );
    m_choiceUnits->SetSelection( select );
}


void DIALOG_EESCHEMA_OPTIONS::SetRefIdSeparator( wxChar aSep, wxChar aFirstId)
{
    // m_choiceSeparatorRefId displays one of
    // "A" ".A" "-A" "_A" ".1" "-1" "_1" option

    int sel = 0;
    switch( aSep )
    {
        default:
        case 0:
            aFirstId = 'A';     // cannot use a number without separator
            break;

        case '.':
            sel = 1;
            break;

        case '-':
            sel = 2;
            break;

        case '_':
            sel = 3;
            break;
    }

    if( aFirstId == '1' )
        sel = 4;

    m_choiceSeparatorRefId->SetSelection( sel );
}

void DIALOG_EESCHEMA_OPTIONS::GetRefIdSeparator( int& aSep, int& aFirstId)
{
    // m_choiceSeparatorRefId displays one of
    // "A" ".A" "-A" "_A" ".1" "-1" "_1" option

    aFirstId = 'A';
    switch(  m_choiceSeparatorRefId->GetSelection() )
    {
        default:
        case 0: aSep = 0; break;
        case 1: aSep = '.'; break;
        case 2: aSep = '-'; break;
        case 3: aSep = '_'; break;
        case 4: aFirstId = '1'; aSep = '.'; break;
        case 5: aFirstId = '1'; aSep = '-'; break;
        case 6: aFirstId = '1'; aSep = '_'; break;
    }
}


void DIALOG_EESCHEMA_OPTIONS::SetGridSizes( const GRIDS& aGridSizes, int aGridId )
{
    wxASSERT( aGridSizes.size() > 0 );

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < aGridSizes.size(); i++ )
    {
        wxString tmp;
        tmp.Printf( wxT( "%0.1f" ), aGridSizes[i].m_Size.x );
        m_choiceGridSize->Append( tmp );

        if( aGridSizes[i].m_CmdId == aGridId )
            select = (int) i;
    }

    m_choiceGridSize->SetSelection( select );
}


void DIALOG_EESCHEMA_OPTIONS::OnAddButtonClick( wxCommandEvent& event )
{
    // If a single row is selected, insert after that row.
    int selected_row = -1;
    int n_found = 0;

    for( int row = 0; row < m_fieldGrid->GetNumberRows(); ++row )
    {
        bool this_row_selected = false;

        for( int col = 0; col < m_fieldGrid->GetNumberCols(); ++col )
        {
            if( m_fieldGrid->IsInSelection( row, col ) )
                this_row_selected = true;
        }

        if( this_row_selected )
        {
            selected_row = row;
            ++n_found;
        }
    }

    TransferDataFromWindow();

    TEMPLATE_FIELDNAMES::iterator pos;

    if( n_found == 1 )
        pos = templateFields.begin() + selected_row + 1;
    else
        pos = templateFields.end();

    // Add a new fieldname to the fieldname list
    TEMPLATE_FIELDNAME newFieldname = TEMPLATE_FIELDNAME( "Fieldname" );
    newFieldname.m_Value = wxT( "Value" );
    newFieldname.m_Visible = false;
    templateFields.insert( pos, newFieldname );
    TransferDataToWindow();

    event.Skip();
}


void DIALOG_EESCHEMA_OPTIONS::OnDeleteButtonClick( wxCommandEvent& event )
{
    // wxGrid has a somewhat complex way of detemining selection.
    // This is pretty much the easiest way to do it, here.

    std::vector<bool> rows_to_delete( templateFields.size(), false );

    for( int row = 0; row < m_fieldGrid->GetNumberRows(); ++row )
    {
        for( int col = 0; col < m_fieldGrid->GetNumberCols(); ++col )
        {
            if( m_fieldGrid->IsInSelection( row, col ) )
                rows_to_delete[row] = true;
        }
    }

    TransferDataFromWindow();

    int n_rows = m_fieldGrid->GetNumberRows();

    for( int count = 0; count < n_rows; ++count )
    {
        // Iterate backwards, unsigned-friendly way for future
        int row = n_rows - count - 1;

        if( rows_to_delete[row] )
        {
            templateFields.erase( templateFields.begin() + row );
        }
    }

    TransferDataToWindow();
}


bool DIALOG_EESCHEMA_OPTIONS::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_hotkeyListCtrl->TransferDataToControl() )
        return false;

    m_fieldGrid->Freeze();

    if( m_fieldGrid->GetNumberRows() )
        m_fieldGrid->DeleteRows( 0, m_fieldGrid->GetNumberRows() );

    m_fieldGrid->AppendRows( templateFields.size() );

    for( int row = 0; row < m_fieldGrid->GetNumberRows(); ++row )
    {
        m_fieldGrid->SetCellValue( row, 0, templateFields[row].m_Name );
        m_fieldGrid->SetCellValue( row, 1, templateFields[row].m_Value );
        m_fieldGrid->SetCellValue( row, 2,
                templateFields[row].m_Visible ? wxT( "1" ) : wxEmptyString );

        // Set cell properties
        m_fieldGrid->SetCellAlignment( row, 0, wxALIGN_LEFT, wxALIGN_CENTRE );
        m_fieldGrid->SetCellAlignment( row, 1, wxALIGN_LEFT, wxALIGN_CENTRE );

        // Render the Visible column as a check box
        m_fieldGrid->SetCellEditor( row, 2, new wxGridCellBoolEditor() );
        m_fieldGrid->SetCellRenderer( row, 2, new wxGridCellBoolRenderer() );
        m_fieldGrid->SetCellAlignment( row, 2, wxALIGN_CENTRE, wxALIGN_CENTRE );
    }

    m_fieldGrid->AutoSizeRows();
    m_fieldGrid->Thaw();

    Layout();
    return true;
}


bool DIALOG_EESCHEMA_OPTIONS::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !m_hotkeyListCtrl->TransferDataFromControl() )
        return false;

    // Refresh hotkeys
    GetParent()->ReCreateMenuBar();
    GetParent()->Refresh();

    for( int row = 0; row < m_fieldGrid->GetNumberRows(); ++row )
    {
        templateFields[row].m_Name  = m_fieldGrid->GetCellValue( row, 0 );
        templateFields[row].m_Value = m_fieldGrid->GetCellValue( row, 1 );
        templateFields[row].m_Visible = ( m_fieldGrid->GetCellValue( row, 2 ) != wxEmptyString );
    }

    return true;
}


void DIALOG_EESCHEMA_OPTIONS::SetTemplateFields( const TEMPLATE_FIELDNAMES& aFields )
{
    // Set the template fields object
    templateFields = aFields;

    // Build and refresh the view
    TransferDataToWindow();
}


TEMPLATE_FIELDNAMES DIALOG_EESCHEMA_OPTIONS::GetTemplateFields( void )
{
    return templateFields;
}

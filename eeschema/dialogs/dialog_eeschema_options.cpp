/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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


DIALOG_EESCHEMA_OPTIONS::DIALOG_EESCHEMA_OPTIONS( wxWindow* parent ) :
    DIALOG_EESCHEMA_OPTIONS_BASE( parent )
{
    m_choiceUnits->SetFocus();
    m_sdbSizer1OK->SetDefault();

    wxListItem col0;
    col0.SetId( 0 );
    col0.SetText( _( "Field Name" ) );
    col0.SetWidth( 150 );

    wxListItem col1;
    col1.SetId( 1 );
    col1.SetText( _( "Default Value" ) );
    col1.SetWidth( 250 );

    wxListItem col2;
    col2.SetId( 2 );
    col2.SetText( _( "Visible" ) );
    col2.SetWidth( 100 );

    templateFieldListCtrl->InsertColumn( 0, col0 );
    templateFieldListCtrl->InsertColumn( 1, col1 );
    templateFieldListCtrl->InsertColumn( 2, col2 );

    // Invalid field selected...
    selectedField = -1;
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


void DIALOG_EESCHEMA_OPTIONS::SetGridSizes( const GRIDS& grid_sizes, int grid_id )
{
    wxASSERT( grid_sizes.size() > 0 );

    int select = wxNOT_FOUND;

    for( size_t i = 0; i < grid_sizes.size(); i++ )
    {
        wxString tmp;
        tmp.Printf( wxT( "%0.1f" ), grid_sizes[i].m_Size.x );
        m_choiceGridSize->Append( tmp );

        if( grid_sizes[i].m_Id == grid_id )
            select = (int) i;
    }

    m_choiceGridSize->SetSelection( select );
}


void DIALOG_EESCHEMA_OPTIONS::RefreshTemplateFieldView( void )
{
    // Delete all items in the template field list control and add all of the
    // current template fields
    templateFieldListCtrl->DeleteAllItems();

    for( TEMPLATE_FIELDNAMES::iterator fld = templateFields.begin(); fld != templateFields.end(); ++fld )
    {
        long itemindex = templateFieldListCtrl->InsertItem( templateFieldListCtrl->GetItemCount(), fld->m_Name );
        templateFieldListCtrl->SetItem( itemindex, 1, fld->m_Value );
        templateFieldListCtrl->SetItem( itemindex, 2, ( fld->m_Visible == true ) ? _( "Visible" ) : _( "Hidden" ) );
    }
}


void DIALOG_EESCHEMA_OPTIONS::SelectTemplateField( int item )
{
    // Only select valid items!
    if( ( item < 0 ) || ( item >= templateFieldListCtrl->GetItemCount() ) )
        return;

    // Make sure we select the new item
    ignoreSelection = true;
    templateFieldListCtrl->SetItemState( item, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
}


void DIALOG_EESCHEMA_OPTIONS::OnAddButtonClick( wxCommandEvent& event )
{
    // If there is currently a valid selection, copy the edit panel to the
    // selected field so as not to lose the data
    if( ( selectedField >= 0 ) && ( selectedField < templateFields.size() ) )
        copyPanelToSelected();

    // Add a new fieldname to the fieldname list
    TEMPLATE_FIELDNAME newFieldname = TEMPLATE_FIELDNAME( "Fieldname" );
    newFieldname.m_Value = wxT( "Value" );
    newFieldname.m_Visible = false;
    templateFields.push_back( newFieldname );

    // Select the newly added field and then copy that data to the edit panel.
    // Make sure any previously selected state is cleared and then select the
    // new field
    selectedField = templateFields.size() - 1;

    // Update the display to reflect the new data
    RefreshTemplateFieldView();
    copySelectedToPanel();

    // Make sure we select the new item
    SelectTemplateField( selectedField );

    event.Skip();
}


void DIALOG_EESCHEMA_OPTIONS::OnDeleteButtonClick( wxCommandEvent& event )
{
    // If there is currently a valid selection, delete the template field from
    // the template field list
    if( ( selectedField >= 0 ) && ( selectedField < templateFields.size() ) )
    {
        // Delete the fieldname from the fieldname list
        templateFields.erase( templateFields.begin() + selectedField );

        // If the selectedField is still not in the templateField range now,
        // make sure we stay in range and when there are no fields present
        // move to -1
        if( selectedField >= templateFields.size() )
            selectedField = templateFields.size() - 1;

        // Update the display to reflect the new data
        RefreshTemplateFieldView();
        copySelectedToPanel();

        // Make sure after the refresh that the selected item is correct
        SelectTemplateField( selectedField );
    }

    event.Skip();
}

void DIALOG_EESCHEMA_OPTIONS::copyPanelToSelected( void )
{
    if( ( selectedField < 0 ) || ( selectedField >= templateFields.size() ) )
        return;

    // Update the template field from the edit panel
    templateFields[selectedField].m_Name = fieldNameTextCtrl->GetValue();
    templateFields[selectedField].m_Value = fieldDefaultValueTextCtrl->GetValue();
    templateFields[selectedField].m_Visible = fieldVisibleCheckbox->GetValue();
}


void DIALOG_EESCHEMA_OPTIONS::copySelectedToPanel( void )
{
    if( ( selectedField < 0 ) || ( selectedField >= templateFields.size() ) )
        return;

    // Update the panel data from the selected template field
    fieldNameTextCtrl->SetValue( templateFields[selectedField].m_Name );
    fieldDefaultValueTextCtrl->SetValue( templateFields[selectedField].m_Value );
    fieldVisibleCheckbox->SetValue( templateFields[selectedField].m_Visible );
}


void DIALOG_EESCHEMA_OPTIONS::OnTemplateFieldSelected( wxListEvent& event )
{
    if( ignoreSelection )
    {
        ignoreSelection = false;
        return;
    }

    // Before getting the new field data, make sure we save the old!
    copyPanelToSelected();

    // Now update the selected field and copy the data from the field to the
    // edit panel
    selectedField = event.GetIndex();
    copySelectedToPanel();

    // Refresh the template field view - this deletes all fields and then
    // re-fills the entire data grid. It then re-selects the currently
    // selected field. This will be recursive, so disable this event while
    // we refresh the view
    RefreshTemplateFieldView();

    // If an item was selected, make sure we re-select it, or at least the
    // same position in the grid
    SelectTemplateField( selectedField );

    event.Skip();
}


void DIALOG_EESCHEMA_OPTIONS::OnTemplateFieldDeselected( wxListEvent& event )
{
    event.Skip();
}


void DIALOG_EESCHEMA_OPTIONS::SetTemplateFields( const TEMPLATE_FIELDNAMES& aFields )
{
    // Set the template fields object
    templateFields = aFields;

    // Refresh the view
    RefreshTemplateFieldView();
}


TEMPLATE_FIELDNAMES DIALOG_EESCHEMA_OPTIONS::GetTemplateFields( void )
{
    return templateFields;
}


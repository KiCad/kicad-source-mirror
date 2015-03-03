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

#include "wx/settings.h"

DIALOG_EESCHEMA_OPTIONS::DIALOG_EESCHEMA_OPTIONS( wxWindow* parent ) :
    DIALOG_EESCHEMA_OPTIONS_BASE( parent )
{
    m_choiceUnits->SetFocus();
    m_sdbSizerOK->SetDefault();

    // Dialog should not shrink beyond it's minimal size.
    GetSizer()->SetSizeHints( this );

    wxListItem col0;
    col0.SetId( 0 );
    col0.SetText( _( "Field Name" ) );

    wxListItem col1;
    col1.SetId( 1 );
    col1.SetText( _( "Default Value" ) );

    wxListItem col2;
    col2.SetId( 2 );
    col2.SetText( _( "Visible" ) );

    templateFieldListCtrl->InsertColumn( 0, col0 );
    templateFieldListCtrl->InsertColumn( 1, col1 );
    templateFieldListCtrl->InsertColumn( 2, col2 );

    templateFieldListCtrl->SetColumnWidth( 0, templateFieldListCtrl->GetSize().GetWidth() / 3.5 );
    templateFieldListCtrl->SetColumnWidth( 1, templateFieldListCtrl->GetSize().GetWidth() / 3.5 );
    templateFieldListCtrl->SetColumnWidth( 2, templateFieldListCtrl->GetSize().GetWidth() / 3.5 );

    // Invalid field selected
    selectedField = -1;

    // Make sure we select the first tab of the options tab page
    m_notebook->SetSelection( 0 );

    // Connect the edit controls for the template field names to the kill focus event which
    // doesn't propogate, hence the need to connect it here.

    fieldNameTextCtrl->Connect( wxEVT_KILL_FOCUS,
            wxFocusEventHandler( DIALOG_EESCHEMA_OPTIONS::OnEditControlKillFocus ), NULL, this );

    fieldDefaultValueTextCtrl->Connect( wxEVT_KILL_FOCUS,
            wxFocusEventHandler( DIALOG_EESCHEMA_OPTIONS::OnEditControlKillFocus ), NULL, this );
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

        if( aGridSizes[i].m_Id == aGridId )
            select = (int) i;
    }

    m_choiceGridSize->SetSelection( select );
}


void DIALOG_EESCHEMA_OPTIONS::RefreshTemplateFieldView( void )
{
    // Loop through the template fieldnames and add them to the list control
    // or just change texts if room exists
    long itemindex = 0;
    wxString tmp;

    for( TEMPLATE_FIELDNAMES::iterator fld = templateFields.begin();
            fld != templateFields.end(); ++fld, itemindex++ )
    {
        if( templateFieldListCtrl->GetItemCount() <= itemindex )
        {
            templateFieldListCtrl->InsertItem(
                templateFieldListCtrl->GetItemCount(), fld->m_Name );
        }

        wxListItem litem;
        litem.SetId( itemindex );
        templateFieldListCtrl->GetItem( litem );

        litem.SetColumn( 0 );
        if( litem.GetText() != fld->m_Name )
            templateFieldListCtrl->SetItem( itemindex, 0, fld->m_Name );

        litem.SetColumn( 1 );
        if( litem.GetText() != fld->m_Value )
            templateFieldListCtrl->SetItem( itemindex, 1, fld->m_Value );

        tmp = ( fld->m_Visible == true ) ? _( "Visible" ) : _( "Hidden" );

        litem.SetColumn( 2 );
        if(  litem.GetText() != tmp )
            templateFieldListCtrl->SetItem( itemindex, 2, tmp );
    }

    // Remove extra items:
    while( templateFieldListCtrl->GetItemCount() > itemindex )
    {
        templateFieldListCtrl->DeleteItem( itemindex );
    }

}


void DIALOG_EESCHEMA_OPTIONS::SelectTemplateField( int aItem )
{
    // Only select valid items!
    if( ( aItem < 0 ) || ( aItem >= templateFieldListCtrl->GetItemCount() ) )
        return;

    // Make sure we select the new item in list control
    if( templateFieldListCtrl->GetFirstSelected() != aItem )
        templateFieldListCtrl->Select( aItem, true );
}


void DIALOG_EESCHEMA_OPTIONS::OnAddButtonClick( wxCommandEvent& event )
{
    // If there is currently a valid selection, copy the edit panel to the
    // selected field so as not to lose the data
    if( fieldSelectionValid( selectedField ) )
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
    if( fieldSelectionValid( selectedField ) )
    {
        // Delete the fieldname from the fieldname list
        templateFields.erase( templateFields.begin() + selectedField );

        // If the selectedField is still not in the templateField range now,
        // make sure we stay in range and when there are no fields present
        // move to -1
        if( selectedField >= int( templateFields.size() ) )
            selectedField = templateFields.size() - 1;

        // Update the display to reflect the new data
        RefreshTemplateFieldView();

        copySelectedToPanel();

        // Make sure after the refresh that the selected item is correct
        SelectTemplateField( selectedField );
    }
}


void DIALOG_EESCHEMA_OPTIONS::copyPanelToSelected( void )
{
    if( !fieldSelectionValid( selectedField ) )
        return;

    // Update the template field from the edit panel
    templateFields[selectedField].m_Name = fieldNameTextCtrl->GetValue();
    templateFields[selectedField].m_Value = fieldDefaultValueTextCtrl->GetValue();
    templateFields[selectedField].m_Visible = fieldVisibleCheckbox->GetValue();
}


void DIALOG_EESCHEMA_OPTIONS::OnEditControlKillFocus( wxFocusEvent& event )
{
    // Update the data + UI
    copyPanelToSelected();
    RefreshTemplateFieldView();
    SelectTemplateField( selectedField );

    event.Skip();
}

void DIALOG_EESCHEMA_OPTIONS::OnEnterKey( wxCommandEvent& event )
{
    // Process the event produced when the user presses enter key
    // in template fieldname text control or template fieldvalue text control
    // Validate the current name or value, and switch focus to the other param
    // (value or name)
    copyPanelToSelected();
    RefreshTemplateFieldView();

    if( fieldNameTextCtrl->HasFocus() )
        fieldDefaultValueTextCtrl->SetFocus();
    else
        fieldNameTextCtrl->SetFocus();
}


void DIALOG_EESCHEMA_OPTIONS::OnVisibleFieldClick( wxCommandEvent& event )
{
    // Process the event produced when the user click on
    // the check box which controls the field visibility
    copyPanelToSelected();
    RefreshTemplateFieldView();
}

void DIALOG_EESCHEMA_OPTIONS::copySelectedToPanel( void )
{
    if( !fieldSelectionValid( selectedField ) )
        return;

    // Update the panel data from the selected template field
    fieldNameTextCtrl->SetValue( templateFields[selectedField].m_Name );
    fieldDefaultValueTextCtrl->SetValue( templateFields[selectedField].m_Value );
    fieldVisibleCheckbox->SetValue( templateFields[selectedField].m_Visible );
}


void DIALOG_EESCHEMA_OPTIONS::OnTemplateFieldSelected( wxListEvent& event )
{
    // Before getting the new field data, make sure we save the old!
    copyPanelToSelected();

    // Now update the selected field and copy the data from the field to the
    // edit panel
    selectedField = event.GetIndex();
    copySelectedToPanel();
}


void DIALOG_EESCHEMA_OPTIONS::SetTemplateFields( const TEMPLATE_FIELDNAMES& aFields )
{
    // Set the template fields object
    templateFields = aFields;

    // select the last field ( will set selectedField to -1 if no field ):
    selectedField = templateFields.size()-1;

    // Build and refresh the view
    RefreshTemplateFieldView();

    if( selectedField >= 0 )
    {
        copySelectedToPanel();
        SelectTemplateField( selectedField );
    }

}


TEMPLATE_FIELDNAMES DIALOG_EESCHEMA_OPTIONS::GetTemplateFields( void )
{
    return templateFields;
}


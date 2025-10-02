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

#include <dialog_filter_selection.h>
#include <pcb_edit_frame.h>


DIALOG_FILTER_SELECTION::DIALOG_FILTER_SELECTION( PCB_BASE_FRAME* aParent, OPTIONS& aOptions ) :
        DIALOG_FILTER_SELECTION_BASE( aParent ),
        m_options( aOptions )
{
    SetupStandardButtons();

    SetFocus();
    GetSizer()->SetSizeHints( this );
    Centre();
}


void DIALOG_FILTER_SELECTION::checkBoxClicked( wxCommandEvent& aEvent )
{
    if( m_Include_Modules->GetValue() )
        m_IncludeLockedModules->Enable();
    else
        m_IncludeLockedModules->Disable();

    // Update "All Items" checkbox based on how many items are currently checked
    m_All_Items->Set3StateValue( GetSuggestedAllItemsState() );
}


bool DIALOG_FILTER_SELECTION::TransferDataToWindow()
{
    m_Include_Modules->SetValue( m_options.includeFootprints );
    m_IncludeLockedModules->SetValue( m_options.includeLockedFootprints );

    if( m_Include_Modules->GetValue() )
        m_IncludeLockedModules->Enable();
    else
        m_IncludeLockedModules->Disable();

    m_Include_Tracks->SetValue( m_options.includeTracks );
    m_Include_Vias->SetValue( m_options.includeVias );
    m_Include_Zones->SetValue( m_options.includeZones );
    m_Include_Draw_Items->SetValue( m_options.includeItemsOnTechLayers );
    m_Include_Edges_Items->SetValue( m_options.includeBoardOutlineLayer );
    m_Include_PcbTexts->SetValue( m_options.includePcbTexts );

    // Update "All Items" checkbox based on how many items are currently checked
    m_All_Items->Set3StateValue( GetSuggestedAllItemsState() );

    return true;
}


void DIALOG_FILTER_SELECTION::forceCheckboxStates( bool aNewState )
{
    m_Include_Modules->SetValue( aNewState );
    m_IncludeLockedModules->SetValue( aNewState );

    if( aNewState ) // Make enable state match checkbox state
        m_IncludeLockedModules->Enable();
    else
        m_IncludeLockedModules->Disable();

    m_Include_Tracks->SetValue( aNewState );
    m_Include_Vias->SetValue( aNewState );
    m_Include_Zones->SetValue( aNewState );
    m_Include_Draw_Items->SetValue( aNewState );
    m_Include_Edges_Items->SetValue( aNewState );
    m_Include_PcbTexts->SetValue( aNewState );
}

wxCheckBoxState DIALOG_FILTER_SELECTION::GetSuggestedAllItemsState( void )
{
    int             numChecked = 0;
    int             numCheckboxesOnDlg = 0;
    wxCheckBoxState suggestedState = wxCHK_UNDETERMINED; // Assume some but not all are checked

    // Find out how many checkboxes are on this dialog.  We do this at runtime so future
    // changes to the dialog are easier to handle or automatic, depending on the change.
    const wxWindowList& list = this->GetChildren();

    for( wxWindowList::compatibility_iterator node = list.GetFirst(); node; node = node->GetNext() )
    {
        wxWindow* current = node->GetData();

        // If casting the child window to a checkbox isn't NULL, then the child is a checkbox
        if( wxCheckBox* currCB = dynamic_cast<wxCheckBox*>( current ) )
        {
            // Need to get count of checkboxes, but not include the "All Items" checkbox (the only
            // one that allows the 3rd state) or the hidden one (the only one with an empty label)
            // that keeps the dialog formatted properly

            if( !( currCB->GetLabelText().IsEmpty() || currCB->Is3State() ) )
                numCheckboxesOnDlg++;
        }
    }

    // Tally how many checkboxes are checked.  Only include "locked footprints" in the total
    // if "footprints" is checked.
    if( m_Include_Modules->GetValue() )
    {
        numChecked++;

        if( m_IncludeLockedModules->GetValue() )
            numChecked++;
    }
    else
    {
        // If include modules isn't checked then ignore the "Locked Footprints" checkbox in tally
        numCheckboxesOnDlg--;
    }

    for( wxCheckBox* cb : { m_Include_Tracks, m_Include_Vias, m_Include_Zones, m_Include_Draw_Items,
                            m_Include_Edges_Items, m_Include_PcbTexts } )
    {
        if( cb->GetValue() )
            numChecked++;
    }

    // Change suggestion if all or none are checked

    if( !numChecked )
        suggestedState = wxCHK_UNCHECKED;
    else if( numChecked == numCheckboxesOnDlg )
        suggestedState = wxCHK_CHECKED;

    return suggestedState;
}


void DIALOG_FILTER_SELECTION::allItemsClicked( wxCommandEvent& aEvent )
{
    if( m_All_Items->Get3StateValue() == wxCHK_CHECKED )
        forceCheckboxStates( true ); // Select all items
    else
        forceCheckboxStates( false ); // Clear all items
}


bool DIALOG_FILTER_SELECTION::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    m_options.includeFootprints        = m_Include_Modules->GetValue();
    m_options.includeLockedFootprints  = m_IncludeLockedModules->GetValue();
    m_options.includeTracks            = m_Include_Tracks->GetValue();
    m_options.includeVias              = m_Include_Vias->GetValue();
    m_options.includeZones             = m_Include_Zones->GetValue();
    m_options.includeItemsOnTechLayers = m_Include_Draw_Items->GetValue();
    m_options.includeBoardOutlineLayer = m_Include_Edges_Items->GetValue();
    m_options.includePcbTexts          = m_Include_PcbTexts->GetValue();

    return true;
}

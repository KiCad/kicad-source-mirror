/**
 * @file dialog_footprint_wizard_list.cpp
 */

#include <wx/grid.h>



#include <fctsys.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <dialog_footprint_wizard_list.h>
#include <class_footprint_wizard.h>



DIALOG_FOOTPRINT_WIZARD_LIST::DIALOG_FOOTPRINT_WIZARD_LIST( wxWindow* aParent )
    : DIALOG_FOOTPRINT_WIZARD_LIST_BASE( aParent )
{
    SetFocus();
    int n_wizards = FOOTPRINT_WIZARDS::GetSize();

    // Current wizard selection, empty or first
    m_FootprintWizard = NULL;

    if (n_wizards)
        m_FootprintWizard = FOOTPRINT_WIZARDS::GetWizard(0);

    // Choose selection mode and insert the needed rows

    m_footprintWizardsGrid->SetColSize( 0, 0 ); // hide the preview for now

    m_footprintWizardsGrid->SetSelectionMode(wxGrid::wxGridSelectRows);
    m_footprintWizardsGrid->InsertRows(0,n_wizards,true);

    // Put all wizards in the list
    for (int i=0;i<n_wizards;i++)
    {
        FOOTPRINT_WIZARD *wizard = FOOTPRINT_WIZARDS::GetWizard(i);
        wxString name = wizard->GetName();
        wxString description = wizard->GetDescription();
        wxString image = wizard->GetImage();

        m_footprintWizardsGrid->SetCellValue(i,1,name);
        m_footprintWizardsGrid->SetCellValue(i,2,description);

    }

    // Select the first row
    m_footprintWizardsGrid->ClearSelection();
    m_footprintWizardsGrid->SelectRow(0,false);

}


void DIALOG_FOOTPRINT_WIZARD_LIST::OnCellWizardClick( wxGridEvent& event )
{
    int click_row = event.GetRow();
    m_FootprintWizard = FOOTPRINT_WIZARDS::GetWizard(click_row);
    m_footprintWizardsGrid->SelectRow(event.GetRow(),false);
}

FOOTPRINT_WIZARD* DIALOG_FOOTPRINT_WIZARD_LIST::GetWizard()
{
    return m_FootprintWizard;
}

void DIALOG_FOOTPRINT_WIZARD_LIST::OnOpenButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_OK );
}

void DIALOG_FOOTPRINT_WIZARD_LIST::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}

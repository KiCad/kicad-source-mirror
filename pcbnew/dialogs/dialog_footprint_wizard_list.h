/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_footprint_wizard_list.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_FOOTPRINT_WIZARD_LIST_H_
#define _DIALOG_FOOTPRINT_WIZARD_LIST_H_

#include <dialog_footprint_wizard_list_base.h>
#include <class_footprint_wizard.h>

class DIALOG_FOOTPRINT_WIZARD_LIST: public DIALOG_FOOTPRINT_WIZARD_LIST_BASE
{
private:
    FOOTPRINT_WIZARD *m_FootprintWizard;

public:
    DIALOG_FOOTPRINT_WIZARD_LIST(wxWindow * parent );

    FOOTPRINT_WIZARD* GetWizard();

private:
    void OnCellWizardClick( wxGridEvent& event );
    void OnOpenButtonClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
};

#endif  // _DIALOG_FOOTPRINT_WIZARD_LIST_H_

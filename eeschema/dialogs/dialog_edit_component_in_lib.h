/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_edit_component_in_lib.h
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////


#ifndef _DIALOG_EDIT_COMPONENT_IN_LIB_H_
#define _DIALOG_EDIT_COMPONENT_IN_LIB_H_

#include "dialog_edit_component_in_lib_base.h"


class DIALOG_EDIT_COMPONENT_IN_LIBRARY: public DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE
{
public:
	LIB_EDIT_FRAME* m_Parent;
	bool m_RecreateToolbar;
	int m_AliasLocation;

public:
    /// Constructors
    DIALOG_EDIT_COMPONENT_IN_LIBRARY( LIB_EDIT_FRAME* parent);
    ~DIALOG_EDIT_COMPONENT_IN_LIBRARY();

private:
    void initDlg();
    void InitPanelDoc();
    void InitBasicPanel();
	void OnCancelClick( wxCommandEvent& event );
	void OnOkClick(wxCommandEvent& event);
	void DeleteAllAliasOfPart(wxCommandEvent& event);
	void DeleteAliasOfPart(wxCommandEvent& event);
	void AddAliasOfPart(wxCommandEvent& event);
	bool ChangeNbUnitsPerPackage(int newUnit);
	bool SetUnsetConvert();
	void CopyDocToAlias(wxCommandEvent& event);
	void BrowseAndSelectDocFile(wxCommandEvent& event);

	void DeleteAllFootprintFilter(wxCommandEvent& event);
	void DeleteOneFootprintFilter(wxCommandEvent& event);
	void AddFootprintFilter(wxCommandEvent& event);

};

#endif
    // _DIALOG_EDIT_COMPONENT_IN_LIB_H_

/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_build_BOM.h
// Copyright:   GNU license
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_BUILD_BOM_H_
#define _DIALOG_BUILD_BOM_H_

#include "dialog_build_BOM_base.h"

class DIALOG_BUILD_BOM : public DIALOG_BUILD_BOM_BASE 
{
private:
    WinEDA_DrawFrame * m_Parent;
    wxConfig* m_Config;
    wxString m_ListFileName;

private:
    void OnRadioboxSelectFormatSelected( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    void SavePreferences();
    void Init( );
    void Create_BOM_Lists(int  aTypeFile,
                          bool aIncludeSubComponents,
                          char aExportSeparatorSymbol,
                          bool aRunBrowser);
    void GenereListeOfItems(const wxString & FullFileName, bool aIncludeSubComponents );
    void CreateExportList(const wxString & FullFileName, bool aIncludeSubComponents);
    void CreatePartsList(const wxString & FullFileName);
    int PrintComponentsListByRef( FILE * f, std::vector <OBJ_CMP_TO_LIST>& aList,
                            bool CompactForm, bool aIncludeSubComponents );
    int PrintComponentsListByVal( FILE *f, std::vector <OBJ_CMP_TO_LIST>& aList,
                            bool aIncludeSubComponents);
    int PrintComponentsListByPart( FILE *f, std::vector <OBJ_CMP_TO_LIST>& aList);
    void PrintFieldData(FILE * f, SCH_COMPONENT * DrawLibItem, bool CompactForm = FALSE);


public:
    DIALOG_BUILD_BOM( WinEDA_DrawFrame* parent );
    ~DIALOG_BUILD_BOM() {};
	
};


#endif
    // _DIALOG_BUILD_BOM_H_

/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_build_BOM.h
// Copyright:   GNU license
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_BUILD_BOM_H_
#define _DIALOG_BUILD_BOM_H_

#include "dialog_build_BOM_base.h"


class EDA_DRAW_FRAME;
class SCH_COMPONENT;
class wxConfig;


class DIALOG_BUILD_BOM : public DIALOG_BUILD_BOM_BASE
{
private:
    EDA_DRAW_FRAME* m_Parent;
    wxConfig*       m_Config;
    wxString        m_ListFileName;

private:
    void    OnRadioboxSelectFormatSelected( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );

    void    SavePreferences();
    void    Init();
    void    Create_BOM_Lists( int   aTypeFile,
                              bool  aIncludeSubComponents,
                              char  aExportSeparatorSymbol,
                              bool  aRunBrowser );

    void    GenereListeOfItems( const wxString& FullFileName, bool aIncludeSubComponents );
    void    CreateExportList( const wxString& FullFileName, bool aIncludeSubComponents );

    /**
     * Function CreateParstList
     * prints a list of components, in a form which can be imported by a
     * spreadsheet.  Form is:
     *  cmp value; number of components; <footprint>; <field1>; ...;
     *  list of references having the same value
     */
    void    CreatePartsList( const wxString& aFullFileName, bool aIncludeSubComponents );

    int     PrintComponentsListByRef( FILE* f, SCH_REFERENCE_LIST& aList,
                                      bool CompactForm, bool aIncludeSubComponents );

    int     PrintComponentsListByVal( FILE* f, SCH_REFERENCE_LIST& aList,
                                      bool aIncludeSubComponents );

    int     PrintComponentsListByPart( FILE* f, SCH_REFERENCE_LIST& aList,
                                       bool aIncludeSubComponents );

    void    PrintFieldData( FILE* f, SCH_COMPONENT* DrawLibItem, bool CompactForm = FALSE );

    bool    IsFieldChecked( int aFieldId );

public:
    DIALOG_BUILD_BOM( EDA_DRAW_FRAME* parent );

    // ~DIALOG_BUILD_BOM() {};
};

#endif  // _DIALOG_BUILD_BOM_H_

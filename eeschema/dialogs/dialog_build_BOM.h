/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_build_BOM.h
// Copyright:   GNU license
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_BUILD_BOM_H_
#define _DIALOG_BUILD_BOM_H_

#include <dialog_build_BOM_base.h>


class EDA_DRAW_FRAME;
class SCH_COMPONENT;
class wxConfig;


class DIALOG_BUILD_BOM : public DIALOG_BUILD_BOM_BASE
{
private:
    EDA_DRAW_FRAME* m_Parent;
    wxConfig*       m_Config;
    wxString        m_ListFileName;     // The full filename of the file report.

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

    void    GenereListeOfItems( bool aIncludeSubComponents );

    /**
     * Function CreateExportList
     * prints a list of components, in a form which can be imported by a
     * spreadsheet.  Form is:
     *  reference; cmp value; \<footprint\>; \<field1\>; ...;
     * Components are sorted by reference
     */
    void    CreateExportList( bool aIncludeSubComponents );

    /**
     * Function CreatePartsList
     * prints a list of components, in a form which can be imported by a spreadsheet.
     * components having the same value and the same footprint
     * are grouped on the same line
     * Form is:
     *  value; number of components; list of references; \<footprint\>; \<field1\>; ...;
     * list is sorted by values
     */
    void    CreatePartsList();

    int     PrintComponentsListByRef( FILE* f, SCH_REFERENCE_LIST& aList,
                                      bool CompactForm, bool aIncludeSubComponents );

    int     PrintComponentsListByVal( FILE* f, SCH_REFERENCE_LIST& aList,
                                      bool aIncludeSubComponents );

    int     PrintComponentsListByPart( FILE* f, SCH_REFERENCE_LIST& aList,
                                       bool aIncludeSubComponents );

    wxString PrintFieldData( SCH_COMPONENT* DrawLibItem, bool CompactForm = false );

    bool    IsFieldChecked( int aFieldId );

public:
    DIALOG_BUILD_BOM( EDA_DRAW_FRAME* parent );

    // ~DIALOG_BUILD_BOM() {};
};

#endif  // _DIALOG_BUILD_BOM_H_

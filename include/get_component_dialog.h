/**
 * This file is part of the common libary.
 * @file  get_component_dialog.h
 * @see   common.h
 */

#ifndef __INCLUDE__GET_COMPONENT_DIALOG_H__
#define __INCLUDE__GET_COMPONENT_DIALOG_H__ 1


wxPoint  GetComponentDialogPosition( void );

void     AddHistoryComponentName( wxArrayString& HistoryList,
                                  const wxString& Name );

/* Add the string "Name" to the history list */

enum selcmp_id {
    ID_ACCEPT_NAME = 3900,
    ID_ACCEPT_KEYWORD,
    ID_LIST_ALL,
    ID_EXTRA_TOOL,
    ID_SEL_BY_LISTBOX
};

/* Dialog frame to choose a component name */
class WinEDA_SelectCmp : public wxDialog
{
private:
    bool        m_AuxTool;
    wxString    m_Text;
    wxTextCtrl* m_TextCtrl;
    wxListBox*  m_List;

public:
    bool        m_GetExtraFunction;

public:
    // Constructor and destructor
    WinEDA_SelectCmp( WinEDA_DrawFrame* parent, const wxPoint& framepos,
                      wxArrayString& HistoryList, const wxString& Title,
                      bool show_extra_tool );
    ~WinEDA_SelectCmp() {};

    wxString GetComponentName( void );
    void     SetComponentName( const wxString& name );

private:
    void     InitDialog( wxArrayString& aHistoryList );
    void     Accept( wxCommandEvent& event );
    void     GetExtraSelection( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


#endif /* __INCLUDE__GET_COMPONENT_DIALOG_H__ */

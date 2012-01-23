/**
 * This file is part of the common libary.
 * @file  dialog_get_component.h
 */

#ifndef __INCLUDE_DIALOG_GET_COMPONENT_H__
#define __INCLUDE_DIALOG_GET_COMPONENT_H__

#include <../common/dialogs/dialog_get_component_base.h>

wxPoint  GetComponentDialogPosition( void );

void     AddHistoryComponentName( wxArrayString& HistoryList,
                                  const wxString& Name );

/* Dialog frame to choose a component name */
class DIALOG_GET_COMPONENT : public DIALOG_GET_COMPONENT_BASE
{
private:
    bool        m_auxToolSelector;
    wxString    m_Text;

public:
    bool        m_GetExtraFunction;

public:
    // Constructor and destructor
    DIALOG_GET_COMPONENT( EDA_DRAW_FRAME* parent, const wxPoint& framepos,
                          wxArrayString& HistoryList, const wxString& Title,
                          bool show_extra_tool );
    ~DIALOG_GET_COMPONENT() {};

    wxString GetComponentName( void );
    void     SetComponentName( const wxString& name );

private:
    void     initDialog( wxArrayString& aHistoryList );
    void     OnCancel( wxCommandEvent& event );
    void     Accept( wxCommandEvent& event );
    void     GetExtraSelection( wxCommandEvent& event );
};


#endif /* __INCLUDE_DIALOG_GET_COMPONENT_H__ */

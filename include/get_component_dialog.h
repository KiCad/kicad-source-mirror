/**
 * This file is part of the common libary.
 * @file  get_component_dialog.h
 * @see   common.h
 */

#ifndef __INCLUDE__GET_COMPONENT_DIALOG_H__
#define __INCLUDE__GET_COMPONENT_DIALOG_H__ 1


wxString GetComponentName( WinEDA_DrawFrame * frame,
                          wxArrayString & HistoryList, const wxString &Title,
                          wxString (*AuxTool)( WinEDA_DrawFrame * parent ) );

/* Dialog frame to choose a component name */
void        AddHistoryComponentName( wxArrayString& HistoryList, const wxString& Name );

/* Add the string "Name" to the history list */


#endif /* __INCLUDE__GET_COMPONENT_DIALOG_H__ */


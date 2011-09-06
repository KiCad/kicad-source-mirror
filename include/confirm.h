/**
 * This file is part of the common libary
 * @file  confirm.h
 * @see   common.h
 */


#ifndef __INCLUDE__CONFIRM_H__
#define __INCLUDE__CONFIRM_H__ 1


void    DisplayError( wxWindow* parent, const wxString& msg,
                      int displaytime = 0 );
void    DisplayInfoMessage( wxWindow* parent, const wxString& msg,
                            int displaytime = 0 );

bool    IsOK( wxWindow* parent, const wxString& msg );

void    DisplayHtmlInfoMessage( wxWindow* parent, const wxString& title,
                                const wxString& msg,
                                const wxSize& size=wxDefaultSize );


#endif /* __INCLUDE__CONFIRM_H__ */

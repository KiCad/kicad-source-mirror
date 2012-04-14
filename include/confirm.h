/**
 * This file is part of the common library
 * @file  confirm.h
 * @see   common.h
 */


#ifndef __INCLUDE__CONFIRM_H__
#define __INCLUDE__CONFIRM_H__ 1

/**
 * Function DisplayExitDialog
 * displays a dialog with 3 buttons:
 * Save and Exit
 * Cancel
 * Exit without save
 *
 * @param aParent = the parent window
 * @param aMessage = the main message to put in dialog
 * If empty, the standard message will be shown:
 * Save the changes before closing?
 * @return wxID_YES, wxID_CANCEL, wxID_NO.
 */
int DisplayExitDialog( wxWindow* aParent, const wxString& aMessage );


/**
 * Function DisplayError
 * displays an error or warning message box with \a aMessage.
 *
 * @warning Setting \a displaytime does not work.  Do not use it.
 */
void DisplayError( wxWindow* parent, const wxString& aMessage, int displaytime = 0 );

/**
 * Function DisplayInfoMessage
 * displays an informational message box with \a aMessage.
 *
 * @warning Setting \a displaytime does not work.  Do not use it.
 */
void DisplayInfoMessage( wxWindow* parent, const wxString& aMessage, int displaytime = 0 );

/**
 * Function IsOK
 * gets the user response to \a aMessage.
 *
 * @return True if user selected the yes button, otherwise false.
 */
bool IsOK( wxWindow* parent, const wxString& aMessage );

/**
 * Function DisplayHtmlInforMessage
 * displays \a aMessage in HTML format.
 */
void DisplayHtmlInfoMessage( wxWindow* parent, const wxString& title,
                             const wxString& aMessage,
                             const wxSize& size = wxDefaultSize );


#endif /* __INCLUDE__CONFIRM_H__ */

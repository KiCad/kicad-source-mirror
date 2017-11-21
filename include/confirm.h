/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * This file is part of the common library
 * @file  confirm.h
 * @see   common.h
 */

#ifndef __INCLUDE__CONFIRM_H__
#define __INCLUDE__CONFIRM_H__

#include <wx/window.h>
#include <vector>

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
 * Function DisplayErrorMessage
 * displays an error message with \a aMessage
 *
 * @param aParent is the parent window
 * @param aMessage is the message text to display
 * @param aExtraInfo is extra data that can be optionally displayed in a collapsible pane
 */
void DisplayErrorMessage( wxWindow* aParent, const wxString& aMessage, const wxString aExtraInfo = wxEmptyString );


/**
 * Function DisplayInfoMessage
 * displays an informational message box with \a aMessage.
 *
 * @param aParent is the parent window
 * @param aMessage is the message text to display
 * @param aExtraInfo is the extra data that can be optionally displayed in a collapsible pane
 */
void DisplayInfoMessage( wxWindow* parent, const wxString& aMessage, const wxString aExtraInfo = wxEmptyString );

/**
 * Function IsOK
 * displays a yes/no dialog with \a aMessage and returns the user response.
 *
 * @param aParent is the parent window.  NULL can be used if the parent is the top level window.
 * @param aMessage is the message to display in the dialog box.
 *
 * @return True if user selected the yes button, otherwise false.
 */
bool IsOK( wxWindow* aParent, const wxString& aMessage );

/**
 * Function YesNoCancelDialog
 * displays a yes/no/cancel dialog with \a aMessage and returns the user response.
 *
 * @param aParent is the parent window.  NULL can be used if the parent is the top level window.
 * @param aPrimaryMessage is the message to display in the top part of the dialog box using
 *                        a bold font.
 * @param aSecondaryMessage is the message to display in the lower part of the dialog box
 *                          using the default system UI font.
 * @param aYesButtonText is the text to display in the yes button when defined.
 * @param aNoButtonText is the text to display in the no button when defiend.
 * @param aCancelButtonText is the text to display in the cancel button when defined.
 *
 * @return wxID_YES, wxID_NO, or wxID_CANCEL depending on the button the user selected.
 */
int YesNoCancelDialog( wxWindow*       aParent,
                       const wxString& aPrimaryMessage,
                       const wxString& aSecondaryMessage,
                       const wxString& aYesButtonText = wxEmptyString,
                       const wxString& aNoButtonText = wxEmptyString,
                       const wxString& aCancelButtonText = wxEmptyString );


/**
 * Function DisplayHtmlInforMessage
 * displays \a aMessage in HTML format.
 */
void DisplayHtmlInfoMessage( wxWindow* parent, const wxString& title,
                             const wxString& aMessage,
                             const wxSize& size = wxDefaultSize );


/**
 * Displays a dialog with radioboxes asking the user to select an option.
 * @param aParent is the parent window.
 * @param aTitle is the dialog title.
 * @param aMessage is a text label displayed in the first row of the dialog.
 * @param aOptions is a vector of possible options.
 * @return Index of the selected option or -1 when the dialog has been cancelled.
 */
int SelectSingleOption( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage,
        const wxArrayString& aOptions );

/**
 * Displays a dialog with checkboxes asking the user to select one or more options.
 * @param aParent is the parent window.
 * @param aTitle is the dialog title.
 * @param aMessage is a text label displayed in the first row of the dialog.
 * @param aOptions is a vector of possible options.
 * @param aDefaultState is the default state for the checkboxes.
 * @return Vector containing indices of the selected option. 
 */
std::pair<bool, std::vector<int>> SelectMultipleOptions( wxWindow* aParent, const wxString& aTitle,
        const wxString& aMessage, const wxArrayString& aOptions, bool aDefaultState = false );

std::pair<bool, std::vector<int>> SelectMultipleOptions( wxWindow* aParent, const wxString& aTitle,
        const wxString& aMessage, const std::vector<std::string>& aOptions, bool aDefaultState = false );

#endif /* __INCLUDE__CONFIRM_H__ */

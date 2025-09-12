/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * This file is part of the common library.
 *
 * @file  confirm.h
 * @see   common.h
 */

#ifndef __INCLUDE__CONFIRM_H__
#define __INCLUDE__CONFIRM_H__

#include <kicommon.h>
#include <wx/string.h>
#include <wx/arrstr.h>

class wxWindow;


#if defined( _WIN32 ) && wxCHECK_VERSION( 3, 3, 0 )
#define KICAD_MESSAGE_DIALOG_BASE wxGenericMessageDialog
#define KICAD_RICH_MESSAGE_DIALOG_BASE wxGenericRichMessageDialog
#else
#define KICAD_MESSAGE_DIALOG_BASE wxMessageDialog
#define KICAD_RICH_MESSAGE_DIALOG_BASE wxRichMessageDialog
#endif


/**
 * Display a dialog indicating the file is already open, with an option to reset the lock.
 * @return true if the lock was reset.
 */
KICOMMON_API bool AskOverrideLock( wxWindow* aParent, const wxString& aMessage );


/**
 * Display a dialog with Save, Cancel and Discard Changes buttons.
 *
 * @param aParent = the parent window
 * @param aMessage = the main message to put in dialog
 * @param aSaveFunction = a function to save changes, if requested.  Must return true if
 *                        the save was successful and false otherwise (which will result
 *                        in HandleUnsavedChanges() returning wxID_CANCEL).
 * @return wxID_YES, wxID_CANCEL, wxID_NO.
 */
KICOMMON_API bool HandleUnsavedChanges( wxWindow* aParent, const wxString& aMessage,
                                        const std::function<bool()>& aSaveFunction );

/** Return the result code from the last call to HandleUnsavedChanges(): wxID_YES, wxID_NO or
 *  wxID_CANCEL (or -1 if none yet). */
KICOMMON_API int GetLastUnsavedChangesResponse();


/**
 * A specialized version of HandleUnsavedChanges which handles an apply-to-all checkbox.
 *
 * @param aParent = the parent window
 * @param aMessage = the main message to put in dialog
 * @param aApplyToAll = if non-null an "Apply to all" checkbox will be shown and it's value
 *                      written back to the bool.
 * @return wxID_YES, wxID_CANCEL, wxID_NO.
 */
KICOMMON_API int UnsavedChangesDialog( wxWindow* aParent, const wxString& aMessage,
                                       bool* aApplyToAll );

KICOMMON_API int UnsavedChangesDialog( wxWindow* aParent, const wxString& aMessage );


/**
 * Display a confirmation dialog for a revert action.
 */
KICOMMON_API bool ConfirmRevertDialog( wxWindow* parent, const wxString& aMessage );


/**
 * Display an error or warning message box with \a aMessage.
 */
KICOMMON_API void DisplayError( wxWindow* aParent, const wxString& aText );

/**
 * Display an error message with \a aMessage
 *
 * @param aParent is the parent window
 * @param aMessage is the message text to display
 * @param aExtraInfo is extra data that can be optionally displayed in a collapsible pane
 */
KICOMMON_API void DisplayErrorMessage( wxWindow* aParent, const wxString& aMessage,
                                       const wxString& aExtraInfo = wxEmptyString );


/**
 * Display an informational message box with \a aMessage.
 *
 * @param aParent is the parent window
 * @param aMessage is the message text to display
 * @param aExtraInfo is the extra data that can be optionally displayed in a collapsible pane
 */
KICOMMON_API void DisplayInfoMessage( wxWindow* parent, const wxString& aMessage,
                                      const wxString& aExtraInfo = wxEmptyString );

/**
 * Display a yes/no dialog with \a aMessage and returns the user response.
 *
 * @param aParent is the parent window.  NULL can be used if the parent is the top level window.
 * @param aMessage is the message to display in the dialog box.
 *
 * @return True if user selected the yes button, otherwise false.
 */
KICOMMON_API bool IsOK( wxWindow* aParent, const wxString& aMessage );

/**
 * Display a warning dialog with \a aMessage and returns the user response.
 *
 * @param aParent is the parent window.  NULL can be used if the parent is the top level window.
 * @param aWarning is the warning to display in the top part of the dialog box using a bold font.
 * @param aMessage is the message to display in the lower part of the dialog box using the
 *                 default system UI font.
 * @param aDetailedMessage is the message to display in the "Show detailed information" section.
 *                         Passing wxEmptyString will hide this portion of the dialog.
 * @param aOKLabel is the text to display in the OK button.
 * @param aCancelLabel is the text to display in the cancel button.
 *
 * @return wxID_OK or wxID_CANCEL depending on the button the user selected.
 */
KICOMMON_API int OKOrCancelDialog( wxWindow* aParent, const wxString& aWarning,
                                   const wxString& aMessage,
                                   const wxString& aDetailedMessage = wxEmptyString,
                                   const wxString& aOKLabel = wxEmptyString,
                                   const wxString& aCancelLabel = wxEmptyString,
                                   bool* aApplyToAll = nullptr );



/**
 * Display a dialog with radioboxes asking the user to select an option.
 *
 * @param aParent is the parent window.
 * @param aTitle is the dialog title.
 * @param aMessage is a text label displayed in the first row of the dialog.
 * @param aOptions is a vector of possible options.
 * @return Index of the selected option or -1 when the dialog has been canceled.
 */
KICOMMON_API int SelectSingleOption( wxWindow* aParent, const wxString& aTitle,
                                     const wxString& aMessage,
                                     const wxArrayString& aOptions );

#endif /* __INCLUDE__CONFIRM_H__ */

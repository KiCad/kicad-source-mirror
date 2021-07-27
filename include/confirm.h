/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/richmsgdlg.h>
#include <vector>
#include <functional>

class wxCheckBox;
class wxStaticBitmap;

/**
 * Helper class to create more flexible dialogs, including 'do not show again' checkbox handling.
 */
class KIDIALOG : public wxRichMessageDialog
{
public:
    ///< Dialog type. Selects appropriate icon and default dialog title
    enum KD_TYPE { KD_NONE, KD_INFO, KD_QUESTION, KD_WARNING, KD_ERROR };

    KIDIALOG( wxWindow* aParent, const wxString& aMessage, const wxString& aCaption,
              long aStyle = wxOK );
    KIDIALOG( wxWindow* aParent, const wxString& aMessage, KD_TYPE aType,
              const wxString& aCaption = "" );

    bool SetOKCancelLabels( const ButtonLabel& ok, const ButtonLabel& cancel ) override
    {
        m_cancelMeansCancel = false;
        return wxRichMessageDialog::SetOKCancelLabels( ok, cancel );
    }

    ///< Shows the 'do not show again' checkbox
    void DoNotShowCheckbox( wxString file, int line );

    ///< Checks the 'do not show again' setting for the dialog
    bool DoNotShowAgain() const;
    void ForceShowAgain();

    bool Show( bool aShow = true ) override;
    int ShowModal() override;

protected:
    // Helper functions for wxRichMessageDialog constructor
    static wxString getCaption( KD_TYPE aType, const wxString& aCaption );
    static long getStyle( KD_TYPE aType );

protected:
    unsigned long m_hash;               // Unique id
    bool          m_cancelMeansCancel;  // If the Cancel button is renamed then it should be
                                        // saved by the DoNotShowAgain checkbox.  If it's really
                                        // a cancel then it should not.
};


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
bool HandleUnsavedChanges( wxWindow* aParent, const wxString& aMessage,
                           const std::function<bool()>& aSaveFunction );


/**
 * A specialized version of HandleUnsavedChanges which handles an apply-to-all checkbox.
 *
 * @param aParent = the parent window
 * @param aMessage = the main message to put in dialog
 * @param aApplyToAll = if non-null an "Apply to all" checkbox will be shown and it's value
 *                      written back to the bool.
 * @return wxID_YES, wxID_CANCEL, wxID_NO.
 */
int UnsavedChangesDialog( wxWindow* aParent, const wxString& aMessage, bool* aApplyToAll );

int UnsavedChangesDialog( wxWindow* aParent, const wxString& aMessage );


/**
 * Display a confirmation dialog for a revert action.
 */
bool ConfirmRevertDialog( wxWindow* parent, const wxString& aMessage );


/**
 * Display an error or warning message box with \a aMessage.
 *
 * @warning Setting \a displaytime does not work.  Do not use it.
 */
void DisplayError( wxWindow* aParent, const wxString& aText, int aDisplayTime = 0 );

/**
 * Display an error message with \a aMessage
 *
 * @param aParent is the parent window
 * @param aMessage is the message text to display
 * @param aExtraInfo is extra data that can be optionally displayed in a collapsible pane
 */
void DisplayErrorMessage( wxWindow* aParent, const wxString& aMessage,
                          const wxString& aExtraInfo = wxEmptyString );


/**
 * Display an informational message box with \a aMessage.
 *
 * @param aParent is the parent window
 * @param aMessage is the message text to display
 * @param aExtraInfo is the extra data that can be optionally displayed in a collapsible pane
 */
void DisplayInfoMessage( wxWindow* parent, const wxString& aMessage,
                         const wxString& aExtraInfo = wxEmptyString );

/**
 * Display a yes/no dialog with \a aMessage and returns the user response.
 *
 * @param aParent is the parent window.  NULL can be used if the parent is the top level window.
 * @param aMessage is the message to display in the dialog box.
 *
 * @return True if user selected the yes button, otherwise false.
 */
bool IsOK( wxWindow* aParent, const wxString& aMessage );

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
int OKOrCancelDialog( wxWindow* aParent, const wxString& aWarning, const wxString& aMessage,
                      const wxString& aDetailedMessage = wxEmptyString,
                      const wxString& aOKLabel = wxEmptyString,
                      const wxString& aCancelLabel = wxEmptyString, bool* aApplyToAll = nullptr );



/**
 * Display a dialog with radioboxes asking the user to select an option.
 *
 * @param aParent is the parent window.
 * @param aTitle is the dialog title.
 * @param aMessage is a text label displayed in the first row of the dialog.
 * @param aOptions is a vector of possible options.
 * @return Index of the selected option or -1 when the dialog has been canceled.
 */
int SelectSingleOption( wxWindow* aParent, const wxString& aTitle, const wxString& aMessage,
                        const wxArrayString& aOptions );

#endif /* __INCLUDE__CONFIRM_H__ */

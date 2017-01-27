/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2016 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef DIALOG_SHIM_
#define DIALOG_SHIM_

#include <wx/dialog.h>
#include <hashtables.h>
#include <kiway_player.h>

#ifdef  __WXMAC__
/**
 * MACOS requires this option to be set to 1 in order to set dialogs focus.
 **/
#define DLGSHIM_USE_SETFOCUS      1
#else
#define DLGSHIM_USE_SETFOCUS      0
#endif

class WDO_ENABLE_DISABLE;
class EVENT_LOOP;

// These macros are for DIALOG_SHIM only, NOT for KIWAY_PLAYER.  KIWAY_PLAYER
// has its own support for quasi modal and its platform specific issues are different
// than for a wxDialog.
 #define SHOWQUASIMODAL     ShowQuasiModal
 #define ENDQUASIMODAL      EndQuasiModal


/**
 * Class DIALOG_SHIM
 * may sit in the inheritance tree between wxDialog and any class written by
 * wxFormBuilder.  To put it there, use wxFormBuilder tool and set:
 * <br> subclass name = DIALOG_SHIM
 * <br> subclass header = dialog_shim.h
 * <br>
 * in the dialog window's properties.
 **/
class DIALOG_SHIM : public wxDialog, public KIWAY_HOLDER
{
    /**
     * Function OnCloseWindow
     *
     * properly handles the wxCloseEvent when in the quasimodal mode when not calling
     * EndQuasiModal which is possible with any dialog derived from #DIALOG_SHIM.
     */
    void OnCloseWindow( wxCloseEvent& aEvent );

    /**
     * Function OnCloseWindow
     *
     * properly handles the default button events when in the quasimodal mode when not
     * calling EndQuasiModal which is possible with any dialog derived from #DIALOG_SHIM.
     */
    void OnButton( wxCommandEvent& aEvent );

public:

    DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
            const   wxPoint& pos = wxDefaultPosition,
            const   wxSize&  size = wxDefaultSize,
            long    style = wxDEFAULT_DIALOG_STYLE,
            const   wxString& name = wxDialogNameStr
            );

    ~DIALOG_SHIM();

    int ShowQuasiModal();      // disable only the parent window, otherwise modal.

    void EndQuasiModal( int retCode );  // End quasi-modal mode

    bool IsQuasiModal()         { return m_qmodal_showing; }

    bool Show( bool show ) override;

    bool Enable( bool enable ) override;

protected:

    /**
     * In all dialogs, we must call the same functions to fix minimal
     * dlg size, the default position and perhaps some others to fix a few issues
     * depending on Windows Managers
     * this helper function does these calls.
     *
     * FinishDialogSettings must be called from derived classes,
     * when all widgets are initialized, and therefore their size fixed.
     * If TransferDataToWindow() is used to initialize widgets, at end of TransferDataToWindow,
     * or better at end of a wxInitDialogEvent handler
     *
     * In any case, the best way is to call it in a wxInitDialogEvent handler
     * after calling TransfertDataToWindow(), which is the default
     * wxInitDialogEvent handler wxDialog
     */
    void FinishDialogSettings();

    /** A ugly hack to fix an issue on OSX:
     * when typing ctrl+c in a wxTextCtrl inside a dialog, it is closed instead of
     * copying a text if a button with wxID_CANCEL is used in a wxStdDialogButtonSizer,
     * when the dlg is created by wxFormBuilder:
     * the label is &Cancel, and this accelerator key has priority
     * to copy text standard accelerator, and the dlg is closed when trying to copy text
     * this function do nothing on other platforms
     */
    void FixOSXCancelButtonIssue();

    std::string m_hash_key;     // alternate for class_map when classname re-used.

    // variables for quasi-modal behavior support, only used by a few derivatives.
    EVENT_LOOP*         m_qmodal_loop;      // points to nested event_loop, NULL means not qmodal and dismissed
    bool                m_qmodal_showing;
    WDO_ENABLE_DISABLE* m_qmodal_parent_disabler;

#if DLGSHIM_USE_SETFOCUS
private:
    void    onInit( wxInitDialogEvent& aEvent );
#endif
};

#endif  // DIALOG_SHIM_

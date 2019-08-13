/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
class wxGridEvent;



struct WINDOW_THAWER
{
    WINDOW_THAWER( wxWindow* aWindow )
    {
        m_window = aWindow;
        m_freezeCount = 0;

        while( m_window->IsFrozen() )
        {
            m_window->Thaw();
            m_freezeCount++;
        }
    }

    ~WINDOW_THAWER()
    {
        while( m_freezeCount > 0 )
        {
            m_window->Freeze();
            m_freezeCount--;
        }
    }

protected:
    wxWindow* m_window;
    int       m_freezeCount;
};


class WDO_ENABLE_DISABLE;
class WX_EVENT_LOOP;

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

    void OnCharHook( wxKeyEvent& aEvt );

public:

    DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
            const   wxPoint& pos = wxDefaultPosition,
            const   wxSize&  size = wxDefaultSize,
            long    style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER,
            const   wxString& name = wxDialogNameStr
            );

    ~DIALOG_SHIM();

    /**
     * Sets the window (usually a wxTextCtrl) that should be focused when the dialog is
     * shown.
     */
    void SetInitialFocus( wxWindow* aWindow )
    {
        m_initialFocusTarget = aWindow;
    }

    int ShowQuasiModal();      // disable only the parent window, otherwise modal.

    void EndQuasiModal( int retCode );  // End quasi-modal mode

    bool IsQuasiModal()         { return m_qmodal_showing; }

    bool Show( bool show ) override;

    bool Enable( bool enable ) override;

    void OnPaint( wxPaintEvent &event );

    EDA_UNITS_T GetUserUnits() const { return m_units; }

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

    /**
     * Set the dialog to the given dimensions in "dialog units". These are units equivalent
     * to 4* the average character width and 8* the average character height, allowing a dialog
     * to be sized in a way that scales it with the system font.
     */
    void SetSizeInDU( int x, int y );

    /**
     * Convert an integer number of dialog units to pixels, horizontally. See SetSizeInDU or
     * wxDialog documentation for more information.
     */
    int HorizPixelsFromDU( int x );

    /**
     * Convert an integer number of dialog units to pixels, vertically. See SetSizeInDU or
     * wxDialog documentation for more information.
     */
    int VertPixelsFromDU( int y );

    EDA_UNITS_T         m_units;        // userUnits for display and parsing
    std::string         m_hash_key;     // alternate for class_map when classname re-used

    // On MacOS (at least) SetFocus() calls made in the constructor will fail because a
    // window that isn't yet visible will return false to AcceptsFocus().  So we must delay
    // the initial-focus SetFocus() call to the first paint event.
    bool                m_firstPaintEvent;
    wxWindow*           m_initialFocusTarget;

    // variables for quasi-modal behavior support, only used by a few derivatives.
    WX_EVENT_LOOP*      m_qmodal_loop;      // points to nested event_loop, NULL means not qmodal and dismissed
    bool                m_qmodal_showing;
    WDO_ENABLE_DISABLE* m_qmodal_parent_disabler;

private:
    void OnGridEditorShown( wxGridEvent& event );
    void OnGridEditorHidden( wxGridEvent& event );

    DECLARE_EVENT_TABLE()
};

#endif  // DIALOG_SHIM_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef DIALOG_SHIM_
#define DIALOG_SHIM_

#include <kicommon.h>
#include <eda_units.h>
#include <kiway_holder.h>
#include <wx/dialog.h>
#include <map>
#include <vector>
#include <wx/variant.h>
#include <core/raii.h>

class EDA_BASE_FRAME;
class UNIT_BINDER;

class wxGridEvent;
class wxGUIEventLoop;
class wxInitDialogEvent;
class wxSpinEvent;
class wxSpinDoubleEvent;
class wxStyledTextEvent;
class wxPropertyGridEvent;
class wxDataViewEvent;


/**
 * Dialog helper object to sit in the inheritance tree between wxDialog and any class written
 * by wxFormBuilder.
 *
 * In addition to common dialog utilities, DIALOG_SHIM will persist the state of
 * all child controls and the dialog geometry in the current project's local
 * settings.  The dialog's type name is used as the default key, but dialogs may
 * override @ref m_hash_key to store settings under a custom name.  Any control
 * can opt out of persistence by setting the boolean window property "persist" to
 * false.  Geometry is stored under the special key "__geometry".
 *
 * To use it in wxFormBuilder set:
 * <br> subclass name = DIALOG_SHIM
 * <br> subclass header = dialog_shim.h
 * <br>
 * in the dialog window's properties.
 */
class KICOMMON_API DIALOG_SHIM : public wxDialog, public KIWAY_HOLDER
{
public:
    DIALOG_SHIM( wxWindow* aParent, wxWindowID id, const wxString& title,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxDEFAULT_FRAME_STYLE | wxRESIZE_BORDER,
                 const wxString& name = wxDialogNameStr );

    ~DIALOG_SHIM();

    /**
     * Sets the window (usually a wxTextCtrl) that should be focused when the dialog is
     * shown.
     */
    void SetInitialFocus( wxWindow* aWindow )
    {
        m_initialFocusTarget = aWindow;
    }

    int ShowModal() override;

    int ShowQuasiModal();      // disable only the parent window, otherwise modal.

    void EndQuasiModal( int retCode );  // End quasi-modal mode

    bool IsQuasiModal() const { return m_qmodal_showing; }

    // A quasi-modal dialog disables its parent window.  Sadly this disabling is more extreme
    // than wxWidgets' normal modal dialog disabling, and prevents things like hotkey Cut/Copy/
    // Paste from working in search controls in standard file dialogs.  So when we put up a modal
    // dialog in front of a quasi-modal, we suspend the quasi-modal dialog parent window
    // disabling, causing us to fall back to the normal modal dialog parent window disabling.
    void PrepareForModalSubDialog();
    void CleanupAfterModalSubDialog();

    bool Show( bool show ) override;

    bool Enable( bool enable ) override;

    void OnPaint( wxPaintEvent &event );

    void OnModify();
    void ClearModify();

    /**
     * Force the position of the dialog to a new position
     * @param aNewPosition is the new forced position
     */
    void SetPosition( const wxPoint& aNewPosition );

    EDA_UNITS GetUserUnits() const
    {
        return m_units;
    }

    void SelectAllInTextCtrls( wxWindowList& children );

    void SetupStandardButtons( std::map<int, wxString> aLabels = {} );

    static bool IsCtrl( int aChar, const wxKeyEvent& e )
    {
        return e.GetKeyCode() == aChar && e.ControlDown() && !e.AltDown() &&
                !e.ShiftDown() && !e.MetaDown();
    }

    static bool IsShiftCtrl( int aChar, const wxKeyEvent& e )
    {
        return e.GetKeyCode() == aChar && e.ControlDown() && !e.AltDown() &&
                e.ShiftDown() && !e.MetaDown();
    }

    /**
     * Load persisted control values from the current project's local settings.
     * Controls may opt out by setting the boolean window property "persist" to
     * false.  Dialog geometry is stored under the special key "__geometry".
     */
    void LoadControlState();

    /**
     * Save control values and geometry to the current project's local settings.
     */
    void SaveControlState();

    /**
     * Opt out of control state saving.
     * @param aWindow can be either a specific control, or the whole dialog
     */
    void OptOut( wxWindow* aWindow );

    /**
     * Register a UNIT_BINDER so that it can handle units in control-state save/restore
     *
     * @param aUnitBinder
     * @param aWindow the control window
     */
    void RegisterUnitBinder( UNIT_BINDER* aUnitBinder, wxWindow* aWindow );

protected:
    /**
     * In all dialogs, we must call the same functions to fix minimal dlg size, the default
     * position and perhaps some others to fix a few issues depending on Windows Managers
     * this helper function does these calls.
     *
     * finishDialogSettings must be called from derived classes after all widgets have been
     * initialized, and therefore their size fixed.  If TransferDataToWindow() is used to
     * initialize widgets, at the end of TransferDataToWindow, or better yet, at end of a
     * wxInitDialogEvent handler.
     */
    void finishDialogSettings();

    /**
     * Set the dialog to the given dimensions in "dialog units". These are units equivalent
     * to 4* the average character width and 8* the average character height, allowing a dialog
     * to be sized in a way that scales it with the system font.
     */
    void setSizeInDU( int x, int y );

    /**
     * Convert an integer number of dialog units to pixels, horizontally. See SetSizeInDU or
     * wxDialog documentation for more information.
     */
    int horizPixelsFromDU( int x ) const;

    /**
     * Convert an integer number of dialog units to pixels, vertically. See SetSizeInDU or
     * wxDialog documentation for more information.
     */
    int vertPixelsFromDU( int y ) const;

    /**
     * Clear the existing dialog size and position.
     *
     * This will cause the dialog size to be clear so the next time the dialog is shown
     * the sizers will layout the dialog accordingly.  This useful when there are dialog
     * windows that size changes due to layout dependency hidden controls.
     */
    void resetSize();

    virtual void OnCharHook( wxKeyEvent& aEvt );

    /**
     * Override this method to perform dialog tear down actions not suitable for object dtor.
     *
     * @warning This only gets called for dialogs that are shown in the quasimodal mode.  If
     *          you need to perform tear down actions in modal or modeless dialogs, create
     *          a close window event handler.
     */
    virtual void TearDownQuasiModal() {}

private:
    /**
     * Properly handle the wxCloseEvent when in the quasimodal mode when not calling
     * EndQuasiModal which is possible with any dialog derived from #DIALOG_SHIM.
     */
    void OnCloseWindow( wxCloseEvent& aEvent );

    void OnSize( wxSizeEvent& aEvent );
    void OnMove( wxMoveEvent& aEvent );

    /**
     * Properly handle the default button events when in the quasimodal mode when not
     * calling EndQuasiModal which is possible with any dialog derived from #DIALOG_SHIM.
     */
    void OnButton( wxCommandEvent& aEvent );

    void onChildSetFocus( wxFocusEvent& aEvent );

    void onInitDialog( wxInitDialogEvent& aEvent );

    std::string generateKey( const wxWindow* aWin ) const;

    void registerUndoRedoHandlers( wxWindowList& aChildren );
    void recordControlChange( wxWindow* aCtrl );
    void onCommandEvent( wxCommandEvent& aEvent );
    void onSpinEvent( wxSpinEvent& aEvent );
    void onSpinDoubleEvent( wxSpinDoubleEvent& aEvent );
    void onStyledTextChanged( wxStyledTextEvent& aEvent );
    void onGridCellChanged( wxGridEvent& aEvent );
    void onPropertyGridChanged( wxPropertyGridEvent& aEvent );
    void onDataViewListChanged( wxDataViewEvent& aEvent );
    void doUndo();
    void doRedo();
    wxVariant getControlValue( wxWindow* aCtrl );
    void setControlValue( wxWindow* aCtrl, const wxVariant& aValue );

    DECLARE_EVENT_TABLE();

protected:
    EDA_UNITS              m_units;    // userUnits for display and parsing
    std::string            m_hash_key; // optional custom key for persistence

    // The following disables the storing of a user size.  It is used primarily for dialogs
    // with conditional content which don't need user sizing.
    bool                   m_useCalculatedSize;

    // On MacOS (at least) SetFocus() calls made in the constructor will fail because a
    // window that isn't yet visible will return false to AcceptsFocus().  So we must delay
    // the initial-focus SetFocus() call to the first paint event.
    bool                   m_firstPaintEvent;
    wxWindow*              m_initialFocusTarget;
    bool                   m_isClosing;

    wxGUIEventLoop*        m_qmodal_loop;  // points to nested event_loop, NULL means not qmodal
                                           // and dismissed
    bool                   m_qmodal_showing;
    WINDOW_DISABLER*       m_qmodal_parent_disabler;

    EDA_BASE_FRAME*        m_parentFrame;

    std::vector<wxWindow*> m_tabOrder;

    // The size asked by the caller, used the first time the dialog is created
    wxSize                 m_initialSize;
    bool                   m_userPositioned;
    bool                   m_userResized;

    // Used to support first-esc-cancels-edit logic
    std::map<wxWindow*, wxString>     m_beforeEditValues;
    std::map<wxWindow*, UNIT_BINDER*> m_unitBinders;

    struct UNDO_STEP
    {
        wxWindow* ctrl;
        wxVariant before;
        wxVariant after;
    };

    std::vector<UNDO_STEP>            m_undoStack;
    std::vector<UNDO_STEP>            m_redoStack;
    std::map<wxWindow*, wxVariant>    m_currentValues;
    bool                              m_handlingUndoRedo;
};

#endif  // DIALOG_SHIM_

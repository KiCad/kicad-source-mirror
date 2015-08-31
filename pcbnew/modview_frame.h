/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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
 * @file modview_frame.h
 */

#ifndef MODVIEWFRM_H_
#define MODVIEWFRM_H_


#include <wx/gdicmn.h>

class wxSashLayoutWindow;
class wxListBox;
class FP_LIB_TABLE;

namespace PCB { struct IFACE; }

/**
 * Component library viewer main window.
 */
class FOOTPRINT_VIEWER_FRAME : public PCB_BASE_FRAME
{
    friend struct PCB::IFACE;       // constructor called from here only

protected:
    FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType );


public:
    ~FOOTPRINT_VIEWER_FRAME();

    virtual EDA_COLOR_T GetGridColor() const;

    /**
     * Function ReCreateLibraryList
     *
     * Creates or recreates the list of current loaded libraries.
     * This list is sorted, with the library cache always at end of the list
     */
    void ReCreateLibraryList();

private:

    wxListBox*          m_libList;               // The list of libs names
    wxListBox*          m_footprintList;         // The list of footprint names

    wxString            m_configPath;            // subpath for configuration

    const wxString      getCurNickname();
    void                setCurNickname( const wxString& aNickname );

    const wxString      getCurFootprintName();
    void                setCurFootprintName( const wxString& aName );

    void OnSize( wxSizeEvent& event );

    void ReCreateFootprintList();
    void OnIterateFootprintList( wxCommandEvent& event );

    /**
     * Function UpdateTitle
     * updates the window title with current library information.
     */
    void UpdateTitle();

    /**
     * Function RedrawActiveWindow
     * Display the current selected component.
     * If the component is an alias, the ROOT component is displayed
     */
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );

    void OnCloseWindow( wxCloseEvent& Event );
    void CloseFootprintViewer( wxCommandEvent& event );

    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void ReCreateMenuBar();

    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnFootprintList( wxCommandEvent& event );
    void DClickOnFootprintList( wxCommandEvent& event );
    void OnSetRelativeOffset( wxCommandEvent& event );

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const;

    /**
     * Function OnHotKey
     * handle hot key events.
     * <p?
     * Some commands are relative to the item under the mouse cursor.  Commands are
     * case insensitive
     * </p>
     */
    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    void LoadSettings( wxConfigBase* aCfg );    // override virtual
    void SaveSettings( wxConfigBase* aCfg );    // override virtual

    /**
     * Function OnActivate
     * is called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    virtual void OnActivate( wxActivateEvent& event );

    void SelectCurrentLibrary( wxCommandEvent& event );

    /**
     * Function SelectCurrentFootprint
     * Selects the current footprint name and display it
     */
    void SelectCurrentFootprint( wxCommandEvent& event );

    /**
     * Function ExportSelectedFootprint
     * exports the current footprint name and close the library browser.
     */
    void ExportSelectedFootprint( wxCommandEvent& event );

    /**
     * Function SelectAndViewFootprint
     * Select and load the next or the previous footprint
     * if no current footprint, Rebuild the list of footprints available in a given footprint
     * library
     * @param aMode = NEXT_PART or PREVIOUS_PART
     */
    void SelectAndViewFootprint( int aMode );

    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );

    /**
     * Function Show3D_Frame (virtual)
     * displays 3D view of the footprint (module) being edited.
     */
    void Show3D_Frame( wxCommandEvent& event );

    /**
     * Function Update3D_Frame
     * must be called after a footprint selection
     * Updates the 3D view and 3D frame title.
     * @param aForceReloadFootprint = true to reload data (default)
     *   = false to update title only -(after creating the 3D viewer)
     */
    void Update3D_Frame( bool aForceReloadFootprint = true );

    /*
     * Virtual functions, not used here, but needed by PCB_BASE_FRAME
     * (virtual pure functions )
     */
    void OnLeftDClick( wxDC*, const wxPoint& ) {}
    void SaveCopyInUndoList( BOARD_ITEM*, UNDO_REDO_T, const wxPoint& ) {}
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST&, UNDO_REDO_T, const wxPoint &) {}

    void updateView();

    DECLARE_EVENT_TABLE()
};

#endif  // MODVIEWFRM_H_

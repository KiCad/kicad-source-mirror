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
class wxSemaphore;
class FOOTPRINT_LIBRARY;


/**
 * Component library viewer main window.
 */
class FOOTPRINT_VIEWER_FRAME : public PCB_BASE_FRAME
{
private:
    // List of libraries (for selection )
    wxSashLayoutWindow* m_LibListWindow;
    wxListBox*          m_LibList;          // The list of libs names
    wxSize              m_LibListSize;      // size of the window

    // List of components in the selected library
    wxSashLayoutWindow* m_FootprintListWindow;
    wxListBox*          m_FootprintList;          // The list of footprint names
    wxSize              m_FootprintListSize;      // size of the window

    // Flags
    wxSemaphore*        m_Semaphore;        // != NULL if the frame must emulate a modal dialog
    wxString            m_configPath;       // subpath for configuration

protected:
    static wxString m_libraryName;                  // Current selected libary
    static wxString m_footprintName;                // Current selected footprint
    static wxString m_selectedFootprintName;   // When the viewer is used to select a footprint
                                                    // the selected footprint is here

public:
    FOOTPRINT_VIEWER_FRAME( wxWindow* parent, wxSemaphore* semaphore = NULL );

    ~FOOTPRINT_VIEWER_FRAME();

    wxString& GetSelectedFootprint( void ) const { return m_selectedFootprintName; }

private:

    void OnSize( wxSizeEvent& event );

    /**
     * Function OnSashDrag
     * resizes the child windows when dragging a sash window border.
     */

    void OnSashDrag( wxSashEvent& event );

    /**
     * Function ReCreateLibraryList
     *
     * Creates or recreates the list of current loaded libraries.
     * This list is sorted, with the library cache always at end of the list
     */
    void ReCreateLibraryList();

    void ReCreateFootprintList();
    void Process_Special_Functions( wxCommandEvent& event );
    void DisplayLibInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnFootprintList( wxCommandEvent& event );
    void OnSetRelativeOffset( wxCommandEvent& event );

    void GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    /**
     * Function LoadSettings
     * loads the library viewer frame specific configuration settings.
     *
     * Don't forget to call this base method from any derived classes or the
     * settings will not get loaded.
     */
    void LoadSettings();

    /**
     * Function SaveSettings
     * save library viewer frame specific configuration settings.
     *
     * Don't forget to call this base method from any derived classes or the
     * settings will not get saved.
     */
    void SaveSettings();

    wxString& GetFootprintName( void ) const { return m_footprintName; }

    /**
     * Function OnActivate
     * is called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    virtual void OnActivate( wxActivateEvent& event );

    void SelectCurrentLibrary( wxCommandEvent& event );

    void SelectCurrentFootprint( wxCommandEvent& event );

    /**
     * Function ExportSelectedFootprint
     * exports the current footprint name and close the library browser.
     */
    void ExportSelectedFootprint( wxCommandEvent& event );

    /**
     * Function SelectAndViewFootprint
     * Select and load the next or the previous footprint
     * if no current footprint, Rebuild the list of footprints availlable in a given footprint library
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
     */
    void Update3D_Frame();

    /*
     * Virtual functions, not used here, but needed by PCB_BASE_FRAME
     * (virtual pure functions )
     */
    void OnLeftDClick(wxDC*, const wxPoint&) {}
    void SaveCopyInUndoList(BOARD_ITEM*, UNDO_REDO_T, const wxPoint&) {}
    void SaveCopyInUndoList(PICKED_ITEMS_LIST&, UNDO_REDO_T, const wxPoint&) {}


    DECLARE_EVENT_TABLE()
};

#endif  // MODVIEWFRM_H_

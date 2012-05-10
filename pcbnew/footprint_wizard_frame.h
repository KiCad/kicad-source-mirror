/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Miguel Angel Ajo Pelayo, miguelangel@nbee.es
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
 * @file footprint_wizard_frame.h
 */

#ifndef FOOTPRINT_WIZARD_FRAME_H_
#define FOOTPRINT_WIZARD_FRAME_H_


#include <wx/gdicmn.h>
#include <class_footprint_wizard.h>
class wxSashLayoutWindow;
class wxListBox;
class wxSemaphore;
class wxGrid;


/**
 * Component library viewer main window.
 */
class FOOTPRINT_WIZARD_FRAME : public PCB_BASE_FRAME
{
private:
    // List of libraries (for selection )
    wxSashLayoutWindow* m_PageListWindow;
    wxListBox*          m_PageList;          // The list of pages
    wxSize              m_PageListSize;      // size of the window

    // List of components in the selected library
    wxSashLayoutWindow* m_ParameterGridWindow;
    wxGrid*             m_ParameterGrid;          // The list of parameters
    wxSize              m_ParameterGridSize;      // size of the window

    // Flags
    wxSemaphore*        m_Semaphore;        // != NULL if the frame must emulate a modal dialog
    wxString            m_configPath;       // subpath for configuration
    
    FOOTPRINT_WIZARD* m_FootprintWizard;

protected:
    wxString m_wizardName;          //< name of the current wizard     
    wxString m_wizardDescription;   //< description of the wizard     
    wxString m_wizardStatus;        //< current wizard status    

public:
    FOOTPRINT_WIZARD_FRAME( wxWindow* parent, wxSemaphore* semaphore = NULL );
    
    ~FOOTPRINT_WIZARD_FRAME();

    MODULE* GetBuiltFootrint( void );

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
    void ReCreatePageList();
    void ReCreateParameterList();
    void SelectFootprintWizard();
    void ReloadFootprint();
    
    
    void Process_Special_Functions( wxCommandEvent& event );
    void DisplayWizardInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void OnCloseWindow( wxCloseEvent& Event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void ClickOnPageList( wxCommandEvent& event );
    void ClickOnParameterGrid( wxCommandEvent& event );
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


    /**
     * Function OnActivate
     * is called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    virtual void OnActivate( wxActivateEvent& event );

    void SelectCurrentWizard( wxCommandEvent& event );

    void ParametersUpdated( wxCommandEvent& event );



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
     *   = false to update title only -(aftre creating the 3D viewer)
     */
    void Update3D_Frame( bool aForceReloadFootprint = true );

    /*
     * Virtual functions, not used here, but needed by PCB_BASE_FRAME
     * (virtual pure functions )
     */
    void OnLeftDClick(wxDC*, const wxPoint&) {}
    void SaveCopyInUndoList(BOARD_ITEM*, UNDO_REDO_T, const wxPoint&) {}
    void SaveCopyInUndoList(PICKED_ITEMS_LIST&, UNDO_REDO_T, const wxPoint&) {}


    DECLARE_EVENT_TABLE()
};

#endif  // FOOTPRINT_WIZARD_FRM_H_

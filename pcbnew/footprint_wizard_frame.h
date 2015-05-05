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
class wxGrid;
class wxGridEvent;
class FOOTPRINT_EDIT_FRAME;


/**
 * Class FOOTPRINT_WIZARD_FRAME
 */
class FOOTPRINT_WIZARD_FRAME : public PCB_BASE_FRAME
{
private:
    wxListBox*      m_pageList;             ///< The list of pages
    int             m_pageListWidth;        ///< width of the window
    wxGrid*         m_parameterGrid;        ///< The list of parameters
    int             m_parameterGridWidth;   ///< size of the grid

    // Flags
    wxString        m_configPath;           ///< subpath for configuration

protected:
    wxString        m_wizardName;           ///< name of the current wizard
    wxString        m_wizardDescription;    ///< description of the wizard
    wxString        m_wizardStatus;         ///< current wizard status

public:

    FOOTPRINT_WIZARD_FRAME( KIWAY* aKiway, wxWindow* parent, FRAME_T aFrameType );

    ~FOOTPRINT_WIZARD_FRAME();

    MODULE*             GetBuiltFootprint();

private:

    void                OnSize( wxSizeEvent& event );

    /**
     * Function ExportSelectedFootprint();
     * will let the caller exit from the wait loop, and get the built footprint
     *
     */
    void                ExportSelectedFootprint( wxCommandEvent& aEvent );

    /**
     * Function OnSashDrag
     * resizes the child windows when dragging a sash window border.
     */

    void                OnSashDrag( wxSashEvent& event );

    /**
     * Function ReCreatePageList
     * Creates or recreates the list of parameter pages for the current wizard.
     * This list is sorted
     */
    void                ReCreatePageList();

    /**
     * Function ReCreateParameterList
     * Creates the list of parameters for the current page
     */
    void                ReCreateParameterList();

    /**
     * Function SelectFootprintWizard
     * Shows the list of footprint wizards available into the system
     */
    void                SelectFootprintWizard();

    /**
     * Function ReloadFootprint
     * Reloads the current footprint
     */
    void                ReloadFootprint();

    /**
     * Function GetMyWizard
     * Reloads the wizard by name
     */
    FOOTPRINT_WIZARD*   GetMyWizard();


    void                Process_Special_Functions( wxCommandEvent& event );

    /**
     * Function DisplayWizardInfos
     * Shows all the details about the current wizard
     */
    void                DisplayWizardInfos();


    void                RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void                OnCloseWindow( wxCloseEvent& Event );
    void                ReCreateHToolbar();
    void                ReCreateVToolbar();
    void                OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    void                ClickOnPageList( wxCommandEvent& event );
    void                OnSetRelativeOffset( wxCommandEvent& event );

    bool                GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    void                LoadSettings( wxConfigBase* aCfg ); // override virtual
    void                SaveSettings( wxConfigBase* aCfg ); // override virtual

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int ) const { return NULL; }

    /**
     * Function OnActivate
     * is called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    virtual void        OnActivate( wxActivateEvent& event );

    void                SelectCurrentWizard( wxCommandEvent& event );

    void                ParametersUpdated( wxGridEvent& event );


    bool                OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );

    /**
     * Function Show3D_Frame (virtual)
     * displays 3D view of the footprint (module) being edited.
     */
    void                Show3D_Frame( wxCommandEvent& event );

    /**
     * Function Update3D_Frame
     * must be called after a footprint selection
     * Updates the 3D view and 3D frame title.
     * @param aForceReloadFootprint = true to reload data (default)
     *   = false to update title only -(aftre creating the 3D viewer)
     */
    void                Update3D_Frame( bool aForceReloadFootprint = true );

    /*
     * Virtual functions, not used here, but needed by PCB_BASE_FRAME
     * (virtual pure functions )
     */
    void OnLeftDClick( wxDC*, const wxPoint& ) {}
    void SaveCopyInUndoList( BOARD_ITEM*, UNDO_REDO_T, const wxPoint& ) {}
    void SaveCopyInUndoList( const PICKED_ITEMS_LIST&, UNDO_REDO_T, const wxPoint& ) {}


    DECLARE_EVENT_TABLE()
};

#endif    // FOOTPRINT_WIZARD_FRM_H_

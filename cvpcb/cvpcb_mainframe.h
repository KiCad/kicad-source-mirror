/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cvpcb_mainframe.h
 */

#ifndef _CVPCB_MAINFRAME_H_
#define _CVPCB_MAINFRAME_H_

#include <wx/listctrl.h>
#include <wx/filename.h>
#include <pcb_netlist.h>
#include <footprint_info.h>

#include <wxBasePcbFrame.h>
#include <config_params.h>


/*  Forward declarations of all top-level window classes. */
class wxAuiToolBar;
class FOOTPRINTS_LISTBOX;
class COMPONENTS_LISTBOX;
class LIBRARY_LISTBOX;
class DISPLAY_FOOTPRINTS_FRAME;
class COMPONENT;
class FP_LIB_TABLE;

namespace CV { struct IFACE; }

/**
 * The CvPcb application main window.
 */
class CVPCB_MAINFRAME : public KIWAY_PLAYER
{
    friend struct CV::IFACE;

    wxArrayString             m_footprintListEntries;

public:
    bool                      m_KeepCvpcbOpen;
    FOOTPRINTS_LISTBOX*       m_footprintListBox;
    LIBRARY_LISTBOX*          m_libListBox;
    COMPONENTS_LISTBOX*       m_compListBox;
    wxAuiToolBar*             m_mainToolBar;
    wxFileName                m_NetlistFileName;
    wxArrayString             m_ModuleLibNames;
    wxArrayString             m_AliasLibNames;
    wxString                  m_NetlistFileExtension;
    wxString                  m_DocModulesFileName;
    FOOTPRINT_LIST            m_footprints;
    NETLIST                   m_netlist;

protected:
    int             m_undefinedComponentCnt;
    bool            m_modified;
    bool            m_isEESchemaNetlist;
    bool            m_skipComponentSelect;      // true to skip OnSelectComponent event
                                                // (in automatic selection/deletion of associations)
    PARAM_CFG_ARRAY m_projectFileParams;

    CVPCB_MAINFRAME( KIWAY* aKiway, wxWindow* aParent );

public:
    ~CVPCB_MAINFRAME();

    bool OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl=0 );   // overload KIWAY_PLAYER

    /**
     * @return a pointer on the Footprint Viewer frame, if exists, or NULL
     */
    DISPLAY_FOOTPRINTS_FRAME* GetFpViewerFrame();

    /**
     * Function OnSelectComponent
     * Called when clicking on a component in component list window
     * * Updates the filtered footprint list, if the filtered list option is selected
     * * Updates the current selected footprint in footprint list
     * * Updates the footprint shown in footprint display window (if opened)
     */
    void             OnSelectComponent( wxListEvent& event );

    /**
     * Function OnEditFootrprintLibraryTable
     * displays the footprint library table editing dialog and updates the global and local
     * footprint tables accordingly.
     */
    void             OnEditFootrprintLibraryTable( wxCommandEvent& event );

    void             OnQuit( wxCommandEvent& event );
    void             OnCloseWindow( wxCloseEvent& Event );
    void             OnSize( wxSizeEvent& SizeEvent );
    void             ReCreateHToolbar();
    virtual void     ReCreateMenuBar();

    void             ChangeFocus( bool aMoveRight );

    void             ToFirstNA( wxCommandEvent& event );
    void             ToPreviousNA( wxCommandEvent& event );

    /**
     * Function DelAssociations
     * removes all component footprint associations already made
     */
    void             DelAssociations( wxCommandEvent& event );

    void             SaveProjectFile( wxCommandEvent& aEvent );
    void             SaveQuitCvpcb( wxCommandEvent& event );

    /**
     * Function LoadNetList
     * reads a netlist selected by user when clicking on load netlist button or any entry
     * in the file history menu.
     */
    void             LoadNetList( wxCommandEvent& event );

    /**
     * Function OnEditLibraryTable
     * envokes the footpirnt library table edit dialog.
     */
    void             OnEditFootprintLibraryTable( wxCommandEvent& aEvent );

    void             OnKeepOpenOnSave( wxCommandEvent& event );
    void             DisplayModule( wxCommandEvent& event );

    /**
     * Called by the automatic association button
     * Read *.equ files to try to find corresponding footprint
     * for each component that is not already linked to a footprint ( a "free"
     * component )
     * format of a line:
     * 'cmp_ref' 'footprint_name'
     */
    void             AssocieModule( wxCommandEvent& event );

    void             DisplayDocFile( wxCommandEvent& event );

    /**
     * Function OnSelectFilteringFootprint
     * is the command event handler for enabling and disabling footprint filtering.
     */
    void             OnSelectFilteringFootprint( wxCommandEvent& event );

    void             OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event );

    /**
     * Function SetNewPkg
     * links the footprint to the current selected component
     * and selects the next component.
     * @param aFootprintName = the selected footprint
     */
    void             SetNewPkg( const wxString& aFootprintName );
    void             BuildCmpListBox();
    void             BuildFOOTPRINTS_LISTBOX();
    void             BuildLIBRARY_LISTBOX();

    /**
     * Create or Update the frame showing the current highlighted footprint
     * and (if showed) the 3D display frame
     */
    void             CreateScreenCmp();

    /**
     * Function SaveCmpLinkFile
     * Saves the component - footprint link file (.cmp file) to \a aFullFileName.
     *
     * @param aFullFileName A reference wxString object containing the full
     *                      file name of the netlist or cmp file.
     * If aFullFileName is empty, a file name will be asked to the user
     * @return  0 if an error occurred saving the link file to \a aFullFileName.
     *          -1 if canceled
     *          1 if OK
     */
    int              SaveCmpLinkFile( const wxString& aFullFileName );


    /**
     * Function WriteComponentLinkFile
     * Writes the component footprint link file \a aFullFileName on disk.
     *
     * @param aFullFileName full filename of .cmp file to write.
     * @return true if OK, false if error.
     */
    bool              WriteComponentLinkFile( const wxString& aFullFileName );

    /**
     * Function ReadNetList
     * reads the netlist (.net) file defined by #m_NetlistFileName.
     * and the corresponding cmp to footprint (.cmp) link file
     */
    bool             ReadNetListAndLinkFiles();

    int              ReadSchematicNetlist();

    /**
     * Function LoadProjectFile
     * reads the configuration parameter from the project (.pro) file \a aFileName
     */
    void             LoadProjectFile( const wxString& aFileName );

    void LoadSettings( wxConfigBase* aCfg );    // override virtual

    void SaveSettings( wxConfigBase* aCfg );    // override virtual

    /**
     * Function DisplayStatus
     * updates the information displayed on the status bar at bottom of the main frame.
     *
     * When the library or component list controls have the focus, the footprint assignment
     * status of the components is displayed in the first status bar pane and the list of
     * filters for the selected component is displayed in the second status bar pane.  When
     * the footprint list control has the focus, the description of the selected footprint is
     * displayed in the first status bar pane and the key words for the selected footprint are
     * displayed in the second status bar pane.  The third status bar pane always displays the
     * current footprint list filtering.
     */
    void             DisplayStatus();

    /**
     * Function LoadFootprintFiles
     * reads the list of footprint (*.mod files) and generate the list of footprints.
     * for each module are stored
     *      the module name
     *      documentation string
     *      associated keywords
     * m_ModuleLibNames is the list of library that must be read (loaded)
     * fills m_footprints
     * @return true if libraries are found, false otherwise.
     */
    bool             LoadFootprintFiles();

    /**
     * Function GetProjectFileParameters
     * return project file parameter list for CvPcb.
     * <p>
     * Populate the project file parameter array specific to CvPcb if it hasn't
     * already been populated and return a reference to the array to the caller.
     * Creating the parameter list at run time has the advantage of being able
     * to define local variables.  The old method of statically building the array
     * at compile time requiring global variable definitions.
     * </p>
     *
     * @return A reference to a PARAM_CFG_ARRAY contain the project settings for CvPcb.
     */
    PARAM_CFG_ARRAY& GetProjectFileParameters( void );

    /**
     * Function UpdateTitle
     * sets the main window title bar text.
     * <p>
     * If file name defined by CVPCB_MAINFRAME::m_NetlistFileName is not set, the title is
     * set to the application name appended with no file.  Otherwise, the title is set to
     * the full path and file name and read only is appended to the title if the user does
     * not have write access to the file.
     */
    void UpdateTitle();

    /**
     * Function SendMessageToEESCHEMA
     * Send a remote command to Eeschema via a socket,
     * Commands are
     * $PART: "reference"   put cursor on component anchor
     */
    void SendMessageToEESCHEMA();

    COMPONENT* GetSelectedComponent();

    DECLARE_EVENT_TABLE()
};

#endif  //#ifndef _CVPCB_MAINFRAME_H_

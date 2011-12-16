/**
 * @file cvpcb_mainframe.h
 */

#ifndef _CVPCB_MAINFRAME_H_
#define _CVPCB_MAINFRAME_H_

#include <wx/listctrl.h>
#include <wx/filename.h>

#include "wxBasePcbFrame.h"
#include "param_config.h"
#include "cvpcb.h"
#include "footprint_info.h"


/*  Forward declarations of all top-level window classes. */
class wxAuiToolBar;
class FOOTPRINTS_LISTBOX;
class COMPONENTS_LISTBOX;
class DISPLAY_FOOTPRINTS_FRAME;


/**
 * The CvPcb application main window.
 */
class CVPCB_MAINFRAME : public EDA_BASE_FRAME
{
public:

    bool m_KeepCvpcbOpen;
    FOOTPRINTS_LISTBOX*       m_FootprintList;
    COMPONENTS_LISTBOX*       m_ListCmp;
    DISPLAY_FOOTPRINTS_FRAME* m_DisplayFootprintFrame;
    wxAuiToolBar* m_mainToolBar;
    wxFileName m_NetlistFileName;
    wxArrayString             m_ModuleLibNames;
    wxArrayString             m_AliasLibNames;
    wxString        m_UserLibraryPath;
    wxString        m_NetlistFileExtension;
    wxString        m_DocModulesFileName;
    FOOTPRINT_LIST  m_footprints;
    COMPONENT_LIST  m_components;

protected:
    int             m_undefinedComponentCnt;
    bool            m_modified;
    bool            m_isEESchemaNetlist;
    PARAM_CFG_ARRAY m_projectFileParams;

public:
    CVPCB_MAINFRAME( const wxString& title, long style = KICAD_DEFAULT_DRAWFRAME_STYLE );
    ~CVPCB_MAINFRAME();

    void             OnLeftClick( wxListEvent& event );
    void             OnLeftDClick( wxListEvent& event );
    void             OnSelectComponent( wxListEvent& event );

    void             OnQuit( wxCommandEvent& event );
    void             OnCloseWindow( wxCloseEvent& Event );
    void             OnSize( wxSizeEvent& SizeEvent );
    void             OnChar( wxKeyEvent& event );
    void             ReCreateHToolbar();
    virtual void     ReCreateMenuBar();

    /**
     * Function SetLanguage
     * is called on a language menu selection.
     */
    void             SetLanguage( wxCommandEvent& event );

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

    void             ConfigCvpcb( wxCommandEvent& event );
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

    void             WriteStuffList( wxCommandEvent& event );
    void             DisplayDocFile( wxCommandEvent& event );

    /**
     * Function OnSelectFilteringFootprint
     * is the command event handler for enabling and disabling footprint filtering.
     */
    void             OnSelectFilteringFootprint( wxCommandEvent& event );

    void             OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event );

    /**
     * Function SetNewPkg
     * set the module to the selected component and selects the next component.
     */
    void             SetNewPkg( const wxString& package );
    void             BuildCmpListBox();
    void             BuildFOOTPRINTS_LISTBOX();
    void             CreateScreenCmp();

    /**
     * Function SaveNetList
     * backup and save netlist (.net) file to \a aFullFileName.
     *
     * @param aFullFileName A reference wxString object containing the full path and
     *                      file name of the netlist to save.
     * @return 0 if an error occurred saving the netlist to \a aFullFileName.
     */
    int              SaveNetList( const wxString& aFullFileName );

    /**
     * Function SaveComponentList
     * backup modules to file \a aFullFileName.
     *
     * @param aFullFileName Name of net list file to save.
     * @returns 1 if OK, 0 if error.
     */
    int              SaveComponentList( const wxString& aFullFileName );

    /**
     * Function ReadNetList
     * reads the netlist (.net) file defined by #m_NetlistFileName.
     */
    bool             ReadNetList();

    int              ReadSchematicNetlist();

    /**
     * Function LoadProjectFile
     * reads the configuration parameter from the project (.pro) file \a aFileName
     */
    void             LoadProjectFile( const wxString& aFileName );

    /**
     * Function LoadSettings
     * loads the CvPcb main frame specific configuration settings.
     *
     * Don't forget to call this base method from any derived classes or the
     * settings will not get loaded.
     */
    virtual void     LoadSettings();

    /**
     * Function SaveSettings
     * save the CvPcb frame specific configuration settings.
     *
     * Don't forget to call this base method from any derived classes or the
     * settings will not get saved.
     */
    virtual void     SaveSettings();

    /**
     * Function DisplayStatus
     * displays info to the status line at bottom of the main frame.
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
     * function GenNetlistPcbnew
     * writes the output netlist file
     *
     */
    int              GenNetlistPcbnew( FILE* f, bool isEESchemaNetlist = true );

    /**
     * Function LoadComponentFile
     * loads the .cmp file \a aCmpFileName that stores the component/footprint association.
     *
     * @param aFileName The full filename of .cmp file to load
     */
    bool             LoadComponentFile( const wxString& aFileName );

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

    DECLARE_EVENT_TABLE()
};

#endif  //#ifndef _CVPCB_MAINFRAME_H_

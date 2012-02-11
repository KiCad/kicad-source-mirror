/**
 * @file cvpcb_mainframe.h
 */

#ifndef _CVPCB_MAINFRAME_H_
#define _CVPCB_MAINFRAME_H_

#include <wx/listctrl.h>
#include <wx/filename.h>

#include <wxBasePcbFrame.h>
#include <param_config.h>
#include <cvpcb.h>
#include <footprint_info.h>


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

    /**
     * Function OnSelectComponent
     * Called when clicking on a component in component list window
     * * Updates the filtered foorprint list, if the filtered list option is selected
     * * Updates the current selected footprint in footprint list
     * * Updates the footprint shown in footprint display window (if opened)
     */
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

    /**
     * Function WriteStuffList
     * Creates a file for Eeschema, import footprint selections
     * in schematic
     * the file format is
     * comp = "<reference>" module = "<footprint name">
     */
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
     * links the footprint to the current selected component
     * and selects the next component.
     * @param aFootprintName = the selected footprint
     */
    void             SetNewPkg( const wxString& aFootprintName );
    void             BuildCmpListBox();
    void             BuildFOOTPRINTS_LISTBOX();
    void             CreateScreenCmp();

    /**
     * Function SaveCmpLinkFile
     * Saves the component - footprint link file (.cmp file) to \a aFullFileName.
     *
     * @param aFullFileName A reference wxString object containing the full
     *                      file name of the netlist or cmp file.
     * If aFullFileName is empty, a file name will be asked to the user
     * @return  0 if an error occurred saving the link file to \a aFullFileName.
     *          -1 if cancelled
     *          1 if OK
     */
    int              SaveCmpLinkFile( const wxString& aFullFileName );


    /**
     * Function LoadComponentFile
     * loads the .cmp link file \a aCmpFileName which stores
     * the component/footprint association.
     *
     * @param aFileName The full filename of .cmp file to load
     * If empty, a filename will be asked to the user
     */
    bool             LoadComponentLinkFile( const wxString& aFileName );

    /**
     * Function WriteComponentLinkFile
     * Writes the component footprint link file \a aFullFileName on disk.
     *
     * @param aFullFileName full filename of .cmp file to write.
     * @return true if OK, false if error.
     */
    bool              WriteComponentLinkFile( const wxString& aFullFileName );

    /**
     * Function ReadComponentLinkFile
     * Reads the component footprint link file \a aFullFileName.
     *
     * @param aFile = the opened the opened file to read.
     *  ReadComponentLinkFile will close the file
     * @return true if OK, false if error.
     */
    bool              ReadComponentLinkFile( FILE * aFile );

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

/**
 * @file cvpcb_mainframe.h
 */

#ifndef _CVPCB_MAINFRAME_H_
#define _CVPCB_MAINFRAME_H_

#include "wx/listctrl.h"
#include <wx/filename.h>
#include "param_config.h"
#include "cvpcb.h"
#include "footprint_info.h"

/*  Forward declarations of all top-level window classes. */
class FOOTPRINTS_LISTBOX;
class COMPONENTS_LISTBOX;
class DISPLAY_FOOTPRINTS_FRAME;



/**
 * The CVPcb application main window.
 */
class CVPCB_MAINFRAME : public EDA_BASE_FRAME
{
public:

    bool m_KeepCvpcbOpen;
    FOOTPRINTS_LISTBOX*  m_FootprintList;
    COMPONENTS_LISTBOX*  m_ListCmp;
    DISPLAY_FOOTPRINTS_FRAME* DrawFrame;
    WinEDA_Toolbar*      m_HToolBar;
    wxFileName           m_NetlistFileName;
    wxArrayString        m_ModuleLibNames;
    wxArrayString        m_AliasLibNames;
    wxString             m_UserLibraryPath;
    wxString             m_NetlistFileExtension;
    wxString             m_DocModulesFileName;
    FOOTPRINT_LIST       m_footprints;
    COMPONENT_LIST       m_components;

protected:
    int             m_undefinedComponentCnt;
    bool            m_modified;
    bool            m_isEESchemaNetlist;
    PARAM_CFG_ARRAY m_projectFileParams;

public:
    CVPCB_MAINFRAME( const wxString& title,
                       long            style = KICAD_DEFAULT_DRAWFRAME_STYLE );
    ~CVPCB_MAINFRAME();

    void             OnLeftClick( wxListEvent& event );
    void             OnLeftDClick( wxListEvent& event );
    void             OnSelectComponent( wxListEvent& event );

    void             Update_Config( wxCommandEvent& event );
    void             OnQuit( wxCommandEvent& event );
    void             OnCloseWindow( wxCloseEvent& Event );
    void             OnSize( wxSizeEvent& SizeEvent );
    void             OnChar( wxKeyEvent& event );
    void             ReCreateHToolbar();
    virtual void     ReCreateMenuBar();
    void             SetLanguage( wxCommandEvent& event );

    void             ToFirstNA( wxCommandEvent& event );
    void             ToPreviousNA( wxCommandEvent& event );
    void             DelAssociations( wxCommandEvent& event );
    void             SaveQuitCvpcb( wxCommandEvent& event );
    void             LoadNetList( wxCommandEvent& event );
    void             ConfigCvpcb( wxCommandEvent& event );
    void             OnKeepOpenOnSave( wxCommandEvent& event );
    void             DisplayModule( wxCommandEvent& event );
    void             AssocieModule( wxCommandEvent& event );
    void             WriteStuffList( wxCommandEvent& event );
    void             DisplayDocFile( wxCommandEvent& event );
    void             OnSelectFilteringFootprint( wxCommandEvent& event );

    void             OnUpdateKeepOpenOnSave( wxUpdateUIEvent& event );

    void             SetNewPkg( const wxString& package );
    void             BuildCmpListBox();
    void             BuildFOOTPRINTS_LISTBOX();
    void             CreateScreenCmp();
    int              SaveNetList( const wxString& FullFileName );
    int              SaveComponentList( const wxString& FullFileName );
    bool             ReadNetList();
    int              ReadSchematicNetlist();
    void             LoadProjectFile( const wxString& FileName );
    void             SaveProjectFile( const wxString& fileName );
    virtual void     LoadSettings();
    virtual void     SaveSettings();

    /**
     * Function DisplayStatus()
     * Displays info to the status line at bottom of the main frame
     */
    void             DisplayStatus();

    /**
     * Function LoadFootprintFiles
     * Read the list of libraries (*.mod files) and generate the list of modules.
     * for each module are stored
     *      the module name
     *      documentation string
     *      associated keywords
     * m_ModuleLibNames is the list of library that must be read (loaded)
     * fills m_footprints
     * @return true if libraries are found, false otherwise.
     */
    bool LoadFootprintFiles( );

    /**
     * function GenNetlistPcbnew
     * writes the output netlist file
     *
     */
    int GenNetlistPcbnew( FILE* f, bool isEESchemaNetlist = true );

    /**
     * Function LoadComponentFile
     * Loads the .cmp file that stores the component/footprint association.
     * @param aCmpFileName = the full filename of .cmp file to load
     */
    bool LoadComponentFile( const wxString& aCmpFileName );

    PARAM_CFG_ARRAY& GetProjectFileParameters( void );

    DECLARE_EVENT_TABLE()
};

#endif  //#ifndef _CVPCB_MAINFRAME_H_

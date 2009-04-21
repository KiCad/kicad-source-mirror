/***************************************************/
/*	tool_cvpcb.cpp: construction du menu principal */
/***************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "id.h"
#include "trigo.h"

#include "bitmaps.h"
#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


/*********************************************/
void WinEDA_CvpcbFrame::ReCreateHToolbar()
/*********************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
    SetToolBar( m_HToolBar );

    m_HToolBar->AddTool( ID_CVPCB_READ_INPUT_NETLIST, wxBitmap( open_xpm ),
                         _( "Open a NetList file" ) );

    m_HToolBar->AddTool( ID_CVPCB_SAVEQUITCVPCB, wxBitmap( save_xpm ),
                         _( "Save NetList and Footprints List files" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_CREATE_CONFIGWINDOW, wxBitmap( config_xpm ),
                         _( "Configuration" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_CREATE_SCREENCMP, wxBitmap( module_xpm ),
                         _( "View selected footprint" ) );

    m_HToolBar->AddTool( ID_CVPCB_AUTO_ASSOCIE, wxBitmap( auto_associe_xpm ),
                         _( "Automatic Association" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_GOTO_PREVIOUSNA, wxBitmap( left_xpm ),
                         _( "Select previous free component" ) );

    m_HToolBar->AddTool( ID_CVPCB_GOTO_FIRSTNA, wxBitmap( right_xpm ),
                         _( "Select next free component" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_DEL_ASSOCIATIONS,
                         wxBitmap( delete_association_xpm ),
                         _( "Delete all associations" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_CREATE_STUFF_FILE,
                         wxBitmap( save_cmpstuff_xpm ),
                         _( "Create export file (component/footprint list, used by eeschema to fill the footprint field of components)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_PCB_DISPLAY_FOOTPRINT_DOC,
                         wxBitmap( datasheet_xpm ),
                         _( "Display footprints list documentation" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddSeparator();
    m_HToolBar->AddRadioTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
                              wxEmptyString,
                              wxBitmap( module_filtered_list_xpm ),
                              wxNullBitmap,
                              _( "Display the filtered footprint list for the current component" ) );
    m_HToolBar->AddRadioTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
                              wxEmptyString, wxBitmap( module_full_list_xpm ),
                              wxNullBitmap,
                              _( "Display the full footprint list (without filtering)" ) );

    if( config )
    {
        wxString key = wxT( FILTERFOOTPRINTKEY );
        int      opt = config->Read( key, (long) 1 );
        m_HToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST, opt );
        m_HToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST, !opt );
    }

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


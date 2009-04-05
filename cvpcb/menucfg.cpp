/***************************************/
/** menucfg : configuration de CVPCB  **/
/***************************************/

/* cree et/ou affiche et modifie la configuration de CVPCB */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


/*****************************************/
/* classe pour la frame de Configuration */
/*****************************************/
#include "dialog_cvpcb_config.cpp"


/***************************************************/
void WinEDA_CvpcbFrame::CreateConfigWindow()
/***************************************************/
/* Creation de la fenetre de configuration de CVPCB */
{
    KiConfigCvpcbFrame* ConfigFrame = new KiConfigCvpcbFrame( this );

    ConfigFrame->ShowModal();
    ConfigFrame->Destroy();
}


/*********************************************/
void KiConfigCvpcbFrame::SetDialogDatas()
/*********************************************/
{
    wxConfig* cfg = wxGetApp().m_EDA_CommonConfig;

    m_ListLibr->InsertItems( g_LibName_List, 0 );
    m_ListEquiv->InsertItems( g_ListName_Equ, 0 );

    m_LibDirCtrl = new WinEDA_EnterText( this, _( "Lib Dir:" ),
                                         g_UserLibDirBuffer,
                                         m_RightBoxSizer, wxDefaultSize );

    m_NetInputExtCtrl = new WinEDA_EnterText( this, _( "Net Input Ext:" ),
                                              g_NetlistFileExtension,
                                              m_NetExtBoxSizer, wxDefaultSize );

    wxString DocModuleFileName = cfg->Read( DOC_FOOTPRINTS_LIST_KEY,
                                            DEFAULT_FOOTPRINTS_LIST_FILENAME );
    m_TextHelpModulesFileName = new WinEDA_EnterText( this,
                                                      _( "Module Doc File:" ),
                                                      DocModuleFileName,
                                                      m_RightBoxSizer,
                                                      wxDefaultSize );

    /* Create info on Files ext */
    wxStaticText* StaticText;
    wxString      text;
    text.Printf( wxT( "%s     .%s" ), _( "Cmp ext:" ),
                 ComponentFileExtension.c_str() );
    StaticText = new wxStaticText( this, -1, text );
    m_FileExtList->Add( StaticText,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM |
                        wxADJUST_MINSIZE );

    text.Printf( wxT( "%s     .%s" ), _( "Lib ext:" ),
                 ModuleFileExtension.c_str() );
    StaticText = new wxStaticText( this, -1, text );
    m_FileExtList->Add( StaticText,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM |
                        wxADJUST_MINSIZE );

    text.Printf( wxT( "%s  .%s" ), _( "NetOut ext:" ), NetExtBuffer.c_str() );
    StaticText = new wxStaticText( this, -1, text );
    m_FileExtList->Add( StaticText,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM |
                        wxADJUST_MINSIZE );

    text.Printf( wxT( "%s   .%s" ), _( "Equiv ext:" ),
                 EquivFileExtension.c_str() );
    StaticText = new wxStaticText( this, -1, text );
    m_FileExtList->Add( StaticText,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM |
                        wxADJUST_MINSIZE );

    text.Printf( wxT( "%s   .%s" ), _( "Retro ext:" ),
                 RetroFileExtension.c_str() );
    StaticText = new wxStaticText( this, -1, text );
    m_FileExtList->Add( StaticText,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxBOTTOM |
                        wxADJUST_MINSIZE );
}


/********************************************************/
void KiConfigCvpcbFrame::AcceptCfg( wxCommandEvent& event )
/********************************************************/
{
    Update();
    Close();
}


/**********************************/
void KiConfigCvpcbFrame::Update()
/**********************************/
{
    wxASSERT( wxGetApp().m_EDA_CommonConfig != NULL );

    wxString  msg;
    wxConfig* cfg = wxGetApp().m_EDA_CommonConfig;

    if( !m_DoUpdate )
        return;
    g_NetlistFileExtension = m_NetInputExtCtrl->GetValue();
    cfg->Write( DOC_FOOTPRINTS_LIST_KEY, m_TextHelpModulesFileName->GetValue() );

    msg = m_LibDirCtrl->GetValue();
    if( msg != g_UserLibDirBuffer )
    {
        g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
        SetRealLibraryPath( wxT( "modules" ) );
        listlib();
        m_Parent->BuildFootprintListBox();
    }
}


/****************************************************/
void KiConfigCvpcbFrame::SaveCfg( wxCommandEvent& event )
/****************************************************/
{
    WinEDA_CvpcbFrame* parent = ( WinEDA_CvpcbFrame* )GetParent();
    wxASSERT( parent && parent->IsKindOf( CLASSINFO( WinEDA_CvpcbFrame ) ) );

    Update();
    Save_Config( this, parent->m_NetlistFileName.GetFullPath() );
}


/******************************************************/
void KiConfigCvpcbFrame::ReadOldCfg( wxCommandEvent& event )
{
    WinEDA_CvpcbFrame* parent = ( WinEDA_CvpcbFrame* )GetParent();
    wxASSERT( parent && parent->IsKindOf( CLASSINFO( WinEDA_CvpcbFrame ) ) );

    wxFileName fn = parent->m_NetlistFileName;
    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Load Project File" ), fn.GetPath(),
                      fn.GetFullName(), ProjectFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    Read_Config( dlg.GetPath() );
    m_DoUpdate = FALSE;
    Close( TRUE );
}


/*******************************************************/
void KiConfigCvpcbFrame::LibDelFct( wxCommandEvent& event )
/*******************************************************/
{
    int ii;

    ii = m_ListLibr->GetSelection();
    if( ii < 0 )
        return;

    g_LibName_List.RemoveAt( ii );

    /* suppression de la reference dans la liste des librairies */
    m_ListLibr->Delete( ii );

    g_UserLibDirBuffer = m_LibDirCtrl->GetValue();

    listlib();

    m_Parent->BuildFootprintListBox();
}


/********************************************************/
void KiConfigCvpcbFrame::LibAddFct( wxCommandEvent& event )
/********************************************************/
{
    int           ii;
    wxFileName    fn;
    wxString      tmp;
    wxArrayString Filenames;

    ii = m_ListLibr->GetSelection();
    if( event.GetId() == ADD_LIB )  /* Ajout apres selection */
    {
        ii++;
    }
    if( ii < 0 )
        ii = 0;

    Update();

    wxFileDialog dlg( this, _( "Foot Print Library Files" ), g_RealLibDirBuffer,
                      wxEmptyString, ModuleFileWildcard,
                      wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    dlg.GetPaths( Filenames );

    if( Filenames.GetCount() == 0 )
        return;

    for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        fn = Filenames[jj];

        /* If the library path is already in the library search paths
         * list, just add the library name to the list.  Otherwise, add
         * the library name with the full path. */
        if( wxGetApp().GetLibraryPathList().Index( fn.GetPath() ) == wxNOT_FOUND )
            tmp = fn.GetPathWithSep() + fn.GetName();
        else
            tmp = fn.GetName();

		// Add or insert new library name.
        if( g_LibName_List.Index( tmp ) == wxNOT_FOUND )
		{
            g_LibName_List.Insert( tmp, ii++ );
        }
        else
        {
            wxString msg = wxT( "<" ) + tmp + wxT( "> : " ) +
                _( "Library already in use" );
            DisplayError( this, msg );
        }
    }

    g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
    listlib();
    m_Parent->BuildFootprintListBox();
    m_ListLibr->Clear();
    m_ListLibr->InsertItems( g_LibName_List, 0 );
}


/********************************************************/
void KiConfigCvpcbFrame::EquDelFct( wxCommandEvent& event )
/********************************************************/
{
    int ii;

    ii = m_ListEquiv->GetSelection();
    if( ii < 0 )
        return;

    g_ListName_Equ.RemoveAt( ii );
    m_ListEquiv->Delete( ii );
}


/********************************************************/
void KiConfigCvpcbFrame::EquAddFct( wxCommandEvent& event )
/********************************************************/
{
    int        ii;
    wxFileName fn;
    wxString   libName;

    ii = m_ListEquiv->GetSelection();
    if( event.GetId() == ADD_EQU )
        ii++;                               /* Ajout apres selection */
    if( ii < 0 )
        ii = 0;

    Update();

    wxFileDialog dlg( this, _( "Open Footprint Alias Files" ),
                      g_RealLibDirBuffer, wxEmptyString, EquivFileWildcard,
                      wxFD_OPEN | wxFD_MULTIPLE | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxArrayString Filenames;
    dlg.GetFilenames( Filenames );

    if( Filenames.GetCount() == 0 )
        return;

    for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        fn = Filenames[jj];

        /* Use the file name without extension if the library path is
         * already in the default library search path.  Otherwise, use
         * the full path and file name without the extension. */
        if( wxGetApp().GetLibraryPathList().Index( fn.GetPath() ) == wxNOT_FOUND )
            libName = fn.GetPathWithSep() + fn.GetName();
        else
            libName = fn.GetName();

        //Add or insert new equiv library name
        if( g_ListName_Equ.Index( libName ) == wxNOT_FOUND )
        {
            g_ListName_Equ.Insert( libName, ii++ );
        }
        else
        {
            wxString msg;
            msg << wxT( "<" ) << libName << wxT( "> : " ) <<
                _( "Library already in use" );
            DisplayError( this, msg );
        }
    }

    /* Update display list */
    g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
    listlib();

    m_ListEquiv->Clear();
    m_ListEquiv->InsertItems( g_ListName_Equ, 0 );
}

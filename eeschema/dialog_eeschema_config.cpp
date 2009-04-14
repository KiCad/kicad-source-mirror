/////////////////////////////////////////////////////////////////////////////

// Name:        dialog_eeschema_config.cpp
// Purpose:
// Author:      jean-pierre Charras
// Created:     17/02/2006 21:14:46
// Copyright:   Kicad Team
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"
#include "netlist.h"

#include "id.h"

#include "dialog_eeschema_config_fbp.h"

class DIALOG_EESCHEMA_CONFIG : public DIALOG_EESCHEMA_CONFIG_FBP
{
private:
    WinEDA_SchematicFrame* m_Parent;
    bool m_LibListChanged;

private:

    // event handlers, overiding the fbp handlers
    void Init();
    void OnCloseWindow( wxCloseEvent& event );
    void OnSaveCfgClick( wxCommandEvent& event );
    void OnRemoveLibClick( wxCommandEvent& event );
    void OnAddOrInsertLibClick( wxCommandEvent& event );
    void OnLibPathSelClick( wxCommandEvent& event );
	void OnOkClick( wxCommandEvent& event );
	void OnCancelClick( wxCommandEvent& event );
    void OnRemoveUserPath( wxCommandEvent& event );


public:
    DIALOG_EESCHEMA_CONFIG( WinEDA_SchematicFrame * parent );
    ~DIALOG_EESCHEMA_CONFIG() {};
};


/******************************************************************/
void WinEDA_SchematicFrame::InstallConfigFrame( const wxPoint& pos )
/******************************************************************/
{
    DIALOG_EESCHEMA_CONFIG CfgFrame( this );

    CfgFrame.ShowModal();
}


/*******************************************************************************/
DIALOG_EESCHEMA_CONFIG::DIALOG_EESCHEMA_CONFIG( WinEDA_SchematicFrame* parent )
    : DIALOG_EESCHEMA_CONFIG_FBP( parent )
/*******************************************************************************/
{
    wxString msg;

    m_Parent = parent;
    m_LibListChanged = false;

    Init();

    msg = _( "from " ) + wxGetApp().m_CurrentOptionFile;
    SetTitle( msg );

    if( GetSizer() )
        GetSizer()->SetSizeHints( this );
}


/***********************************/
void DIALOG_EESCHEMA_CONFIG::Init()
/***********************************/
{
    SetFont( *g_DialogFont );
    SetFocus();

    // Display current files extension (info)
    wxString msg = m_InfoCmpFileExt->GetLabel() + g_NetCmpExtBuffer;
    m_InfoCmpFileExt->SetLabel( msg );

    msg = m_InfoNetFileExt->GetLabel() + NetlistFileExtension;
    m_InfoNetFileExt->SetLabel( msg );

    msg = m_InfoLibFileExt->GetLabel() + CompLibFileExtension;
    m_InfoLibFileExt->SetLabel( msg );

    msg = m_InfoSymbFileExt->GetLabel() + g_SymbolExtBuffer;
    m_InfoSymbFileExt->SetLabel( msg );

    msg = m_InfoSchFileExt->GetLabel() + SchematicFileExtension;
    m_InfoSchFileExt->SetLabel( msg );

    // Init currently availlable netlist formats
    wxArrayString NetlistNameItems;
    NetlistNameItems.Add( wxT( "Pcbnew" ) );
    NetlistNameItems.Add( wxT( "OrcadPcb2" ) );
    NetlistNameItems.Add( wxT( "CadStar" ) );
    NetlistNameItems.Add( wxT( "Spice" ) );

    // Add extra neltlist format (using external converter)
    msg = ReturnUserNetlistTypeName( true );
    while( !msg.IsEmpty() )
    {
        NetlistNameItems.Add( msg );
        msg = ReturnUserNetlistTypeName( false );
    }

    m_NetFormatBox->InsertItems( NetlistNameItems, 0 );

    if( g_NetFormat > (int) m_NetFormatBox->GetCount() )
        g_NetFormat = NET_TYPE_PCBNEW;
    m_NetFormatBox->SetSelection( g_NetFormat - NET_TYPE_PCBNEW );

    m_ListLibr->InsertItems( g_LibName_List, 0 );
    m_LibDirCtrl->SetValue( g_UserLibDirBuffer );

    // Display actual libraries paths:
    wxPathList libpaths = wxGetApp().GetLibraryPathList();
    for( unsigned ii = 0; ii < libpaths.GetCount(); ii++ )
    {
        m_DefaultLibraryPathslistBox->Append( libpaths[ii]);
    }
}


/******************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnCancelClick( wxCommandEvent& event )
/******************************************************************/
{
    EndModal( -1 );
}


/**************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnOkClick( wxCommandEvent& event )
/**************************************************************/
{
    // Set new netlist format
    g_NetFormat = m_NetFormatBox->GetSelection() + NET_TYPE_PCBNEW;

    // Set new default path lib
    if ( g_UserLibDirBuffer != m_LibDirCtrl->GetValue() )
    {
        wxGetApp().RemoveLibraryPath( g_UserLibDirBuffer );
        g_UserLibDirBuffer = m_LibDirCtrl->GetValue();
        wxGetApp().InsertLibraryPath( g_UserLibDirBuffer, 1 );
        m_LibListChanged = true;
    }

    // Set new active library list if the list of default path was modified
    if( m_LibListChanged )
    {
        // Recreate lib list
        g_LibName_List.Clear();
        for ( unsigned ii = 0; ii < m_ListLibr->GetCount(); ii ++ )
            g_LibName_List.Add(m_ListLibr->GetString(ii) );

        // take new list in account
        LoadLibraries( m_Parent );
        if( m_Parent->m_ViewlibFrame )
            m_Parent->m_ViewlibFrame->ReCreateListLib();
    }
    if ( event.GetId() != ID_SAVE_CFG )
        EndModal( 0 );
}

/**************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnCloseWindow( wxCloseEvent& event )
/**************************************************************/
{
    EndModal( 0 );
}



/*********************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnRemoveLibClick( wxCommandEvent& event )
/*********************************************************************/
/* Remove a library to the library list.
 * The real list (g_LibName_List) is not changed, so the change can be cancelled
 */
{
    int ii;

    ii = m_ListLibr->GetSelection();
    if( ii < 0 )
        return;

    m_ListLibr->Delete(ii);
    m_LibListChanged = TRUE;
}


/**************************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnAddOrInsertLibClick( wxCommandEvent& event )
/**************************************************************************/

/* Insert or add a library to the library list:
 *   The new library is put in list before (insert button) the selection,
 *   or added (add button) to end of list
 * The real list (g_LibName_List) is not changed, so the change can be cancelled
 */
{
    int        ii;
    wxString   libfilename;
    wxFileName fn;

    ii = m_ListLibr->GetSelection();
    if( ii == wxNOT_FOUND && event.GetId() != ID_ADD_LIB )
        ii = 0;

    wxString libpath =  m_LibDirCtrl->GetValue();
    if ( libpath.IsEmpty() )
        libpath = wxGetApp().ReturnLastVisitedLibraryPath();

    wxFileDialog FilesDialog( this, _( "Library files:" ), libpath,
                              wxEmptyString, CompLibFileWildcard,
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    wxArrayString Filenames;
    FilesDialog.GetPaths( Filenames );

     for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        fn = Filenames[jj];
        if ( jj == 0 )
            wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

        /* If the library path is already in the library search paths
         * list, just add the library name to the list.  Otherwise, add
         * the library name with the full or relative path.
         * the relative path, when possible is preferable,
         * because it preserve use of default libraries paths, when the path is a sub path of these default paths
         *
        */
        if( wxGetApp().GetLibraryPathList().Index( fn.GetPath() ) != wxNOT_FOUND )  // Ok, trivial case
            libfilename = fn.GetName();
        else    // not in the default, : see if this file is in a subpath:
        {
            libfilename = fn.GetPathWithSep() + fn.GetName();
            for ( unsigned kk = 0; kk < wxGetApp().GetLibraryPathList().GetCount(); kk ++ )
            {
                if( fn.MakeRelativeTo(wxGetApp().GetLibraryPathList()[kk] ) )
                {
                    libfilename = fn.GetPathWithSep() + fn.GetName();
                    break;
                }
            }
        }

        //Add or insert new library name, if not already in list
        if( m_ListLibr->FindString( libfilename, fn.IsCaseSensitive() ) == wxNOT_FOUND )
        {
            m_LibListChanged = TRUE;
            if( event.GetId() == ID_ADD_LIB )
                m_ListLibr->Append( libfilename );
            else
                m_ListLibr->Insert( libfilename, ii++ );
        }
        else
        {
            wxString msg = wxT( "<" ) + libfilename + wxT( "> : " ) +
                _( "Library already in use" );
            DisplayError( this, msg );
        }
    }
}



/*******************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnSaveCfgClick( wxCommandEvent& event )
/*******************************************************************/
{
    OnOkClick( event );
    m_Parent->Save_Config( this );
}


/***********************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnLibPathSelClick( wxCommandEvent& event )
/***********************************************************************/
{
    wxString path =  m_LibDirCtrl->GetValue();
    if ( path.IsEmpty() )
        path = wxGetApp().ReturnLastVisitedLibraryPath();

    bool     select = EDA_DirectorySelector( _( " Default Path for libraries" ),    /* Titre de la fenetre */
                                             path,                                  /* Chemin par defaut */
                                             wxDD_DEFAULT_STYLE,
                                             this,                                  /* parent frame */
                                             wxDefaultPosition );

    if( !select )
        return;

    m_LibDirCtrl->SetValue( path );
    wxGetApp().SaveLastVisitedLibraryPath( path );

}


/***********************************************************************/
void DIALOG_EESCHEMA_CONFIG::OnRemoveUserPath( wxCommandEvent& event )
/***********************************************************************/
{
    m_LibDirCtrl->Clear( );
 }

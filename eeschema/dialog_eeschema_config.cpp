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

    // Virtual event handlers, overide them in your derived class
    void Init();
    virtual void OnCloseWindow( wxCloseEvent& event );
    virtual void OnSaveCfgClick( wxCommandEvent& event );
    virtual void OnRemoveLibClick( wxCommandEvent& event );
    virtual void OnAddOrInsertLibClick( wxCommandEvent& event );
    virtual void OnLibPathSelClick( wxCommandEvent& event );
	virtual void OnOkClick( wxCommandEvent& event );
	virtual void OnCancelClick( wxCommandEvent& event );


public:
    DIALOG_EESCHEMA_CONFIG( WinEDA_SchematicFrame * parent );
    ~DIALOG_EESCHEMA_CONFIG() {};
};


/******************************************************************/
void WinEDA_SchematicFrame::InstallConfigFrame( const wxPoint& pos )
/******************************************************************/
{
    DIALOG_EESCHEMA_CONFIG* CfgFrame = new DIALOG_EESCHEMA_CONFIG( this );

    CfgFrame->ShowModal(); CfgFrame->Destroy();
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
    g_UserLibDirBuffer = m_LibDirCtrl->GetValue();

    // Set new active lib list
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

/* Insert or add a library to the existing library list:
 *   New library is put in list before (insert) or after (add)
 *   the selection
 */
{
    int      ii;
    wxString FullLibName, ShortLibName, Mask;

    ii = m_ListLibr->GetSelection();
    if( ii < 0 )
        ii = 0;

    if( event.GetId() == ID_ADD_LIB )
    {
        if( m_ListLibr->GetCount() != 0 )
            ii++;                                   /* Add after selection */
    }


    wxString libpath =  m_LibDirCtrl->GetValue();
    if ( libpath.IsEmpty() )
       libpath = g_RealLibDirBuffer;

    Mask = wxT( "*" ) + CompLibFileExtension;

    wxFileDialog FilesDialog( this, _( "Library files:" ), libpath,
                              wxEmptyString, Mask,
                              wxFD_DEFAULT_STYLE | wxFD_MULTIPLE );

    int diag = FilesDialog.ShowModal();
    if( diag != wxID_OK )
        return;

    wxArrayString Filenames;
    FilesDialog.GetPaths( Filenames );

    for( unsigned jj = 0; jj < Filenames.GetCount(); jj++ )
    {
        FullLibName  = Filenames[jj];
        ShortLibName = MakeReducedFileName( FullLibName, libpath, CompLibFileExtension );
        if( ShortLibName.IsEmpty() )    //Just in case...
            continue;

        //Add or insert new library name, if not already in list
        #ifdef __WINDOWS__
        bool case_sensitive = false;
        #else
        bool case_sensitive = true;
        #endif
        if( m_ListLibr->FindString( ShortLibName, case_sensitive ) == wxNOT_FOUND )
        {
            m_LibListChanged = TRUE;
            m_ListLibr->Insert( ShortLibName, ii );
        }
        else
        {
            wxString msg;
            msg << wxT( "<" ) << ShortLibName << wxT( "> : " ) << _( "Library already in use" );
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
       path = g_RealLibDirBuffer;

    bool     select = EDA_DirectorySelector( _( " Default Path for libraries" ),    /* Titre de la fenetre */
                                             path,                                  /* Chemin par defaut */
                                             wxDD_DEFAULT_STYLE,
                                             this,                                  /* parent frame */
                                             wxDefaultPosition );

    if( !select )
        return;

    m_LibDirCtrl->SetValue( path );
}

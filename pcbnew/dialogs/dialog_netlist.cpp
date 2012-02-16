/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_netlist.cpp
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <confirm.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <macros.h>
#include <pcbcommon.h>

#include <pcbnew_config.h>
#include <class_board_design_settings.h>
#include <wildcards_and_files_ext.h>

#include <dialog_netlist.h>

void PCB_EDIT_FRAME::InstallNetlistFrame( wxDC* DC )
{
    /* Setup the netlist file name to the last net list file read or the board file
     * name if no last file read is not set.
     */
    wxFileName fn = GetLastNetListRead();
    wxString lastNetlistName = GetLastNetListRead();

    if( !fn.FileExists() )
    {
        fn = GetScreen()->GetFileName();
        fn.SetExt( NetExtBuffer );
        lastNetlistName = fn.GetFullPath();
    }

    DIALOG_NETLIST frame( this, DC, lastNetlistName );

    frame.ShowModal();

    // Save project settings if needed.
    // Project settings are saved in the corresponding <board name>.pro file
    if( lastNetlistName != GetLastNetListRead() &&
        !GetScreen()->GetFileName().IsEmpty() &&
        IsOK(NULL, _("Project config has changed. Save it ?") ) )
    {
        wxFileName fn = GetScreen()->GetFileName();
        fn.SetExt( ProjectFileExtension );
        wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP,
                                       GetProjectFileParameters() );
    }
}


DIALOG_NETLIST::DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, wxDC * aDC,
                                const wxString & aNetlistFull_Filename )
    : DIALOG_NETLIST_FBP( aParent )
{
    m_Parent = aParent;
    m_DC = aDC;
    m_NetlistFilenameCtrl->SetValue( aNetlistFull_Filename );

    Init();

    GetSizer()->SetSizeHints( this );
}


void DIALOG_NETLIST::Init()
{
    SetFocus();
}

void DIALOG_NETLIST::OnOpenNelistClick( wxCommandEvent& event )
{
    wxString lastPath = wxFileName::GetCwd();
    wxString lastNetlistRead = m_Parent->GetLastNetListRead();

    if( !lastNetlistRead.IsEmpty() && !wxFileName::FileExists( lastNetlistRead ) )
    {
        lastNetlistRead = wxEmptyString;
    }
    else
    {
        wxFileName fn = lastNetlistRead;
        lastPath = fn.GetPath();
        lastNetlistRead = fn.GetFullName();
    }

    wxLogDebug( wxT( "Last net list read path <%s>, file name <%s>." ),
                GetChars( lastPath ), GetChars( lastNetlistRead ) );

    wxFileDialog FilesDialog( this, _( "Select Netlist" ), lastPath, lastNetlistRead,
                              NetlistFileWildcard, wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    m_NetlistFilenameCtrl->SetValue( FilesDialog.GetPath() );
}


void DIALOG_NETLIST::OnReadNetlistFileClick( wxCommandEvent& event )
{
    wxFileName fn = m_NetlistFilenameCtrl->GetValue();
    fn.SetExt( NetCmpExtBuffer );

    m_Parent->ReadPcbNetlist( m_NetlistFilenameCtrl->GetValue(),
                              fn.GetFullPath(), m_MessageWindow,
                              m_ChangeExistingFootprintCtrl->GetSelection() == 1 ? true : false,
                              m_DeleteBadTracks->GetSelection() == 1 ? true : false,
                              m_RemoveExtraFootprintsCtrl->GetSelection() == 1 ? true : false,
                              m_Select_By_Timestamp->GetSelection() == 1 ? true : false );
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    m_Parent->Test_Duplicate_Missing_And_Extra_Footprints( m_NetlistFilenameCtrl->GetValue() );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_COMPILE_RATSNEST
 */

void DIALOG_NETLIST::OnCompileRatsnestClick( wxCommandEvent& event )
{
    m_Parent->Compile_Ratsnest( m_DC, true );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_NETLIST::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}

/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_netlist.cpp
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"


#include "dialog_netlist.h"

extern void TestFor_Duplicate_Missing_And_Extra_Footprints( wxWindow*       frame,
                                                            const wxString& NetlistFullFilename,
                                                            BOARD*          Pcb );



void WinEDA_PcbFrame::InstallNetlistFrame( wxDC* DC, const wxPoint& pos )
{
    /* Setup the netlist file name to the last net list file read or the board file
     * name if no last file read is not set.
     */
    wxFileName fn = GetLastNetListRead();

    if( !fn.FileExists() )
    {
        fn = GetScreen()->m_FileName;
        fn.SetExt( NetExtBuffer );
    }

    DIALOG_NETLIST frame( this, DC, fn.GetFullPath() );

    frame.ShowModal();
}


DIALOG_NETLIST::DIALOG_NETLIST( WinEDA_PcbFrame* aParent, wxDC * aDC,
                                const wxString & aNetlistFull_Filename )
    : DIALOG_NETLIST_FBP( aParent )
{
    m_Parent = aParent;
    m_DC = aDC;
    m_NetlistFilenameCtrl->SetValue( aNetlistFull_Filename );

    Init();

    if( GetSizer() )
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
                              m_ChangeExistingFootprintCtrl->GetSelection() == 1 ? TRUE : FALSE,
                              m_DeleteBadTracks->GetSelection() == 1 ? TRUE : FALSE,
                              m_RemoveExtraFootprintsCtrl->GetSelection() == 1 ? TRUE : FALSE,
                              m_Select_By_Timestamp->GetSelection() == 1 ? TRUE : FALSE );
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    TestFor_Duplicate_Missing_And_Extra_Footprints( this, m_NetlistFilenameCtrl->GetValue(),
                                                    m_Parent->GetBoard() );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_COMPILE_RATSNEST
 */

void DIALOG_NETLIST::OnCompileRatsnestClick( wxCommandEvent& event )
{
    m_Parent->Compile_Ratsnest( m_DC, TRUE );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
 */

void DIALOG_NETLIST::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}

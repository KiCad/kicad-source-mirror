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

extern void ReadPcbNetlist( WinEDA_PcbFrame* aFrame,
                            const wxString&  aNetlistFullFilename,
                            const wxString&  aCmpFullFileName,
                            wxTextCtrl*      aMessageWindow,
                            bool             aChangeFootprint,
                            bool             aDeleteBadTracks,
                            bool             aDeleteExtraFootprints,
                            bool             aSelect_By_Timestamp );

extern void TestFor_Duplicate_Missing_And_Extra_Footprints( wxWindow*       frame,
                                                            const wxString& NetlistFullFilename,
                                                            BOARD*          Pcb );



/*************************************************************************/
void WinEDA_PcbFrame::InstallNetlistFrame( wxDC* DC, const wxPoint& pos )
/*************************************************************************/
{
    /* Setup the default netlist file name according to the board file name */
    wxFileName fn = GetScreen()->m_FileName;

    fn.SetExt( NetExtBuffer );

    DIALOG_NETLIST frame( this, DC, fn.GetFullPath() );

    frame.ShowModal();
}


DIALOG_NETLIST::DIALOG_NETLIST( WinEDA_PcbFrame* aParent, wxDC * aDC, const wxString & aNetlistFull_Filename )
    : DIALOG_NETLIST_FBP(aParent)
{
    m_Parent = aParent;
    m_DC = aDC;
    m_NetlistFilenameCtrl->SetValue(aNetlistFull_Filename);

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
    wxString fullfilename;

    wxFileDialog FilesDialog( this, _( "Netlist Selection:" ), wxGetCwd(),
                              wxEmptyString, NetlistFileWildcard,
                              wxFD_DEFAULT_STYLE | wxFD_FILE_MUST_EXIST );

    if( FilesDialog.ShowModal() != wxID_OK )
        return;

    fullfilename = FilesDialog.GetPath( );

    m_NetlistFilenameCtrl->SetValue( fullfilename );
}


void DIALOG_NETLIST::OnReadNetlistFileClick( wxCommandEvent& event )
{
    wxFileName fn = m_NetlistFilenameCtrl->GetValue();
    fn.SetExt( NetCmpExtBuffer );

    ReadPcbNetlist( m_Parent, m_NetlistFilenameCtrl->GetValue(),
                    fn.GetFullPath(), m_MessageWindow,
                    m_ChangeExistingFootprintCtrl->GetSelection() == 1 ? TRUE : FALSE,
                    m_DeleteBadTracks->GetSelection() == 1 ? TRUE : FALSE,
                    m_RemoveExtraFootprintsCtrl->GetSelection() == 1 ? TRUE : FALSE,
                    m_Select_By_Timestamp->GetSelection() == 1 ? TRUE : FALSE );
}


void DIALOG_NETLIST::OnTestFootprintsClick( wxCommandEvent& event )
{
    TestFor_Duplicate_Missing_And_Extra_Footprints( this, m_NetlistFilenameCtrl->GetValue(), m_Parent->GetBoard() );
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

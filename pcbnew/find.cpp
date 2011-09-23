/********************************************/
/* PCBNEW - Find dialog box implementation. */
/********************************************/


#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "wxPcbStruct.h"

#include "class_board.h"
#include "class_module.h"
#include "class_marker_pcb.h"

#include "pcbnew.h"
#include "pcbnew_id.h"
#include "protos.h"
#include "find.h"


static wxString s_OldStringFound;
static int      s_ItemCount, s_MarkerCount;


void PCB_EDIT_FRAME::InstallFindFrame( const wxPoint& pos, wxDC* DC )
{
    WinEDA_PcbFindFrame* frame = new WinEDA_PcbFindFrame( this, DC, pos );

    frame->ShowModal();
    frame->Destroy();
}


void WinEDA_PcbFindFrame::FindItem( wxCommandEvent& event )
{
    PCB_SCREEN* screen = (PCB_SCREEN*) ( m_Parent->GetScreen() );
    wxPoint     locate_pos;
    wxString    msg;
    bool        FindMarker = false;
    BOARD_ITEM* foundItem  = 0;

    switch( event.GetId() )
    {
    case ID_FIND_ITEM:
        s_ItemCount = 0;
        break;

    case ID_FIND_MARKER:
        s_MarkerCount = 0;

    // fall thru

    case ID_FIND_NEXT_MARKER:
        FindMarker = true;
        break;
    }

    s_OldStringFound = m_NewText->GetValue();

    m_Parent->DrawPanel->GetViewStart( &screen->m_StartVisu.x,
                                       &screen->m_StartVisu.y );

    if( FindMarker )
    {
        MARKER_PCB* marker = m_Parent->GetBoard()->GetMARKER( s_MarkerCount++ );

        if( marker )
        {
            foundItem  = marker;
            locate_pos = marker->GetPosition();
        }
    }
    else
    {
        int StartCount = 0;

        for( MODULE* module = m_Parent->GetBoard()->m_Modules; module; module = module->Next() )
        {
            if( WildCompareString( s_OldStringFound, module->GetReference().GetData(), false ) )
            {
                StartCount++;

                if( StartCount > s_ItemCount )
                {
                    foundItem  = module;
                    locate_pos = module->GetPosition();
                    s_ItemCount++;
                    break;
                }
            }

            if( WildCompareString( s_OldStringFound, module->m_Value->m_Text.GetData(), false ) )
            {
                StartCount++;

                if( StartCount > s_ItemCount )
                {
                    foundItem  = module;
                    locate_pos = module->m_Pos;
                    s_ItemCount++;
                    break;
                }
            }
        }
    }

    if( foundItem )
    {
        m_Parent->SetCurItem( foundItem );

        if( FindMarker )
            msg = _( "Marker found" );
        else
            msg.Printf( _( "<%s> Found" ), GetChars( s_OldStringFound ) );

        m_Parent->SetStatusText( msg );

        m_Parent->CursorGoto( locate_pos );

        EndModal( 1 );
    }
    else
    {
        m_Parent->SetStatusText( wxEmptyString );

        if( FindMarker )
            msg = _( "Marker not found" );
        else
            msg.Printf( _( "<%s> Not Found" ), GetChars( s_OldStringFound ) );

        DisplayError( this, msg, 10 );
        EndModal( 0 );
    }
}


/*!
 * WinEDA_PcbFindFrame type definition
 */

IMPLEMENT_DYNAMIC_CLASS( WinEDA_PcbFindFrame, wxDialog )

/*!
 * WinEDA_PcbFindFrame event table definition
 */

BEGIN_EVENT_TABLE( WinEDA_PcbFindFrame, wxDialog )

////@begin WinEDA_PcbFindFrame event table entries
    EVT_BUTTON( ID_FIND_ITEM, WinEDA_PcbFindFrame::OnFindItemClick )
    EVT_BUTTON( ID_FIND_NEXT_ITEM, WinEDA_PcbFindFrame::OnFindNextItemClick )
    EVT_BUTTON( ID_FIND_MARKER, WinEDA_PcbFindFrame::OnFindMarkerClick )
    EVT_BUTTON( ID_FIND_NEXT_MARKER, WinEDA_PcbFindFrame::OnFindNextMarkerClick )

////@end WinEDA_PcbFindFrame event table entries

END_EVENT_TABLE()
/*!
 * WinEDA_PcbFindFrame constructors
 */

WinEDA_PcbFindFrame::WinEDA_PcbFindFrame()
{
}


WinEDA_PcbFindFrame::WinEDA_PcbFindFrame( PCB_BASE_FRAME* parent,
                                          wxDC*           DC,
                                          const wxPoint&  pos,
                                          wxWindowID      id,
                                          const wxString& caption,
                                          const wxSize&   size,
                                          long            style )
{
    m_Parent = parent;
    m_DC     = DC;

    Create( parent, id, caption, pos, size, style );

    m_NewText->SetFocus();
}


/*!
 * WinEDA_PcbFindFrame creator
 */

bool WinEDA_PcbFindFrame::Create( wxWindow*       parent,
                                  wxWindowID      id,
                                  const wxString& caption,
                                  const wxPoint&  pos,
                                  const wxSize&   size,
                                  long            style )
{
////@begin WinEDA_PcbFindFrame member initialisation
    m_NewText = NULL;

////@end WinEDA_PcbFindFrame member initialisation

////@begin WinEDA_PcbFindFrame creation
    SetExtraStyle( wxWS_EX_BLOCK_EVENTS );
    wxDialog::Create( parent, id, caption, pos, size, style );

    CreateControls();

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

    Centre();

////@end WinEDA_PcbFindFrame creation
    return true;
}


/*!
 * Control creation for WinEDA_PcbFindFrame
 */

void WinEDA_PcbFindFrame::CreateControls()
{
////@begin WinEDA_PcbFindFrame content construction
    // Generated by DialogBlocks, 29/04/2009 15:15:49 (unregistered)

    WinEDA_PcbFindFrame* itemDialog1 = this;

    wxBoxSizer*          itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );

    itemDialog1->SetSizer( itemBoxSizer2 );

    wxStaticText* itemStaticText3 = new wxStaticText( itemDialog1,
                                                      wxID_STATIC,
                                                      _( "Item to find:" ),
                                                      wxDefaultPosition,
                                                      wxDefaultSize,
                                                      0 );
    itemBoxSizer2->Add( itemStaticText3,
                        0,
                        wxGROW | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE,
                        5 );

    m_NewText = new wxTextCtrl( itemDialog1, ID_TEXTCTRL, _T( "" ),
                                wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer2->Add( m_NewText, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    wxBoxSizer* itemBoxSizer5 = new wxBoxSizer( wxHORIZONTAL );
    itemBoxSizer2->Add( itemBoxSizer5,
                        0,
                        wxALIGN_CENTER_HORIZONTAL | wxLEFT | wxRIGHT |
                        wxBOTTOM,
                        5 );

    wxBoxSizer* itemBoxSizer6 = new wxBoxSizer( wxVERTICAL );
    itemBoxSizer5->Add( itemBoxSizer6,
                        0,
                        wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                        5 );

    wxButton* itemButton7 =
        new wxButton( itemDialog1, ID_FIND_ITEM, _( "Find Item" ),
                      wxDefaultPosition, wxDefaultSize, 0 );
    itemButton7->SetDefault();
    itemBoxSizer6->Add( itemButton7, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    wxButton* itemButton8 =
        new wxButton( itemDialog1, ID_FIND_NEXT_ITEM, _( "Find Next Item" ),
                      wxDefaultPosition, wxDefaultSize,
                      0 );
    itemBoxSizer6->Add( itemButton8,
                        0,
                        wxGROW | wxLEFT | wxRIGHT | wxBOTTOM,
                        5 );

    wxBoxSizer* itemBoxSizer9 = new wxBoxSizer( wxVERTICAL );
    itemBoxSizer5->Add( itemBoxSizer9,
                        0,
                        wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT,
                        5 );

    wxButton* itemButton10 =
        new wxButton( itemDialog1, ID_FIND_MARKER, _( "Find Marker" ),
                      wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer9->Add( itemButton10, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    wxButton* itemButton11 = new wxButton( itemDialog1,
                                           ID_FIND_NEXT_MARKER,
                                           _( "Find Next Marker" ),
                                           wxDefaultPosition,
                                           wxDefaultSize,
                                           0 );
    itemBoxSizer9->Add( itemButton11,
                        0,
                        wxGROW | wxLEFT | wxRIGHT | wxBOTTOM,
                        5 );

////@end WinEDA_PcbFindFrame content construction
}


/*!
 * Should we show tooltips?
 */

bool WinEDA_PcbFindFrame::ShowToolTips()
{
    return true;
}


/*!
 * Get bitmap resources
 */

wxBitmap WinEDA_PcbFindFrame::GetBitmapResource( const wxString& name )
{
    // Bitmap retrieval
////@begin WinEDA_PcbFindFrame bitmap retrieval
    wxUnusedVar( name );
    return wxNullBitmap;

////@end WinEDA_PcbFindFrame bitmap retrieval
}


/*!
 * Get icon resources
 */

wxIcon WinEDA_PcbFindFrame::GetIconResource( const wxString& name )
{
    // Icon retrieval
////@begin WinEDA_PcbFindFrame icon retrieval
    wxUnusedVar( name );
    return wxNullIcon;

////@end WinEDA_PcbFindFrame icon retrieval
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_ITEM
 */

void WinEDA_PcbFindFrame::OnFindItemClick( wxCommandEvent& event )
{
    FindItem( event );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_NEXT_ITEM
 */

void WinEDA_PcbFindFrame::OnFindNextItemClick( wxCommandEvent& event )
{
    FindItem( event );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_MARKER
 */

void WinEDA_PcbFindFrame::OnFindMarkerClick( wxCommandEvent& event )
{
    FindItem( event );
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_FIND_NEXT_MARKER
 */

void WinEDA_PcbFindFrame::OnFindNextMarkerClick( wxCommandEvent& event )
{
    FindItem( event );
}

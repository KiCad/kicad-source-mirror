/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <widgets/mdw_status_pill.h>
#include <widgets/mdw_editing.h>
#include <widgets/ui_common.h>

#include <ui_events.h>

#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_source.h>

#include <algorithm>

#include <wx/dcbuffer.h>
#include <wx/menu.h>
#include <wx/renderer.h>
#include <wx/statusbr.h>
#include <wx/datetime.h>
#include <wx/intl.h>


namespace
{
enum
{
    ID_SHOW_VERSION_CONTROL = wxID_HIGHEST + 1,
    ID_CLAIM_ACTION ///< The first of the claim actions, numbered in menu order
};
}


MDW_STATUS_PILL::MDW_STATUS_PILL( wxWindow* aParent ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxWANTS_CHARS ),
        m_topLevel( wxGetTopLevelParent( aParent ) ),
        m_statusFieldId( -1 )
{
    SetBackgroundStyle( wxBG_STYLE_PAINT );
    Bind( wxEVT_PAINT, &MDW_STATUS_PILL::onPaint, this );
    Bind( wxEVT_KEY_DOWN, &MDW_STATUS_PILL::onKey, this );
    Bind( wxEVT_LEFT_UP,
          [this]( wxMouseEvent& )
          {
              showMenu();
          } );

    auto repaint = [this]( wxFocusEvent& aEvent )
    {
        Refresh();
        aEvent.Skip();
    };

    Bind( wxEVT_SET_FOCUS, repaint );
    Bind( wxEVT_KILL_FOCUS, repaint );

    SetCursor( wxCursor( wxCURSOR_HAND ) );

    // Not every path that replaces a frame's document tells the pill, so a changed path is caught here
    Bind( wxEVT_IDLE,
          [this]( wxIdleEvent& aEvent )
          {
              if( m_tracker )
                  m_tracker->RefreshIfMoved();

              aEvent.Skip();
          } );

    // Coming back to the window rereads a claim or a git add made meanwhile, since nothing polls for them
    if( m_topLevel )
        m_topLevel->Bind( wxEVT_ACTIVATE, &MDW_STATUS_PILL::onActivate, this );

    Hide();
}


MDW_STATUS_PILL::~MDW_STATUS_PILL()
{
    if( m_topLevel )
        m_topLevel->Unbind( wxEVT_ACTIVATE, &MDW_STATUS_PILL::onActivate, this );

    if( m_statusFieldId >= 0 && GetParent() )
        GetParent()->Unbind( wxEVT_SIZE, &MDW_STATUS_PILL::onParentSize, this );
}


wxString MDW_STATUS_PILL::LabelFor( const STATE& aState, const wxDateTime& aNow )
{
    if( !aState.bound )
        return wxEmptyString;

    if( aState.phase == MDW_CONNECTION::OFFLINE )
        return wxString::Format( wxS( "%s \u00B7 %s" ), MdwConnectionName( aState.phase ), _( "status unknown" ) );

    // The same phase the workspace strip names, so the two never disagree
    if( aState.phase != MDW_CONNECTION::ONLINE )
        return MdwConnectionName( aState.phase );

    if( !aState.known )
        return _( "Checking" ) + wxS( "\u2026" );

    if( aState.lock.is_locked )
        return MdwClaimLabel( aState.lock, aNow );

    wxString label = MdwCategoryName( aState.category );
    wxString revision = MdwRevisionText( aState.revision );

    if( aState.category == MDW_LIFECYCLE_CATEGORY::RELEASED && !revision.IsEmpty() )
        label << wxS( " \u00B7 " ) << revision;

    if( !aState.lock.known )
        label << wxS( " \u00B7 " ) << MdwClaimLabel( aState.lock, aNow );

    return label;
}


std::vector<MDW_CLAIM_ACTION> MDW_STATUS_PILL::ActionsFor( const STATE& aState )
{
    if( !aState.bound || aState.phase != MDW_CONNECTION::ONLINE || !aState.known || !aState.can_lock )
        return {};

    return MdwClaimActions( aState.lock );
}


int MDW_STATUS_PILL::FieldWidth( wxWindow* aBar )
{
    STATE offline;
    offline.bound = true;

    // Sized for a claim held for days by an unnamed holder, so a long holder name clips
    wxDateTime now = wxDateTime::Now();
    STATE      claimed = offline;
    claimed.phase = MDW_CONNECTION::ONLINE;
    claimed.known = true;
    claimed.lock.is_locked = true;
    claimed.lock.lock_timestamp = now - wxDateSpan::Days( 30 );

    STATE released = offline;
    released.phase = MDW_CONNECTION::ONLINE;
    released.known = true;
    released.category = MDW_LIFECYCLE_CATEGORY::RELEASED;
    released.revision.label = wxS( "000" );
    released.revision.kind = MDW_REVISION_KIND::RELEASE;
    released.lock.known = false;

    int widest = 0;

    for( const STATE& state : { offline, claimed, released } )
        widest = std::max( widest, KIUI::GetTextSize( LabelFor( state, now ), aBar ).x );

    // SetState pads the label by 20 and reposition deflates the field by 2 on each side
    return widest + 24;
}


void MDW_STATUS_PILL::SetState( const STATE& aState )
{
    m_state = aState;

    if( !m_state.bound )
    {
        Hide();
        fitField();
        return;
    }

    wxString label = LabelFor( m_state, wxDateTime::Now() );
    wxSize   text = GetTextExtent( label );

    SetMinSize( wxSize( text.GetWidth() + 20, text.GetHeight() + 6 ) );
    SetName( label );
    SetToolTip( label );

    Show();
    fitField();
    Refresh();
    announceClaim();
}


void MDW_STATUS_PILL::announceClaim()
{
    if( !m_tracker || m_state.phase != MDW_CONNECTION::ONLINE || !m_state.known )
        return;

    const MDW_LOCK_INFO& lock = m_state.lock;
    wxString             claim;

    // Keyed by holder and time as well, so a claim taken while the document is open is announced too
    if( lock.is_locked && !lock.locked_by_me )
    {
        claim = m_tracker->Item().item_id + wxS( "\n" ) + lock.owner_id + wxS( "\n" )
                + ( lock.lock_timestamp.IsValid() ? lock.lock_timestamp.FormatISOCombined() : wxString() );
    }

    if( claim == m_announcedClaim )
        return;

    if( claim.IsEmpty() )
    {
        m_announcedClaim = claim;
        return;
    }

    wxCommandEvent notice( EDA_EVT_SHOW_WARNING_INFOBAR );
    notice.SetString( MdwClaimLabel( m_state.lock, wxDateTime::Now() ) );

    // Command events bubble up to the owning frame, which skips it while another notice shows
    if( GetEventHandler()->ProcessEvent( notice ) )
        m_announcedClaim = claim;
}


void MDW_STATUS_PILL::fitField()
{
    wxStatusBar* bar = dynamic_cast<wxStatusBar*>( GetParent() );

    if( !bar || m_statusFieldId < 0 )
        return;

    // A hidden pill takes no room from the status text beside it
    int width = IsShown() ? FieldWidth( bar ) : 0;

    if( bar->GetStatusWidth( m_statusFieldId ) != width )
    {
        std::vector<int> widths( bar->GetFieldsCount() );

        for( int ii = 0; ii < bar->GetFieldsCount(); ++ii )
            widths[ii] = bar->GetStatusWidth( ii );

        widths[m_statusFieldId] = width;
        bar->SetStatusWidths( static_cast<int>( widths.size() ), widths.data() );
    }

    reposition();
}


void MDW_STATUS_PILL::TrackProject( const wxString& aProjectPath, bool aEnabled,
                                    MDW_DOCUMENT_TRACKER::DOCUMENT_RESOLVER aDocument )
{
    if( !m_tracker )
    {
        m_tracker = std::make_unique<MDW_DOCUMENT_TRACKER>(
                [this]( const STATE& aState )
                {
                    SetState( aState );
                } );
    }

    m_tracker->Track( aProjectPath, aEnabled, std::move( aDocument ) );
}


void MDW_STATUS_PILL::onActivate( wxActivateEvent& aEvent )
{
    if( aEvent.GetActive() && m_tracker )
        m_tracker->Refresh();

    aEvent.Skip();
}


void MDW_STATUS_PILL::runAction( const MDW_CLAIM_ACTION& aAction )
{
    if( !m_tracker )
        return;

    std::shared_ptr<MDW_SOURCE> source = m_tracker->Source();
    MDW_ITEM_REF                item = m_tracker->Item();

    if( !source || item.item_id.IsEmpty() )
        return;

    MdwRunClaimAction( this, source, item, aAction,
                       [this]()
                       {
                           m_tracker->Refresh();
                       } );
}


void MDW_STATUS_PILL::showMenu()
{
    if( !m_state.bound )
        return;

    wxMenu                        menu;
    std::vector<MDW_CLAIM_ACTION> actions = ActionsFor( m_state );

    for( size_t ii = 0; ii < actions.size(); ++ii )
    {
        menu.Append( ID_CLAIM_ACTION + static_cast<int>( ii ), actions[ii].label );
        menu.Enable( ID_CLAIM_ACTION + static_cast<int>( ii ), actions[ii].enabled );
    }

    if( m_showVersionControl )
    {
        if( menu.GetMenuItemCount() )
            menu.AppendSeparator();

        menu.Append( ID_SHOW_VERSION_CONTROL, _( "Show in Version Control" ) );
    }

    if( !menu.GetMenuItemCount() )
        return;

    int choice = GetPopupMenuSelectionFromUser( menu, wxPoint( 0, 0 ) );

    if( choice >= ID_CLAIM_ACTION && choice < ID_CLAIM_ACTION + static_cast<int>( actions.size() ) )
        runAction( actions[choice - ID_CLAIM_ACTION] );
    else if( choice == ID_SHOW_VERSION_CONTROL )
        m_showVersionControl();
}


void MDW_STATUS_PILL::onKey( wxKeyEvent& aEvent )
{
    switch( aEvent.GetKeyCode() )
    {
    case WXK_SPACE:
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
    case WXK_WINDOWS_MENU:
        showMenu();
        break;

    default:
        aEvent.Skip();
    }
}


void MDW_STATUS_PILL::PlaceInStatusBar( int aFieldId )
{
    wxStatusBar* bar = dynamic_cast<wxStatusBar*>( GetParent() );

    if( !bar )
        return;

    m_statusFieldId = aFieldId;
    bar->Bind( wxEVT_SIZE, &MDW_STATUS_PILL::onParentSize, this );

    reposition();
}


void MDW_STATUS_PILL::onParentSize( wxSizeEvent& aEvent )
{
    reposition();
    aEvent.Skip();
}


void MDW_STATUS_PILL::reposition()
{
    wxStatusBar* bar = dynamic_cast<wxStatusBar*>( GetParent() );

    if( !bar || m_statusFieldId < 0 )
        return;

    wxRect rect;

    if( bar->GetFieldRect( m_statusFieldId, rect ) )
        SetSize( rect.Deflate( 2, 2 ) );
}


void MDW_STATUS_PILL::onPaint( wxPaintEvent& aEvent )
{
    wxAutoBufferedPaintDC dc( this );

    dc.SetBackground( wxBrush( GetParent()->GetBackgroundColour() ) );
    dc.Clear();

    if( !m_state.bound )
        return;

    // Grey until the backend has answered, so a guess never shows in a lifecycle color
    KIGFX::COLOR4D fill = m_state.phase == MDW_CONNECTION::ONLINE && m_state.known
                                  ? MdwCategoryColor( m_state.category )
                                  : KIGFX::COLOR4D( 0.5, 0.5, 0.5, 1.0 );

    // Another user's claim outranks the lifecycle color, as the label names the claim instead
    if( m_state.phase == MDW_CONNECTION::ONLINE && m_state.known && m_state.lock.is_locked
        && !m_state.lock.locked_by_me )
    {
        fill = KIGFX::COLOR4D( 0.95, 0.65, 0.1, 1.0 );
    }

    wxColour fillColour = fill.ToColour();

    wxColour textColour = fill.GetBrightness() > 0.5 ? *wxBLACK : *wxWHITE;

    wxRect area = GetClientRect();
    area.Deflate( 1, 1 );

    dc.SetBrush( wxBrush( fillColour ) );
    dc.SetPen( wxPen( fillColour ) );
    dc.DrawRoundedRectangle( area, area.GetHeight() / 2 );

    dc.SetTextForeground( textColour );
    dc.DrawLabel( LabelFor( m_state, wxDateTime::Now() ), area, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );

    if( HasFocus() )
        wxRendererNative::Get().DrawFocusRect( this, dc, area.Deflate( 2, 2 ) );
}

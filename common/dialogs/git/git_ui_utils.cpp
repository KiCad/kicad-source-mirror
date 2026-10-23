/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dialogs/git/git_ui_utils.h>

#include <algorithm>

#include <wx/button.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/string.h>
#include <wx/splitter.h>
#include <wx/utils.h>

#include <confirm.h>
#include <diff_merge/diff_scene.h>
#include <git/kicad_git_common.h>
#include <kiplatform/ui.h>


void KIGIT::ShowBusyMessage( wxWindow* aParent )
{
    DisplayInfoMessage( aParent, BusyMessage() );
}


std::unique_lock<std::mutex> KIGIT::TryLockAction( KIGIT_COMMON& aCommon, wxWindow* aParent )
{
    std::unique_lock<std::mutex> lock = aCommon.TryLockAction();

    if( !lock.owns_lock() )
        ShowBusyMessage( aParent );

    return lock;
}


std::unique_lock<std::mutex> KIGIT::WaitForActionLock( KIGIT_COMMON& aCommon, wxWindow* aParent )
{
    std::unique_lock<std::mutex> lock;

    {
        wxBusyCursor busy;
        lock = aCommon.TryLockActionFor( KIGIT_COMMON::ACTION_LOCK_WAIT );
    }

    if( !lock.owns_lock() )
        ShowBusyMessage( aParent );

    return lock;
}


void KIGIT::SortBranchesForDisplay( std::vector<KIGIT_BRANCH_INFO>& aBranches )
{
    std::sort( aBranches.begin(), aBranches.end(),
               []( const KIGIT_BRANCH_INFO& aLeft, const KIGIT_BRANCH_INFO& aRight )
               {
                   if( aLeft.isCurrent != aRight.isCurrent )
                       return aLeft.isCurrent;

                   if( aLeft.isRemote != aRight.isRemote )
                       return !aLeft.isRemote;

                   if( aLeft.when.IsValid() != aRight.when.IsValid() )
                       return aLeft.when.IsValid();

                   if( aLeft.when.IsValid() && aLeft.when != aRight.when )
                       return aLeft.when > aRight.when;

                   return aLeft.name < aRight.name;
               } );
}


wxString KIGIT::RelativeDate( const wxDateTime& aWhen )
{
    return RelativeDate( aWhen, wxDateTime::Now() );
}


wxString KIGIT::RelativeDate( const wxDateTime& aWhen, const wxDateTime& aNow )
{
    if( !aWhen.IsValid() )
        return wxEmptyString;

    wxTimeSpan diff = aNow - aWhen;

    if( diff.GetSeconds() < 60 )
        return _( "just now" );

    if( int minutes = diff.GetMinutes(); minutes < 60 )
        return wxString::Format( wxPLURAL( "%d minute ago", "%d minutes ago", minutes ), minutes );

    if( int hours = diff.GetHours(); hours < 24 )
        return wxString::Format( wxPLURAL( "%d hour ago", "%d hours ago", hours ), hours );

    if( int days = diff.GetDays(); days < 30 )
        return wxString::Format( wxPLURAL( "%d day ago", "%d days ago", days ), days );

    return aWhen.FormatISODate();
}


void KIGIT::UnsplitKeepingSash( wxSplitterWindow* aSplitter, wxWindow* aPane, int& aSashPos )
{
    if( !aSplitter->IsSplit() )
        return;

    aSashPos = aSplitter->GetSashPosition();
    aSplitter->Unsplit( aPane );
}


// Markup is what reaches the native button title on every platform, where a foreground colour
// does not.
void KIGIT::MarkDestructive( wxButton* aButton )
{
    const KICAD_DIFF::DIFF_COLOR_THEME theme = KICAD_DIFF::DIFF_COLOR_THEME::ForText( KIPLATFORM::UI::IsDarkTheme() );
    wxColour                           colour = theme.removed.ToColour();

    aButton->SetLabelMarkup( wxString::Format( wxS( "<span foreground='%s'>%s</span>" ),
                                               colour.GetAsString( wxC2S_HTML_SYNTAX ), aButton->GetLabel() ) );
}


// Git records the address without checking it, so this only catches a typo before every
// commit carries it. A local part, one at sign, and a domain with a dot inside it.
static bool looksLikeAnEmailAddress( const wxString& aEmail )
{
    const size_t at = aEmail.find( '@' );

    if( at == wxString::npos || at == 0 || aEmail.find( '@', at + 1 ) != wxString::npos )
        return false;

    const wxString domain = aEmail.Mid( at + 1 );
    const size_t   dot = domain.find( '.' );

    if( dot == wxString::npos || dot == 0 || dot + 1 == domain.length() )
        return false;

    for( const wxUniChar& c : aEmail )
    {
        if( wxIsspace( c ) )
            return false;
    }

    return true;
}


bool KIGIT::ValidateIdentity( const wxString& aName, const wxString& aEmail, wxString& aError )
{
    wxString name = aName;

    if( name.Trim( true ).Trim( false ).IsEmpty() )
    {
        aError = _( "Enter your name. It is recorded as the author of your commits." );
        return false;
    }

    if( !looksLikeAnEmailAddress( aEmail ) )
    {
        aError = _( "Enter a valid email address. It is recorded as the author of your commits." );
        return false;
    }

    return true;
}


wxString KIGIT::SelectedText( wxListCtrl* aList, int aColumn )
{
    long sel = aList->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( sel == -1 )
        return wxEmptyString;

    return aList->GetItemText( sel, aColumn );
}

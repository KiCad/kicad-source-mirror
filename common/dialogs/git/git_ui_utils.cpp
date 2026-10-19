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

#include <wx/button.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#include <wx/listctrl.h>
#include <wx/string.h>
#include <wx/splitter.h>

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


wxString KIGIT::RelativeDate( const wxDateTime& aWhen )
{
    if( !aWhen.IsValid() )
        return wxEmptyString;

    wxTimeSpan diff = wxDateTime::Now() - aWhen;

    if( diff.GetSeconds() < 60 )
        return _( "just now" );

    if( diff.GetMinutes() < 60 )
        return wxString::Format( _( "%d minutes ago" ), static_cast<int>( diff.GetMinutes() ) );

    if( diff.GetHours() < 24 )
        return wxString::Format( _( "%d hours ago" ), static_cast<int>( diff.GetHours() ) );

    if( diff.GetDays() < 30 )
        return wxString::Format( _( "%d days ago" ), static_cast<int>( diff.GetDays() ) );

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
    KICAD_DIFF::DIFF_COLOR_THEME theme;
    wxColour colour = ( KIPLATFORM::UI::IsDarkTheme() ? theme.removed.Brightened( 0.4 ) : theme.removed ).ToColour();

    aButton->SetLabelMarkup( wxString::Format( wxS( "<span foreground='%s'>%s</span>" ),
                                               colour.GetAsString( wxC2S_HTML_SYNTAX ), aButton->GetLabel() ) );
}


bool KIGIT::ValidateIdentity( const wxString& aName, const wxString& aEmail, wxString& aError )
{
    if( aName.IsEmpty() )
    {
        aError = _( "Enter your name. It is recorded as the author of your commits." );
        return false;
    }

    if( !aEmail.Contains( wxS( "@" ) ) )
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

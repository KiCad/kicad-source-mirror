/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_mdw_library_checkin.h>

#include <confirm.h>
#include <dialogs/dialog_library_source_setup.h>
#include <kidialog.h>
#include <kiface_services.h>
#include <kiway.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <mdw/client/mdw_types.h>
#include <pgm_base.h>
#include <widgets/mdw_member_index.h>
#include <widgets/ui_common.h>

#include <wx/button.h>
#include <wx/filename.h>
#include <wx/intl.h>

#include <algorithm>
#include <memory>
#include <optional>


namespace
{

enum COLUMN
{
    COL_TICK = 0,
    COL_LIBRARY,
    COL_NAME,
    COL_CHANGE
};

std::optional<LIBRARY_TABLE_ROW*> teamRow( LIBRARY_TABLE_TYPE aType, const wxString& aNickname )
{
    if( !MDW_LIBRARY_SOURCES::TeamLibrariesEnabled() || aNickname.IsEmpty() )
        return std::nullopt;

    std::optional<LIBRARY_TABLE_ROW*> row = Pgm().GetLibraryManager().GetRow( aType, aNickname );

    if( !row || !*row || MDW_LIBRARY_SOURCES::MarkerOf( ( *row )->Options() ).IsEmpty() )
        return std::nullopt;

    return row;
}

} // namespace


wxString MdwTeamLibraryMarker( LIBRARY_TABLE_TYPE aType, const wxString& aNickname )
{
    std::optional<LIBRARY_TABLE_ROW*> row = teamRow( aType, aNickname );

    return row ? MDW_LIBRARY_SOURCES::MarkerOf( ( *row )->Options() ) : wxString();
}


bool MdwCanCheckInLibrary( LIBRARY_TABLE_TYPE aType, const wxString& aNickname )
{
    wxString marker = MdwTeamLibraryMarker( aType, aNickname );

    if( marker.IsEmpty() )
        return false;

    // An unpinned copy is marked with its source's id, which a per-project binding row never matches
    return MDW_LIBRARY_SOURCES::IsPinned( marker ) || MDW_LIBRARY_SOURCES::Get().Find( marker ).has_value();
}


bool MdwRefusePinnedCheckIn( wxWindow* aParent, LIBRARY_TABLE_TYPE aType, const wxString& aNickname )
{
    if( !MDW_LIBRARY_SOURCES::IsPinned( MdwTeamLibraryMarker( aType, aNickname ) ) )
        return false;

    DisplayInfoMessage( aParent, _( "Pinned copies are snapshots and cannot be checked in." ) );
    return true;
}


void ShowTeamLibraryCheckIn( wxWindow* aParent, KIWAY& aKiway, LIBRARY_TABLE_TYPE aType, const wxString& aNickname )
{
    std::optional<LIBRARY_TABLE_ROW*> tableRow = teamRow( aType, aNickname );

    if( !tableRow )
        return;

    wxString marker = MDW_LIBRARY_SOURCES::MarkerOf( ( *tableRow )->Options() );

    if( MdwRefusePinnedCheckIn( aParent, aType, aNickname ) )
        return;

    const MDW_LIBRARY_TYPE type =
            aType == LIBRARY_TABLE_TYPE::SYMBOL ? MDW_LIBRARY_TYPE::SYMBOL : MDW_LIBRARY_TYPE::FOOTPRINT;

    MDW_LIBRARY_SOURCES&           sources = MDW_LIBRARY_SOURCES::Get();
    std::optional<MDW_LIBRARY_ROW> row = sources.ForRowAt( type, ( *tableRow )->Options(),
                                                           LIBRARY_MANAGER::GetFullURI( *tableRow, true ) );
    std::optional<MDW_LIBRARY_SOURCE_CONFIG> config;
    std::optional<MDW_LIBRARY_COPY>          copy;

    if( row )
    {
        config = sources.Find( row->source_id );
        copy = sources.Copy( marker );
    }

    if( !row || !config || !copy )
    {
        wxString msg = wxString::Format( _( "'%s' is not set up as a team library on this computer." ), aNickname );
        DisplayErrorMessage( aParent, msg );
        return;
    }

    // A refresh re-reads the copy a sync moved, and the check-in must then use that one
    auto current = std::make_shared<MDW_LIBRARY_COPY>( *copy );

    KIGIT::LIBRARY_DIFF diffSymbols =
            GetKifaceService<KIFACE_DIFF_SYMBOL_LIBRARY_TEXT_SERVICE>( aKiway.KiFACE( KIWAY::FACE_SCH ) );

    auto list =
            [current, marker, diffSymbols, libraryId = row->library_id]() -> MDW_CHECKIN_LISTING
            {
                std::optional<MDW_LIBRARY_COPY> latest = MDW_LIBRARY_SOURCES::Get().Copy( marker );

                if( !latest )
                    return { MDW_RESULT::Fail( MDW_ERROR_CODE::ITEM_NOT_FOUND ), {} };

                *current = std::move( *latest );

                MDW_LIBRARY_CHANGES_RESULT changes = MDW_SETUP::LibraryChangesFor( *current, diffSymbols );

                if( !changes.result.IsOk() )
                    return { changes.result, {} };

                return { changes.result, MdwCheckInRows( changes.changes, *current, libraryId ) };
            };

    auto checkIn =
            [current]( const std::vector<MDW_LOCAL_CHANGE>& aChanges, const wxString& aComment )
            {
                return MDW_SETUP::CheckInLibraryCopy( *current, aChanges, aComment );
            };

    auto signIn =
            [requirement = MDW_LIBRARY_SOURCES::RequirementFor( *config )]( wxWindow* aDialogParent )
            {
                // Library credentials are kept per source, not under the project's workspace
                DIALOG_LIBRARY_SOURCE_SETUP dlg( aDialogParent, requirement );
                return dlg.ShowModal() == wxID_OK;
            };

    MDW_CHECKIN_LISTING listing =
            RunWithProgress<MDW_CHECKIN_LISTING>( aParent, _( "Reading local changes..." ), list );
    bool signInNeeded = listing.result.error == MDW_ERROR_CODE::NOT_AUTHENTICATED;

    if( listing.result.error == MDW_ERROR_CODE::CANCELLED )
        return;

    if( !listing.result.IsOk() && !signInNeeded )
    {
        DisplayErrorMessage( aParent, wxString::Format( _( "Could not read the changes to '%s'." ), config->name ),
                             MdwErrorText( listing.result ) );
        return;
    }

    if( listing.result.IsOk() && listing.rows.empty() )
    {
        DisplayInfoMessage( aParent, wxString::Format( _( "'%s' has no changes to check in." ), config->name ) );
        return;
    }

    auto getUpdates =
            [&aKiway, config = *config]( wxWindow* aDialogParent )
            {
                UpdateLibrarySources( aDialogParent, aKiway, &aKiway.Prj(), { config } );
            };

    DIALOG_MDW_LIBRARY_CHECKIN dlg( aParent, config->name, std::move( listing ), list, checkIn, signIn, getUpdates );

    dlg.ShowModal();
}


void SaveAndCheckInTeamLibrary( wxWindow* aParent, KIWAY& aKiway, LIBRARY_TABLE_TYPE aType, const wxString& aNickname,
                                bool aModified, const std::function<bool()>& aSave )
{
    if( aNickname.IsEmpty() || MdwRefusePinnedCheckIn( aParent, aType, aNickname ) )
        return;

    // A check-in reads the files, so edits still in the editor would be left out
    if( aModified )
    {
        KIDIALOG dlg( aParent, wxString::Format( _( "Save the changes to '%s' before checking in?" ), aNickname ),
                      _( "Check In" ), wxOK | wxCANCEL | wxICON_QUESTION );
        dlg.SetOKLabel( _( "Save" ) );

        if( dlg.ShowModal() != wxID_OK )
            return;

        // The check-in follows at once, so a notice that the save stays local would only mislead
        MDW_SILENT_SAVE quiet;

        if( !aSave() )
        {
            DisplayErrorMessage( aParent, wxString::Format( _( "Could not save '%s'." ), aNickname ) );
            return;
        }
    }

    ShowTeamLibraryCheckIn( aParent, aKiway, aType, aNickname );
}


void UpdateTeamLibrarySource( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject, LIBRARY_TABLE_TYPE aType,
                              const wxString& aNickname )
{
    std::optional<LIBRARY_TABLE_ROW*> tableRow = teamRow( aType, aNickname );

    if( !tableRow )
        return;

    const MDW_LIBRARY_TYPE type =
            aType == LIBRARY_TABLE_TYPE::SYMBOL ? MDW_LIBRARY_TYPE::SYMBOL : MDW_LIBRARY_TYPE::FOOTPRINT;

    MDW_LIBRARY_SOURCES&           sources = MDW_LIBRARY_SOURCES::Get();
    std::optional<MDW_LIBRARY_ROW> row =
            sources.ForRowAt( type, ( *tableRow )->Options(), LIBRARY_MANAGER::GetFullURI( *tableRow, true ) );
    std::optional<MDW_LIBRARY_SOURCE_CONFIG> source = row ? sources.Find( row->source_id ) : std::nullopt;

    if( !source )
    {
        DisplayErrorMessage(
                aParent, wxString::Format( _( "'%s' is not set up as a team library on this computer." ), aNickname ) );
        return;
    }

    UpdateLibrarySources( aParent, aKiway, aProject, { *source } );
}


bool MdwCanUpgradeLibraryFormat( const wxString& aNickname, long aVersion )
{
    std::optional<LIBRARY_TABLE_ROW*> row = teamRow( LIBRARY_TABLE_TYPE::SYMBOL, aNickname );
    const wxString                    marker = row ? MDW_LIBRARY_SOURCES::MarkerOf( ( *row )->Options() ) : wxString();
    std::optional<MDW_LIBRARY_SOURCE_CONFIG> source;

    if( !marker.IsEmpty() && !MDW_LIBRARY_SOURCES::IsPinned( marker ) )
        source = MDW_LIBRARY_SOURCES::Get().Find( marker );

    if( !source || MDW_SETUP::CheckInKnownDenied( source->locator ) )
        return false;

    return !MDW_SETUP::OutdatedSymbolFiles( LIBRARY_MANAGER::GetFullURI( *row, true ), aVersion ).empty();
}


bool UpgradeTeamLibraryFormat( wxWindow* aParent, KIWAY& aKiway, const wxString& aNickname, long aVersion,
                               const std::function<bool( const wxString&, const wxString& )>& aSave )
{
    std::optional<LIBRARY_TABLE_ROW*> tableRow = teamRow( LIBRARY_TABLE_TYPE::SYMBOL, aNickname );
    const wxString marker = tableRow ? MDW_LIBRARY_SOURCES::MarkerOf( ( *tableRow )->Options() ) : wxString();
    std::optional<MDW_LIBRARY_COPY> copy = marker.IsEmpty() ? std::nullopt : MDW_LIBRARY_SOURCES::Get().Copy( marker );

    if( !copy || copy->pinned )
    {
        DisplayErrorMessage(
                aParent, wxString::Format( _( "'%s' is not set up as a team library on this computer." ), aNickname ) );
        return false;
    }

    KIDIALOG dlg( aParent,
                  wxString::Format( _( "Save every symbol of '%s' in the current file format, and check that in for "
                                       "the whole team?" ),
                                    aNickname ),
                  _( "Upgrade Team Library Format" ), wxOK | wxCANCEL | wxICON_WARNING );
    dlg.SetExtendedMessage( _( "KiCad versions older than this one cannot read the upgraded files. Teammates take "
                               "the upgrade in with Get Library Updates. No symbol is revised, and released symbols "
                               "stay released." ) );
    dlg.SetOKLabel( _( "Upgrade" ) );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    const wxString path = LIBRARY_MANAGER::GetFullURI( *tableRow, true );
    wxString       error;

    // Saved aside here, since the save and the symbol comparison both run editor code the UI thread owns
    std::optional<MDW_FORMAT_UPGRADE> staged = MDW_SETUP::StageFormatUpgrade(
            path,
            [&]( const wxString& aOutput )
            {
                return aSave( path, aOutput );
            },
            error );

    KIGIT::LIBRARY_FILES_DIFF diffSymbols =
            GetKifaceService<KIFACE_DIFF_SYMBOL_LIBRARY_FILES_SERVICE>( aKiway.KiFACE( KIWAY::FACE_SCH ) );
    MDW_RESULT checked = staged ? MDW_SETUP::CheckFormatUpgrade( *staged, diffSymbols )
                                : MDW_RESULT::Fail( MDW_ERROR_CODE::INTERNAL, error );

    if( !checked.IsOk() )
    {
        DisplayErrorMessage( aParent, wxString::Format( _( "'%s' was not upgraded." ), aNickname ),
                             MdwErrorText( checked ) );
        return false;
    }

    // Named by file rather than nickname, which differs between teammates, and never translated, as it is shared
    const wxString comment =
            wxString::Format( wxS( "Upgrade %s to file format %ld" ), wxFileName( path ).GetFullName(), aVersion );

    MDW_LIBRARY_CHECKIN_RESULT upgraded = RunWithProgress<MDW_LIBRARY_CHECKIN_RESULT>(
            aParent, _( "Checking in the upgraded library..." ),
            [&]()
            {
                return MDW_SETUP::CheckInFormatUpgrade( *copy, *staged, aVersion, comment );
            } );

    if( upgraded.result.error == MDW_ERROR_CODE::CANCELLED )
        return true;

    if( !upgraded.result.IsOk() )
    {
        DisplayErrorMessage( aParent, wxString::Format( _( "'%s' was not upgraded." ), aNickname ),
                             MdwErrorText( upgraded.result ) );
    }
    else if( upgraded.checkin.copy_snapshot.token.IsEmpty() )
    {
        DisplayInfoMessage( aParent, wxString::Format( _( "'%s' already uses the current file format." ), aNickname ) );
    }
    else
    {
        DisplayInfoMessage( aParent, wxString::Format( _( "'%s' now uses the current file format, and the upgrade is "
                                                          "checked in for the team." ),
                                                       aNickname ) );
    }

    return true;
}


DIALOG_MDW_LIBRARY_CHECKIN::DIALOG_MDW_LIBRARY_CHECKIN( wxWindow* aParent, const wxString& aSourceName,
                                                        MDW_CHECKIN_LISTING aListing, LIST aList,
                                                        CHECK_IN aCheckIn, SIGN_IN aSignIn, GET_UPDATES aGetUpdates ) :
        DIALOG_MDW_LIBRARY_CHECKIN_BASE( aParent ),
        m_rows( std::move( aListing.rows ) ),
        m_list( std::move( aList ) ),
        m_checkIn( std::move( aCheckIn ) ),
        m_signIn( std::move( aSignIn ) ),
        m_getUpdates( std::move( aGetUpdates ) )
{
    SetTitle( wxString::Format( _( "Check In to %s" ), aSourceName ) );
    m_note->SetFont( KIUI::GetInfoFont( this ) );

    showChecklist();

    // A restored comment would describe the previous check-in
    OptOut( m_comment );

    m_changesList->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DIALOG_MDW_LIBRARY_CHECKIN::onToggled, this );
    Bind( wxEVT_BUTTON, &DIALOG_MDW_LIBRARY_CHECKIN::onCheckIn, this, wxID_OK );

    fillChanges();

    SetupStandardButtons( { { wxID_OK, _( "Check In" ) } } );
    updateCheckInButton();

    // Sized with the list shown, so hiding it for a sign-in cannot leave the dialog too narrow. A saved size
    // would undo the fit to the dialog's state
    m_useCalculatedSize = true;
    finishDialogSettings();
    m_listWidth = GetSize().x;

    if( !aListing.result.IsOk() )
    {
        needsSignIn( aListing.result );
        updateCheckInButton();
    }
}


void DIALOG_MDW_LIBRARY_CHECKIN::fitToState()
{
    Layout();

    wxSize fit = GetSizer()->ComputeFittingWindowSize( this );

    SetMinSize( wxSize( m_listWidth, fit.y ) );
    SetSize( wxSize( std::max( GetSize().x, m_listWidth ), fit.y ) );
}


void DIALOG_MDW_LIBRARY_CHECKIN::showChecklist()
{
    m_changesList->DeleteAllItems();
    m_changesList->ClearColumns();
    m_changesList->AppendToggleColumn( wxEmptyString, wxDATAVIEW_CELL_ACTIVATABLE, FromDIP( 30 ) );
    m_changesList->AppendTextColumn( _( "Library" ), wxDATAVIEW_CELL_INERT, FromDIP( 160 ) );
    m_changesList->AppendTextColumn( _( "Name" ), wxDATAVIEW_CELL_INERT, FromDIP( 260 ) );
    m_changesList->AppendTextColumn( _( "Change" ), wxDATAVIEW_CELL_INERT, FromDIP( 160 ) );

    m_intro->SetLabel( _( "Local changes not yet checked in:" ) );
    m_intro->Show();
    m_changesList->Show();
    m_commentLabel->Show();
    m_comment->Show();
}


void DIALOG_MDW_LIBRARY_CHECKIN::fillChanges()
{
    m_changesList->DeleteAllItems();

    bool     held = false;
    wxString warning;

    for( const MDW_CHECKIN_ROW& row : m_rows )
    {
        wxString change = MdwChangeText( row.change.change );

        if( row.change.moved )
            change = wxString::Format( _( "Moved to %s" ), wxFileName( row.change.relative_path ).GetFullName() );
        else if( !row.change.format_digest.IsEmpty() )
            change = _( "Formatting only" );

        if( !row.allowed )
        {
            wxString reason = wxString::FromUTF8( row.change.checkin.reason() );

            change = wxString::Format( wxS( "%s (%s)" ), change,
                                       reason.IsEmpty() ? wxString( _( "not accepted here" ) ) : reason );
            held = true;
        }
        else if( warning.IsEmpty() )
        {
            warning = wxString::FromUTF8( row.change.checkin.reason() );
        }

        wxVector<wxVariant> values;
        values.push_back( wxVariant( row.ticked ) );
        values.push_back( wxVariant( row.library ) );
        values.push_back( wxVariant( row.change.name ) );
        values.push_back( wxVariant( change ) );
        m_changesList->AppendItem( values );
    }

    SetWrappedStatus( this, m_note,
                      held ? _( "Changes the library does not accept here stay in your copy." ) : warning );
}


void DIALOG_MDW_LIBRARY_CHECKIN::onToggled( wxDataViewEvent& aEvent )
{
    int row = m_changesList->ItemToRow( aEvent.GetItem() );

    if( m_done || row < 0 || row >= static_cast<int>( m_rows.size() ) || aEvent.GetColumn() != COL_TICK )
        return;

    // The list cannot disable one row's box, so a change it may not take is set back
    bool ticked = m_changesList->GetToggleValue( row, COL_TICK );

    if( !m_rows[row].allowed && ticked )
    {
        m_changesList->SetToggleValue( false, row, COL_TICK );
        ticked = false;
    }

    m_rows[row].ticked = ticked;
    updateCheckInButton();
}


void DIALOG_MDW_LIBRARY_CHECKIN::updateCheckInButton()
{
    switch( m_next )
    {
    case NEXT::REFRESH:
        m_sdbSizerOK->SetLabel( _( "Refresh" ) );
        m_sdbSizerOK->Enable( !m_done );
        break;

    case NEXT::SIGN_IN:
        m_sdbSizerOK->SetLabel( _( "Sign In..." ) );
        m_sdbSizerOK->Enable( !m_done && m_signIn != nullptr );
        break;

    case NEXT::CHECK_IN:
        m_sdbSizerOK->SetLabel( _( "Check In" ) );
        m_sdbSizerOK->Enable( !m_done && !MdwTickedChanges( m_rows ).empty() );
        break;

    case NEXT::UPDATE_AND_RETRY:
        m_sdbSizerOK->SetLabel( _( "Get Library Updates and Check In Again" ) );
        m_sdbSizerOK->Enable( !m_done );
        break;

    case NEXT::RESOLVE:
        m_sdbSizerOK->SetLabel( _( "Get Library Updates" ) );
        m_sdbSizerOK->Enable( !m_done );
        break;
    }

    Layout();
}


void DIALOG_MDW_LIBRARY_CHECKIN::onCheckIn( wxCommandEvent& aEvent )
{
    if( m_done )
        return;

    m_sdbSizerOK->Disable();

    switch( m_next )
    {
    case NEXT::REFRESH: refresh(); break;
    case NEXT::SIGN_IN: signIn(); break;
    case NEXT::CHECK_IN: checkIn(); break;
    case NEXT::UPDATE_AND_RETRY: updateAndRetry(); break;
    case NEXT::RESOLVE: resolve(); break;
    }
}


void DIALOG_MDW_LIBRARY_CHECKIN::updateAndRetry()
{
    // Once, so a copy that stays behind is reported rather than retried without end
    m_retried = true;
    m_next = NEXT::CHECK_IN;

    // Every exit short of the results starts from the check-in layout
    showChecklist();
    fillChanges();
    m_getUpdates( this );

    MDW_CHECKIN_LISTING listing = RunWithProgress<MDW_CHECKIN_LISTING>( this, _( "Reading local changes..." ), m_list );

    if( !listing.result.IsOk() )
    {
        SetWrappedStatus( this, m_status,
                          wxString::Format( _( "Could not read the changes: %s" ), MdwErrorText( listing.result ) ) );
        m_next = NEXT::REFRESH;
        updateCheckInButton();
        return;
    }

    MdwCarryTicks( listing.rows, m_rows );
    m_rows = std::move( listing.rows );
    fillChanges();
    checkIn();
}


void DIALOG_MDW_LIBRARY_CHECKIN::resolve()
{
    m_getUpdates( this );
    EndModal( wxID_CANCEL );
}


void DIALOG_MDW_LIBRARY_CHECKIN::checkIn()
{
    std::vector<MDW_LOCAL_CHANGE> changes = MdwTickedChanges( m_rows );

    if( changes.empty() )
    {
        updateCheckInButton();
        return;
    }

    auto work =
            [checkIn = m_checkIn, changes, comment = m_comment->GetValue()]()
            {
                return checkIn( changes, comment );
            };

    MDW_LIBRARY_CHECKIN_RESULT result =
            RunWithProgress<MDW_LIBRARY_CHECKIN_RESULT>( this, _( "Checking in..." ), work );

    // The server may have taken the check-in before the cancel reached it, so only a fresh list can tell
    if( result.result.error == MDW_ERROR_CODE::CANCELLED )
    {
        m_next = NEXT::REFRESH;
        updateCheckInButton();
        return;
    }

    // Members and files the backend took stay reported even when the copy then failed to record them
    if( !result.result.IsOk()
        && ( !result.checkin.results.empty() || !MdwAdoptedFiles( changes, result.checkin ).empty() ) )
    {
        showResults(
                result.checkin, changes,
                wxString::Format( _( "The copy could not record this check-in: %s" ), MdwErrorText( result.result ) ) );
        return;
    }

    if( !result.result.IsOk() )
    {
        if( needsSignIn( result.result ) )
        {
            updateCheckInButton();
            return;
        }

        // A sync moved the copy under this list, so only a fresh list can be checked in
        if( result.result.error == MDW_ERROR_CODE::VERSION_CONFLICT )
            m_next = NEXT::REFRESH;

        SetWrappedStatus( this, m_status,
                          wxString::Format( _( "Nothing was checked in: %s" ), MdwErrorText( result.result ) ) );
        updateCheckInButton();
        return;
    }

    showResults( result.checkin, changes );
}


MDW_RESULT DIALOG_MDW_LIBRARY_CHECKIN::refresh()
{
    MDW_CHECKIN_LISTING listing = RunWithProgress<MDW_CHECKIN_LISTING>( this, _( "Reading local changes..." ), m_list );

    if( !listing.result.IsOk() )
    {
        if( listing.result.error != MDW_ERROR_CODE::CANCELLED && !needsSignIn( listing.result ) )
        {
            wxString msg = wxString::Format( _( "Could not read the changes: %s" ), MdwErrorText( listing.result ) );
            SetWrappedStatus( this, m_status, msg );
        }

        updateCheckInButton();
        return listing.result;
    }

    MdwCarryTicks( listing.rows, m_rows );
    m_rows = std::move( listing.rows );
    m_next = NEXT::CHECK_IN;
    m_changesList->Show();
    m_intro->Show();

    fillChanges();
    SetWrappedStatus( this, m_status, m_rows.empty() ? _( "No changes are left to check in." ) : wxString() );
    updateCheckInButton();
    fitToState();
    return listing.result;
}


void DIALOG_MDW_LIBRARY_CHECKIN::signIn()
{
    if( !m_signIn || !m_signIn( this ) )
    {
        updateCheckInButton();
        return;
    }

    // The setup dialog only stores the credentials, so listing again is what logs in
    MDW_RESULT relisted = refresh();

    if( relisted.error != MDW_ERROR_CODE::NOT_AUTHENTICATED )
        return;

    SetWrappedStatus( this, m_status, _( "The sign-in was not accepted." ) + wxS( " " ) + MdwErrorMessage( relisted ) );
}


bool DIALOG_MDW_LIBRARY_CHECKIN::needsSignIn( const MDW_RESULT& aResult )
{
    if( aResult.error != MDW_ERROR_CODE::NOT_AUTHENTICATED )
        return false;

    // Without a login the source names no candidates, so the list would only mislead. The rows keep their ticks
    // for the refresh after signing in
    m_changesList->DeleteAllItems();
    m_changesList->Hide();
    m_intro->Hide();
    SetWrappedStatus( this, m_note, wxString() );
    m_next = NEXT::SIGN_IN;

    SetWrappedStatus( this, m_status, MdwErrorMessage( aResult ) );

    fitToState();
    return true;
}


void DIALOG_MDW_LIBRARY_CHECKIN::showResults( const MDW_LIBRARY_CHECKIN&           aCheckIn,
                                              const std::vector<MDW_LOCAL_CHANGE>& aSent, const wxString& aProblem )
{
    m_done = true;

    m_changesList->DeleteAllItems();
    m_changesList->ClearColumns();
    m_changesList->AppendTextColumn( _( "Library" ), wxDATAVIEW_CELL_INERT, FromDIP( 160 ) );
    m_changesList->AppendTextColumn( _( "Name" ), wxDATAVIEW_CELL_INERT, FromDIP( 220 ) );
    m_changesList->AppendTextColumn( _( "Result" ), wxDATAVIEW_CELL_INERT, FromDIP( 300 ) );

    int  refused = 0;
    bool behind = false;
    bool conflicted = false;

    for( const MDW_MEMBER_RESULT& result : aCheckIn.results )
    {
        behind = behind || result.status.error == MDW_ERROR_CODE::COPY_BEHIND;
        conflicted = conflicted || result.status.error == MDW_ERROR_CODE::VERSION_CONFLICT;

        wxString library = result.library_id;

        for( const MDW_CHECKIN_ROW& row : m_rows )
        {
            if( row.change.library_id == result.library_id )
            {
                library = row.library;
                break;
            }
        }

        if( !result.status.IsOk() )
            ++refused;

        wxVector<wxVariant> values;
        values.push_back( wxVariant( library ) );
        values.push_back( wxVariant( result.name ) );
        values.push_back( wxVariant( MdwMemberOutcome( result ) ) );
        m_changesList->AppendItem( values );
    }

    auto libraryOf = [&]( const wxString& aPath )
    {
        wxString library;

        for( const MDW_CHECKIN_ROW& row : m_rows )
        {
            if( row.change.relative_path == aPath )
                library = row.library;
        }

        return library;
    };

    const std::vector<wxString> adopted = MdwAdoptedFiles( aSent, aCheckIn );

    for( const wxString& path : adopted )
    {
        wxVector<wxVariant> values;
        values.push_back( wxVariant( libraryOf( path ) ) );
        values.push_back( wxVariant( path ) );
        values.push_back( wxVariant( _( "Checked in" ) ) );
        m_changesList->AppendItem( values );
    }

    for( const MDW_SKIPPED_FILE& skipped : aCheckIn.skipped_files )
    {
        const wxString library = libraryOf( skipped.path );

        wxVector<wxVariant> values;
        values.push_back( wxVariant( library ) );
        values.push_back( wxVariant( skipped.path ) );
        values.push_back( wxVariant( wxString::Format( _( "Not checked in. %s" ), skipped.reason ) ) );
        m_changesList->AppendItem( values );
    }

    m_intro->SetLabel( _( "Check-in results:" ) );
    m_note->SetLabel( wxEmptyString );
    m_commentLabel->Hide();
    m_comment->Hide();

    int accepted = static_cast<int>( aCheckIn.results.size() + adopted.size() ) - refused;

    refused += static_cast<int>( aCheckIn.skipped_files.size() );

    wxString status = refused == 0 ? wxString::Format( _( "%d checked in." ), accepted )
                                   : wxString::Format( _( "%d checked in, %d refused." ), accepted, refused );

    if( !aProblem.IsEmpty() )
        status << wxS( "\n" ) << aProblem;

    SetWrappedStatus( this, m_status, status );

    // A copy that is behind is brought up to date once, and a conflicting edit is resolved
    const bool offer = m_getUpdates && ( ( behind && !m_retried ) || conflicted );

    if( offer )
    {
        m_next = behind && !m_retried ? NEXT::UPDATE_AND_RETRY : NEXT::RESOLVE;
        m_done = false;
        m_sdbSizerOK->Show();
        updateCheckInButton();
    }
    else
    {
        m_sdbSizerOK->Hide();
    }

    m_sdbSizerCancel->SetLabel( _( "Close" ) );
    m_sdbSizerCancel->SetFocus();
    Layout();
}

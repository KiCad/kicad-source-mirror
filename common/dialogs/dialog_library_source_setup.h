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

#ifndef DIALOG_LIBRARY_SOURCE_SETUP_H
#define DIALOG_LIBRARY_SOURCE_SETUP_H

#include <dialog_shim.h>
#include <libraries/library_table.h>
#include <mdw/client/mdw_library_sources.h>
#include <mdw/client/mdw_op.h>
#include <thread_pool.h>
#include <widgets/wx_progress_reporters.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <wx/app.h>

class KIWAY;
class MDW_SESSION;
struct MDW_PART_TARGET;
class PROJECT;
class wxChoice;
class wxDialog;
class wxStaticText;
class wxTextCtrl;


/**
 * Set up one library source for this user, with its backend, location, local folder, nickname prefix and
 * credentials. On OK the source is in the registry, or was already, and its credentials are stored; nothing
 * is synced yet.
 */
class DIALOG_LIBRARY_SOURCE_SETUP : public DIALOG_SHIM
{
public:
    /// Returns why the source was not set up, or empty when it was. The bool is whether OK added it.
    using SET_UP = std::function<wxString( wxWindow*, const MDW_LIBRARY_SOURCE_CONFIG&, bool )>;

    /**
     * With aRequirement the backend and location are the ones a project asked for. aProjectNeeds are the
     * sources the open project already requires, which a source at the same location must satisfy. OK runs
     * aSetUp, when given, and the dialog stays open showing its answer until the source is set up.
     */
    DIALOG_LIBRARY_SOURCE_SETUP( wxWindow* aParent, const std::optional<MDW_LIBRARY_REQUIREMENT>& aRequirement,
                                 const std::vector<MDW_LIBRARY_REQUIREMENT>& aProjectNeeds = {},
                                 SET_UP                                      aSetUp = nullptr );

    const MDW_LIBRARY_SOURCE_CONFIG& GetSource() const { return m_source; }

    /// Show aText in the dialog's status line
    void ShowStatus( const wxString& aText );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onTestConnection( wxCommandEvent& aEvent );

    MDW_LIBRARY_SOURCE_CONFIG collectSource() const;

    /// Swap aSource for the configured source it names, or refuse a name another source already uses
    bool resolveSource( MDW_LIBRARY_SOURCE_CONFIG& aSource );

    /// False, with the reason shown, when the project requires aSource's location under another backend or prefix
    bool servesProject( const MDW_LIBRARY_SOURCE_CONFIG& aSource );


    std::optional<MDW_LIBRARY_REQUIREMENT> m_requirement;
    std::vector<MDW_LIBRARY_REQUIREMENT>   m_projectNeeds;
    SET_UP                                 m_setUp;
    MDW_LIBRARY_SOURCE_CONFIG              m_source;

    wxChoice*     m_backendChoice;
    wxTextCtrl*   m_locatorCtrl;
    wxTextCtrl*   m_nameCtrl;
    wxTextCtrl*   m_prefixCtrl;
    wxTextCtrl*   m_usernameCtrl;
    wxTextCtrl*   m_passwordCtrl;
    wxStaticText* m_statusText;

    std::vector<wxString> m_backendIds; ///< Parallel to m_backendChoice entries
};


/**
 * Set up the library source of each of aRequirements, or with none ask for one. A requirement whose server has a
 * saved credential, or needs none, is set up without a dialog. The others open the dialog, which syncs the first
 * copy and registers its libraries on OK. A new source whose fetch fails or whose every library name is taken is
 * removed again with its copy.
 *
 * @return the id of the last source set up, or empty when none was.
 */
wxString SetUpLibrarySource( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                             const std::vector<MDW_LIBRARY_REQUIREMENT>& aRequirements = {} );

/// What a part saved over a newer version of its copy came to once settled
enum class MDW_OVERWRITE_ANSWER
{
    RELEASE,   ///< Nothing to settle, or the version on disk was kept, so it can be released
    REWRITTEN, ///< The file now holds the team library's version or a merge, to review before releasing
    CANCELLED  ///< Not settled, so it must not be released
};

/**
 * Before a save of aRow writes them, mark the parts aNames as going over team versions the editor never loaded.
 *
 * @return false, after telling the user, when the marks cannot be stored and the save must not happen.
 */
bool MarkOverwrittenParts( wxWindow* aParent, PROJECT& aProject, MDW_LIBRARY_TYPE aType, const LIBRARY_TABLE_ROW& aRow,
                           const std::vector<wxString>& aNames );

/**
 * Ask over aParent whether to save over the parts aNames, whose library file changed since the editor loaded them.
 *
 * @return true when the user agrees to replace those changes.
 */
bool ConfirmSaveOverChangedParts( wxWindow* aParent, const std::vector<wxString>& aNames );

/**
 * Before aTarget is released, settle it like an update conflict when a save wrote over its listed version unread.
 * The answer is written into aTarget's file and the copy is then updated with it kept, so the release does not
 * refuse the part for lacking the replaced version.
 */
MDW_OVERWRITE_ANSWER SettleOverwrittenPart( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                            const MDW_PART_TARGET& aTarget );

/// The sources set up here that serve aProject's needs, none while team libraries are turned off
std::vector<MDW_LIBRARY_SOURCE_CONFIG> ProjectLibrarySources( const PROJECT& aProject );

/**
 * Bring each of aSources up to date at every snapshot aProject's needs pin, or the latest when none does, then
 * register and reload what changed and offer to review placed parts that changed. Library syncs never nest, so this,
 * SetUpLibrarySource() and SettleOverwrittenPart() each refuse to start while another one is running.
 *
 * @param aAnswered conflicts the user settled beforehand.
 * @return CANCELLED when the user cancelled a conflict, otherwise the first failure, already reported.
 */
MDW_RESULT UpdateLibrarySources( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                 const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                 const std::vector<MDW_CONFLICT_RESOLUTION>&   aAnswered = {} );

/**
 * Apply aSync to a copy of the aScope table of aType, with the nicknames its nested tables hold, and save the
 * copy when aSync reports a change. The copy keeps the edit away from rows that background loads may be reading.
 *
 * @param aFailed set when the table could not be read or saved.
 * @return whether the table changed.
 */
bool SyncLibraryTable( LIBRARY_TABLE_TYPE aType, LIBRARY_TABLE_SCOPE aScope, bool& aFailed,
                       const std::function<bool( LIBRARY_TABLE&, const std::set<wxString>& )>& aSync );

/// Preload the reloaded library tables and have every open library editor reload them
void ReloadLibraryEditors( KIWAY& aKiway );

/// What RegisterLibrarySourceRows() left in the tables for its sources
struct MDW_ROWS_REGISTERED
{
    int                   rows = 0;         ///< The sources' rows in the global and project tables
    std::vector<wxString> conflicts;        ///< Their nicknames another global row holds
    std::vector<wxString> projectConflicts; ///< Their pinned nicknames another project row holds
};

/**
 * Make the global tables' rows of aSources match their unpinned copies on disk, and aProject's tables match the
 * pinned copies it needs. Reload the libraries when a row changed or, with aFilesChanged, when a copy's files did.
 * Names already taken are returned, not shown.
 */
MDW_ROWS_REGISTERED RegisterLibrarySourceRows( wxWindow* aParent, KIWAY& aKiway, PROJECT* aProject,
                                               const std::vector<MDW_LIBRARY_SOURCE_CONFIG>& aSources,
                                               bool                                          aFilesChanged = false );

/// Show aText in aStatus wrapped to its width, growing aDialog to fit but never shrinking it. Empty hides aStatus
void SetWrappedStatus( wxWindow* aDialog, wxStaticText* aStatus, const wxString& aText );

/**
 * Test a binding behind a progress dialog, signing in with aUsername and aPassword, and report it in aStatus. A
 * typed password is saved once the server accepts it.
 *
 * @param aKeep receives a passing test's running session, for the caller to shut down.
 * @param aKeyFile is the private key to sign in with, empty for the backend's default.
 * @return CANCELLED when aDialog closed meanwhile.
 */
MDW_RESULT ShowConnectionTest( wxWindow* aDialog, wxStaticText* aStatus, const wxString& aBackendId,
                               const wxString& aWorkspaceId, const wxString& aLocator, const wxString& aProjectPath,
                               wxTextCtrl* aUsername, wxTextCtrl* aPassword,
                               std::shared_ptr<MDW_SESSION>* aKeep = nullptr,
                               const wxString&               aKeyFile = wxEmptyString );

/**
 * ShowConnectionTest() for OK when aPassword holds a password, so it is saved only once the server accepts it.
 * An unreachable server does not hold up a binding that can sign in later.
 *
 * @return false, keeping the dialog open, when the server refused the password or aDialog closed.
 */
bool VerifyTypedLogin( wxWindow* aDialog, wxStaticText* aStatus, const wxString& aBackendId,
                       const wxString& aWorkspaceId, const wxString& aLocator, const wxString& aProjectPath,
                       wxTextCtrl* aUsername, wxTextCtrl* aPassword, const wxString& aKeyFile = wxEmptyString );

/// Fill an empty aUsername with the one the binding signs in to aRealm as, and hint at a saved password
void PrefillCredentials( const wxString& aRealm, const wxString& aBindingAccount, wxTextCtrl* aUsername,
                         wxTextCtrl* aPassword );


/**
 * Run aTask on the thread pool, keeping aParent responsive behind a progress dialog until it returns. With
 * aCancellable the user, or a quit, can stop it, and every MdwSync inside aTask then answers CANCELLED as
 * MDW_CANCEL_SCOPE describes. aTask must then return promptly, and the caller must not report the cancel as an error.
 */
template <typename T>
T RunWithProgress( wxWindow* aParent, const wxString& aTitle, std::function<T()> aTask, bool aCancellable = true )
{
    struct LATEST
    {
        std::mutex                  mutex;
        std::optional<MDW_PROGRESS> progress;
    };

    auto                 cancelled = std::make_shared<std::atomic<bool>>( false );
    auto                 latest = std::make_shared<LATEST>();
    WX_PROGRESS_REPORTER reporter( aParent, aTitle, 1, aCancellable ? PR_CAN_ABORT : PR_NO_ABORT );
    std::future<T>       result = GetKiCadThreadPool().submit_task(
            [cancelled, latest, task = std::move( aTask )]()
            {
                MDW_CANCEL_SCOPE   scope( cancelled );
                MDW_PROGRESS_SCOPE progress(
                        [latest]( const MDW_PROGRESS& aProgress )
                        {
                            std::lock_guard<std::mutex> lock( latest->mutex );
                            latest->progress = aProgress;
                        } );

                return task();
            } );

    // Progress arrives on a transport thread, so the dialog is touched only from this one
    auto show = [&]()
    {
        std::optional<MDW_PROGRESS> reported;

        {
            std::lock_guard<std::mutex> lock( latest->mutex );
            reported = std::exchange( latest->progress, std::nullopt );
        }

        if( !reported || cancelled->load() )
            return;

        reporter.SetIndeterminate( reported->total == 0 );

        if( reported->total != 0 )
            reporter.SetCurrentProgress( static_cast<double>( reported->done ) / reported->total );

        reporter.Report( MdwProgressText( *reported ) );
    };

    auto stop = [&]()
    {
        if( aCancellable && !cancelled->exchange( true ) )
            reporter.Report( _( "Stopping..." ) );
    };

    // Quitting under a running task would tear down what it uses, and a quit queued for later would land inside
    // the caller's next task, so a quit is refused and the user quits again once the dialog closes
    auto onQuit = [&]( wxCloseEvent& aEvent )
    {
        stop();

        if( aEvent.CanVeto() )
            aEvent.Veto();
        else
            aEvent.Skip();
    };

    auto onExit = [&]( wxCommandEvent& )
    {
        stop();
    };

    wxWindow* top = wxTheApp->GetTopWindow();

    // The disabled frames never see their quit key, so the dialog takes their accelerators
    if( top && top->GetAcceleratorTable()->IsOk() )
        reporter.SetAcceleratorTable( *top->GetAcceleratorTable() );

    wxTheApp->Bind( wxEVT_QUERY_END_SESSION, onQuit );
    wxTheApp->Bind( wxEVT_END_SESSION, onQuit );
    reporter.Bind( wxEVT_MENU, onExit, wxID_EXIT );

    // Until a backend reports a total, a gauge stuck at zero never advances the elapsed time
    reporter.SetIndeterminate( true );
    reporter.Report( aTitle );

    // Paint the message now, since a short task would end before the first refresh
    reporter.KeepRefreshing();

    while( result.wait_for( std::chrono::milliseconds( 100 ) ) != std::future_status::ready )
    {
        show();
        reporter.KeepRefreshing();

        // The dialog stays up until the task returns, so it can say why it has not closed
        if( reporter.WasCancelled() )
        {
            reporter.Resume();
            stop();
        }
    }

    wxTheApp->Unbind( wxEVT_QUERY_END_SESSION, onQuit );
    wxTheApp->Unbind( wxEVT_END_SESSION, onQuit );

    return result.get();
}

#endif // DIALOG_LIBRARY_SOURCE_SETUP_H

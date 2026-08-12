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

#include <widgets/mdw_save_prompt.h>

#include <kidialog.h>
#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_lifecycle.h>
#include <mdw/client/mdw_project_file.h>
#include <mdw/mdw_claim_text.h>
#include <trace_helpers.h>
#include <widgets/wx_infobar.h>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/intl.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>


namespace
{

/// Past this a save goes ahead unchecked
constexpr std::chrono::milliseconds SAVE_CHECK_TIMEOUT( 3000 );


bool askSave( wxWindow* aParent, const wxString& aMessage, const wxString& aSaveLabel )
{
    KIDIALOG dialog( aParent, aMessage, _( "Design Workspace" ), wxYES_NO | wxNO_DEFAULT | wxICON_WARNING );
    dialog.SetYesNoLabels( aSaveLabel, _( "Cancel" ) );

    return dialog.ShowModal() == wxID_YES;
}

} // namespace


bool MdwConfirmSave( wxWindow* aParent, WX_INFOBAR* aInfoBar, const PROJECT& aProject, const wxString& aLocalPath )
{
    using namespace MDW_SAVE_GUARD;

    // Without a workspace there is nothing to ask, so skip the blocking round trip to the client worker
    if( !MDW_CLIENT::Get().ProjectSource() )
        return true;

    struct WAIT
    {
        std::mutex              mutex;
        std::condition_variable cv;
        bool                    settled = false;
        MDW_SAVE_CHECK          check;
    };

    auto   wait = std::make_shared<WAIT>();
    auto   progress = std::make_shared<MDW_SAVE_PROGRESS>();
    MDW_OP op;

    {
        // Delivered on the client worker, so this thread may block on it
        MDW_SYNC_SCOPE scope;

        op = MDW_CLIENT::Get().CheckSave( aProject, aLocalPath, progress,
                                          [wait]( const MDW_RESULT&, const MDW_SAVE_CHECK& aCheck )
                                          {
                                              std::lock_guard<std::mutex> guard( wait->mutex );
                                              wait->check = aCheck;
                                              wait->settled = true;
                                              wait->cv.notify_all();
                                          } );
    }

    {
        std::unique_lock<std::mutex> lock( wait->mutex );
        wait->cv.wait_for( lock, SAVE_CHECK_TIMEOUT,
                           [&]()
                           {
                               return wait->settled;
                           } );
    }

    // Waits out a delivery already running, so what is read below no longer moves
    op.Cancel();

    MDW_SAVE_CHECK check;
    SAVE_PHASE     phase = progress->phase.load();

    {
        std::lock_guard<std::mutex> guard( wait->mutex );
        check = wait->check;
    }

    const MDW_LOCK_INFO lock = phase == SAVE_PHASE::SETTLED ? check.lock : progress->Lock();
    const bool          foreignLock = lock.is_locked && !lock.locked_by_me;
    const SAVE_ACTION   action = SaveAction( foreignLock, phase, check );

    wxLogTrace( traceMdw, wxS( "Save of %s: phase %d, action %d" ), aLocalPath, static_cast<int>( phase ),
                static_cast<int>( action ) );

    if( action == SAVE_ACTION::SAVE_UNCHECKED && aInfoBar )
    {
        // An earlier message's close button would let the save's own cleanup dismiss this at once
        aInfoBar->RemoveAllButtons();
        aInfoBar->ShowMessageFor( _( "The workspace didn't answer, so this save wasn't checked against the "
                                     "file's release state." ),
                                  10000, wxICON_INFORMATION );
    }

    if( action != SAVE_ACTION::PROMPT )
        return true;

    if( check.item_name.IsEmpty() )
        check.item_name = wxFileName( aLocalPath ).GetFullName();

    if( foreignLock )
    {
        return askSave( aParent, MdwClaimSavePrompt( lock, check.item_name ),
                        lock.advisory ? _( "Save" ) : _( "Save Locally" ) );
    }

    return askSave( aParent,
                    wxString::Format( _( "\"%s\" is %s in the Design Workspace and may not accept edits.\n\n"
                                         "Save the local changes anyway?" ),
                                      check.item_name, MdwCategoryName( check.category ) ),
                    _( "Save Anyway" ) );
}

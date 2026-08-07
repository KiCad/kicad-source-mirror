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

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_source.h>

#include "mdw_call.h"
#include "mdw_item_locator.h"
#include "mdw_op_state.h"

#include <mdw/mdw_dirty_tracker.h>
#include <mdw/client/mdw_item.h>
#include <mdw/client/mdw_project_file.h>
#include <mdw/mdw_session.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <trace_helpers.h>

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/translation.h>
#include <wx/intl.h>

#include <mutex>
#include <utility>


namespace
{

/// The op in flight for a chain of steps. A step's callback may start the next step before the starter of
/// its own op has stored it, so a later step always wins
struct CHAIN
{
    std::mutex mutex;
    MDW_OP     current;
    int        currentStep = -1;
    bool       cancelled = false;
    bool       released = false;

    void Run( int aStep, MDW_OP aOp )
    {
        std::unique_lock<std::mutex> lock( mutex );

        if( cancelled )
        {
            lock.unlock();
            aOp.Cancel();
            return;
        }

        // A finished step is let go rather than cancelled, since its callback may be the caller
        MDW_OP stale = std::move( aOp );

        if( !released && aStep > currentStep )
        {
            currentStep = aStep;
            std::swap( stale, current );
        }

        lock.unlock();
        stale.Detach();
    }

    void Release()
    {
        std::unique_lock<std::mutex> lock( mutex );
        released = true;
        MDW_OP op = std::move( current );
        lock.unlock();
        op.Detach();
    }

    void Cancel()
    {
        std::unique_lock<std::mutex> lock( mutex );
        cancelled = true;
        MDW_OP op = std::move( current );
        lock.unlock();
        op.Cancel();
    }
};


/// No item recorded or located for the design, as opposed to a server reporting its item gone
MDW_RESULT notLinked()
{
    return MDW_RESULT::Fail( MDW_ERROR_CODE::ITEM_NOT_FOUND, _( "This project is not linked to a workspace item." ) );
}


MDW_OP itemRef( const std::shared_ptr<MDW_SOURCE>& aSource, const PROJECT& aProject, const wxString& aLocalPath,
                MDW_DONE<MDW_ITEM_REF> aDone )
{
    if( !aSource )
    {
        return MdwNotSupported<MDW_ITEM_REF>( std::move( aDone ) );
    }

    return MDW_ITEM_LOCATOR::Get().Locate(
            aSource, aProject, aLocalPath,
            [done = std::move( aDone )]( const MDW_RESULT& aResult, const std::optional<MDW_LOCATED_ITEM>& aLocated )
            {
                if( !done )
                    return;

                if( !aResult.IsOk() )
                    done( aResult, MDW_ITEM_REF() );
                else if( !aLocated )
                    done( notLinked(), MDW_ITEM_REF() );
                else
                    done( MDW_RESULT::Ok(), aLocated->item );
            } );
}

} // namespace


MDW_OP MDW_CLIENT::ItemRef( const PROJECT& aProject, const wxString& aLocalPath, MDW_DONE<MDW_ITEM_REF> aDone )
{
    return itemRef( MDW_ITEM_LOCATOR::SourceFor( aProject ), aProject, aLocalPath, std::move( aDone ) );
}


MDW_OP MDW_CLIENT::ProjectRef( const PROJECT& aProject, MDW_DONE<MDW_ITEM_REF> aDone )
{
    return ItemRef( aProject, aProject.GetProjectFullName(), std::move( aDone ) );
}


MDW_OP MDW_CLIENT::ProjectState( const PROJECT& aProject, MDW_DONE<MDW_PROJECT_STATE> aDone )
{
    auto state = std::make_shared<MDW_OP_STATE>();
    auto chain = std::make_shared<CHAIN>();

    // A cancelled step never calls back, so the chain's own op is completed here to end its Wait
    state->SetCanceller(
            [chain, weak = std::weak_ptr<MDW_OP_STATE>( state )]()
            {
                chain->Cancel();

                if( std::shared_ptr<MDW_OP_STATE> live = weak.lock() )
                    live->Complete( {} );
            } );

    state->Hold<MDW_PROJECT_STATE>( std::move( aDone ) );

    auto finish = [state, chain]( const MDW_RESULT& aResult, const MDW_PROJECT_STATE& aState )
    {
        chain->Release();
        state->Answer( aResult, aState );
    };

    std::shared_ptr<MDW_SOURCE> source = MDW_ITEM_LOCATOR::SourceFor( aProject );

    if( !source )
    {
        finish( MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), MDW_PROJECT_STATE() );
        return MDW_OP( state );
    }

    // Copied now, since the project may close before the backend answers
    const bool            workingCopy = source->Info().backend.checkin_model == MDW_CHECKIN_MODEL::WORKING_COPY;
    // Spelled as the session's local root, which a backend may compare as written
    const wxString        localRoot = aProject.GetProjectDirectory();
    std::vector<wxString> checkedOut;

    for( const auto& [path, itemId] : aProject.GetLocalSettings().WorkspaceItems() )
        checkedOut.push_back( itemId );

    auto modified = [source, chain, finish, workingCopy, localRoot, checkedOut]( MDW_PROJECT_STATE aState )
    {
        if( !workingCopy )
        {
            std::shared_ptr<MDW_SESSION> session = source->Session();

            for( const wxString& itemId : checkedOut )
                aState.locally_modified |= session && session->DirtyTracker().IsDirty( itemId );

            // A design located but never checked out has no entry, and a save since then still marks it
            aState.locally_modified |= session && session->DirtyTracker().IsDirty( aState.item.item_id );

            finish( MDW_RESULT::Ok(), aState );
            return;
        }

        // One row is enough to tell a changed design from a clean one
        MDW_PAGE first;
        first.page_size = 1;

        chain->Run( 2, source->LocalChanges(
                               localRoot, first,
                               [finish, aState]( const MDW_RESULT&                  aResult,
                                                 const MDW_PAGED<MDW_LOCAL_CHANGE>& aChanges ) mutable
                               {
                                   // A backend that cannot say counts as modified, so nothing is claimed clean unseen
                                   aState.locally_modified = !aResult.IsOk() || !aChanges.rows.empty()
                                                             || !aChanges.next_page_token.IsEmpty();

                                   finish( MDW_RESULT::Ok(), aState );
                               } ) );
    };

    auto describe = [source, chain, finish, modified, workingCopy]( const MDW_LOCATED_ITEM& aLocated )
    {
        MDW_PROJECT_STATE projectState;
        projectState.item = aLocated.item;
        projectState.base_revision_id = aLocated.base_revision_id;
        projectState.base_revision_label = aLocated.base_revision_label;

        if( workingCopy || !projectState.base_revision_id.IsEmpty() )
        {
            modified( projectState );
            return;
        }

        // The latest revision may be another writer's, so without a base it stays unknown
        chain->Run( 1, source->Item( aLocated.item, wxEmptyString,
                                     [finish, modified, projectState]( const MDW_RESULT&    aResult,
                                                                       const MDW_ITEM_INFO& aInfo ) mutable
                                     {
                                         if( !aResult.IsOk() )
                                         {
                                             finish( aResult, MDW_PROJECT_STATE() );
                                             return;
                                         }

                                         // The reply's id is canonical
                                         if( !aInfo.item_id.IsEmpty() )
                                             projectState.item.item_id = aInfo.item_id;

                                         modified( projectState );
                                     } ) );
    };

    auto located = [finish, describe]( const MDW_RESULT& aResult, const std::optional<MDW_LOCATED_ITEM>& aLocated )
    {
        if( !aResult.IsOk() )
            finish( aResult, MDW_PROJECT_STATE() );
        else if( !aLocated )
            finish( notLinked(), MDW_PROJECT_STATE() );
        else
            describe( *aLocated );
    };

    // A located base moves with any commit, update or save of a design file, so it is asked each time rather
    // than kept. A checked-out design keeps the base its checkout recorded
    if( workingCopy || aProject.GetLocalSettings().GetWorkspaceItemId( aProject.GetProjectFullName() ).IsEmpty() )
    {
        wxString relative = MdwRootRelative( localRoot, aProject.GetProjectFullName() );
        wxString locator = source->Info().locator;

        chain->Run( 0, source->Locate( { relative },
                                       [located, relative, locator, workingCopy](
                                               const MDW_RESULT& aResult, const std::vector<MDW_LOCATED_ITEM>& aFound )
                                       {
                                           std::optional<MDW_LOCATED_ITEM> item;
                                           MDW_RESULT result = MdwLocatedItem( workingCopy, aResult, aFound, locator,
                                                                               relative, item );

                                           located( result, item );
                                       } ) );

        return MDW_OP( state );
    }

    chain->Run( 0, MDW_ITEM_LOCATOR::Get().Locate( source, aProject, aProject.GetProjectFullName(), located ) );

    return MDW_OP( state );
}


MDW_OP MDW_CLIENT::CheckSave( const PROJECT& aProject, const wxString& aLocalPath,
                              std::shared_ptr<MDW_SAVE_PROGRESS> aProgress, MDW_DONE<MDW_SAVE_CHECK> aDone )
{
    using MDW_SAVE_GUARD::SAVE_PHASE;

    auto state = std::make_shared<MDW_OP_STATE>();
    auto chain = std::make_shared<CHAIN>();
    auto progress = aProgress ? std::move( aProgress ) : std::make_shared<MDW_SAVE_PROGRESS>();

    progress->phase = SAVE_PHASE::LOOKUP;

    // A cancelled step never calls back, so the chain's own op is completed here to end its Wait
    state->SetCanceller(
            [chain, weak = std::weak_ptr<MDW_OP_STATE>( state )]()
            {
                chain->Cancel();

                if( std::shared_ptr<MDW_OP_STATE> live = weak.lock() )
                    live->Complete( {} );
            } );

    state->Hold<MDW_SAVE_CHECK>(
            [progress, done = std::move( aDone )]( const MDW_RESULT& aResult, const MDW_SAVE_CHECK& aCheck )
            {
                // Set only on delivery, so a caller that cancelled never reads SETTLED without the result
                progress->phase = SAVE_PHASE::SETTLED;

                if( done )
                    done( aResult, aCheck );
            } );

    auto finish = [state, chain]( const MDW_SAVE_CHECK& aCheck )
    {
        chain->Release();
        state->Answer( MDW_RESULT::Ok(), aCheck );
    };

    MDW_SAVE_CHECK check;
    check.item_name = wxFileName( aLocalPath ).GetFullName();

    std::shared_ptr<MDW_SOURCE> source = MDW_ITEM_LOCATOR::SourceFor( aProject );

    // A known item's lock is read now, so a stalled lookup delivery cannot hide another user's lock
    if( std::optional<std::optional<MDW_ITEM_REF>> known =
                MDW_ITEM_LOCATOR::Get().Known( source, aProject, aLocalPath ); source && known && *known )
    {
        progress->SetLock( source->Claim( **known ) );
    }

    auto readLifecycle = [source, chain, finish, progress, check]( const MDW_ITEM_REF& aRef ) mutable
    {
        check.managed = true;

        // A known lock is shown offline or while the lifecycle read stalls
        check.lock = source->Claim( aRef );
        progress->SetLock( check.lock );

        // Another user's lock outranks any lifecycle, so the read would not change the answer
        if( check.lock.is_locked && !check.lock.locked_by_me )
        {
            finish( check );
            return;
        }

        progress->phase = SAVE_PHASE::LIFECYCLE;

        chain->Run( 1, source->Item( aRef, wxEmptyString,
                                     [finish, check]( const MDW_RESULT& aResult, const MDW_ITEM_INFO& aInfo ) mutable
                                     {
                                         if( aResult.IsOk() )
                                             check.category = aInfo.state.category;
                                         else
                                             wxLogTrace( traceMdw, wxS( "Save check of %s: no lifecycle, %s" ),
                                                         check.item_name, aResult.message );

                                         finish( check );
                                     } ) );
    };

    chain->Run(
            0, itemRef( source, aProject, aLocalPath,
                        [finish, readLifecycle, check]( const MDW_RESULT& aResult, const MDW_ITEM_REF& aRef ) mutable
                        {
                            if( aResult.IsOk() )
                            {
                                readLifecycle( aRef );
                                return;
                            }

                            wxLogTrace( traceMdw, wxS( "Save check of %s: no item, %s" ), check.item_name,
                                        aResult.message );

                            finish( check );
                        } ) );

    return MDW_OP( state );
}


void MDW_CLIENT::NoteSaved( const PROJECT& aProject, const wxString& aLocalPath )
{
    std::shared_ptr<MDW_SOURCE> source = MDW_ITEM_LOCATOR::SourceFor( aProject );

    if( !source )
        return;

    auto mark = [source]( const MDW_ITEM_REF& aRef )
    {
        std::shared_ptr<MDW_SESSION> session = source->Session();
        std::optional<std::string>   wireId = MdwWireId( *source, aRef );

        // Stays dirty until a check-in, which is where the service clears it again
        if( session && wireId )
            session->DirtyTracker().SetDirty( wxString::FromUTF8( *wireId ) );
    };

    // Marked before returning when the item is known, so a state read right after the save sees it
    if( std::optional<std::optional<MDW_ITEM_REF>> known =
                MDW_ITEM_LOCATOR::Get().Known( source, aProject, aLocalPath ) )
    {
        if( *known )
            mark( **known );

        return;
    }

    MDW_ITEM_LOCATOR::Get()
            .Locate( source, aProject, aLocalPath,
                     [mark]( const MDW_RESULT& aResult, const std::optional<MDW_LOCATED_ITEM>& aLocated )
                     {
                         if( aResult.IsOk() && aLocated )
                             mark( aLocated->item );
                     } )
            .Detach();
}

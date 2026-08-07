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

#include "mdw_item_locator.h"

#include "mdw_op_state.h"

#include <mdw/client/mdw_client.h>
#include <mdw/client/mdw_source.h>
#include <mdw/client/mdw_item.h>
#include <pgm_base.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>

#include <cstdint>
#include <map>
#include <mutex>


struct MDW_ITEM_LOCATOR::IMPL
{
    struct KNOWN
    {
        std::weak_ptr<MDW_SOURCE> source;

        /// Items by path relative to the local root. A non-item is asked again, since an outside add may track it
        std::map<wxString, MDW_LOCATED_ITEM> answers;

        /// The backend answered NOT_SUPPORTED, so no file of this source is asked about again
        bool refused = false;
    };

    std::mutex                         mutex;
    std::map<const MDW_SOURCE*, KNOWN> known;
    std::uint64_t                      epoch = 0; ///< Raised by Forget, so an answer asked before it is dropped
    MDW_SUBSCRIPTION                   events;

    /// The kept answers for aSource; a dead source's are dropped first, so a reused address starts empty
    KNOWN& For( const std::shared_ptr<MDW_SOURCE>& aSource )
    {
        std::erase_if( known,
                       []( const auto& aEntry )
                       {
                           return aEntry.second.source.expired();
                       } );

        KNOWN& entry = known[aSource.get()];
        entry.source = aSource;
        return entry;
    }

    /// The item the backend named at aRelativePath, or nullopt when none is kept
    std::optional<MDW_LOCATED_ITEM> Answer( const std::shared_ptr<MDW_SOURCE>& aSource, const wxString& aRelativePath )
    {
        std::lock_guard<std::mutex> lock( mutex );
        KNOWN&                      entry = For( aSource );
        auto                        it = entry.answers.find( aRelativePath );

        if( it == entry.answers.end() )
            return std::nullopt;

        return it->second;
    }

    bool Refused( const std::shared_ptr<MDW_SOURCE>& aSource )
    {
        std::lock_guard<std::mutex> lock( mutex );
        return For( aSource ).refused;
    }
};


namespace
{

/// The id and base the local settings recorded when the file was checked out
std::optional<MDW_LOCATED_ITEM> recorded( const std::shared_ptr<MDW_SOURCE>& aSource, const PROJECT& aProject,
                                          const wxString& aLocalPath )
{
    wxString itemId = aProject.GetLocalSettings().GetWorkspaceItemId( aLocalPath );

    if( itemId.IsEmpty() )
        return std::nullopt;

    MDW_LOCATED_ITEM located;
    located.item.item_id = itemId;
    located.item.path_hint = MdwRootRelativeUp( aProject.GetProjectPath(), aLocalPath );
    located.item.source = aSource ? aSource->Info().locator : wxString();

    if( std::optional<MDW_ITEM_BASE> base = aProject.GetLocalSettings().GetWorkspaceBase( aLocalPath ) )
    {
        located.base_revision_id = base->revision_id;
        located.base_revision_label = base->revision_label;
    }

    return located;
}


bool isWorkingCopy( const std::shared_ptr<MDW_SOURCE>& aSource )
{
    return aSource && aSource->Info().backend.checkin_model == MDW_CHECKIN_MODEL::WORKING_COPY;
}


MDW_OP completeNow( MDW_DONE<std::optional<MDW_LOCATED_ITEM>> aDone, const MDW_RESULT& aResult,
                    std::optional<MDW_LOCATED_ITEM> aLocated )
{
    auto state = std::make_shared<MDW_OP_STATE>();

    state->Hold<std::optional<MDW_LOCATED_ITEM>>( std::move( aDone ) );
    state->Answer( aResult, std::move( aLocated ) );
    return MDW_OP( state );
}

} // namespace


MDW_RESULT MdwLocatedItem( bool aWorkingCopy, const MDW_RESULT& aResult, const std::vector<MDW_LOCATED_ITEM>& aFound,
                           const wxString& aLocator, const wxString& aRelative,
                           std::optional<MDW_LOCATED_ITEM>& aLocated )
{
    // An upload backend that cannot locate knows only the files it checked out, which is no failure
    if( !aWorkingCopy && aResult.error == MDW_ERROR_CODE::NOT_SUPPORTED )
        return MDW_RESULT::Ok();

    MDW_RESULT result = aResult;

    // A non-item is an empty item_id; ITEM_NOT_FOUND here must not read as "not managed"
    if( result.error == MDW_ERROR_CODE::ITEM_NOT_FOUND )
        result = MDW_RESULT::Fail( MDW_ERROR_CODE::PROTOCOL, aResult.message );

    if( result.IsOk() && !aFound.empty() && !aFound.front().item.item_id.IsEmpty() )
    {
        aLocated = aFound.front();

        if( aLocated->item.source.IsEmpty() )
            aLocated->item.source = aLocator;

        if( aLocated->item.path_hint.IsEmpty() )
            aLocated->item.path_hint = aRelative;
    }

    return result;
}


MDW_ITEM_LOCATOR& MDW_ITEM_LOCATOR::Get()
{
    static MDW_ITEM_LOCATOR locator;
    return locator;
}


std::shared_ptr<MDW_SOURCE> MDW_ITEM_LOCATOR::SourceFor( const PROJECT& aProject )
{
    PGM_BASE* pgm = PgmOrNull();

    if( !pgm || !pgm->GetSettingsManager().IsProjectOpen() || &pgm->GetSettingsManager().Prj() != &aProject )
        return nullptr;

    return MDW_CLIENT::Get().ProjectSource();
}


MDW_ITEM_LOCATOR::MDW_ITEM_LOCATOR() :
        m_impl( std::make_unique<IMPL>() )
{
    // A reconnect may follow a backend restart, and a check-in may rename what an id stands for
    m_impl->events = MDW_CLIENT::Get().Subscribe(
            [this]( const MDW_EVENT& aEvent )
            {
                switch( aEvent.type )
                {
                case MDW_EVENT_TYPE::WORKSPACE_BOUND:
                case MDW_EVENT_TYPE::WORKSPACE_UNBOUND:
                case MDW_EVENT_TYPE::WORKSPACE_RECONNECTED:
                case MDW_EVENT_TYPE::REVISION_CHECKED_IN:
                case MDW_EVENT_TYPE::REVISION_RELEASED: Forget(); break;
                default: break;
                }
            } );
}


MDW_ITEM_LOCATOR::~MDW_ITEM_LOCATOR() = default;


std::optional<std::optional<MDW_ITEM_REF>> MDW_ITEM_LOCATOR::Known( const std::shared_ptr<MDW_SOURCE>& aSource,
                                                                    const PROJECT&                     aProject,
                                                                    const wxString&                    aLocalPath )
{
    std::optional<std::optional<MDW_LOCATED_ITEM>> answer =
            known( aSource, aProject, aLocalPath, MdwRootRelativeUp( aProject.GetProjectPath(), aLocalPath ) );

    if( !answer )
        return std::nullopt;

    if( !*answer )
        return std::optional<std::optional<MDW_ITEM_REF>>( std::in_place, std::nullopt );

    return std::optional<std::optional<MDW_ITEM_REF>>( std::in_place, ( *answer )->item );
}


std::optional<std::optional<MDW_LOCATED_ITEM>> MDW_ITEM_LOCATOR::known( const std::shared_ptr<MDW_SOURCE>& aSource,
                                                                        const PROJECT&                     aProject,
                                                                        const wxString&                    aLocalPath,
                                                                        const wxString&                    aRelative )
{
    if( std::optional<MDW_LOCATED_ITEM> located = recorded( aSource, aProject, aLocalPath ) )
        return std::optional<std::optional<MDW_LOCATED_ITEM>>( std::in_place, located );

    // An upload backend that cannot locate knows only the files it checked out
    if( !aSource || aRelative.IsEmpty() || ( !isWorkingCopy( aSource ) && m_impl->Refused( aSource ) ) )
        return std::optional<std::optional<MDW_LOCATED_ITEM>>( std::in_place, std::nullopt );

    if( std::optional<MDW_LOCATED_ITEM> answer = m_impl->Answer( aSource, aRelative ) )
        return std::optional<std::optional<MDW_LOCATED_ITEM>>( std::in_place, answer );

    return std::nullopt;
}


MDW_OP MDW_ITEM_LOCATOR::Locate( const std::shared_ptr<MDW_SOURCE>& aSource, const PROJECT& aProject,
                                 const wxString& aLocalPath, MDW_DONE<std::optional<MDW_LOCATED_ITEM>> aDone )
{
    wxString relative = MdwRootRelativeUp( aProject.GetProjectPath(), aLocalPath );

    if( std::optional<std::optional<MDW_LOCATED_ITEM>> answer = known( aSource, aProject, aLocalPath, relative ) )
        return completeNow( std::move( aDone ), MDW_RESULT::Ok(), *answer );

    if( m_impl->Refused( aSource ) )
        return completeNow( std::move( aDone ), MDW_RESULT::Fail( MDW_ERROR_CODE::NOT_SUPPORTED ), std::nullopt );

    wxString                  locator = aSource->Info().locator;
    const bool                workingCopy = isWorkingCopy( aSource );
    std::weak_ptr<MDW_SOURCE> weak = aSource;
    std::uint64_t             epoch;

    {
        std::lock_guard<std::mutex> lock( m_impl->mutex );
        epoch = m_impl->epoch;
    }

    return aSource->Locate( { relative },
                            [this, weak, locator, workingCopy, relative, epoch, done = std::move( aDone )](
                                    const MDW_RESULT& aResult, const std::vector<MDW_LOCATED_ITEM>& aFound )
                            {
                                std::optional<MDW_LOCATED_ITEM> located;
                                MDW_RESULT                      result =
                                        MdwLocatedItem( workingCopy, aResult, aFound, locator, relative, located );

                                // A refusal is kept until Forget, so a backend that cannot locate costs one call
                                if( std::shared_ptr<MDW_SOURCE> source = weak.lock() )
                                {
                                    std::lock_guard<std::mutex> lock( m_impl->mutex );

                                    if( epoch == m_impl->epoch && result.IsOk() && located )
                                        m_impl->For( source ).answers[relative] = *located;
                                    else if( epoch == m_impl->epoch && aResult.error == MDW_ERROR_CODE::NOT_SUPPORTED )
                                        m_impl->For( source ).refused = true;
                                }

                                if( done )
                                    done( result, located );
                            } );
}


void MDW_ITEM_LOCATOR::Forget()
{
    std::lock_guard<std::mutex> lock( m_impl->mutex );
    ++m_impl->epoch;
    m_impl->known.clear();
}

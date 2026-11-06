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

#include <git/git_transfer_busy.h>
#include <git/kicad_git_common.h>

#include <utility>
#include <vector>


void GIT_TRANSFER_BUSY::HOLD::Begin( const std::shared_ptr<KIGIT_COMMON>& aRepo, const wxString& aStatus,
                                     std::function<void()> aCancel )
{
    // One counter for every repository, so a hold on another repository never matches a live one here
    static uint64_t s_nextId = 0;

    Release();
    m_repo = aRepo;
    m_id = ++s_nextId;

    GIT_TRANSFER_BUSY& transfers = aRepo->Transfers();
    transfers.m_live.emplace( m_id, LIVE{ aStatus, std::move( aCancel ) } );
    transfers.Notify( &GIT_TRANSFER_WATCHER::OnGitTransfersChanged );
}


void GIT_TRANSFER_BUSY::HOLD::Release()
{
    std::shared_ptr<KIGIT_COMMON> repo = std::exchange( m_repo, {} ).lock();
    uint64_t                      id = std::exchange( m_id, 0 );

    if( repo && repo->Transfers().m_live.erase( id ) )
        repo->Transfers().Notify( &GIT_TRANSFER_WATCHER::OnGitTransfersChanged );
}


bool GIT_TRANSFER_BUSY::IsBusy( const HOLD& aExcept ) const
{
    return m_live.size() > m_live.count( aExcept.m_id );
}


wxString GIT_TRANSFER_BUSY::Status( const HOLD& aExcept ) const
{
    for( auto it = m_live.rbegin(); it != m_live.rend(); ++it )
    {
        if( it->first != aExcept.m_id )
            return it->second.status;
    }

    return wxString();
}


bool GIT_TRANSFER_BUSY::CanCancel( const HOLD& aExcept ) const
{
    for( const auto& [id, live] : m_live )
    {
        if( id != aExcept.m_id && live.cancel )
            return true;
    }

    return false;
}


void GIT_TRANSFER_BUSY::CancelAll()
{
    std::vector<std::function<void()>> cancels;

    for( auto& [id, live] : m_live )
    {
        if( live.cancel )
            cancels.push_back( std::exchange( live.cancel, nullptr ) );
    }

    // Collect first, since a cancel may end its transfer and erase it from m_live
    for( std::function<void()>& cancel : cancels )
        cancel();
}

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


#include <dialogs/git/git_transfer.h>

#include <wx/app.h>

#include <git/git_backend.h>


void KIGIT::CallOnUi( std::function<void()> aFn )
{
    if( !wxTheApp )
        return;

    wxTheApp->CallAfter( std::move( aFn ) );
}


void KIGIT::CallIfAlive( const std::weak_ptr<bool>& aWatch, std::function<void()> aFn )
{
    CallOnUi(
            [aWatch, fn = std::move( aFn )]()
            {
                if( aWatch.lock() )
                    fn();
            } );
}


bool KIGIT::StartTransfer( const std::string& aLabel, std::function<void()> aWork )
{
    GIT_BACKEND* backend = GetGitBackend();

    return backend && backend->OrphanRegistry().Register( aLabel, std::move( aWork ) );
}


KIGIT_TRANSFER::~KIGIT_TRANSFER()
{
    Cancel();
    m_alive.reset();
}


void KIGIT_TRANSFER::Cancel()
{
    if( m_cancel )
        m_cancel->store( true );
}


bool KIGIT_TRANSFER::Start( const std::string&                                              aLabel,
                            const std::function<void( const KIGIT_COMMON::CANCEL_TOKEN& )>& aWork,
                            const std::function<void()>&                                    aDone )
{
    // Made before the thread starts, so a cancel pressed while the task is still queued reaches
    // this transfer rather than being discarded when it begins.
    m_cancel = KIGIT_COMMON::NewCancelToken();

    std::weak_ptr<bool>        alive = m_alive;
    KIGIT_COMMON::CANCEL_TOKEN token = m_cancel;

    m_busy = true;

    bool started = KIGIT::StartTransfer( aLabel,
                                         [this, alive, token, aWork, aDone]()
                                         {
                                             aWork( token );

                                             KIGIT::CallIfAlive( alive,
                                                                 [this, aDone]()
                                                                 {
                                                                     m_busy = false;
                                                                     aDone();
                                                                 } );
                                         } );

    if( !started )
        m_busy = false;

    return started;
}

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

#ifndef GIT_TRANSFER_BUSY_H
#define GIT_TRANSFER_BUSY_H

#include <import_export.h>
#include <core/observable.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>

#include <wx/string.h>

class KIGIT_COMMON;


/// Told whenever a transfer on the repository it subscribed to begins or ends
class GIT_TRANSFER_WATCHER
{
public:
    virtual ~GIT_TRANSFER_WATCHER() = default;

    virtual void OnGitTransfersChanged() = 0;
};


/**
 * The transfers running on one repository, so every window can show and cancel the others'. UI thread only.
 *
 * An Advanced Git fetch and force push can overlap, so neither may end the busy state or replace the cancel
 * of the other.
 */
class APIEXPORT GIT_TRANSFER_BUSY : public UTIL::OBSERVABLE<GIT_TRANSFER_WATCHER>
{
public:
    /// One transfer's hold on its repository, released when reset, begun again or destroyed
    class APIEXPORT HOLD
    {
    public:
        HOLD() = default;
        ~HOLD() { Release(); }

        HOLD( const HOLD& ) = delete;
        HOLD& operator=( const HOLD& ) = delete;

        /// Release any earlier hold, then hold aRepo busy for one transfer. aCancel may be empty for a
        /// transfer that cannot stop.
        void Begin( const std::shared_ptr<KIGIT_COMMON>& aRepo, const wxString& aStatus,
                    std::function<void()> aCancel );

        void Release();

    private:
        friend class GIT_TRANSFER_BUSY;

        std::weak_ptr<KIGIT_COMMON> m_repo;
        uint64_t                    m_id = 0;
    };

    /// True while a transfer other than aExcept's is live
    bool IsBusy( const HOLD& aExcept ) const;

    /// The status of the latest live transfer other than aExcept's, or empty when none is
    wxString Status( const HOLD& aExcept ) const;

    /// True while a live transfer other than aExcept's has a cancel that has not been called
    bool CanCancel( const HOLD& aExcept ) const;

    /// Call each live transfer's cancel once. They stay live until their owners end them.
    void CancelAll();

private:
    struct LIVE
    {
        wxString              status;
        std::function<void()> cancel;
    };

    /// Keyed by an id that only grows, so the last entry is the latest transfer
    std::map<uint64_t, LIVE> m_live;
};

#endif // GIT_TRANSFER_BUSY_H

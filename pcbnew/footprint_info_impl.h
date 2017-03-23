/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_INFO_IMPL_H
#define FOOTPRINT_INFO_IMPL_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <footprint_info.h>
#include <sync_queue.h>

class LOCALE_IO;

class FOOTPRINT_INFO_IMPL : public FOOTPRINT_INFO
{
public:
    FOOTPRINT_INFO_IMPL(
            FOOTPRINT_LIST* aOwner, const wxString& aNickname, const wxString& aFootprintName )
    {
        m_owner = aOwner;
        m_loaded = false;
        m_nickname = aNickname;
        m_fpname = aFootprintName;
        m_num = 0;
        m_pad_count = 0;
        m_unique_pad_count = 0;
#if !USE_FPI_LAZY
        load();
#endif
    }

protected:
    virtual void load() override;
};


class FOOTPRINT_LIST_IMPL : public FOOTPRINT_LIST
{
    FOOTPRINT_ASYNC_LOADER*  m_loader;
    std::vector<std::thread> m_threads;
    SYNC_QUEUE<wxString>     m_queue_in;
    SYNC_QUEUE<wxString>     m_queue_out;
    std::atomic_size_t       m_count_finished;
    std::atomic_bool         m_first_to_finish;

    /**
     * Call aFunc, pushing any IO_ERRORs and std::exceptions it throws onto m_errors.
     *
     * @return true if no error occurred.
     */
    bool CatchErrors( std::function<void()> aFunc );

protected:
    virtual void StartWorkers( FP_LIB_TABLE* aTable, wxString const* aNickname,
            FOOTPRINT_ASYNC_LOADER* aLoader, unsigned aNThreads ) override;
    virtual bool   JoinWorkers() override;
    virtual size_t CountFinished() override;

    /**
     * Function loader_job
     * loads footprints from m_queue_in.
     */
    void loader_job();

public:
    FOOTPRINT_LIST_IMPL();
    virtual ~FOOTPRINT_LIST_IMPL();

    virtual bool ReadFootprintFiles(
            FP_LIB_TABLE* aTable, const wxString* aNickname = NULL ) override;
};

#endif // FOOTPRINT_INFO_IMPL_H

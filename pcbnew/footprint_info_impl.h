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
#include <core/sync_queue.h>

class LOCALE_IO;

class FOOTPRINT_INFO_IMPL : public FOOTPRINT_INFO
{
public:
    FOOTPRINT_INFO_IMPL( FOOTPRINT_LIST* aOwner, const wxString& aNickname, const wxString& aFootprintName )
    {
        m_nickname = aNickname;
        m_fpname = aFootprintName;
        m_num = 0;
        m_pad_count = 0;
        m_unique_pad_count = 0;

        m_owner = aOwner;
        m_loaded = false;
        load();
    }

    // A constructor for cached items
    FOOTPRINT_INFO_IMPL( const wxString& aNickname, const wxString& aFootprintName,
                         const wxString& aDescription, const wxString& aKeywords,
                         int aOrderNum, unsigned int aPadCount, unsigned int aUniquePadCount )
    {
        m_nickname = aNickname;
        m_fpname = aFootprintName;
        m_num = aOrderNum;
        m_pad_count = aPadCount;
        m_unique_pad_count = aUniquePadCount;
        m_doc = aDescription;
        m_keywords = aKeywords;

        m_owner = nullptr;
        m_loaded = true;
    }


    // A dummy constructor for use as a target in a binary search
    FOOTPRINT_INFO_IMPL( const wxString& aNickname, const wxString& aFootprintName )
    {
        m_nickname = aNickname;
        m_fpname = aFootprintName;

        m_owner = nullptr;
        m_loaded = true;
    }

protected:
    virtual void load() override;
};


class FOOTPRINT_LIST_IMPL : public FOOTPRINT_LIST
{
public:
    FOOTPRINT_LIST_IMPL();
    virtual ~FOOTPRINT_LIST_IMPL() {};

    void WriteCacheToFile( const wxString& aFilePath ) override;
    void ReadCacheFromFile( const wxString& aFilePath ) override;

    bool ReadFootprintFiles( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, const wxString* aNickname = nullptr,
                             PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    void Clear() override;

protected:
    void loadFootprints();

private:
    /**
     * Call aFunc, pushing any IO_ERRORs and std::exceptions it throws onto m_errors.
     *
     * @return true if no error occurred.
     */
    bool CatchErrors( const std::function<void()>& aFunc );

    SYNC_QUEUE<wxString>     m_queue;
    long long                m_list_timestamp;
    PROGRESS_REPORTER*       m_progress_reporter;
    std::atomic_bool         m_cancelled;
    std::mutex               m_join;
    std::mutex               m_loadInProgress;
};

extern FOOTPRINT_LIST_IMPL GFootprintList;        // KIFACE scope.


#endif // FOOTPRINT_INFO_IMPL_H

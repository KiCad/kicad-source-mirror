/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DESIGN_BLOCK_INFO_IMPL_H
#define DESIGN_BLOCK_INFO_IMPL_H

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <kicommon.h>
#include <design_block_info.h>
#include <core/sync_queue.h>

class LOCALE_IO;

class KICOMMON_API DESIGN_BLOCK_INFO_IMPL : public DESIGN_BLOCK_INFO
{
public:
    DESIGN_BLOCK_INFO_IMPL( DESIGN_BLOCK_LIST* aOwner, const wxString& aNickname,
                            const wxString& aDesignBlockName )
    {
        m_nickname = aNickname;
        m_dbname = aDesignBlockName;
        m_num = 0;

        m_owner = aOwner;
        m_loaded = false;
        load();
    }

    // A constructor for cached items
    DESIGN_BLOCK_INFO_IMPL( const wxString& aNickname, const wxString& aDesignBlockName,
                            const wxString& aDescription, const wxString& aKeywords, int aOrderNum )
    {
        m_nickname = aNickname;
        m_dbname = aDesignBlockName;
        m_num = aOrderNum;
        m_doc = aDescription;
        m_keywords = aKeywords;

        m_owner = nullptr;
        m_loaded = true;
    }


    // A dummy constructor for use as a target in a binary search
    DESIGN_BLOCK_INFO_IMPL( const wxString& aNickname, const wxString& aDesignBlockName )
    {
        m_nickname = aNickname;
        m_dbname = aDesignBlockName;

        m_owner = nullptr;
        m_loaded = true;
    }

protected:
    virtual void load() override;
};


class KICOMMON_API DESIGN_BLOCK_LIST_IMPL : public DESIGN_BLOCK_LIST
{
public:
    DESIGN_BLOCK_LIST_IMPL();
    virtual ~DESIGN_BLOCK_LIST_IMPL(){};

    bool ReadDesignBlockFiles( DESIGN_BLOCK_LIB_TABLE* aTable, const wxString* aNickname = nullptr,
                               PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

protected:
    void loadLibs();
    void loadDesignBlocks();

private:
    /**
     * Call aFunc, pushing any IO_ERRORs and std::exceptions it throws onto m_errors.
     *
     * @return true if no error occurred.
     */
    bool CatchErrors( const std::function<void()>& aFunc );

    SYNC_QUEUE<wxString> m_queue_in;
    SYNC_QUEUE<wxString> m_queue_out;
    long long            m_list_timestamp;
    PROGRESS_REPORTER*   m_progress_reporter;
    std::atomic_bool     m_cancelled;
    std::mutex           m_join;
};

#endif // DESIGN_BLOCK_INFO_IMPL_H

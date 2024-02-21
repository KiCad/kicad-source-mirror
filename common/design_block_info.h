/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

/*
 * @file design_block_info.h
 */

#ifndef DESIGN_BLOCK_INFO_H_
#define DESIGN_BLOCK_INFO_H_


#include <kicommon.h>
#include <boost/ptr_container/ptr_vector.hpp>
#include <import_export.h>
#include <ki_exception.h>
#include <core/sync_queue.h>
#include <lib_tree_item.h>
#include <atomic>
#include <functional>
#include <memory>


class DESIGN_BLOCK_LIB_TABLE;
class DESIGN_BLOCK_LIST;
class DESIGN_BLOCK_LIST_IMPL;
class PROGRESS_REPORTER;
class wxTopLevelWindow;
class KIWAY;
class wxTextFile;


/*
 * Helper class to handle the list of design blocks available in libraries. It stores
 * design block names, doc and keywords.
 *
 * This is a virtual class; its implementation lives in common/design_block_info_impl.cpp.
 * To get instances of these classes, see DESIGN_BLOCK_LIST::GetInstance().
 */
class KICOMMON_API DESIGN_BLOCK_INFO : public LIB_TREE_ITEM
{
public:
    virtual ~DESIGN_BLOCK_INFO() {}

    // These two accessors do not have to call ensure_loaded(), because constructor
    // fills in these fields:

    const wxString& GetDesignBlockName() const { return m_dbname; }

    wxString GetLibNickname() const override { return m_nickname; }

    wxString GetName() const override { return m_dbname; }

    LIB_ID GetLIB_ID() const override { return LIB_ID( m_nickname, m_dbname ); }

    wxString GetDesc() override
    {
        ensure_loaded();
        return m_doc;
    }

    wxString GetKeywords()
    {
        ensure_loaded();
        return m_keywords;
    }

    std::vector<SEARCH_TERM> GetSearchTerms() override;

    int GetOrderNum()
    {
        ensure_loaded();
        return m_num;
    }

    /**
     * Test if the #DESIGN_BLOCK_INFO object was loaded from \a aLibrary.
     *
     * @param aLibrary is the nickname of the library to test.
     *
     * @return true if the #DESIGN_BLOCK_INFO object was loaded from \a aLibrary.  Otherwise
     *         false.
     */
    bool InLibrary( const wxString& aLibrary ) const;

    /**
     * Less than comparison operator, intended for sorting DESIGN_BLOCK_INFO objects
     */
    friend bool operator<( const DESIGN_BLOCK_INFO& lhs, const DESIGN_BLOCK_INFO& rhs );

protected:
    void ensure_loaded()
    {
        if( !m_loaded )
            load();
    }

    /// lazily load stuff not filled in by constructor.  This may throw IO_ERRORS.
    virtual void load(){};

    DESIGN_BLOCK_LIST* m_owner; ///< provides access to DESIGN_BLOCK_LIB_TABLE

    bool m_loaded;

    wxString m_nickname; ///< library as known in DESIGN_BLOCK_LIB_TABLE
    wxString m_dbname;   ///< Module name.
    int      m_num;      ///< Order number in the display list.
    wxString m_doc;      ///< Design block description.
    wxString m_keywords; ///< Design block keywords.
};


/**
 * Holds a list of #DESIGN_BLOCK_INFO objects, along with a list of IO_ERRORs or
 * PARSE_ERRORs that were thrown acquiring the DESIGN_BLOCK_INFOs.
 *
 * This is a virtual class; its implementation lives in common/design_block_info_impl.cpp.
 * To get instances of these classes, see DESIGN_BLOCK_LIST::GetInstance().
 */
class KICOMMON_API DESIGN_BLOCK_LIST
{
public:
    typedef std::vector<std::unique_ptr<DESIGN_BLOCK_INFO>> DBILIST;
    typedef SYNC_QUEUE<std::unique_ptr<IO_ERROR>>           ERRLIST;

    DESIGN_BLOCK_LIST() : m_lib_table( nullptr ) {}

    virtual ~DESIGN_BLOCK_LIST() {}

    virtual void WriteCacheToFile( const wxString& aFilePath ){};
    virtual void ReadCacheFromFile( const wxString& aFilePath ){};

    /**
     * @return the number of items stored in list
     */
    unsigned GetCount() const { return m_list.size(); }

    /// Was forced to add this by modview_frame.cpp
    const DBILIST& GetList() const { return m_list; }

    /**
     * @return Clears the design block info cache
     */
    void Clear() { m_list.clear(); }

    /**
     * Get info for a design block by id.
     */
    DESIGN_BLOCK_INFO* GetDesignBlockInfo( const wxString& aDesignBlockName );

    /**
     * Get info for a design block by libNickname/designBlockName
     */
    DESIGN_BLOCK_INFO* GetDesignBlockInfo( const wxString& aLibNickname,
                                           const wxString& aDesignBlockName );

    /**
     * Get info for a design block by index.
     *
     * @param aIdx index of the given item.
     * @return the aIdx item in list.
     */
    DESIGN_BLOCK_INFO& GetItem( unsigned aIdx ) const { return *m_list[aIdx]; }

    unsigned GetErrorCount() const { return m_errors.size(); }

    std::unique_ptr<IO_ERROR> PopError()
    {
        std::unique_ptr<IO_ERROR> error;

        m_errors.pop( error );
        return error;
    }

    /**
     * Read all the design blocks provided by the combination of aTable and aNickname.
     *
     * @param aTable defines all the libraries.
     * @param aNickname is the library to read from, or if NULL means read all design blocks
     *                  from all known libraries in aTable.
     * @param aProgressReporter is an optional progress reporter.  ReadDesignBlockFiles() will
     *                          use 2 phases within the reporter.
     * @return true if it ran to completion, else false if it aborted after some number of
     *         errors.  If true, it does not mean there were no errors, check GetErrorCount()
     *         for that, should be zero to indicate success.
     */
    virtual bool ReadDesignBlockFiles( DESIGN_BLOCK_LIB_TABLE* aTable,
                                       const wxString*         aNickname = nullptr,
                                       PROGRESS_REPORTER*      aProgressReporter = nullptr ) = 0;

    DESIGN_BLOCK_LIB_TABLE* GetTable() const { return m_lib_table; }

protected:
    DESIGN_BLOCK_LIB_TABLE* m_lib_table = nullptr; ///< no ownership

    DBILIST m_list;
    ERRLIST m_errors; ///< some can be PARSE_ERRORs also
};

#endif // DESIGN_BLOCK_INFO_H_

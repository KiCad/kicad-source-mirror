/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file footprint_info.h
 */

#ifndef FOOTPRINT_INFO_H_
#define FOOTPRINT_INFO_H_


#include <boost/ptr_container/ptr_vector.hpp>
#include <import_export.h>
#include <ki_exception.h>
#include <sync_queue.h>
#include <lib_tree_item.h>
#include <atomic>
#include <functional>
#include <memory>


class FP_LIB_TABLE;
class FOOTPRINT_LIST;
class FOOTPRINT_LIST_IMPL;
class FOOTPRINT_ASYNC_LOADER;
class PROGRESS_REPORTER;
class wxTopLevelWindow;
class KIWAY;
class wxTextFile;


/*
 * Helper class to handle the list of footprints available in libraries. It stores
 * footprint names, doc and keywords.
 *
 * This is a virtual class; its implementation lives in pcbnew/footprint_info_impl.cpp.
 * To get instances of these classes, see FOOTPRINT_LIST::GetInstance().
 */
class APIEXPORT FOOTPRINT_INFO : public LIB_TREE_ITEM
{
public:
    virtual ~FOOTPRINT_INFO()
    {
    }

    // These two accessors do not have to call ensure_loaded(), because constructor
    // fills in these fields:

    const wxString& GetFootprintName() const { return m_fpname; }

    wxString GetLibNickname() const override { return m_nickname; }

    wxString GetName() const override { return m_fpname; }

    LIB_ID GetLibId() const override
    {
        return LIB_ID( m_nickname, m_fpname );
    }

    wxString GetDescription() override
    {
        ensure_loaded();
        return m_doc;
    }

    wxString GetKeywords()
    {
        ensure_loaded();
        return m_keywords;
    }

    wxString GetSearchText() override
    {
        // Matches are scored by offset from front of string, so inclusion of this spacer
        // discounts matches found after it.
        static const wxString discount( wxT( "        " ) );

        return GetKeywords() + discount + GetDescription();
    }

    unsigned GetPadCount()
    {
        ensure_loaded();
        return m_pad_count;
    }

    unsigned GetUniquePadCount()
    {
        ensure_loaded();
        return m_unique_pad_count;
    }

    int GetOrderNum()
    {
        ensure_loaded();
        return m_num;
    }

    /**
     * Test if the #FOOTPRINT_INFO object was loaded from \a aLibrary.
     *
     * @param aLibrary is the nickname of the library to test.
     *
     * @return true if the #FOOTPRINT_INFO object was loaded from \a aLibrary.  Otherwise
     *         false.
     */
    bool InLibrary( const wxString& aLibrary ) const;

    /**
     * Less than comparison operator, intended for sorting FOOTPRINT_INFO objects
     */
    friend bool operator<( const FOOTPRINT_INFO& lhs, const FOOTPRINT_INFO& rhs );

protected:
    void ensure_loaded()
    {
        if( !m_loaded )
            load();
    }

    /// lazily load stuff not filled in by constructor.  This may throw IO_ERRORS.
    virtual void load() { };

    FOOTPRINT_LIST* m_owner; ///< provides access to FP_LIB_TABLE

    bool            m_loaded;

    wxString        m_nickname;         ///< library as known in FP_LIB_TABLE
    wxString        m_fpname;           ///< Module name.
    int             m_num;              ///< Order number in the display list.
    unsigned        m_pad_count;        ///< Number of pads
    unsigned        m_unique_pad_count; ///< Number of unique pads
    wxString        m_doc;              ///< Footprint description.
    wxString        m_keywords;         ///< Footprint keywords.
};


/**
 * Holds a list of #FOOTPRINT_INFO objects, along with a list of IO_ERRORs or
 * PARSE_ERRORs that were thrown acquiring the FOOTPRINT_INFOs.
 *
 * This is a virtual class; its implementation lives in pcbnew/footprint_info_impl.cpp.
 * To get instances of these classes, see FOOTPRINT_LIST::GetInstance().
 */
class APIEXPORT FOOTPRINT_LIST
{
public:
    typedef std::vector<std::unique_ptr<FOOTPRINT_INFO>> FPILIST;
    typedef SYNC_QUEUE<std::unique_ptr<IO_ERROR>>        ERRLIST;

    FOOTPRINT_LIST() : m_lib_table( nullptr )
    {
    }

    virtual ~FOOTPRINT_LIST()
    {
    }

    virtual void WriteCacheToFile( const wxString& aFilePath ) {};
    virtual void ReadCacheFromFile( const wxString& aFilePath ){};

    /**
     * @return the number of items stored in list
     */
    unsigned GetCount() const
    {
        return m_list.size();
    }

    /// Was forced to add this by modview_frame.cpp
    const FPILIST& GetList() const
    {
        return m_list;
    }

    /**
     * @return Clears the footprint info cache
     */
    void Clear()
    {
        m_list.clear();
    }

    /**
     * Get info for a footprint by id.
     */
    FOOTPRINT_INFO* GetFootprintInfo( const wxString& aFootprintName );

    /**
     * Get info for a footprint by libNickname/footprintName
     */
    FOOTPRINT_INFO* GetFootprintInfo( const wxString& aLibNickname,
                                      const wxString& aFootprintName );

    /**
     * Get info for a footprint by index.
     *
     * @param aIdx index of the given item.
     * @return the aIdx item in list.
     */
    FOOTPRINT_INFO& GetItem( unsigned aIdx ) const
    {
        return *m_list[aIdx];
    }

    unsigned GetErrorCount() const
    {
        return m_errors.size();
    }

    std::unique_ptr<IO_ERROR> PopError()
    {
        std::unique_ptr<IO_ERROR> error;

        m_errors.pop( error );
        return error;
    }

    /**
     * Read all the footprints provided by the combination of aTable and aNickname.
     *
     * @param aTable defines all the libraries.
     * @param aNickname is the library to read from, or if NULL means read all footprints
     *                  from all known libraries in aTable.
     * @param aProgressReporter is an optional progress reporter.  ReadFootprintFiles() will
     *                          use 2 phases within the reporter.
     * @return true if it ran to completion, else false if it aborted after some number of
     *         errors.  If true, it does not mean there were no errors, check GetErrorCount()
     *         for that, should be zero to indicate success.
     */
    virtual bool ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname = nullptr,
                                     PROGRESS_REPORTER* aProgressReporter = nullptr ) = 0;

    void DisplayErrors( wxTopLevelWindow* aCaller = nullptr );

    FP_LIB_TABLE* GetTable() const
    {
        return m_lib_table;
    }

    /**
     * Factory function to return a #FOOTPRINT_LIST via Kiway.
     *
     * This is not guaranteed to succeed and will return null if the kiface is not available.
     *
     * @param aKiway active kiway instance.
     */
    static FOOTPRINT_LIST* GetInstance( KIWAY& aKiway );

protected:
    /**
     * Launch worker threads to load footprints. Part of the #FOOTPRINT_ASYNC_LOADER
     * implementation.
     */
    virtual void startWorkers( FP_LIB_TABLE* aTable, const wxString* aNickname,
                               FOOTPRINT_ASYNC_LOADER* aLoader, unsigned aNThreads ) = 0;

    /**
     * Join worker threads. Part of the FOOTPRINT_ASYNC_LOADER implementation.
     */
    virtual bool joinWorkers() = 0;

    /**
     * Stop worker threads. Part of the FOOTPRINT_ASYNC_LOADER implementation.
     */
    virtual void stopWorkers() = 0;

private:
    friend class FOOTPRINT_ASYNC_LOADER;

protected:
    FP_LIB_TABLE* m_lib_table; ///< no ownership

    FPILIST m_list;
    ERRLIST m_errors; ///< some can be PARSE_ERRORs also
};


/**
 * Object used to populate a #FOOTPRINT_LIST asynchronously.
 *
 * Construct one, calling #Start(), and then waiting until it reports completion.  This is
 * equivalent to calling #FOOTPRINT_LIST::ReadFootprintFiles().
 */
class APIEXPORT FOOTPRINT_ASYNC_LOADER
{
public:
    /**
     * Construct an asynchronous loader.
     */
    FOOTPRINT_ASYNC_LOADER();

    ~FOOTPRINT_ASYNC_LOADER();

    /**
     * Assign a FOOTPRINT_LIST to the loader. This does not take ownership of
     * the list.
     */
    void SetList( FOOTPRINT_LIST* aList );

    /**
     * Launch the worker threads.
     *
     * @param aTable defines all the libraries.
     * @param aNickname is the library to read from, or if NULL means read all footprints from
     *                  all known libraries in \a aTable.
     * @param aNThreads is the number of worker threads.
     */
    void Start( FP_LIB_TABLE* aTable, const wxString* aNickname = nullptr,
                unsigned aNThreads = DEFAULT_THREADS );

    /**
     * Wait until the worker threads are finished, and then perform any required
     * single-threaded finishing on the list. This must be called before using
     * the list, even if the completion callback was used!
     *
     * It is safe to call this method from a thread, but it is not safe to use
     * the list from ANY thread until it completes. It is recommended to call
     * this from the main thread because of this.
     *
     * It is safe to call this multiple times, but after the first it will
     * always return true.
     *
     * @return true if no errors occurred
     */
    bool Join();

    /**
     * Safely stop the current process.
     */
    void Abort();

private:
    /**
     * Default number of worker threads. Determined empirically (by dickelbeck):
     * More than 6 is not significantly faster, less than 6 is likely slower.
     */
    static constexpr unsigned DEFAULT_THREADS = 6;

    friend class FOOTPRINT_LIST;
    friend class FOOTPRINT_LIST_IMPL;

    FOOTPRINT_LIST*  m_list;
    std::string      m_last_table;

    int              m_total_libs;
};


#endif // FOOTPRINT_INFO_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <ki_mutex.h>
#include <kicad_string.h>
#include <sync_queue.h>

#include <atomic>
#include <functional>
#include <memory>


#define USE_FPI_LAZY 0 // 1:yes lazy,  0:no early


class FP_LIB_TABLE;
class FOOTPRINT_LIST;
class FOOTPRINT_LIST_IMPL;
class FOOTPRINT_ASYNC_LOADER;
class wxTopLevelWindow;
class KIWAY;


/*
 * Helper class to handle the list of footprints available in libraries. It stores
 * footprint names, doc and keywords.
 *
 * This is a virtual class; its implementation lives in pcbnew/footprint_info_impl.cpp.
 * To get instances of these classes, see FOOTPRINT_LIST::GetInstance().
 */
class APIEXPORT FOOTPRINT_INFO
{
    friend bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 );

public:
    virtual ~FOOTPRINT_INFO()
    {
    }

    // These two accessors do not have to call ensure_loaded(), because constructor
    // fills in these fields:

    const wxString& GetFootprintName() const
    {
        return m_fpname;
    }

    const wxString& GetNickname() const
    {
        return m_nickname;
    }

    const wxString& GetDoc()
    {
        ensure_loaded();
        return m_doc;
    }

    const wxString& GetKeywords()
    {
        ensure_loaded();
        return m_keywords;
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

protected:
    void ensure_loaded()
    {
        if( !m_loaded )
            load();
    }

    /// lazily load stuff not filled in by constructor.  This may throw IO_ERRORS.
    virtual void load() = 0;

    FOOTPRINT_LIST* m_owner; ///< provides access to FP_LIB_TABLE

    bool m_loaded;

    wxString m_nickname;         ///< library as known in FP_LIB_TABLE
    wxString m_fpname;           ///< Module name.
    int      m_num;              ///< Order number in the display list.
    int      m_pad_count;        ///< Number of pads
    int      m_unique_pad_count; ///< Number of unique pads
    wxString m_doc;              ///< Footprint description.
    wxString m_keywords;         ///< Footprint keywords.
};


/// FOOTPRINT object list sort function.
inline bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 )
{
    int retv = StrNumCmp( item1.m_nickname, item2.m_nickname, INT_MAX, true );

    if( retv != 0 )
        return retv < 0;

    return StrNumCmp( item1.m_fpname, item2.m_fpname, INT_MAX, true ) < 0;
}


/**
 * Holds a list of FOOTPRINT_INFO objects, along with a list of IO_ERRORs or
 * PARSE_ERRORs that were thrown acquiring the FOOTPRINT_INFOs.
 *
 * This is a virtual class; its implementation lives in pcbnew/footprint_info_impl.cpp.
 * To get instances of these classes, see FOOTPRINT_LIST::GetInstance().
 */
class APIEXPORT FOOTPRINT_LIST
{
    friend class FOOTPRINT_ASYNC_LOADER;

protected:
    FP_LIB_TABLE* m_lib_table; ///< no ownership

    typedef std::vector<std::unique_ptr<FOOTPRINT_INFO>> FPILIST;
    typedef SYNC_QUEUE<std::unique_ptr<IO_ERROR>>        ERRLIST;

    FPILIST m_list;
    ERRLIST m_errors; ///< some can be PARSE_ERRORs also

    MUTEX m_list_lock;


public:
    FOOTPRINT_LIST() : m_lib_table( 0 )
    {
    }

    virtual ~FOOTPRINT_LIST()
    {
    }

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
     * Get info for a module by name.
     * @param aFootprintName = the footprint name inside the FOOTPRINT_INFO of interest.
     * @return FOOTPRINT_INF* - the item stored in list if found
     */
    FOOTPRINT_INFO* GetModuleInfo( const wxString& aFootprintName );

    /**
     * Get info for a module by index.
     * @param aIdx = index of the given item
     * @return the aIdx item in list
     */
    FOOTPRINT_INFO& GetItem( unsigned aIdx )
    {
        return *m_list[aIdx];
    }

    /**
     * Add aItem to list
     * @param aItem = item to add
     */
    void AddItem( FOOTPRINT_INFO* aItem );

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
     * @param aNickname is the library to read from, or if NULL means read all
     *         footprints from all known libraries in aTable.
     * @return bool - true if it ran to completion, else false if it aborted after
     *  some number of errors.  If true, it does not mean there were no errors, check
     *  GetErrorCount() for that, should be zero to indicate success.
     */
    virtual bool ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname = NULL ) = 0;

    void DisplayErrors( wxTopLevelWindow* aCaller = NULL );

    FP_LIB_TABLE* GetTable() const
    {
        return m_lib_table;
    }

    /**
     * Factory function to return a new FOOTPRINT_LIST via Kiway. NOT guaranteed
     * to succeed; will return null if the kiface is not available.
     *
     * @param aKiway - active kiway instance
     */
    static std::unique_ptr<FOOTPRINT_LIST> GetInstance( KIWAY& aKiway );

protected:
    /**
     * Launch worker threads to load footprints. Part of the
     * FOOTPRINT_ASYNC_LOADER implementation.
     */
    virtual void StartWorkers( FP_LIB_TABLE* aTable, wxString const* aNickname,
            FOOTPRINT_ASYNC_LOADER* aLoader, unsigned aNThreads ) = 0;

    /**
     * Join worker threads. Part of the FOOTPRINT_ASYNC_LOADER implementation.
     */
    virtual bool JoinWorkers() = 0;


    /**
     * Return the number of libraries finished (successfully or otherwise).
     */
    virtual size_t CountFinished() = 0;
};


/**
 * This class can be used to populate a FOOTPRINT_LIST asynchronously.
 * Constructing one, calling .Start(), then waiting until it reports completion
 * is equivalent to calling FOOTPRINT_LIST::ReadFootprintFiles().
 */
class APIEXPORT FOOTPRINT_ASYNC_LOADER
{
    friend class FOOTPRINT_LIST;
    friend class FOOTPRINT_LIST_IMPL;

    FOOTPRINT_LIST*       m_list;
    std::function<void()> m_completion_cb;
    std::string           m_last_table;

    bool m_started; ///< True if Start() has been called - does not reset
    int  m_total_libs;

public:
    /**
     * Construct an asynchronous loader.
     */
    FOOTPRINT_ASYNC_LOADER();

    /**
     * Assign a FOOTPRINT_LIST to the loader. This does not take ownership of
     * the list.
     */
    void SetList( FOOTPRINT_LIST* aList );

    /**
     * Launch the worker threads.
     * @param aTable defines all the libraries.
     * @param aNickname is the library to read from, or if NULL means read all
     *         footprints from all known libraries in aTable.
     * @param aNThreads is the number of worker threads.
     */
    void Start( FP_LIB_TABLE* aTable, wxString const* aNickname = nullptr,
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
     * Get the current completion percentage. 0 and 100 are reserved values:
     * 0 will only be returned if Start() has not yet been called, and 100
     * will only be returned if totally complete (i.e. rounding errors will
     * never cause a 100% progress despite not being complete).
     *
     * If there are no libraries at all, returns 100 (as loading zero libraries
     * is always complete).
     *
     * Threadsafe.
     */
    int GetProgress() const;

    /**
     * Set a callback to receive notice when loading is complete.
     *
     * Callback MUST be threadsafe, and must be set before calling Start
     * if you want to use it (it is safe not to set it at all).
     */
    void SetCompletionCallback( std::function<void()> aCallback );

    /**
     * Return true if the given table is the same as the last table loaded.
     * Useful for checking if the table has been modified and needs to be
     * reloaded.
     */
    bool IsSameTable( FP_LIB_TABLE* aOther );

    /**
     * Default number of worker threads. Determined empirically (by dickelbeck):
     * More than 6 is not significantly faster, less than 6 is likely slower.
     */
    static constexpr unsigned DEFAULT_THREADS = 6;
};


#endif // FOOTPRINT_INFO_H_

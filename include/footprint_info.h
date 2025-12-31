/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
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

/*
 * @file footprint_info.h
 */

#ifndef FOOTPRINT_INFO_H_
#define FOOTPRINT_INFO_H_


#include <boost/ptr_container/ptr_vector.hpp>
#include <import_export.h>
#include <ki_exception.h>
#include <core/sync_queue.h>
#include <lib_tree_item.h>
#include <atomic>
#include <functional>
#include <memory>


class FOOTPRINT_LIBRARY_ADAPTER;
class FOOTPRINT_LIST;
class FOOTPRINT_LIST_IMPL;
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

    int GetPinCount() override { return GetUniquePadCount(); }

    LIB_ID GetLIB_ID() const override
    {
        return LIB_ID( m_nickname, m_fpname );
    }

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
    FOOTPRINT_LIST() :
            m_adapter( nullptr )
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
    const std::vector<std::unique_ptr<FOOTPRINT_INFO>>& GetList() const
    {
        return m_list;
    }

    /**
     * @return Clears the footprint info cache
     */
    virtual void Clear() = 0;

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

    void PushError( std::unique_ptr<IO_ERROR> aError )
    {
        m_errors.move_push( std::move( aError ) );
    }

    /**
     * Read all the footprints provided by the combination of aTable and aNickname.
     *
     * @param aAdapter is used to access the libraries.
     * @param aNickname is the library to read from, or if NULL means read all footprints
     *                  from all known libraries in aTable.
     * @param aProgressReporter is an optional progress reporter.  ReadFootprintFiles() will
     *                          use 2 phases within the reporter.
     * @return true if it ran to completion, else false if it aborted after some number of
     *         errors.  If true, it does not mean there were no errors, check GetErrorCount()
     *         for that, should be zero to indicate success.
     */
    virtual bool ReadFootprintFiles( FOOTPRINT_LIBRARY_ADAPTER* aAdapter, const wxString* aNickname = nullptr,
                                     PROGRESS_REPORTER* aProgressReporter = nullptr ) = 0;

    void DisplayErrors( wxTopLevelWindow* aCaller = nullptr );

    /**
     * Returns all accumulated errors as a newline-separated string for display in the
     * status bar. This consumes the errors (pops them from the queue).
     */
    wxString GetErrorMessages();

    FOOTPRINT_LIBRARY_ADAPTER* GetAdapter() const { return m_adapter; }

    /**
     * Factory function to return a #FOOTPRINT_LIST via Kiway.
     *
     * This is not guaranteed to succeed and will return null if the kiface is not available.
     *
     * @param aKiway active kiway instance.
     */
    static FOOTPRINT_LIST* GetInstance( KIWAY& aKiway );

protected:
    FOOTPRINT_LIBRARY_ADAPTER*                   m_adapter; ///< no ownership

    std::vector<std::unique_ptr<FOOTPRINT_INFO>> m_list;
    SYNC_QUEUE<std::unique_ptr<IO_ERROR>>        m_errors; ///< some can be PARSE_ERRORs also
};



#endif // FOOTPRINT_INFO_H_

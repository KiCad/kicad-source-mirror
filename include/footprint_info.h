/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, <jp.charras@wanadoo.fr>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <boost/foreach.hpp>

#include <ki_mutex.h>
#include <kicad_string.h>


#define USE_FPI_LAZY            0   // 1:yes lazy,  0:no early


class FP_LIB_TABLE;
class FOOTPRINT_LIST;
class wxTopLevelWindow;


/*
 * Class FOOTPRINT_INFO
 * is a helper class to handle the list of footprints available in libraries. It stores
 * footprint names, doc and keywords
 */
class FOOTPRINT_INFO
{
    friend bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 );

public:

    // These two accessors do not have to call ensure_loaded(), because constructor
    // fills in these fields:

    const wxString& GetFootprintName() const            { return m_fpname; }
    const wxString& GetNickname() const                 { return m_nickname; }

    FOOTPRINT_INFO( FOOTPRINT_LIST* aOwner, const wxString& aNickname, const wxString& aFootprintName ) :
        m_owner( aOwner ),
        m_loaded( false ),
        m_nickname( aNickname ),
        m_fpname( aFootprintName ),
        m_num( 0 ),
        m_pad_count( 0 ),
        m_unique_pad_count( 0 )
    {
#if !USE_FPI_LAZY
        load();
#endif
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
     * Function InLibrary
     * tests if the #FOOTPRINT_INFO object was loaded from \a aLibrary.
     *
     * @param aLibrary is the nickname of the library to test.
     *
     * @return true if the #FOOTPRINT_INFO object was loaded from \a aLibrary.  Otherwise
     *         false.
     */
    bool InLibrary( const wxString& aLibrary ) const;

private:

    void ensure_loaded()
    {
        if( !m_loaded )
            load();
    }

    /// lazily load stuff not filled in by constructor.  This may throw IO_ERRORS.
    void load();

    FOOTPRINT_LIST* m_owner;            ///< provides access to FP_LIB_TABLE

    bool        m_loaded;

    wxString    m_nickname;             ///< library as known in FP_LIB_TABLE
    wxString    m_fpname;               ///< Module name.
    int         m_num;                  ///< Order number in the display list.
    int         m_pad_count;            ///< Number of pads
    int         m_unique_pad_count;     ///< Number of unique pads
    wxString    m_doc;                  ///< Footprint description.
    wxString    m_keywords;             ///< Footprint keywords.
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
 * Class FOOTPRINT_LIST
 * holds a list of FOOTPRINT_INFO objects, along with a list of IO_ERRORs or
 * PARSE_ERRORs that were thrown acquiring the FOOTPRINT_INFOs.
 */
class FOOTPRINT_LIST
{
    FP_LIB_TABLE*   m_lib_table;        ///< no ownership
    volatile int    m_error_count;      ///< thread safe to read.

    typedef boost::ptr_vector< FOOTPRINT_INFO >         FPILIST;
    typedef boost::ptr_vector< IO_ERROR >               ERRLIST;

    FPILIST m_list;
    ERRLIST m_errors;                   ///< some can be PARSE_ERRORs also

    MUTEX   m_errors_lock;
    MUTEX   m_list_lock;

    /**
     * Function loader_job
     * loads footprints from @a aNicknameList and calls AddItem() on to help fill
     * m_list.
     *
     * @param aNicknameList is a wxString[] holding libraries to load all footprints from.
     * @param aJobZ is the size of the job, i.e. the count of nicknames.
     */
    void loader_job( const wxString* aNicknameList, int aJobZ );

    void addItem( FOOTPRINT_INFO* aItem )
    {
        // m_list is not thread safe, and this function is called from
        // worker threads, lock m_list.
        MUTLOCK lock( m_list_lock );

        m_list.push_back( aItem );
    }


public:

    FOOTPRINT_LIST() :
        m_lib_table( 0 ),
        m_error_count( 0 )
    {
    }

    /**
     * Function GetCount
     * @return the number of items stored in list
     */
    unsigned GetCount() const { return m_list.size(); }

    /// Was forced to add this by modview_frame.cpp
    const FPILIST& GetList() const { return m_list; }

    /**
     * Function GetModuleInfo
     * @param aFootprintName = the footprint name inside the FOOTPRINT_INFO of interest.
     * @return FOOTPRINT_INF* - the item stored in list if found
     */
    FOOTPRINT_INFO* GetModuleInfo( const wxString& aFootprintName );

    /**
     * Function GetItem
     * @param aIdx = index of the given item
     * @return the aIdx item in list
     */
    FOOTPRINT_INFO& GetItem( unsigned aIdx )            { return m_list[aIdx]; }

    /**
     * Function AddItem
     * add aItem in list
     * @param aItem = item to add
     */
    void AddItem( FOOTPRINT_INFO* aItem );

    unsigned GetErrorCount() const  { return m_errors.size(); }

    const IO_ERROR* GetError( unsigned aIdx ) const     { return &m_errors[aIdx]; }

    /**
     * Function ReadFootprintFiles
     * reads all the footprints provided by the combination of aTable and aNickname.
     *
     * @param aTable defines all the libraries.
     * @param aNickname is the library to read from, or if NULL means read all
     *         footprints from all known libraries in aTable.
     * @return bool - true if it ran to completion, else false if it aborted after
     *  some number of errors.  If true, it does not mean there were no errors, check
     *  GetErrorCount() for that, should be zero to indicate success.
     */
    bool ReadFootprintFiles( FP_LIB_TABLE* aTable, const wxString* aNickname = NULL );

    void DisplayErrors( wxTopLevelWindow* aCaller = NULL );

    FP_LIB_TABLE* GetTable() const { return m_lib_table; }
};

#endif  // FOOTPRINT_INFO_H_

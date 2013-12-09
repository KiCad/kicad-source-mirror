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

#if defined( USE_FP_LIB_TABLE )
  #include <ki_mutex.h>
#endif

#include <kicad_string.h>


class FP_LIB_TABLE;
class wxTopLevelWindow;


/*
 * Class FOOTPRINT_INFO
 * is a helper class to handle the list of footprints available in libraries. It stores
 * footprint names, doc and keywords
 */
class FOOTPRINT_INFO
{
public:

    // friend bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 );

    wxString    m_nickname;     ///< the library nickname, eventually

#if !defined(USE_FP_LIB_TABLE)
    wxString    m_lib_path;
#endif


    wxString    m_Module;       ///< Module name.
    int         m_Num;          ///< Order number in the display list.
    wxString    m_Doc;          ///< Footprint description.
    wxString    m_KeyWord;      ///< Footprint key words.
    unsigned    m_padCount;     ///< Number of pads

    FOOTPRINT_INFO()
    {
        m_Num = 0;
        m_padCount = 0;
    }

    const wxString& GetFootprintName() const            { return m_Module; }

    void SetNickname( const wxString& aLibNickname )    { m_nickname = aLibNickname; }
    const wxString& GetNickname() const                 { return m_nickname; }

#if !defined(USE_FP_LIB_TABLE)
    void SetLibPath( const wxString& aLibPath )         { m_lib_path = aLibPath; }
    const wxString& GetLibPath() const                  { return m_lib_path; }
#endif

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
};


/// FOOTPRINT object list sort function.
inline bool operator<( const FOOTPRINT_INFO& item1, const FOOTPRINT_INFO& item2 )
{
#if defined( USE_FP_LIB_TABLE )
    int retv = StrNumCmp( item1.m_nickname, item2.m_nickname, INT_MAX, true );

    if( retv != 0 )
        return retv < 0;
#endif

    return StrNumCmp( item1.m_Module, item2.m_Module, INT_MAX, true ) < 0;
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

#if defined( USE_FP_LIB_TABLE )
    MUTEX   m_errors_lock;
    MUTEX   m_list_lock;
#endif

    /**
     * Function loader_job
     * loads footprints from @a aNicknameList and calls AddItem() on to help fill
     * m_list.
     *
     * @param aNicknameList is a wxString[] holding libraries to load all footprints from.
     * @param aJobZ is the size of the job, i.e. the count of nicknames.
     */
    void loader_job( const wxString* aNicknameList, int aJobZ );

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
     * @return const FOOTPRINT_INF* - the item stored in list if found
     */
    const FOOTPRINT_INFO* GetModuleInfo( const wxString& aFootprintName );

    /**
     * Function GetItem
     * @param aIdx = index of the given item
     * @return the aIdx item in list
     */
    const FOOTPRINT_INFO& GetItem( unsigned aIdx )  const     { return m_list[aIdx]; }

    /**
     * Function AddItem
     * add aItem in list
     * @param aItem = item to add
     */
    void AddItem( FOOTPRINT_INFO* aItem );

    unsigned GetErrorCount() const  { return m_errors.size(); }

    const IO_ERROR* GetError( unsigned aIdx ) const     { return &m_errors[aIdx]; }

#if !defined( USE_FP_LIB_TABLE )
    /**
     * Function ReadFootprintFiles
     *
     * @param aFootprintsLibNames = an array string giving the list of libraries to load
     */
    bool ReadFootprintFiles( wxArrayString& aFootprintsLibNames );
#endif

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
};

#endif  // FOOTPRINT_INFO_H_

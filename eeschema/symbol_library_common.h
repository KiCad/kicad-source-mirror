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

#ifndef _SYMBOL_LIBRARY_COMMON_H_
#define _SYMBOL_LIBRARY_COMMON_H_

#include <map>

#include <wx/arrstr.h>


class LIB_SYMBOL;
class SCH_BASE_FRAME;


enum class SCH_LIB_TYPE
{
    LT_EESCHEMA,
    LT_SYMBOL
};


/**
 * Symbol library map sorting helper.
 */
struct LibSymbolMapSort
{
    bool operator() ( const wxString& aItem1, const wxString& aItem2 ) const
    {
        return aItem1 < aItem2;
    }
};


///< Symbol library map sorted by the symbol name.
typedef std::map< wxString, LIB_SYMBOL*, LibSymbolMapSort > LIB_SYMBOL_MAP;


/**
 * Helper object to filter a list of libraries.
 */
class SYMBOL_LIBRARY_FILTER
{
public:
    SYMBOL_LIBRARY_FILTER()
    {
        m_filterPowerSymbols = false;
        m_forceLoad = false;
    }

    /**
     * Add \a aLibName to the allowed libraries list.
     */
    void AddLib( const wxString& aLibName )
    {
        m_allowedLibs.Add( aLibName );
        m_forceLoad = false;
    }


    /**
     * Add \a aLibName to the allowed libraries list.
     */
    void LoadFrom( const wxString& aLibName )
    {
        m_allowedLibs.Clear();
        m_allowedLibs.Add( aLibName );
        m_forceLoad = true;
    }

    /**
     * Clear the allowed libraries list (allows all libraries).
     */
    void ClearLibList()
    {
        m_allowedLibs.Clear();
        m_forceLoad = false;
    }

    /**
     * Enable or disable the filtering of power symbols.
     */
    void FilterPowerSymbols( bool aFilterEnable )
    {
        m_filterPowerSymbols = aFilterEnable;
    }

    /**
     * @return true if the filtering of power symbols is on.
     */
    bool GetFilterPowerSymbols() const { return m_filterPowerSymbols; }


    /**
     * @return the list of the names of allowed libraries.
     */
    const wxArrayString& GetAllowedLibList() const { return m_allowedLibs; }

    /**
     * @return the name of the library to use to load a symbol or an a empty string if no
     *         library source available.
     */
    const wxString& GetLibSource() const
    {
        static wxString dummy;

        if( m_forceLoad && m_allowedLibs.GetCount() > 0 )
            return m_allowedLibs[0];
        else
            return dummy;
    }

private:
    wxArrayString m_allowedLibs;        ///< List of filtered library names.
    bool          m_filterPowerSymbols; ///< Enable or disable power symbol filtering.
    bool          m_forceLoad;          ///< Force loading symbol from library m_allowedLibs[0].
};


#endif    // _SYMBOL_LIBRARY_COMMON_H_

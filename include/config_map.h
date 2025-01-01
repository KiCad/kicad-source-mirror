/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef CONFIG_MAP__H_
#define CONFIG_MAP__H_

#include <map>

namespace UTIL
{

/**
 * A config value table is a list of native values (usually enums)
 * to a different set of values, for example, the values used to
 * represent the enum in a config file, or the index used to represent
 * it in a selection list.
 *
 * It can be important to decouple from the internal representation,
 * especially in the case of persistent config files, as adding,
 * removing or modifying the order of items internally can easily
 * result in configs being read incorrectly, and, even if otherwise
 * carefully managed, results in obsolete values being kept in enums
 * as placeholders.
 *
 * The first item in the list is used default if no matching value is
 * found during lookup.
 */
template<typename T>
using CFG_MAP = std::vector<std::pair<T, long> >;

/**
 * The "native" type of a CFG_MAP: probably an enum type
 */
template<typename MAP>
using CFG_NATIVE_VAL = typename MAP::value_type::first_type;


/**
 * Get the mapped config value (the one to write to file, or use in
 * an index) from the given native (probably enum) value.
 *
 * The default (first item) is returned if the value is not found
 * in the list.
 *
 * @param aMap the value-config mapping table
 * @param aVal the value to look up
 */
template<typename MAP>
static long GetConfigForVal( const MAP& aMap, CFG_NATIVE_VAL<MAP> aVal )
{
    // default is first entry
    long aConf = aMap[0].second;

    for( const auto& mapping : aMap )
    {
        if( mapping.first == aVal )
        {
            aConf = mapping.second;
            break;
        }
    }

    return aConf;
}

/**
 * Get the native value corresponding to the config value
 * (read from file or UI, probably) and find it in the mapping table.
 *
 * The default item is returned if the mapping fails.
 *
 * @param aMap the value-config mapping table
 * @param aConf the config value to look up
 */
template<typename MAP>
static CFG_NATIVE_VAL<MAP> GetValFromConfig( const MAP& aMap, long aConf )
{
    // default is first entry
    CFG_NATIVE_VAL<MAP> aVal = aMap[0].first;

    for( const auto& mapping : aMap )
    {
        if( mapping.second == aConf )
        {
            aVal = mapping.first;
            break;
        }
    }

    return aVal;
}

}

#endif /* CONFIG_MAP__H_ */

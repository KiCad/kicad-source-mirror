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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "conn_cache.h"
#include <wx/string.h>

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace SCH_CONNECTIVITY
{
// Normalize names and resolve duplicate definitions in file order before constructing this table.
using BUS_ALIASES = std::map<wxString, std::vector<wxString>>;

struct BUS_SCHEMA
{
    enum class SHAPE
    {
        VECTOR,
        GROUP
    };

    struct LEAF
    {
        wxString name;
        wxString localName;
        // The outermost prefix is excluded so named groups can align across a sheet boundary.
        std::vector<wxString> groupPath;

        bool operator==( const LEAF& ) const = default;
    };

    struct NODE
    {
        enum class KIND
        {
            NET,
            VECTOR,
            GROUP
        };

        KIND              kind = KIND::NET;
        // Container label text; NET names are obtained from the referenced flattened leaf.
        wxString          text;
        wxString          prefix;
        std::vector<NODE> members;
        // NET nodes refer to a flattened leaf; duplicate names retain distinct ordinals.
        std::optional<size_t> leaf;

        bool operator==( const NODE& ) const = default;
    };

    SHAPE    shape = SHAPE::GROUP;
    wxString prefix;
    // Source boundary in the escaped input; nonzero for named groups, whose quotes and escapes can add length.
    size_t            prefixEnd = 0;
    std::vector<LEAF> leaves;
    NODE              root;
    // Unresolved lookups are dependencies too; a later alias definition can change these leaves.
    std::set<wxString> aliasesUsed;
    // Alias dependencies and presentation can change without changing the electrical schema.
    bool operator==( const BUS_SCHEMA& aOther ) const
    {
        return shape == aOther.shape && prefix == aOther.prefix && leaves == aOther.leaves;
    }

    // Input and output names use CTX_NETNAME escaping; alias definitions are unescaped text.
    static std::optional<BUS_SCHEMA> Parse( const wxString& aText, const BUS_ALIASES& aAliases = {} );
};

class BUS_PARSE_CACHE
{
public:
    struct RESULT
    {
        std::shared_ptr<const BUS_SCHEMA> schema;
        // Cached electrical schemas omit root; presentation has independent ownership and lifetime.
        std::shared_ptr<const BUS_SCHEMA::NODE> tree;
        std::set<wxString>                      aliasesUsed;
        bool                              operator==( const RESULT& aOther ) const
        {
            return aliasesUsed == aOther.aliasesUsed
                   && ( schema == aOther.schema || ( schema && aOther.schema && *schema == *aOther.schema ) )
                   && ( tree == aOther.tree || ( tree && aOther.tree && *tree == *aOther.tree ) );
        }
    };

    using ENTRY = CACHE_TABLE<wxString, RESULT>::ENTRY;

    explicit BUS_PARSE_CACHE( CACHE_VERSIONS& aVersions ) :
            m_cache( aVersions )
    {
    }
    bool         SetAliases( const BUS_ALIASES& aAliases );
    void         Retain( const std::set<wxString>& aLive );
    const ENTRY& Parse( const wxString& aText );

    // Current tree without cache writes. Missing, invalidated, scalar or failed parses return null.
    std::shared_ptr<const BUS_SCHEMA::NODE> FindTree( const wxString& aText ) const;

private:
    BUS_ALIASES                   m_aliases;
    std::set<wxString>            m_dirty;
    CACHE_TABLE<wxString, RESULT> m_cache;
};

struct BUS_ALIGNMENT
{
    std::vector<std::pair<size_t, size_t>> matched;
    std::vector<size_t>                    unmappedLeft;
};

BUS_ALIGNMENT Align( const BUS_SCHEMA& aLeft, const BUS_SCHEMA& aRight );
} // namespace SCH_CONNECTIVITY

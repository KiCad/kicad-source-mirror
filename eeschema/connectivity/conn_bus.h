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
/**
 * Bus alias table, from alias name to member texts.
 *
 * Normalize names and resolve duplicate definitions in file order before constructing this table.
 */
using BUS_ALIASES = std::map<wxString, std::vector<wxString>>;

/**
 * The parsed form of one bus text.
 *
 * The flat leaves define the electrical members. The tree keeps nested buses for presentation only.
 */
struct BUS_SCHEMA
{
    enum class SHAPE
    {
        VECTOR, ///< A range such as `D[0..3]`. Members align by ordinal.
        GROUP   ///< A list such as `I2C{SDA SCL}`. Members align by name with another group.
    };

    /** One member net. Its ordinal in BUS_SCHEMA::leaves is the member identity in a SLOT_KEY. */
    struct LEAF
    {
        wxString name;      ///< Member name with all group prefixes, such as `I2C.SDA`.
        wxString localName; ///< Member name without group prefixes, such as `SDA`.

        /**
         * Prefixes of the inner named groups.
         * The outermost prefix is excluded so named groups can align across a sheet boundary.
         */
        std::vector<wxString> groupPath;

        bool operator==( const LEAF& ) const = default;
    };

    /** One node of the presentation tree. Only the tree holds nested vectors and groups. */
    struct NODE
    {
        enum class KIND
        {
            NET,
            VECTOR,
            GROUP
        };

        KIND              kind = KIND::NET;
        /** Container label text. A NET node takes its name from the referenced flattened leaf. */
        wxString          text;
        wxString          prefix;
        std::vector<NODE> members;
        /** Leaf ordinal of a NET node. Duplicate names keep distinct ordinals. */
        std::optional<size_t> leaf;

        bool operator==( const NODE& ) const = default;
    };

    SHAPE    shape = SHAPE::GROUP;
    wxString prefix; ///< Root prefix, such as `D` or `I2C`. It is empty for an unnamed group.
    /**
     * End of the root prefix in the escaped input text. It is nonzero only for a named group, whose quotes and
     * escapes can make this length differ from the length of BUS_SCHEMA::prefix.
     */
    size_t            prefixEnd = 0;
    std::vector<LEAF> leaves; ///< Member nets in declaration order, with nested buses flattened.
    NODE              root;   ///< Presentation tree. BUS_PARSE_CACHE stores it apart from the schema.
    /**
     * Every member name that the parser looked up in the alias table. A lookup that found no alias is also a
     * dependency, because a later alias definition can change these leaves.
     */
    std::set<wxString> aliasesUsed;
    /**
     * Compare the electrical schema only. Alias dependencies and presentation can change without an electrical
     * change, and such a change must not create a new record version.
     */
    bool operator==( const BUS_SCHEMA& aOther ) const
    {
        return shape == aOther.shape && prefix == aOther.prefix && leaves == aOther.leaves;
    }

    /**
     * Parse a vector or group bus text.
     *
     * Input and output names use CTX_NETNAME escaping. Alias definitions are unescaped text.
     *
     * @return no value for scalar text, malformed text or an alias cycle.
     */
    static std::optional<BUS_SCHEMA> Parse( const wxString& aText, const BUS_ALIASES& aAliases = {} );
};

/**
 * Parse results for each bus text, with invalidation by alias dependency.
 *
 * An entry whose schema did not change keeps its old schema pointer, so claims and records that hold it stay equal.
 */
class BUS_PARSE_CACHE
{
public:
    struct RESULT
    {
        std::shared_ptr<const BUS_SCHEMA> schema;
        /** Presentation tree. Cached electrical schemas omit BUS_SCHEMA::root, so the tree has its own lifetime. */
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

    /**
     * Replace the alias table.
     *
     * Marks dirty each cached text that looked up a name whose definition changed.
     *
     * @return true if the normalized table differs from the previous one.
     */
    bool         SetAliases( const BUS_ALIASES& aAliases );

    /** Erase the entries whose text is not in \a aLive. */
    void         Retain( const std::set<wxString>& aLive );

    /** Return the cached entry, and parse again if the entry is missing or dirty. */
    const ENTRY& Parse( const wxString& aText );

    /** Current tree without cache writes. Missing, invalidated, scalar or failed parses return null. */
    std::shared_ptr<const BUS_SCHEMA::NODE> FindTree( const wxString& aText ) const;

private:
    BUS_ALIASES                   m_aliases;
    std::set<wxString>            m_dirty;
    CACHE_TABLE<wxString, RESULT> m_cache;
};

/**
 * Member correspondence between two bus schemas.
 */
struct BUS_ALIGNMENT
{
    std::vector<std::pair<size_t, size_t>> matched;      ///< Pairs of left and right leaf ordinals.
    std::vector<size_t>                    unmappedLeft; ///< Left leaf ordinals without a right partner.
};

/**
 * Match the leaves of \a aLeft to the leaves of \a aRight.
 *
 * Two groups match by group path and local name, and equal duplicates match in declaration order. All other
 * shape pairs match by ordinal. Right leaves without a partner are not reported.
 */
BUS_ALIGNMENT Align( const BUS_SCHEMA& aLeft, const BUS_SCHEMA& aRight );
} // namespace SCH_CONNECTIVITY

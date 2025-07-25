/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * KiCad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KiCad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KiCad.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <set>
#include <sstream>
#include <map>
#include <vector>
#include <functional>
#include <algorithm>

// Forward declaration
class SCH_REFERENCE;

/**
 * Function type for external units availability checking.
 *
 * This allows the REFDES_TRACKER to work with mock objects or custom logic
 * without requiring actual SCH_REFERENCE dependencies.
 *
 * @param aTestRef Reference object being tested for compatibility
 * @param aExistingRefs Vector of existing references for the same reference number
 * @param aRequiredUnits Vector of unit numbers needed
 * @return true if all required units are available (no conflicts)
 */
template<typename T>
using UNITS_CHECKER_FUNC = std::function<bool(const T& aTestRef,
                                              const std::vector<T>& aExistingRefs,
                                              const std::vector<int>& aRequiredUnits)>;

/**
 * Class to efficiently track reference designators and provide next available designators.
 *
 * Maintains internal data structures for O(1) lookup of existing designators and efficient
 * retrieval of next available numerical suffixes for any given prefix.
 */
class REFDES_TRACKER
{
public:
    /**
     * Constructor.
     *
     * @param aThreadSafe if true, enables mutex locking for thread-safe operation
     */
    explicit REFDES_TRACKER( bool aThreadSafe = false );

    /**
     * Insert a reference designator into the tracker.
     *
     * @param aRefDes the reference designator to insert
     * @return true if inserted successfully, false if already exists
     */
    bool Insert( const std::string& aRefDes );

    /**
     * Check if a reference designator exists in the tracker.
     *
     * @param aRefDes the reference designator to check
     * @return true if the reference designator exists
     */
    bool Contains( const std::string& aRefDes ) const;

    /**
     * Get the next available reference designator for a given prefix and reserve it.
     *
     * The returned designator is automatically inserted into the tracker.
     *
     * @param aPrefix the alphabetic prefix (e.g., "R", "C", "IC")
     * @param aMinValue the minimum numerical value to use (default 1)
     * @return the next available number for the given prefix (now reserved)
     */
    int GetNextRefDes( const std::string& aPrefix, int aMinValue = 1 );

    /**
     * Get the next available reference designator number for multi-unit symbols.
     *
     * Finds the smallest unused reference number for the given prefix, or the smallest
     * number where all required units are available. The returned number is automatically
     * inserted into the tracker as a base reference designator.
     *
     * @param aRef the schematic reference to use for prefix and unit filtering
     * @param aRefNumberMap map from reference numbers to vectors of SCH_REFERENCE for currently used references
     * @param aRequiredUnits vector of unit numbers needed (negative values are ignored)
     * @param aMinValue the minimum value to start searching from
     * @return the next available reference number
     */
    int GetNextRefDesForUnits( const SCH_REFERENCE& aRef,
                               const std::map<int, std::vector<SCH_REFERENCE>>& aRefNumberMap,
                               const std::vector<int>& aRequiredUnits,
                               int aMinValue );

    /**
     * Set an external units checker function for SCH_REFERENCE objects.
     *
     * This allows overriding the default units availability logic without
     * requiring LIB_SYMBOL dependencies.
     *
     * @param aChecker function to use for checking unit availability
     */
    void SetUnitsChecker( const UNITS_CHECKER_FUNC<SCH_REFERENCE>& aChecker );

    /**
     * Clear the external units checker, reverting to default behavior.
     */
    void ClearUnitsChecker();

    /**
     * Serialize the tracker data to a compact string representation.
     *
     * Uses range notation for consecutive numbers (e.g., "R1-3,R5-7,R10").
     *
     * @return serialized string representation
     */
    std::string Serialize() const;

    /**
     * Deserialize tracker data from string representation.
     *
     * @param aData the serialized data string
     * @return true if deserialization was successful
     */
    bool Deserialize( const std::string& aData );

    /**
     * Clear all stored reference designators.
     */
    void Clear();

    /**
     * Get the total count of stored reference designators.
     *
     * @return number of reference designators stored
     */
    size_t Size() const;

    bool GetReuseRefDes() const { return m_reuseRefDes; }
    void SetReuseRefDes( bool aReuse ) { m_reuseRefDes = aReuse; }

private:
    /**
     * Data structure for tracking used numbers and caching next available values.
     */
    struct PREFIX_DATA
    {
        std::set<int>              m_usedNumbers; ///< Sorted set of used numbers for this prefix
        mutable std::map<int, int> m_nextCache;   ///< Cache of next available number for given min values
        mutable int                m_baseNext;    ///< Next available from 1 (cached)
        mutable bool               m_cacheValid;  ///< True if m_baseNext cache is valid

        PREFIX_DATA() : m_baseNext( 1 ), m_cacheValid( false ) {}
    };

    mutable std::mutex      m_mutex;            ///< Mutex for thread safety
    bool                    m_threadSafe;       ///< True if thread safety is enabled

    /// Map from prefix to its tracking data
    std::unordered_map<std::string, PREFIX_DATA> m_prefixData;
    std::unordered_set<std::string>              m_allRefDes;

    bool m_reuseRefDes; ///< If true, allows reusing existing reference designators

    /// External units checker function (optional)
    UNITS_CHECKER_FUNC<SCH_REFERENCE> m_externalUnitsChecker;

    /**
     * Internal implementation of Insert without locking.
     *
     * @param aRefDes reference designator to insert
     * @return true if inserted, false if already exists
     */
    bool insertImpl( const std::string& aRefDes );

    /**
     * Clear all internal data structures without locking.
     *
     * This is used internally to reset the tracker state.
     */
    void clearImpl();

    /**
     * Check if a reference designator exists in the tracker without locking.
     *
     * @param aRefDes reference designator to check
     * @return true if the reference designator exists
     */
    bool containsImpl( const std::string& aRefDes ) const;

    /**
     * Parse a reference designator into prefix and numerical suffix.
     *
     * @param aRefDes the reference designator to parse
     * @return pair of (prefix, number) where number is 0 if no numerical suffix
     */
    std::pair<std::string, int> parseRefDes( const std::string& aRefDes ) const;

    /**
     * Update cached next available values when a number is inserted.
     *
     * @param aData the prefix data to update
     * @param aInsertedNumber the number that was just inserted
     */
    void updateCacheOnInsert( PREFIX_DATA& aData, int aInsertedNumber ) const;

    /**
     * Find next available number for a prefix starting from a minimum value.
     *
     * @param aData the prefix data
     * @param aMinValue minimum value to start search from
     * @return next available number >= aMinValue
     */
    int findNextAvailable( const PREFIX_DATA& aData, int aMinValue ) const;

    /**
     * Check if all required units are available for a given reference number.
     *
     * @param aRef the SCH_REFERENCE object to check against
     * @param aRefVector vector of SCH_REFERENCE objects for a specific reference number
     * @param aRequiredUnits vector of unit numbers needed (negative values ignored)
     * @return true if all required units are available (not conflicting)
     */
    bool areUnitsAvailable( const SCH_REFERENCE& aRef,
                            const std::vector<SCH_REFERENCE>& aRefVector,
                            const std::vector<int>& aRequiredUnits ) const;

    /**
     * Insert a number for a specific prefix, updating internal structures.
     *
     * @param aPrefix the prefix
     * @param aNumber the number to insert (0 for prefix-only)
     * @return true if inserted, false if already exists
     */
    bool insertNumber( const std::string& aPrefix, int aNumber );

    /**
     * Escape special characters for serialization.
     *
     * @param aStr string to escape
     * @return escaped string
     */
    std::string escapeForSerialization( const std::string& aStr ) const;

    /**
     * Unescape special characters from serialization.
     *
     * @param aStr escaped string
     * @return unescaped string
     */
    std::string unescapeFromSerialization( const std::string& aStr ) const;

    /**
     * Split string by delimiter, handling escaped characters.
     *
     * @param aStr string to split
     * @param aDelimiter delimiter character
     * @return vector of split parts
     */
    std::vector<std::string> splitString( const std::string& aStr, char aDelimiter ) const;

    void updateBaseNext( PREFIX_DATA& aData ) const;
};
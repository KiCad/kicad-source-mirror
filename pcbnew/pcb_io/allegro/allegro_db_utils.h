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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

/**
 * @file allegro_db_utils.h
 *
 * Utility functions that operate over BRD_DBs and BLOCKs
 */

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <unordered_set>
#include <variant>

#include <ki_exception.h>

#include <convert/allegro_pcb_structs.h>
#include <convert/allegro_db.h>

#include <wx/log.h>
#include <wx/string.h>


namespace ALLEGRO
{

inline const wxChar* const traceAllegroUtils = wxT( "KICAD_ALLEGRO_BUILDER" );


/**
 * Check whether a runtime block type code matches the expected type for T.
 */
template <ALLEGRO_BLOCK_DATA BLK_T>
constexpr bool BlockTypeMatches( uint8_t aType )
{
    // Segments can be any of three code (H/oblique/V)
    if constexpr( std::is_same_v<BLK_T, BLK_0x15_16_17_SEGMENT> )
        return aType == 0x15 || aType == 0x16 || aType == 0x17;
    else
        return aType == BLK_T::BLOCK_TYPE_CODE;
}

/**
 * Cast a BLOCK_BASE to a typed BLOCK<T> and return the data.
 *
 * The caller is responsible for ensuring the block type matches T before calling this.
 */
template <ALLEGRO_BLOCK_DATA BLK_T>
const BLK_T& BlockDataAs( const BLOCK_BASE& aBlock )
{
    wxASSERT( BlockTypeMatches<BLK_T>( aBlock.GetBlockType() ) );
    return static_cast<const BLOCK<BLK_T>&>( aBlock ).GetData();
}


/**
 * Non-owning, typed reference to a BLOCK in the database.
 *
 * Wraps a raw BLOCK_BASE pointer and provides typed access to the block's data
 * using std::optional-like operator-> and operator*.
 */
template <ALLEGRO_BLOCK_DATA BLK_T>
class BLOCK_REF
{
public:
    BLOCK_REF() :
            m_block( nullptr )
    {
    }

    explicit BLOCK_REF( const BLOCK_BASE* aBlock ) :
            m_block( aBlock )
    {
    }

    /**
     * True when this BLOCK_REF contains a non-null block pointer (i.e. the block
     * resolved by its key).
     */
    explicit operator bool() const { return m_block != nullptr; }

    const BLK_T& operator*() const
    {
        wxASSERT( m_block != nullptr );
        return BlockDataAs<BLK_T>( *m_block );
    }

    const BLK_T* operator->() const
    {
        wxASSERT( m_block != nullptr );
        return &BlockDataAs<BLK_T>( *m_block );
    }

    const BLOCK_BASE* Block() const { return m_block; }

private:
    const BLOCK_BASE* m_block;
};


/**
 * Get the next block key in the linked list for a given block.
 *
 * Each block type stores its linked-list pointer in a different member. This function
 * dispatches on block type to return the appropriate m_Next value.
 *
 * @return the next block key, or 0 if there is none
 */
uint32_t GetPrimaryNext( const BLOCK_BASE& aBlock );


/**
 * Range-for-compatible walker over a linked list of BLOCK_BASE objects in a BRD_DB.
 *
 * By default, follows the primary linked-list pointer for each block type
 * (via GetPrimaryNext). A custom next-function can be provided via SetNextFunc()
 * for lists that use a different link field (e.g. m_NextInFp for pad lists).
 */
class LL_WALKER
{
public:
    using NEXT_FUNC_T = std::function<uint32_t( const BLOCK_BASE& )>;

    class ITERATOR
    {
    public:
        /**
         * Usually constructed as an end-of-list sentinel
         */
        ITERATOR() :
                m_current( 0 ),
                m_currBlock( nullptr ),
                m_tail( 0 ),
                m_board( nullptr ),
                m_nextFunc( nullptr )
        {
        }

        /**
         * Actual object ITERATOR
         */
        ITERATOR( uint32_t aCurrent, uint32_t aTail, const BRD_DB& aBoard,
                  const NEXT_FUNC_T& aNextFunc ) :
                m_current( aCurrent ),
                m_tail( aTail ),
                m_board( &aBoard ),
                m_nextFunc( &aNextFunc )
        {
            m_currBlock = m_board->GetObjectByKey( m_current );

            if( !m_currBlock )
                m_current = 0;
            else
                m_visited.insert( m_current );
        }

        const BLOCK_BASE* operator*() const
        {
            return m_currBlock;
        }

        ITERATOR& operator++()
        {
            if( m_current == m_tail || !m_currBlock )
            {
                m_current = 0;
            }
            else
            {
                m_current = ( *m_nextFunc )( *m_currBlock );

                if( m_current == m_tail || m_board->IsSentinel( m_current ) )
                {
                    m_current = 0;
                }
                else if( !m_visited.insert( m_current ).second )
                {
                    THROW_IO_ERROR( wxString::Format( "Cycle detected in linked list at key %#010x", m_current ) );
                }
                else
                {
                    m_currBlock = m_board->GetObjectByKey( m_current );

                    if( m_currBlock == nullptr )
                        m_current = 0;
                }
            }

            return *this;
        }

        friend bool operator==( const ITERATOR& aLhs, const ITERATOR& aRhs )
        {
            return aLhs.m_current == aRhs.m_current;
        }

    private:
        uint32_t                     m_current;
        const BLOCK_BASE*            m_currBlock;
        uint32_t                     m_tail;
        const BRD_DB*                m_board;
        const NEXT_FUNC_T*           m_nextFunc;
        std::unordered_set<uint32_t> m_visited;
    };

    /**
     * General constructor for walking any linked list given head/tail keys, a database,
     * and a next-function that returns the next key for a given block.
     *
     * @param aHead key of the first block in the list
     * @param aTail key of the last block in the list (the list ends when the next key is this value)
     * @param aBoard the database to look up blocks in
     * @param aNextFunc function that takes some block and returns the key of the next block
     */
    LL_WALKER( uint32_t aHead, uint32_t aTail, const BRD_DB& aBoard,
               NEXT_FUNC_T aNextFunc = GetPrimaryNext ) :
            m_head( aHead ),
            m_tail( aTail ),
            m_board( aBoard ),
            m_nextFunction( std::move( aNextFunc ) )
    {
    }

    /**
     * Convenience constructor for linked lists in the file header.
     */
    LL_WALKER( const FILE_HEADER::LINKED_LIST& aList, const BRD_DB& aBoard,
               NEXT_FUNC_T aNextFunc = GetPrimaryNext ) :
            LL_WALKER( aList.m_Head, aList.m_Tail, aBoard, std::move( aNextFunc ) )
    {
    }

    ITERATOR begin() const
    {
        return ITERATOR( m_head, m_tail, m_board, m_nextFunction );
    }

    ITERATOR end() const
    {
        return ITERATOR{};
    }

private:
    uint32_t      m_head;
    uint32_t      m_tail;
    const BRD_DB& m_board;
    NEXT_FUNC_T   m_nextFunction;
};


/**
 * Policy for TYPED_LL_WALKER when a block with an unexpected type code is encountered.
 */
enum class MISMATCH_POLICY
{
    SKIP,      ///< silently skip blocks with the wrong type
    LOG_TRACE, ///< wxLogTrace the mismatch and skip
    THROW,     ///< THROW_IO_ERROR on mismatch
    REPORT     ///< invoke a user-supplied callback, then skip
};


/**
 * Callback signature for MISMATCH_POLICY::REPORT.
 *
 * @param aGotType the block type code that was found
 * @param aBlock   the raw block that did not match
 */
using MISMATCH_REPORTER = std::function<void( uint8_t aGotType, const BLOCK_BASE& aBlock )>;

/**
 * Range-for-compatible walker that yields only typed data from a linked list.
 *
 * Wraps LL_WALKER and applies a MISMATCH_POLICY to blocks whose type code does not match T.
 * Dereferences yield const T& directly, avoiding repeated checks/casts in loop bodies.
 */
template <ALLEGRO_BLOCK_DATA T>
class TYPED_LL_WALKER
{
public:
    class ITERATOR
    {
    public:
        /// End sentinel.
        ITERATOR() = default;

        ITERATOR( LL_WALKER::ITERATOR aCurrent, MISMATCH_POLICY aPolicy, const MISMATCH_REPORTER* aReporter ) :
                m_baseIter( aCurrent ),
                m_policy( aPolicy ),
                m_reporter( aReporter )
        {
            skipMismatches();
        }

        const T& operator*() const { return BlockDataAs<T>( **m_baseIter ); }
        const T* operator->() const { return &BlockDataAs<T>( **m_baseIter ); }

        /// File offset of the current underlying block (for diagnostics).
        size_t GetBlockOffset() const { return ( *m_baseIter )->GetOffset(); }

        ITERATOR& operator++()
        {
            ++m_baseIter;
            skipMismatches();
            return *this;
        }

        friend bool operator==( const ITERATOR& aLhs, const ITERATOR& aRhs )
        {
            return aLhs.m_baseIter == aRhs.m_baseIter;
        }

    private:
        void skipMismatches()
        {
            while( m_baseIter != LL_WALKER::ITERATOR{} )
            {
                const BLOCK_BASE* block = *m_baseIter;
                const uint8_t     blockType = block->GetBlockType();

                if( BlockTypeMatches<T>( blockType ) )
                    break;

                const auto blockTypeDesc = []() -> wxString
                {
                    if constexpr( std::is_same_v<T, BLK_0x15_16_17_SEGMENT> )
                        return wxS( "0x15/0x16/0x17 (segment)" );
                    else
                        return wxString::Format( wxS( "%#04x" ), T::BLOCK_TYPE_CODE );
                };

                switch( m_policy )
                {
                case MISMATCH_POLICY::THROW:
                    THROW_IO_ERROR(
                            wxString::Format( "Expected block type %s, got %#04x", blockTypeDesc(), blockType ) );
                case MISMATCH_POLICY::LOG_TRACE:
                    wxLogTrace( traceAllegroUtils, "Expected block type %s, got %#04x, skipping", blockTypeDesc(),
                                blockType );
                    break;
                case MISMATCH_POLICY::REPORT:
                    if( m_reporter )
                        ( *m_reporter )( blockType, *block );
                    break;
                case MISMATCH_POLICY::SKIP: break;
                }

                ++m_baseIter;
            }
        }

        LL_WALKER::ITERATOR      m_baseIter;
        MISMATCH_POLICY          m_policy = MISMATCH_POLICY::SKIP;
        const MISMATCH_REPORTER* m_reporter = nullptr;
    };

    TYPED_LL_WALKER( uint32_t aHead, uint32_t aTail, const BRD_DB& aBoard,
                     MISMATCH_POLICY        aPolicy = MISMATCH_POLICY::SKIP,
                     LL_WALKER::NEXT_FUNC_T aNextFunc = GetPrimaryNext ) :
            m_walker( aHead, aTail, aBoard, std::move( aNextFunc ) ),
            m_policy( aPolicy )
    {
    }

    TYPED_LL_WALKER( const FILE_HEADER::LINKED_LIST& aList, const BRD_DB& aBoard,
                     MISMATCH_POLICY        aPolicy = MISMATCH_POLICY::SKIP,
                     LL_WALKER::NEXT_FUNC_T aNextFunc = GetPrimaryNext ) :
            TYPED_LL_WALKER( aList.m_Head, aList.m_Tail, aBoard, aPolicy, std::move( aNextFunc ) )
    {
    }

    void SetMismatchReporter( MISMATCH_REPORTER aReporter )
    {
        m_mismatchReporter = std::move( aReporter );
    }

    ITERATOR begin() const
    {
        return ITERATOR( m_walker.begin(), m_policy, &m_mismatchReporter );
    }

    ITERATOR end() const
    {
        return {};
    }

private:
    LL_WALKER         m_walker;
    MISMATCH_POLICY   m_policy;
    MISMATCH_REPORTER m_mismatchReporter;
};


/**
 * Some value stored in an 0x03 FIELD block.
 */
using FIELD_VALUE = std::variant<wxString, uint32_t>;

/**
 * Look up the first 0x03 FIELD value of a given type in a linked field chain.
 *
 * @param aDb the database to look in
 * @param aFieldsPtr pointer to the head of the field chain
 * @param aEndKey key of the end-of-chain sentinel node
 * @param aFieldCode the field code to look for (e.g. 0x68)
 * @return the field value as a string or integer, or nullopt if not found
 */
std::optional<FIELD_VALUE> GetFirstFieldOfType( const BRD_DB& aDb, uint32_t aFieldsPtr, uint32_t aEndKey,
                                                uint16_t aFieldCode );

/**
 * Convenience wrapper around GetFirstFieldOfType() for integer-valued fields.
 */
std::optional<int> GetFirstFieldOfTypeInt( const BRD_DB& aDb, uint32_t aFieldsPtr, uint32_t aEndKey,
                                           uint16_t aFieldCode );

} // namespace ALLEGRO

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

#include <allegro_builder.h>

#include <footprint.h>


using namespace ALLEGRO;

/**
 * Gets the next block in the linked list. Exactly which member does this depends on the block type.
 *
 * This is done as dispatch like this to avoid forcing all the blocks into an inheritance hierarchy or mess around
 * with traits.
 *
 * @param block The block to get the next block from.
 * @return The next block in the linked list, or 0 if there is no next block.
 */
static uint32_t GetNext( const BLOCK_BASE& aBlock )
{
    const uint8_t type = aBlock.GetBlockType();

    switch( type )
    {
    case 0x2B: return static_cast<const BLOCK<BLK_0x2B>&>( aBlock ).GetData().m_Next;
    case 0x2D: return static_cast<const BLOCK<BLK_0x2D>&>( aBlock ).GetData().m_Next;
    default: return 0;
    }
}


class LL_WALKER
{
public:
    LL_WALKER( uint32_t aHead, uint32_t aTail, const RAW_BOARD& aBoard ) :
        m_currentBlock( aHead ),
        m_tail( aTail ),
        m_board( aBoard )
    {
    }

    LL_WALKER( const FILE_HEADER::LINKED_LIST& aList, const RAW_BOARD& aBoard ) :
            LL_WALKER( aList.m_Head, aList.m_Tail, aBoard )
    {
    }

    const BLOCK_BASE* GetNext()
    {
        if( m_currentBlock && m_currentBlock != m_tail )
        {
            const BLOCK_BASE* const curr = m_board.GetObjectByKey( m_currentBlock );
            const uint32_t          next = ::GetNext( *curr );
            m_currentBlock = next;
            return curr;
        }

        return nullptr;
    }

private:
    uint32_t         m_currentBlock;
    const uint32_t   m_tail;
    const RAW_BOARD& m_board;

};


BOARD_BUILDER::BOARD_BUILDER( const RAW_BOARD& aRawBoard, BOARD& aBoard, REPORTER& aReporter,
                              PROGRESS_REPORTER* aProgressReporter ) :
        m_rawBoard( aRawBoard ), m_board( aBoard ), m_reporter( aReporter ), m_progressReporter( aProgressReporter )
{
}


std::unique_ptr<FOOTPRINT> BOARD_BUILDER::buildFootprint( const BLK_0x2D& aFpInstance )
{
    auto fp = std::make_unique<FOOTPRINT>( &m_board );



    return fp;
}


bool BOARD_BUILDER::BuildBoard()
{
    if( m_progressReporter )
    {
        m_progressReporter->AddPhases( 1 );
        m_progressReporter->AdvancePhase( _( "Converting footprints" ) );
    }

    LL_WALKER fp_walker( aRawBoard.m_Header->m_LL_0x2B, aRawBoard );
    std::vector<FOOTPRINT*> bulkAddedItems;

    while( const BLOCK_BASE* block = fp_walker.GetNext() )
    {
        if( block->GetBlockType() == 0x2B )
        {
            const BLOCK<BLK_0x2B>& fpBlock = static_cast<const BLOCK<BLK_0x2B>&>( *block );

            std::cout << "Footprint: " << fpBlock.GetData().m_Key << std::endl;
            // m_board.AddFootprint( fp_block.GetData().m_Footprint );

            LL_WALKER instWalker( fpBlock.GetData().m_FirstInstPtr, fpBlock.GetData().m_Key, aRawBoard );

            unsigned numInsts = 0;
            while( const BLOCK_BASE* instBlock = instWalker.GetNext() )
            {
                if( instBlock->GetBlockType() != 0x2D )
                {
                    m_reporter.Report( wxString::Format( "Unexpected object of type %#04x found in footprint %#010x",
                                                         instBlock->GetBlockType(), fpBlock.GetData().m_Key ),
                                                         RPT_SEVERITY_ERROR );
                }
                else
                {
                    const auto& inst = static_cast<const BLOCK<BLK_0x2D>&>( *instBlock ).GetData();

                    std::cout << "  Instance: " << numInsts << ", " << inst.m_Key << std::endl;

                    std::unique_ptr<FOOTPRINT> fp = buildFootprint( inst );

                    bulkAddedItems.push_back( fp.get() );
                    m_board.Add( fp.release(), ADD_MODE::BULK_APPEND, true );
                }
                numInsts++;
            }
        }
    }

    if( bulkAddedItems.size() > 0 )
        m_board.FinalizeBulkAdd( bulkAddedItems );

    return false;
}

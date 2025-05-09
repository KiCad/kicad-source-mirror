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
#include <pcb_text.h>
#include <pcb_shape.h>


using namespace ALLEGRO;


#define BLK_FIELD( BLK_T, FIELD ) static_cast<const BLOCK<BLK_T>&>( aBlock ).GetData().FIELD


/**
 * Gets the next block in the linked list. Exactly which member does this depends on the block type.
 *
 * It's not yet clear if any blocks can be in multiple linked lists at once - for now just follow the "main"
 * one.
 * This is done as dispatch like this to avoid forcing all the blocks into an inheritance hierarchy.
 *
 * @param aBlock The block to get the next block from.
 * @return The next block in the linked list, or 0 if there is no next block.
 */
static uint32_t GetPrimaryNext( const BLOCK_BASE& aBlock )
{
    const uint8_t type = aBlock.GetBlockType();

    switch( type )
    {
    case 0x14: return BLK_FIELD( BLK_0x14, m_Next );
    case 0x15: return BLK_FIELD( BLK_0x15_SEGMENT, m_Next );
    case 0x16: return BLK_FIELD( BLK_0x16_SEGMENT, m_Next );
    case 0x17: return BLK_FIELD( BLK_0x17_SEGMENT, m_Next );
    case 0x2B: return BLK_FIELD( BLK_0x2B, m_Next );
    case 0x2D: return BLK_FIELD( BLK_0x2D, m_Next );
    default: return 0;
    }
}


class LL_WALKER
{
public:
    class iterator
    {
    public:
        iterator( uint32_t aCurrent, uint32_t aTail, const RAW_BOARD& aBoard ) :
                m_current( aCurrent ), m_tail( aTail ), m_board( aBoard )
        {
            m_currBlock = m_board.GetObjectByKey( m_current );
        }

        const BLOCK_BASE* operator*() const { return m_currBlock; }

        iterator& operator++()
        {
            if( m_current == m_tail || !m_currBlock )
            {
                m_current = 0;
            }
            else
            {
                m_current = ::GetPrimaryNext( *m_currBlock );

                // Reached the tail - this isn't actually a node we should return
                if( m_current == m_tail )
                {
                    m_current = 0;
                }
                else
                {
                    // Look up the next block. If it exists, advance.
                    // REVIEW: This may be a place we want to throw in some cases as it implies a corrupt list
                    // but I'm not 100% sure there aren't lists that end in 0x0.
                    m_currBlock = m_board.GetObjectByKey( m_current );

                    if( m_currBlock == nullptr )
                    {
                        m_current = 0;
                    }
                }
            }
            return *this;
        }

        bool operator!=( const iterator& other ) const { return m_current != other.m_current; }

    private:
        uint32_t          m_current;
        const BLOCK_BASE* m_currBlock;
        uint32_t          m_tail;
        const RAW_BOARD&  m_board;
    };

    LL_WALKER( uint32_t aHead, uint32_t aTail, const RAW_BOARD& aBoard ) : m_head( aHead ), m_tail( aTail ), m_board( aBoard )
    {
    }

    LL_WALKER( const FILE_HEADER::LINKED_LIST& aList, const RAW_BOARD& aBoard ) :
            LL_WALKER( aList.m_Head, aList.m_Tail, aBoard )
    {
    }

    iterator begin() const { return iterator( m_head, m_tail, m_board ); }
    iterator end() const { return iterator( 0, m_tail, m_board ); }

private:
    uint32_t         m_head;
    uint32_t         m_tail;
    const RAW_BOARD& m_board;
};


BOARD_BUILDER::BOARD_BUILDER( const RAW_BOARD& aRawBoard, BOARD& aBoard, REPORTER& aReporter,
                              PROGRESS_REPORTER* aProgressReporter ) :
        m_rawBoard( aRawBoard ), m_board( aBoard ), m_reporter( aReporter ), m_progressReporter( aProgressReporter )
{
    double scale = 1;
    switch( m_rawBoard.m_Header->m_BoardUnits )
    {
    case BOARD_UNITS::IMPERIAL:
        // 1 mil = 25400 nm
        scale = 25400;
        break;
    case BOARD_UNITS::METRIC:
        // TODO: is the metric scale um?
        scale = 1000;
        break;
    default:
        THROW_IO_ERROR( "Unknown board units" );
        break;
    }

    if( m_rawBoard.m_Header->m_UnitsDivisor == 0 )
        THROW_IO_ERROR( "Board units divisor is 0" );

    m_scale = scale * m_rawBoard.m_Header->m_UnitsDivisor;
}


VECTOR2I BOARD_BUILDER::scale( const VECTOR2I& aVector ) const
{
    const VECTOR2I vec{ aVector.x * m_scale, -aVector.y * m_scale };
    return vec;
}


std::unique_ptr<PCB_TEXT> BOARD_BUILDER::buildPcbText( const BLK_0x30_STR_WRAPPER& aStrWrapper )
{
    return nullptr;
}


std::unique_ptr<FOOTPRINT> BOARD_BUILDER::buildFootprint( const BLK_0x2D& aFpInstance )
{
    auto fp = std::make_unique<FOOTPRINT>( &m_board );

    PCB_FIELD* refDes = fp->GetField( FIELD_T::REFERENCE );

    const VECTOR2I  fpPos = scale( VECTOR2I{ aFpInstance.m_CoordX, aFpInstance.m_CoordY } );
    const EDA_ANGLE rotation{ aFpInstance.m_Rotation / 1000. };

    fp->SetPosition( fpPos );
    fp->SetOrientation( rotation );

    std::cout << "Footprint pos: " << fpPos.x << ", " << fpPos.y << " angle " << rotation.AsDegrees() << std::endl;

    const LL_WALKER graphicsWalker( aFpInstance.m_GraphicPtr, aFpInstance.m_Key, m_rawBoard );
    for( const BLOCK_BASE* graphicsBlock : graphicsWalker )
    {
        std::cout << "  Graphics: " << wxString::Format( "%#04x", graphicsBlock->GetBlockType() ) << std::endl;
        if( graphicsBlock->GetBlockType() == 0x14 )
        {
            const auto& graphics = static_cast<const BLOCK<BLK_0x14>&>( *graphicsBlock ).GetData();

            const auto& layerInfo = graphics.m_Layer;

            PCB_LAYER_ID l = User_1;

            if( layerInfo.m_Family == LAYER_INFO::FAMILY::SILK )
                l = F_SilkS;
            else if( layerInfo.m_Family == LAYER_INFO::FAMILY::COPPER )
                l = F_Cu;
            else if( layerInfo.m_Family == LAYER_INFO::FAMILY::BOARD_GEOM )
                l = User_3;
            else
            {
                // Good question!
                // 0x07 means something here
            }

            const LL_WALKER segWalker{ graphics.m_SegmentPtr, graphics.m_Key, m_rawBoard };

            for( const BLOCK_BASE* segBlock : segWalker )
            {
                std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &m_board );
                shape->SetLayer( l );

                std::cout << "    Seg: " << wxString::Format( "%#04x", segBlock->GetBlockType() ) << std::endl;

                if( segBlock->GetBlockType() == 0x17 )
                {
                    const auto& seg = static_cast<const BLOCK<BLK_0x17_SEGMENT>&>( *segBlock ).GetData();
                    VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );
                    VECTOR2I    end = scale( { seg.m_EndX, seg.m_EndY } );
                    const int   width = seg.m_Width;

                    // The positions are pre-rotation, but in KiCad, they're post-rotation
                    RotatePoint( start, fpPos, rotation );
                    RotatePoint( end, fpPos, rotation );

                    shape->SetStart( start );
                    shape->SetEnd( end );
                    shape->SetWidth( width * m_scale );

                    fp->Add( shape.release() );
                }
                else if( segBlock->GetBlockType() == 0x16 )
                {
                    const auto& seg = static_cast<const BLOCK<BLK_0x16_SEGMENT>&>( *segBlock ).GetData();
                    VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );
                    VECTOR2I    end = scale( { seg.m_EndX, seg.m_EndY } );
                    const int   width = seg.m_Width;

                    // The positions are pre-rotation, but in KiCad, they're post-rotation
                    RotatePoint( start, fpPos, rotation );
                    RotatePoint( end, fpPos, rotation );

                    shape->SetStart( start );
                    shape->SetEnd( end );
                    shape->SetWidth( width * m_scale );

                    fp->Add( shape.release() );
                }
                else if( segBlock->GetBlockType() == 0x15 )
                {
                    const auto& seg = static_cast<const BLOCK<BLK_0x15_SEGMENT>&>( *segBlock ).GetData();
                    VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );
                    VECTOR2I    end = scale( { seg.m_EndX, seg.m_EndY } );
                    const int   width = seg.m_Width;

                    // The positions are pre-rotation, but in KiCad, they're post-rotation
                    RotatePoint( start, fpPos, rotation );
                    RotatePoint( end, fpPos, rotation );

                    shape->SetStart( start );
                    shape->SetEnd( end );
                    shape->SetWidth( width * m_scale );

                    fp->Add( shape.release() );
                }
            }
            // shape->SetStart( scale( VECTOR2I{ graphics.m_}) )
        }
    }
    return fp;
}


bool BOARD_BUILDER::BuildBoard()
{
    if( m_progressReporter )
    {
        m_progressReporter->AddPhases( 1 );
        m_progressReporter->AdvancePhase( _( "Converting footprints" ) );
    }

    const LL_WALKER          fpWalker( m_rawBoard.m_Header->m_LL_0x2B, m_rawBoard );
    std::vector<BOARD_ITEM*> bulkAddedItems;

    for( const BLOCK_BASE* fpContainer : fpWalker )
    {
        if( fpContainer->GetBlockType() == 0x2B )
        {
            const BLK_0x2B& fpBlock = static_cast<const BLOCK<BLK_0x2B>&>( *fpContainer ).GetData();

            std::cout << "Footprint: " << fpBlock.m_Key << std::endl;
            // m_board.AddFootprint( fp_block.GetData().m_Footprint );

            const LL_WALKER instWalker( fpBlock.m_FirstInstPtr, fpBlock.m_Key, m_rawBoard );

            unsigned numInsts = 0;
            for( const BLOCK_BASE* instBlock : instWalker )
            {
                if( instBlock->GetBlockType() != 0x2D )
                {
                    m_reporter.Report( wxString::Format( "Unexpected object of type %#04x found in footprint %#010x",
                                                         instBlock->GetBlockType(), fpBlock.m_Key ),
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

    if( !bulkAddedItems.empty() )
        m_board.FinalizeBulkAdd( bulkAddedItems );

    return false;
}

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

#include "allegro_pcb_structs.h"


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
    case 0x15:
    case 0x16:
    case 0x17: return BLK_FIELD( BLK_0x15_16_17_SEGMENT, m_Next );
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

    m_scale = scale / m_rawBoard.m_Header->m_UnitsDivisor;
}


VECTOR2I BOARD_BUILDER::scale( const VECTOR2I& aVector ) const
{
    const VECTOR2I vec{ aVector.x * m_scale, -aVector.y * m_scale };
    return vec;
}


/**
 * Set up the layers on the board.
 */
static void SetupBoardLayers( const RAW_BOARD& aRawBoard, BOARD& aBoard )
{
}


std::unique_ptr<PCB_TEXT> BOARD_BUILDER::buildPcbText( const BLK_0x30_STR_WRAPPER& aStrWrapper )
{
    return nullptr;
}


template <>
struct std::hash<LAYER_INFO>
{
    size_t operator()( const LAYER_INFO& aLayerInfo ) const noexcept
    {
        return ( aLayerInfo.m_Class << 8 ) + aLayerInfo.m_Subclass;
    }
};


/**
 * Map of the pre-set class:subclass pairs to standard layers
 */
// clang-format off
static const std::unordered_map<LAYER_INFO, PCB_LAYER_ID> s_LayerKiMap = {
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},     F_SilkS},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::ASSEMBLY_TOP},       F_Fab},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PLACE_BOUND_TOP},    F_CrtYd},
};
// clang-format on


PCB_LAYER_ID BOARD_BUILDER::getLayer( const LAYER_INFO& aLayerInfo ) const
{
    PCB_LAYER_ID layer = User_1;

    // Look up the layer in the presets list
    if( s_LayerKiMap.count( aLayerInfo ) )
    {
        layer = s_LayerKiMap.at( aLayerInfo );
    }
    else
    {
        // Etch?
    }

    return layer;
}


std::vector<std::unique_ptr<PCB_SHAPE>> BOARD_BUILDER::buildShapes( const BLK_0x14&       aGraphic,
                                                                    BOARD_ITEM_CONTAINER& aParent )
{
    std::vector<std::unique_ptr<PCB_SHAPE>> shapes;

    PCB_LAYER_ID layer = getLayer( aGraphic.m_Layer );

    // Within the graphics list, we can get various lines and arcs on PLACE_BOUND_TOP, which
    // aren't actually the courtyard, which is a polygon in the 0x28 list. So, if we see such items,
    // remap them now to a specific other layer
    if( layer == F_CrtYd )
        layer = User_2;

    const LL_WALKER segWalker{ aGraphic.m_SegmentPtr, aGraphic.m_Key, m_rawBoard };

    for( const BLOCK_BASE* segBlock : segWalker )
    {
        std::unique_ptr<PCB_SHAPE>& shape = shapes.emplace_back( std::make_unique<PCB_SHAPE>( &aParent ) );
        shape->SetLayer( layer );

        switch( segBlock->GetBlockType() )
        {
        case 0x01:
        {
            const auto& arc = static_cast<const BLOCK<BLK_0x01_ARC>&>( *segBlock ).GetData();

            VECTOR2I start{ arc.m_StartX, arc.m_StartY };
            VECTOR2I end{ arc.m_EndX, arc.m_EndY };

            start = scale( start );
            end = scale( end );

            VECTOR2I c = scale( KiROUND( VECTOR2D{ arc.m_CenterX, arc.m_CenterY } ) );

            int radius = static_cast<int>( arc.m_Radius * m_scale );

            bool clockwise = false; // TODO - flag?
            // Probably follow fabmaster here for flipping, as I guess it's identical.

            shape->SetWidth( m_scale * arc.m_Width );

            if( start == end )
            {
                shape->SetShape( SHAPE_T::CIRCLE );
                shape->SetCenter( c );
                shape->SetRadius( radius );
            }
            else
            {
                shape->SetShape( SHAPE_T::ARC );
                EDA_ANGLE startangle( start - c );
                EDA_ANGLE endangle( end - c );

                startangle.Normalize();
                endangle.Normalize();

                EDA_ANGLE angle = endangle - startangle;

                if( clockwise && angle < ANGLE_0 )
                    angle += ANGLE_360;
                if( !clockwise && angle > ANGLE_0 )
                    angle -= ANGLE_360;

                if( start == end )
                    angle = -ANGLE_360;

                VECTOR2I mid = start;
                RotatePoint( mid, c, -angle / 2.0 );

                shape->SetArcGeometry( start, mid, end );
            }
            break;
        }
        case 0x15:
        case 0x16:
        case 0x17:
        {
            shape->SetShape( SHAPE_T::SEGMENT );

            const auto& seg = static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *segBlock ).GetData();
            VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );
            VECTOR2I    end = scale( { seg.m_EndX, seg.m_EndY } );
            const int   width = static_cast<int>( seg.m_Width );

            shape->SetStart( start );
            shape->SetEnd( end );
            shape->SetWidth( width * m_scale );
            break;
        }
        default:
        {
            break;
        }
        }
    }

    return shapes;
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
        const uint8_t type = graphicsBlock->GetBlockType();
        if( type == 0x14 )
        {
            const auto& graphics = static_cast<const BLOCK<BLK_0x14>&>( *graphicsBlock ).GetData();

            std::vector<std::unique_ptr<PCB_SHAPE>> shapes = buildShapes( graphics, *fp );

            for( std::unique_ptr<PCB_SHAPE>& shape : shapes )
            {
                shape->Rotate( fpPos, rotation );
                fp->Add( shape.release() );
            }
        }
        else
        {
            m_reporter.Report( wxString::Format( "Unexpected type in graphics list: %#04x", type ),
                               RPT_SEVERITY_WARNING );
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

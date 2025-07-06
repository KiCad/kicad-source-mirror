/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <boost/test/unit_test.hpp>
#include <layer_ids.h>
#include <lset.h>
#include <lseq.h>


BOOST_AUTO_TEST_SUITE( LayerIds )


BOOST_AUTO_TEST_CASE( LseqTestLayers )
{
    LSET allLayers = LSET::AllLayersMask();
    LSEQ seq1 = allLayers.SeqStackupTop2Bottom();

    BOOST_CHECK_EQUAL( seq1.TestLayers( PCB_LAYER_ID::F_Cu, PCB_LAYER_ID::F_Cu ), 0 );
    BOOST_CHECK_GT( seq1.TestLayers( PCB_LAYER_ID::F_Cu, PCB_LAYER_ID::In1_Cu ), 0 );
    BOOST_CHECK_LT( seq1.TestLayers( PCB_LAYER_ID::In1_Cu, PCB_LAYER_ID::F_Cu ), 0 );

    // Pretend like inner copper layer one is the currently selected layer.
    LSEQ seq2 = allLayers.SeqStackupTop2Bottom( PCB_LAYER_ID::In1_Cu );
    BOOST_CHECK_LT( seq2.TestLayers( PCB_LAYER_ID::F_Cu, PCB_LAYER_ID::In1_Cu ), 0 );
    BOOST_CHECK_GT( seq2.TestLayers( PCB_LAYER_ID::In1_Cu, PCB_LAYER_ID::F_Cu ), 0 );
}


BOOST_AUTO_TEST_CASE( CopperLayers )
{
    BOOST_TEST( IsCopperLayer( PCB_LAYER_ID::F_Cu ) );
    BOOST_TEST( IsCopperLayer( PCB_LAYER_ID::B_Cu ) );
    BOOST_TEST( IsCopperLayer( PCB_LAYER_ID::In1_Cu ) );
    BOOST_TEST( !IsCopperLayer( PCB_LAYER_ID::UNSELECTED_LAYER ) );
    BOOST_TEST( !IsCopperLayer( PCB_LAYER_ID::UNDEFINED_LAYER ) );
    BOOST_TEST( !IsCopperLayer( PCB_LAYER_ID::F_SilkS ) );

    BOOST_TEST( !IsCopperLayerLowerThan( PCB_LAYER_ID::F_Cu, PCB_LAYER_ID::B_Cu ) );
    BOOST_TEST( IsCopperLayerLowerThan( PCB_LAYER_ID::In1_Cu, PCB_LAYER_ID::F_Cu ) );
    BOOST_TEST( IsCopperLayerLowerThan( PCB_LAYER_ID::In2_Cu, PCB_LAYER_ID::F_Cu ) );
    BOOST_TEST( IsCopperLayerLowerThan( PCB_LAYER_ID::B_Cu, PCB_LAYER_ID::F_Cu ) );
    BOOST_TEST( !IsCopperLayerLowerThan( PCB_LAYER_ID::In1_Cu, PCB_LAYER_ID::B_Cu ) );
    BOOST_TEST( !IsCopperLayerLowerThan( PCB_LAYER_ID::B_Cu, PCB_LAYER_ID::B_Cu ) );
}


BOOST_AUTO_TEST_CASE( FlipLset )
{
    LSET front( { F_Cu, In1_Cu, In2_Cu } );
    LSET back( { B_Cu, In3_Cu, In4_Cu } );

    front.FlipStandardLayers( 6 );
    back.FlipStandardLayers( 6 );

    BOOST_TEST( front.compare( LSET { B_Cu, In3_Cu, In4_Cu } ) == 0 );
    BOOST_TEST( back.compare( LSET { F_Cu, In1_Cu, In2_Cu } ) == 0 );
}


BOOST_AUTO_TEST_SUITE_END()

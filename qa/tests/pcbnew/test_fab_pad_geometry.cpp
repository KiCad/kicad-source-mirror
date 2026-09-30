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
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/pdf_test_utils.h>
#include <qa_utils/file_utils.h>
#include <boost/test/unit_test.hpp>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <gbr_metadata.h>
#include <pad.h>
#include <pcb_plot_params.h>
#include <pcbplot.h>
#include <pcbnew/exporters/fab_model/fab_pad_geometry.h>
#include <pcbnew/pcb_io/odbpp/odb_feature.h>
#include <pcbnew/pcb_io/odbpp/odb_util.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>
#include <pcbnew/pcb_io/ipc2581/pcb_io_ipc2581.h>
#include <plotters/plotter.h>
#include <convert_basic_shapes_to_polygon.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>


class PAD_CAPTURE_PLOTTER : public PLOTTER
{
public:
    explicit PAD_CAPTURE_PLOTTER( int aMaxError ) :
            m_maxError( aMaxError )
    {
        SetViewport( VECTOR2I( 0, 0 ), aMaxError / 2.0, 1.0, false );
    }

    PLOT_FORMAT GetPlotterType() const override { return PLOT_FORMAT::SVG; }
    bool        StartPlot( const wxString& ) override { return true; }
    bool        EndPlot() override { return true; }
    void        SetCurrentLineWidth( int, void* = nullptr ) override {}
    void        SetColor( const COLOR4D& ) override {}
    void        SetDash( int, LINE_STYLE ) override {}
    void        SetViewport( const VECTOR2I&, double aIusPerDecimil, double, bool ) override
    {
        m_IUsPerDecimil = aIusPerDecimil;
    }
    void Rect( const VECTOR2I&, const VECTOR2I&, FILL_T, int, int = 0 ) override {}
    void Circle( const VECTOR2I&, int, FILL_T, int ) override {}
    void PenTo( const VECTOR2I&, char ) override {}
    void PlotPoly( const std::vector<VECTOR2I>&, FILL_T, int, void* ) override {}

    void FlashPadCircle( const VECTOR2I& aPos, int aDiameter, void* aData ) override
    {
        SHAPE_POLY_SET shape;
        TransformCircleToPolygon( shape, aPos, aDiameter / 2, m_maxError, ERROR_INSIDE, 16 );
        Record( shape, aData );
    }

    void FlashPadOval( const VECTOR2I& aPos, const VECTOR2I& aSize, const EDA_ANGLE& aOrient, void* aData ) override
    {
        SHAPE_POLY_SET shape;
        const int      halfWidth = std::min( aSize.x, aSize.y ) / 2;
        VECTOR2I       delta( aSize.x / 2 - halfWidth, aSize.y / 2 - halfWidth );
        RotatePoint( delta, aOrient );
        TransformOvalToPolygon( shape, aPos - delta, aPos + delta, halfWidth * 2, m_maxError, ERROR_INSIDE, 16 );
        Record( shape, aData );
    }

    void FlashPadRect( const VECTOR2I& aPos, const VECTOR2I& aSize, const EDA_ANGLE& aOrient, void* aData ) override
    {
        SHAPE_POLY_SET shape;
        TransformTrapezoidToPolygon( shape, aPos, aSize, aOrient, 0, 0, 0, m_maxError, ERROR_INSIDE );
        Record( shape, aData );
    }

    void FlashPadRoundRect( const VECTOR2I& aPos, const VECTOR2I& aSize, int aRadius, const EDA_ANGLE& aOrient,
                            void* aData ) override
    {
        SHAPE_POLY_SET shape;
        TransformRoundChamferedRectToPolygon( shape, aPos, aSize, aOrient, aRadius, 0, 0, 0, m_maxError, ERROR_INSIDE );
        Record( shape, aData );
    }

    void FlashPadCustom( const VECTOR2I&, const VECTOR2I&, const EDA_ANGLE&, SHAPE_POLY_SET* aPolygons,
                         void* aData ) override
    {
        Record( *aPolygons, aData );
    }

    void FlashPadTrapez( const VECTOR2I& aPos, const VECTOR2I* aCorners, const EDA_ANGLE& aOrient,
                         void* aData ) override
    {
        SHAPE_POLY_SET shape;
        shape.NewOutline();

        for( int i = 0; i < 4; ++i )
        {
            VECTOR2I point = aCorners[i];
            RotatePoint( point, aOrient );
            shape.Append( point + aPos );
        }

        Record( shape, aData );
    }

    void FlashRegularPolygon( const VECTOR2I&, int, int, const EDA_ANGLE&, void* ) override {}

    std::vector<SHAPE_POLY_SET> m_padShapes;

private:
    void Record( const SHAPE_POLY_SET& aShape, void* aData )
    {
        if( aData && !static_cast<const GBR_METADATA*>( aData )->m_NetlistMetadata.m_Cmpref.IsEmpty() )
            m_padShapes.push_back( aShape );
    }

    int m_maxError;
};


BOOST_AUTO_TEST_CASE( FabPadLayerMatchesPlotterOnRealBoards )
{
    const std::vector<std::string> paths = { "api_kitchen_sink.kicad_pcb", "connect/connect.kicad_pcb",
                                             "odbpp/custom_pad_mask_split.kicad_pcb",
                                             "odbpp/pad_margin_shapes.kicad_pcb" };
    int                            compared = 0;
    int                            comparedMargins = 0;

    for( const std::string& path : paths )
    {
        std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir() + path );
        BOOST_REQUIRE( board );
        const int              maxError = board->GetDesignSettings().m_MaxError;
        SIMPLE_RENDER_SETTINGS renderSettings;
        int                    boardCompared = 0;
        LSET                   layers = board->GetEnabledLayers() | LSET( { F_Mask, B_Mask, F_Paste, B_Paste } );

        for( PCB_LAYER_ID layer : layers.Seq() )
        {
            if( !IsCopperLayer( layer ) && layer != F_Mask && layer != B_Mask && layer != F_Paste && layer != B_Paste )
            {
                continue;
            }

            PAD_CAPTURE_PLOTTER plotter( maxError );
            plotter.SetRenderSettings( &renderSettings );
            PCB_PLOT_PARAMS options;
            options.SetPlotFPText( false );
            PlotStandardLayer( board.get(), &plotter, LSET( { layer } ), options );

            size_t index = 0;

            for( const FOOTPRINT* footprint : board->Footprints() )
            {
                for( const PAD* pad : footprint->Pads() )
                {
                    if( !pad->GetLayerSet().Contains( layer )
                        || ( IsCopperLayer( layer )
                             && ( !pad->IsOnCopperLayer() || !pad->FlashLayer( LSET( { layer } ) ) ) ) )
                    {
                        continue;
                    }

                    PAD_LAYER_GEOMETRY geometry = ResolvePadLayer( *pad, layer, maxError );

                    if( geometry.IsEmpty() )
                        continue;

                    BOOST_REQUIRE_LT( index, plotter.m_padShapes.size() );
                    SHAPE_POLY_SET actual = geometry.Polygon();
                    SHAPE_POLY_SET missing = plotter.m_padShapes[index];
                    SHAPE_POLY_SET excess = actual;
                    missing.BooleanSubtract( actual );
                    excess.BooleanSubtract( plotter.m_padShapes[index] );

                    const double tolerance = 1000000.0;
                    BOOST_CHECK_LE( std::abs( missing.Area() ), tolerance );
                    BOOST_CHECK_LE( std::abs( excess.Area() ), tolerance );

                    if( geometry.Margin() != VECTOR2I( 0, 0 ) )
                        ++comparedMargins;

                    ++index;
                    ++compared;
                    ++boardCompared;
                }
            }

            BOOST_CHECK_EQUAL( index, plotter.m_padShapes.size() );
        }

        BOOST_CHECK_GT( boardCompared, 0 );
    }

    BOOST_CHECK_GT( compared, 100 );
    BOOST_CHECK_GT( comparedMargins, 0 );
}



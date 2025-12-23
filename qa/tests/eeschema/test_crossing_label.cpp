#include <qa_utils/wx_utils/unit_test_utils.h>

#include <connection_graph.h>
#include <sch_line.h>
#include <sch_label.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_screen.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <project.h>

BOOST_AUTO_TEST_CASE( LabelDrivesCrossingWires )
{

    SETTINGS_MANAGER manager;
    manager.LoadProject( "" );

    SCHEMATIC schematic( &manager.Prj() );
    schematic.Reset();
    SCH_SHEET* defaultSheet = schematic.GetTopLevelSheet( 0 );

    SCH_SCREEN* screen = new SCH_SCREEN( nullptr );
    SCH_SHEET*  sheet  = new SCH_SHEET( nullptr, VECTOR2I( 0, 0 ), VECTOR2I( 1000, 1000 ) );
    sheet->SetScreen( screen );
    schematic.AddTopLevelSheet( sheet );
    schematic.RemoveTopLevelSheet( defaultSheet );
    delete defaultSheet;

    CONNECTION_GRAPH graph;
    graph.SetSchematic( &schematic );

    SCH_SHEET_PATH sheetPath;
    sheetPath.push_back( sheet );

    SCH_LINE* h = new SCH_LINE( VECTOR2I( -1000, 0 ), LAYER_WIRE );
    h->SetEndPoint( VECTOR2I( 1000, 0 ) );
    SCH_LINE* v = new SCH_LINE( VECTOR2I( 0, -1000 ), LAYER_WIRE );
    v->SetEndPoint( VECTOR2I( 0, 1000 ) );
    SCH_LABEL* label = new SCH_LABEL( VECTOR2I( 0, 0 ), wxS( "N" ) );
    screen->Append( h, false );
    screen->Append( v, false );
    screen->Append( label, false );

    std::vector<SCH_ITEM*> items = { h, v, label };

    label->GetOrInitConnection( sheetPath, &graph )->ConfigureFromLabel(wxS("N"));

    SCH_SHEET_LIST sheets = schematic.BuildSheetListSortedByPageNumbers();
    graph.Recalculate( sheets, true );

    CONNECTION_SUBGRAPH* sg1 = graph.GetSubgraphForItem( h );
    CONNECTION_SUBGRAPH* sg2 = graph.GetSubgraphForItem( v );

    BOOST_REQUIRE( sg1 );
    BOOST_REQUIRE( sg2 );
    BOOST_CHECK( sg1 == sg2 );
    BOOST_CHECK( sg1->GetDriver() == label );
    BOOST_CHECK( sg2->GetDriver() == label );
}

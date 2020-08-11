#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_test_provider.h>

test::DRC_TEST_PROVIDER::DRC_TEST_PROVIDER() :
    m_drcEngine( nullptr ),
    m_enable( false )
{

}

void test::DRC_TEST_PROVIDER::Enable( bool enable )
{
    m_enable = enable;
}

bool test::DRC_TEST_PROVIDER::IsEnabled() const
{
    return m_enable;
}

const wxString test::DRC_TEST_PROVIDER::GetName() const { return "<no name test>"; }
const wxString test::DRC_TEST_PROVIDER::GetDescription() const { return ""; }

void test::DRC_TEST_PROVIDER::Report( std::shared_ptr<DRC_ITEM> item )
{
    item->SetViolatingTest( this );
    m_drcEngine->Report( item, nullptr );
}


void test::DRC_TEST_PROVIDER::ReportWithMarker( std::shared_ptr<DRC_ITEM> item, VECTOR2I aMarkerPos )
{
    item->SetViolatingTest( this );
    MARKER_PCB* marker = new MARKER_PCB( item, wxPoint( aMarkerPos.x, aMarkerPos.y) );
    m_drcEngine->Report( item, marker );
}

void test::DRC_TEST_PROVIDER::ReportWithMarker( std::shared_ptr<DRC_ITEM> item, wxPoint aMarkerPos )
{
    item->SetViolatingTest( this );
    MARKER_PCB* marker = new MARKER_PCB( item, wxPoint( aMarkerPos.x, aMarkerPos.y) );
    m_drcEngine->Report( item, marker ); // fixme: create marker
}

void test::DRC_TEST_PROVIDER::ReportProgress( double aProgress )
{
    m_drcEngine->ReportProgress( aProgress );
}

void test::DRC_TEST_PROVIDER::ReportStage ( const wxString& aStageName, int index, int total )
{
    m_drcEngine->ReportStage( aStageName, index, total );
    ReportAux( aStageName );
}

void test::DRC_TEST_PROVIDER::ReportAux( const wxString fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    wxString str;
    str.PrintfV( fmt, vargs );
    va_end( vargs );
    m_drcEngine->ReportAux( str );
}

bool test::DRC_TEST_PROVIDER::isErrorLimitExceeded( int error_code )
{
    // fixme: implement error limit (or timeout)
    return false;
}

EDA_UNITS test::DRC_TEST_PROVIDER::userUnits() const
{
    return m_drcEngine->UserUnits();
}

void test::DRC_TEST_PROVIDER::accountCheck( const test::DRC_RULE* ruleToTest )
{
    auto it = m_stats.find( ruleToTest );
    if( it == m_stats.end() )
        m_stats[ ruleToTest ] = 1;
    else
        it->second++;
}

void test::DRC_TEST_PROVIDER::accountCheck( const test::DRC_CONSTRAINT& constraintToTest )
{
    accountCheck( constraintToTest.GetParentRule() );
}


void test::DRC_TEST_PROVIDER::reportRuleStatistics()
{
    m_drcEngine->ReportAux("Rule hit statistics: ");
    for( auto stat : m_stats )
    {
        m_drcEngine->ReportAux( wxString::Format( " - rule '%s': %d hits ", stat.first->GetName().c_str(), stat.second ) );
    }
}

int test::DRC_TEST_PROVIDER::forEachGeometryItem( const std::vector<KICAD_T> aTypes, const LSET aLayers, std::function<int(BOARD_ITEM*)> aFunc )
{
    BOARD *brd = m_drcEngine->GetBoard();
    std::bitset<MAX_STRUCT_TYPE_ID> typeMask;
    int n = 0;

    if( aTypes.size() == 0 )
    {
        for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
        {
            typeMask[i] = true;
        }
    }
    else
    {
        for( int i = 0; i < aTypes.size(); i++ )
        {
            typeMask[ aTypes[i] ] = 1;
        }
    }

    /* case PCB_TRACE_T:
    case PCB_VIA_T:
    case PCB_ARC_T:*/
    for ( auto item : brd->Tracks() )
    {
        if( typeMask[ PCB_TRACE_T ] && item->Type() == PCB_TRACE_T )
        {
            aFunc( item );
            n++;
        }
        else if( typeMask[ PCB_VIA_T ] && item->Type() == PCB_VIA_T )
        {
            aFunc( item );
            n++;
        }
        else if( typeMask[ PCB_ARC_T ] && item->Type() == PCB_ARC_T )
        {
            aFunc( item );
            n++;
        }
    }

    /* case PCB_DIMENSION_T:
    case PCB_LINE_T:
    case PCB_TEXT_T:
    case PCB_TARGET_T:
    */
    for( auto item : brd->Drawings() )
    {
        if( typeMask[ PCB_DIMENSION_T ] && item->Type() == PCB_DIMENSION_T )
        {
            aFunc( item );
            n++;
        }
        else if( typeMask[ PCB_LINE_T ] && item->Type() == PCB_LINE_T )
        {
            aFunc( item );
            n++;
        }
        else if( typeMask[ PCB_TEXT_T ] && item->Type() == PCB_TEXT_T )
        {
            aFunc( item );
            n++;
        }
        else if( typeMask[ PCB_TARGET_T ] && item->Type() == PCB_TARGET_T )
        {
            aFunc( item );
            n++;
        }
    }

    for( auto item : brd->Zones() )
    {
        if( typeMask[ PCB_ZONE_AREA_T ] && item->Type() == PCB_ZONE_AREA_T )
        {
            aFunc( item );
            n++;
        }
    }

    for( auto mod : brd->Modules() )
    {
        if( typeMask[ PCB_MODULE_TEXT_T ] )
        {
            aFunc( &mod->Reference() );
            n++;
            aFunc( &mod->Value() );
            n++;
        }

        for( auto pad : mod->Pads() )
        {
            if( typeMask[ PCB_PAD_T ] && pad->Type() == PCB_PAD_T )
            {
                aFunc( pad );
                n++;
            }
        }

        for( auto dwg : mod->GraphicalItems() )
        {
            if( typeMask[ PCB_MODULE_TEXT_T ] && dwg->Type() == PCB_MODULE_TEXT_T )
            {
                aFunc( dwg );
                n++;
            }
            else if( typeMask[ PCB_MODULE_EDGE_T ] && dwg->Type() == PCB_MODULE_EDGE_T )
            {
                aFunc( dwg );
                n++;
            }
        }

        for( auto zone : mod->Zones() )
        {
            if( typeMask[ PCB_MODULE_ZONE_AREA_T ] && zone->Type() == PCB_MODULE_ZONE_AREA_T )
            {
                aFunc( zone );
                n++;
            }
        }
    }

    return n;
}

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>

DRC_TEST_PROVIDER::DRC_TEST_PROVIDER() :
    m_drcEngine( nullptr )
{
}


const wxString DRC_TEST_PROVIDER::GetName() const { return "<no name test>"; }
const wxString DRC_TEST_PROVIDER::GetDescription() const { return ""; }


void DRC_TEST_PROVIDER::Report( std::shared_ptr<DRC_ITEM> item )
{
    item->SetViolatingTest( this );
    m_drcEngine->Report( item, nullptr );
}


void DRC_TEST_PROVIDER::ReportWithMarker( std::shared_ptr<DRC_ITEM> item, VECTOR2I aMarkerPos )
{
    item->SetViolatingTest( this );
    MARKER_PCB* marker = new MARKER_PCB( item, wxPoint( aMarkerPos.x, aMarkerPos.y) );
    m_drcEngine->Report( item, marker );
}


void DRC_TEST_PROVIDER::ReportWithMarker( std::shared_ptr<DRC_ITEM> item, wxPoint aMarkerPos )
{
    item->SetViolatingTest( this );
    MARKER_PCB* marker = new MARKER_PCB( item, wxPoint( aMarkerPos.x, aMarkerPos.y) );
    m_drcEngine->Report( item, marker ); // fixme: create marker
}


void DRC_TEST_PROVIDER::ReportProgress( double aProgress )
{
    m_drcEngine->ReportProgress( aProgress );
}


void DRC_TEST_PROVIDER::ReportStage ( const wxString& aStageName, int index, int total )
{
    m_drcEngine->ReportStage( aStageName, index, total );
    ReportAux( aStageName );
}


void DRC_TEST_PROVIDER::ReportAux( wxString fmt, ... )
{
    va_list vargs;
    va_start( vargs, fmt );
    wxString str;
    str.PrintfV( fmt, vargs );
    va_end( vargs );
    m_drcEngine->ReportAux( str );
}


EDA_UNITS DRC_TEST_PROVIDER::userUnits() const
{
    return m_drcEngine->UserUnits();
}


void DRC_TEST_PROVIDER::accountCheck( const DRC_RULE* ruleToTest )
{
    auto it = m_stats.find( ruleToTest );

    if( it == m_stats.end() )
        m_stats[ ruleToTest ] = 1;
    else
        it->second++;
}


void DRC_TEST_PROVIDER::accountCheck( const DRC_CONSTRAINT& constraintToTest )
{
    accountCheck( constraintToTest.GetParentRule() );
}


void DRC_TEST_PROVIDER::reportRuleStatistics()
{
    if( !m_isRuleDriven )
        return;

    m_drcEngine->ReportAux("Rule hit statistics: ");

    for( const std::pair<const DRC_RULE* const, int>& stat : m_stats )
    {
        m_drcEngine->ReportAux( wxString::Format( " - rule '%s': %d hits ",
                                                  stat.first->m_Name,
                                                  stat.second ) );
    }
}


int DRC_TEST_PROVIDER::forEachGeometryItem( const std::vector<KICAD_T>& aTypes, LSET aLayers,
                                            const std::function<bool( BOARD_ITEM*)>& aFunc )
{
    BOARD *brd = m_drcEngine->GetBoard();
    std::bitset<MAX_STRUCT_TYPE_ID> typeMask;
    int n = 0;

    if( aTypes.size() == 0 )
    {
        for( int i = 0; i < MAX_STRUCT_TYPE_ID; i++ )
            typeMask[ i ] = true;
    }
    else
    {
        for( KICAD_T aType : aTypes )
            typeMask[ aType ] = true;
    }

    /* case PCB_TRACE_T:
    case PCB_VIA_T:
    case PCB_ARC_T:*/
    for( TRACK* item : brd->Tracks() )
    {
        if( (item->GetLayerSet() & aLayers).any() )
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
    }

    /* case PCB_DIMENSION_T:
    case PCB_LINE_T:
    case PCB_TEXT_T:
    case PCB_TARGET_T:
    */
    for( BOARD_ITEM* item : brd->Drawings() )
    {
        if( (item->GetLayerSet() & aLayers).any() )
        {
            if( typeMask[ PCB_DIMENSION_T ] && item->Type() == PCB_DIMENSION_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_LINE_T ] && item->Type() == PCB_LINE_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_TEXT_T ] && item->Type() == PCB_TEXT_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
            else if( typeMask[ PCB_TARGET_T ] && item->Type() == PCB_TARGET_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
        }
    }

    for( ZONE_CONTAINER* item : brd->Zones() )
    {
        if( (item->GetLayerSet() & aLayers).any() )
        {
            if( typeMask[ PCB_ZONE_AREA_T ] && item->Type() == PCB_ZONE_AREA_T )
            {
                if( !aFunc( item ) )
                    return n;

                n++;
            }
        }
    }

    for( MODULE* mod : brd->Modules() )
    {
        if( typeMask[ PCB_MODULE_TEXT_T ] )
        {
            if( (mod->Reference().GetLayerSet() & aLayers).any() )
            {
                if( !aFunc( &mod->Reference() ) )
                    return n;

                n++;
            }

            if( (mod->Value().GetLayerSet() & aLayers).any() )
            {
                if( !aFunc( &mod->Value() ) )
                    return n;

                n++;
            }
        }

        for( D_PAD* pad : mod->Pads() )
        {
            if( (pad->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_PAD_T ] && pad->Type() == PCB_PAD_T )
                {
                    if( !aFunc( pad ) )
                        return n;

                    n++;
                }
            }
        }

        for( BOARD_ITEM* dwg : mod->GraphicalItems() )
        {
            if( (dwg->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_MODULE_TEXT_T ] && dwg->Type() == PCB_MODULE_TEXT_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
                else if( typeMask[ PCB_MODULE_EDGE_T ] && dwg->Type() == PCB_MODULE_EDGE_T )
                {
                    if( !aFunc( dwg ) )
                        return n;

                    n++;
                }
            }
        }

        for( ZONE_CONTAINER* zone : mod->Zones() )
        {
            if( (zone->GetLayerSet() & aLayers).any() )
            {
                if( typeMask[ PCB_MODULE_ZONE_AREA_T ] && zone->Type() == PCB_MODULE_ZONE_AREA_T )
                {
                    if( ! aFunc( zone ) )
                        return n;

                    n++;
                }
            }
        }
    }

    return n;
}

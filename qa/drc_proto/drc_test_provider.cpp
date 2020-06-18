#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_test_provider.h>

test::DRC_TEST_PROVIDER::DRC_TEST_PROVIDER () 
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

void test::DRC_TEST_PROVIDER::Report( DRC_ITEM* item )
{
    item->SetViolatingTest( this );
    m_drcEngine->Report( item, nullptr );
}

void test::DRC_TEST_PROVIDER::ReportWithMarker( DRC_ITEM* item, wxPoint aMarkerPos )
{
    item->SetViolatingTest( this );
    m_drcEngine->Report( item, nullptr ); // fixme: create marker
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
    return false;
}

EDA_UNITS test::DRC_TEST_PROVIDER::userUnits() const
{
    return m_drcEngine->UserUnits();
}

void test::DRC_TEST_PROVIDER::accountCheck( test::DRC_RULE* ruleToTest )
{
    auto it = m_stats.find( ruleToTest );
    if( it == m_stats.end() )
        m_stats[ ruleToTest ] = 0;
    else
        it->second++;
}

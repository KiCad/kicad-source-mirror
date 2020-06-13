#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_test_provider.h>

test::DRC_TEST_PROVIDER::DRC_TEST_PROVIDER ( DRC_ENGINE *aDrc ) :
    m_drcEngine( aDrc )
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

void test::DRC_TEST_PROVIDER::AddMarkerToPcb( MARKER_PCB* aMarker )
{

}

EDA_UNITS test::DRC_TEST_PROVIDER::userUnits() const
{
    return m_drcEngine->UserUnits();
}
#include <boost/test/unit_test.hpp>
#include <schematic_utils/schematic_file_util.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <connection_graph.h>
#include <locale_io.h>

struct ERC_SIGNAL_TEST_FIXTURE
{
    ERC_SIGNAL_TEST_FIXTURE() : m_settingsManager( true ) {}
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};

BOOST_AUTO_TEST_SUITE( ERCSignal )

// Validate that pins incompatible across multi-net grouped signals trigger ERC.
// Schematic now defines 3 separate two-pin signals (X1-X2 power_out vs power_out,
// X3-X4 power_out vs output, X6-X7 output vs output). All three should produce
// pin-to-pin mismatch markers (warning or error depending on matrix severity).
BOOST_FIXTURE_TEST_CASE( ERCSignalPinToPin, ERC_SIGNAL_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "erc_signal_pin_to_pin" ), m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
    // Ensure signals are constructed before ERC tests.
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    // Manually promote all potential signals prior to ERC.
    {
        CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
        int idx = 1;
        for( const auto& pot : graph->GetPotentialSignals() )
        {
            if( !pot ) continue;
            wxString name = wxString::Format( wxS("ERC_SIG_%d"), idx++ );
            graph->CreateSignalFromPotential( pot.get(), name );
        }
    }
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestPinToPin();

    SHEETLIST_ERC_ITEMS_PROVIDER provider( m_schematic.get() );
    provider.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    const int expectedMismatches = 3;

    int mismatchCount = 0;
    for( size_t i = 0; i < provider.GetCount(); ++i )
    {
        auto item = provider.GetItem( i );
        if( !item )
            continue;
        auto code = item->GetErrorCode();
        if( code == ERCE_PIN_TO_PIN_WARNING || code == ERCE_PIN_TO_PIN_ERROR )
            mismatchCount++;
    }

    if( mismatchCount != expectedMismatches )
    {
        // Debug dump of signals when expectation fails
        auto* graph = m_schematic->ConnectionGraph();
        // Ensure signals are rebuilt explicitly in case RunERC did not force it
        graph->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
        const auto& signals = graph->GetSignals();
        std::ostringstream oss;
        oss << "DEBUG Pin-to-Pin mismatch failure: expected=" << expectedMismatches
            << " got=" << mismatchCount << " totalItems=" << provider.GetCount()
            << " signals=" << signals.size() << "\n";
        for( size_t si = 0; si < signals.size(); ++si )
        {
            const auto& sig = signals[si];
            if( !sig ) continue;
            oss << "  Signal[" << si << "] name='" << sig->GetName() << "' nets={";
            for( const auto& n : sig->GetNets() ) oss << n << ",";
            oss << "} pins={";
            // Collect pins per net via public GetAllSubgraphs API
            for( const auto& n : sig->GetNets() )
            {
                const auto& subgraphs = graph->GetAllSubgraphs( n );
                for( CONNECTION_SUBGRAPH* sg : subgraphs )
                {
                    for( SCH_ITEM* item : sg->GetItems() )
                    {
                        if( item->Type() == SCH_PIN_T )
                        {
                            SCH_PIN* p = static_cast<SCH_PIN*>( item );
                            oss << p->GetParentSymbol()->GetRef( &sg->GetSheet() ) << ":" << p->GetNumber() << "=" << (int) p->GetType() << ";";
                        }
                    }
                }
            }
            oss << "}\n";
        }
        // Also list all item codes for clarity
        oss << "All ERC item codes (severity filtered): ";
        for( size_t i = 0; i < provider.GetCount(); ++i )
        {
            auto it = provider.GetItem( i );
            if( it )
                oss << (int) it->GetErrorCode() << ",";
        }
        oss << "\n";

        BOOST_CHECK_MESSAGE( mismatchCount == expectedMismatches,
                             oss.str() + reportWriter.GetTextReport() );
    }
    else
        BOOST_CHECK_MESSAGE( mismatchCount == expectedMismatches,
                             "Expected 3 pin-to-pin mismatch errors but got " << mismatchCount
                             << "\n" << reportWriter.GetTextReport() );
}

// New test: ensure power input pin on one net is considered driven by power output on another net in same signal.
BOOST_FIXTURE_TEST_CASE( ERCSignalPowerInputDrivenAcrossSignal, ERC_SIGNAL_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "erc_signal_input_power_driven" ), m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    // Promote potential signals prior to ERC.
    {
        CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
        int idx = 1;
        for( const auto& pot : graph->GetPotentialSignals() )
        {
            if( !pot ) continue;
            wxString name = wxString::Format( wxS("ERC_SIG_%d"), idx++ );
            graph->CreateSignalFromPotential( pot.get(), name );
        }
    }
    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestPinToPin();

    SHEETLIST_ERC_ITEMS_PROVIDER provider( m_schematic.get() );
    provider.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    // Count any POWER PIN NOT DRIVEN markers (should be zero due to suppression across signal)
    int powerNotDriven = 0;
    for( size_t i = 0; i < provider.GetCount(); ++i )
    {
        auto item = provider.GetItem( i );
        if( item && item->GetErrorCode() == ERCE_POWERPIN_NOT_DRIVEN )
            powerNotDriven++;
    }

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );
    BOOST_CHECK_MESSAGE( powerNotDriven == 0, "Expected no ERCE_POWERPIN_NOT_DRIVEN errors due to cross-signal driver; got " << powerNotDriven << "\n" << reportWriter.GetTextReport() );
}

BOOST_AUTO_TEST_SUITE_END()

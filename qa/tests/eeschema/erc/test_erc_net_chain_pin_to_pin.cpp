#include <boost/test/unit_test.hpp>
#include <schematic_utils/schematic_file_util.h>
#include <erc/erc.h>
#include <erc/erc_settings.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <connection_graph.h>
#include <locale_io.h>
#include <pin_type.h>

struct ERC_SIGNAL_TEST_FIXTURE
{
    ERC_SIGNAL_TEST_FIXTURE() : m_settingsManager() {}
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
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "erc_net_chain_pin_to_pin" ), m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
    // Ensure signals are constructed before ERC tests.
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    // Manually promote all potential net chains
    {
        CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
        int idx = 1;
        for( const auto& pot : graph->GetPotentialNetChains() )
        {
            if( !pot ) continue;
            wxString name = wxString::Format( wxS("ERC_SIG_%d"), idx++ );
            graph->CreateNetChainFromPotential( pot.get(), name );
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
        const auto& netChains = graph->GetCommittedNetChains();
        std::ostringstream oss;
        oss << "DEBUG Pin-to-Pin mismatch failure: expected=" << expectedMismatches
            << " got=" << mismatchCount << " totalItems=" << provider.GetCount()
            << " chains=" << netChains.size() << "\n";
        for( size_t si = 0; si < netChains.size(); ++si )
        {
            const auto& sig = netChains[si];
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
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "erc_net_chain_input_power_driven" ), m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(), true );
    // Promote potential net chains prior to ERC.
    {
        CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
        int idx = 1;
        for( const auto& pot : graph->GetPotentialNetChains() )
        {
            if( !pot ) continue;
            wxString name = wxString::Format( wxS("ERC_SIG_%d"), idx++ );
            graph->CreateNetChainFromPotential( pot.get(), name );
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

// Regression test for chain-cross pin-to-pin gating. The chain-cross gate
// previously consulted IsTestEnabled( ERCE_PIN_TO_PIN_WARNING ) regardless of
// the resolved code, so silencing pin-to-pin warnings also suppressed
// pin-to-pin errors. The fix selects ercCode (warning vs error) from the
// resolved PIN_ERROR first, then gates IsTestEnabled( ercCode ). To make the
// regression observable, ERCE_PIN_TO_PIN_WARNING and ERCE_PIN_TO_PIN_ERROR
// must be independently controllable; that decoupling lives in
// ERC_SETTINGS::GetSeverity.
BOOST_FIXTURE_TEST_CASE( ERCSignalPinToPinErrorMatrixMapped, ERC_SIGNAL_TEST_FIXTURE )
{
    LOCALE_IO dummy;
    KI_TEST::LoadSchematic( m_settingsManager, wxString( "erc_net_chain_pin_to_pin" ), m_schematic );

    ERC_SETTINGS& settings = m_schematic->ErcSettings();
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;

    // Force OUTPUT-OUTPUT pairs to PP_ERROR so the X6/X7 chain-cross pair
    // resolves to ERCE_PIN_TO_PIN_ERROR rather than ERCE_PIN_TO_PIN_WARNING.
    settings.SetPinMapValue( ELECTRICAL_PINTYPE::PT_OUTPUT, ELECTRICAL_PINTYPE::PT_OUTPUT,
                             PIN_ERROR::PP_ERROR );

    // Silence ERCE_PIN_TO_PIN_WARNING while leaving ERCE_PIN_TO_PIN_ERROR active.
    // Under the OLD gate (always IsTestEnabled( ERCE_PIN_TO_PIN_WARNING )) this
    // would suppress the marker; under the NEW gate the resolved ercCode is
    // ERCE_PIN_TO_PIN_ERROR, which is still enabled, so the marker reports.
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_WARNING] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_PIN_TO_PIN_ERROR]   = RPT_SEVERITY_ERROR;

    BOOST_REQUIRE( !settings.IsTestEnabled( ERCE_PIN_TO_PIN_WARNING ) );
    BOOST_REQUIRE( settings.IsTestEnabled( ERCE_PIN_TO_PIN_ERROR ) );

    m_schematic->ConnectionGraph()->Recalculate( m_schematic->BuildSheetListSortedByPageNumbers(),
                                                 true );

    {
        CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();
        int idx = 1;

        for( const auto& pot : graph->GetPotentialNetChains() )
        {
            if( !pot )
                continue;

            wxString name = wxString::Format( wxS( "ERC_SIG_%d" ), idx++ );
            graph->CreateNetChainFromPotential( pot.get(), name );
        }
    }

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestPinToPin();

    SHEETLIST_ERC_ITEMS_PROVIDER provider( m_schematic.get() );
    provider.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    int errorCount = 0;
    int warningCount = 0;

    for( size_t i = 0; i < provider.GetCount(); ++i )
    {
        auto item = provider.GetItem( i );

        if( !item )
            continue;

        if( item->GetErrorCode() == ERCE_PIN_TO_PIN_ERROR )
            errorCount++;
        else if( item->GetErrorCode() == ERCE_PIN_TO_PIN_WARNING )
            warningCount++;
    }

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    BOOST_CHECK_MESSAGE( errorCount > 0,
                         "Expected ERCE_PIN_TO_PIN_ERROR markers to be reported when only"
                         " ERCE_PIN_TO_PIN_WARNING is silenced, but got "
                                 << errorCount << "\n"
                                 << reportWriter.GetTextReport() );

    BOOST_CHECK_MESSAGE( warningCount == 0,
                         "Expected zero ERCE_PIN_TO_PIN_WARNING markers when warning is silenced,"
                         " got " << warningCount << "\n"
                                 << reportWriter.GetTextReport() );
}

BOOST_AUTO_TEST_SUITE_END()

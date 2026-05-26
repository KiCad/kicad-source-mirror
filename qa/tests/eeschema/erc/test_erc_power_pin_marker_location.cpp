/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one at
 * http://www.gnu.org/licenses/
 */

/**
 * @file test_erc_power_pin_marker_location.cpp
 * Regression test for issue #24328.
 *
 * When ERC reports an ERCE_POWERPIN_NOT_DRIVEN error on a net that has both
 * PT_POWER_IN and PT_INPUT pins, the marker must be anchored to a PT_POWER_IN
 * pin (the pin the error message refers to) rather than a PT_INPUT pin.
 * The bug introduced by ce22bf37de ("ERC: Added show all errors") caused the
 * marker to be placed on a non-power-symbol pin even when that pin was a
 * regular input pin, contradicting the "Input Power pin not driven by any
 * Output Power pins" error text.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <connection_graph.h>
#include <schematic.h>
#include <sch_marker.h>
#include <sch_pin.h>
#include <sch_sheet_path.h>
#include <erc/erc_settings.h>
#include <erc/erc.h>
#include <erc/erc_item.h>
#include <erc/erc_report.h>
#include <settings/settings_manager.h>
#include <locale_io.h>


struct ERC_POWER_PIN_MARKER_FIXTURE
{
    ERC_POWER_PIN_MARKER_FIXTURE() :
            m_settingsManager()
    { }

    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


BOOST_FIXTURE_TEST_CASE( ERCPowerPinNotDrivenMarkerOnPowerInputPin, ERC_POWER_PIN_MARKER_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, wxS( "issue24328/issue24328" ), m_schematic );

    ERC_SETTINGS&                settings = m_schematic->ErcSettings();
    SHEETLIST_ERC_ITEMS_PROVIDER errors( m_schematic.get() );

    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_ISSUES] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_LIB_SYMBOL_MISMATCH] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_UNANNOTATED] = RPT_SEVERITY_IGNORE;
    settings.m_ERCSeverities[ERCE_POWERPIN_NOT_DRIVEN] = RPT_SEVERITY_ERROR;

    m_schematic->ConnectionGraph()->RunERC();

    ERC_TESTER tester( m_schematic.get() );
    tester.TestPinToPin();

    errors.SetSeverities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );

    ERC_REPORT reportWriter( m_schematic.get(), EDA_UNITS::MM );

    int                       powerPinErrors = 0;
    bool                      allPowerInputAnchors = true;
    std::vector<wxString>     anchorDescriptions;

    for( int ii = 0; ii < errors.GetCount(); ++ii )
    {
        std::shared_ptr<RC_ITEM> item = errors.GetItem( ii );

        if( item->GetErrorCode() != ERCE_POWERPIN_NOT_DRIVEN )
            continue;

        powerPinErrors++;

        SCH_SHEET_PATH sheetPath;
        SCH_ITEM*      mainItem = m_schematic->ResolveItem( item->GetMainItemID(),
                                                            &sheetPath, true );

        BOOST_REQUIRE_MESSAGE( mainItem,
                               "ERCE_POWERPIN_NOT_DRIVEN marker has no main item" );
        BOOST_REQUIRE_MESSAGE( mainItem->Type() == SCH_PIN_T,
                               "ERCE_POWERPIN_NOT_DRIVEN marker main item is not a pin" );

        SCH_PIN* pin = static_cast<SCH_PIN*>( mainItem );

        anchorDescriptions.push_back(
                wxString::Format( "ref=%s pin=%s type=%s",
                                  pin->GetParentSymbol()->GetRef( &sheetPath ),
                                  pin->GetNumber(),
                                  ElectricalPinTypeGetText( pin->GetType() ) ) );

        if( pin->GetType() != ELECTRICAL_PINTYPE::PT_POWER_IN )
            allPowerInputAnchors = false;
    }

    wxString anchorDump;

    for( const wxString& s : anchorDescriptions )
        anchorDump << s << wxS( "\n" );

    BOOST_CHECK_MESSAGE( powerPinErrors >= 1,
                         "Expected at least 1 ERCE_POWERPIN_NOT_DRIVEN error\n"
                         << reportWriter.GetTextReport() );

    BOOST_CHECK_MESSAGE( allPowerInputAnchors,
                         "At least one ERCE_POWERPIN_NOT_DRIVEN marker was anchored on a "
                         "non-PT_POWER_IN pin. The marker should refer to the pin the "
                         "error message is about.\nAnchors seen:\n"
                         << anchorDump.ToStdString()
                         << "\n"
                         << reportWriter.GetTextReport() );
}

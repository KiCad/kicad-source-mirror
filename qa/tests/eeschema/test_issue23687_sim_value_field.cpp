/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <dialog_sim_model.h>
#include <sim/sim_model.h>
#include <sim/sim_value.h>
#include <sch_field.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <reporter.h>

/**
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/23687
 *
 * Editing an inferred passive in the Simulation Model dialog parks the ${SIM.PARAMS} placeholder in
 * the Value field. If the user does not store the parameters in Value, the Value field must be left
 * exactly as it was, not overwritten by the placeholder (or a normalized value).
 */
BOOST_AUTO_TEST_SUITE( Issue23687SimValueField )


/**
 * Build the field set the dialog hands to WriteFields() for an inferred passive, mirroring
 * DIALOG_SIM_MODEL::TransferDataToWindow(). The inferred parameters are written to the Sim.* fields
 * and the Value field is parked with the placeholder. Returns the original Value text the dialog
 * remembers for restoration.
 */
static wxString buildDialogFields( LIB_SYMBOL& aSymbol, std::vector<SCH_FIELD>& aFields )
{
    aFields.clear();
    aFields.emplace_back( aSymbol.GetReferenceField() );
    aFields.emplace_back( aSymbol.GetValueField() );

    wxString deviceType;
    wxString modelType;
    wxString modelParams;
    wxString pinMap;

    BOOST_REQUIRE( SIM_MODEL::InferSimModel( aSymbol, &aFields, false, 0,
                                             SIM_VALUE_GRAMMAR::NOTATION::SI, &deviceType, &modelType,
                                             &modelParams, &pinMap ) );

    SetFieldValue( aFields, SIM_DEVICE_FIELD, deviceType.ToStdString() );

    if( !modelType.IsEmpty() )
        SetFieldValue( aFields, SIM_DEVICE_SUBTYPE_FIELD, modelType.ToStdString() );

    SetFieldValue( aFields, SIM_PARAMS_FIELD, modelParams.ToStdString() );
    SetFieldValue( aFields, SIM_PINS_FIELD, pinMap.ToStdString() );

    SCH_FIELD* valueField = FindField( aFields, FIELD_T::VALUE );
    wxString   original = valueField->GetText();
    valueField->SetText( wxT( "${SIM.PARAMS}" ) );

    return original;
}


BOOST_AUTO_TEST_CASE( ValuePreservedWhenNotStored )
{
    // "3R3" normalizes to "3.3" in the model, so a normalization-based restore would visibly change
    // the user's value. The original text must survive verbatim.
    auto symbol = std::make_unique<LIB_SYMBOL>( "symbol", nullptr );
    symbol->AddDrawItem( new SCH_PIN( symbol.get() ) );
    symbol->AddDrawItem( new SCH_PIN( symbol.get() ) );
    symbol->GetReferenceField().SetText( "R1" );
    symbol->GetValueField().SetText( "3R3" );

    std::vector<SCH_PIN*>  pins = symbol->GetPins();
    std::vector<SCH_FIELD> fields;
    wxString               original = buildDialogFields( *symbol, fields );

    WX_STRING_REPORTER         reporter;
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( fields, false, 0, pins, reporter );

    // Mirror DIALOG_SIM_MODEL::TransferDataFromWindow() with the checkbox unchecked.
    model->SetIsStoredInValue( false );
    model->WriteFields( fields );
    DIALOG_SIM_MODEL<LIB_SYMBOL>::RestoreInferredValue( fields, original, true,
                                                        model->IsStoredInValue() );

    BOOST_CHECK_EQUAL( GetFieldValue( &fields, FIELD_T::VALUE ), wxString( "3R3" ) );
}


BOOST_AUTO_TEST_CASE( ValueReplacedWhenStored )
{
    auto symbol = std::make_unique<LIB_SYMBOL>( "symbol", nullptr );
    symbol->AddDrawItem( new SCH_PIN( symbol.get() ) );
    symbol->AddDrawItem( new SCH_PIN( symbol.get() ) );
    symbol->GetReferenceField().SetText( "R1" );
    symbol->GetValueField().SetText( "3R3" );

    std::vector<SCH_PIN*>  pins = symbol->GetPins();
    std::vector<SCH_FIELD> fields;
    wxString               original = buildDialogFields( *symbol, fields );

    WX_STRING_REPORTER         reporter;
    std::unique_ptr<SIM_MODEL> model = SIM_MODEL::Create( fields, false, 0, pins, reporter );

    // Mirror DIALOG_SIM_MODEL::TransferDataFromWindow() with the checkbox checked. WriteFields()
    // replaces the placeholder with the model value and the restore is a no-op.
    model->SetIsStoredInValue( true );
    model->WriteFields( fields );
    DIALOG_SIM_MODEL<LIB_SYMBOL>::RestoreInferredValue( fields, original, true,
                                                        model->IsStoredInValue() );

    wxString value = GetFieldValue( &fields, FIELD_T::VALUE );
    BOOST_CHECK_MESSAGE( value != wxT( "${SIM.PARAMS}" ),
                         "Value left as placeholder when parameters stored in Value" );
    BOOST_CHECK( !value.IsEmpty() );
}


BOOST_AUTO_TEST_SUITE_END()

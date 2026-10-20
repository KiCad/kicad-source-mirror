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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <dialogs/dialog_kicad_merge_3way.h>
#include <qa_utils/env_var_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


namespace
{
class TEST_MERGE_DIALOG : public DIALOG_KICAD_MERGE_3WAY
{
public:
    TEST_MERGE_DIALOG( const KICAD_DIFF::MERGE_PLAN& aPlan ) :
            DIALOG_KICAD_MERGE_3WAY( nullptr, aPlan )
    {
    }

    using DIALOG_KICAD_MERGE_3WAY::m_listConflicts;
    using DIALOG_KICAD_MERGE_3WAY::m_radioAncestor;
    using DIALOG_KICAD_MERGE_3WAY::m_radioOurs;
    using DIALOG_KICAD_MERGE_3WAY::m_radioTheirs;
    using DIALOG_KICAD_MERGE_3WAY::m_radioUnresolved;

    void SelectConflict( int aIndex )
    {
        m_listConflicts->SetSelection( aIndex );
        wxCommandEvent event;
        OnConflictSelected( event );
    }

    void Choose( wxRadioButton* aChoice )
    {
        aChoice->SetValue( true );
        wxCommandEvent event;
        OnResolutionChanged( event );
    }
};


struct MERGE_DIALOG_FIXTURE
{
    KI_TEST::SCOPED_PROCESS_ENV_VAR autoResolution{ wxS( "KICAD_MERGETOOL_AUTO" ), std::nullopt };
    TEST_MERGE_DIALOG*              dialog = nullptr;

    ~MERGE_DIALOG_FIXTURE()
    {
        if( dialog )
            dialog->Destroy();
    }

    bool CreateDialog()
    {
        if( !KI_TEST::CanDoDisplayTests() )
        {
            BOOST_TEST_MESSAGE( "Skipping test - no display available" );
            return false;
        }

        KICAD_DIFF::MERGE_PLAN plan;

        for( int i = 0; i < 3; ++i )
        {
            KICAD_DIFF::ITEM_RESOLUTION action;
            action.id.push_back( KIID() );
            plan.unresolved.push_back( action.id );
            plan.actions.push_back( action );
        }

        dialog = new TEST_MERGE_DIALOG( plan );
        return true;
    }
};
} // namespace


BOOST_FIXTURE_TEST_SUITE( MergeDialog, MERGE_DIALOG_FIXTURE )


BOOST_AUTO_TEST_CASE( ConflictsStartUnresolved )
{
    if( !CreateDialog() )
        return;

    BOOST_CHECK_EQUAL( dialog->m_listConflicts->GetSelection(), 0 );
    BOOST_CHECK( dialog->m_radioUnresolved->GetValue() );
    BOOST_CHECK( !dialog->m_radioOurs->GetValue() );
    BOOST_CHECK( !dialog->m_radioTheirs->GetValue() );
    BOOST_CHECK( !dialog->m_radioAncestor->GetValue() );
    BOOST_CHECK( !dialog->m_sdbSizerApply->IsEnabled() );

    for( const KICAD_DIFF::ITEM_RESOLUTION& action : dialog->GetResolvedPlan().actions )
        BOOST_CHECK( action.kind == KICAD_DIFF::ITEM_RES::KEEP );
}


BOOST_AUTO_TEST_CASE( SelectingAnotherConflictDoesNotChooseOurs )
{
    if( !CreateDialog() )
        return;

    dialog->Choose( dialog->m_radioOurs );
    BOOST_CHECK( dialog->GetResolvedPlan().actions[0].kind == KICAD_DIFF::ITEM_RES::TAKE_OURS );

    dialog->SelectConflict( 1 );
    BOOST_CHECK( dialog->m_radioUnresolved->GetValue() );
    BOOST_CHECK( !dialog->m_radioOurs->GetValue() );
    BOOST_CHECK( !dialog->m_radioTheirs->GetValue() );
    BOOST_CHECK( !dialog->m_radioAncestor->GetValue() );
    BOOST_CHECK( dialog->GetResolvedPlan().actions[1].kind == KICAD_DIFF::ITEM_RES::KEEP );
    BOOST_CHECK( !dialog->m_sdbSizerApply->IsEnabled() );

    dialog->SelectConflict( 0 );
    BOOST_CHECK( dialog->m_radioOurs->GetValue() );
    BOOST_CHECK( !dialog->m_radioUnresolved->GetValue() );
    BOOST_CHECK( dialog->GetResolvedPlan().actions[0].kind == KICAD_DIFF::ITEM_RES::TAKE_OURS );
}


BOOST_AUTO_TEST_CASE( ApplyRequiresAChoiceForEveryConflict )
{
    if( !CreateDialog() )
        return;

    dialog->Choose( dialog->m_radioOurs );
    BOOST_CHECK( !dialog->m_sdbSizerApply->IsEnabled() );

    dialog->SelectConflict( 1 );
    dialog->Choose( dialog->m_radioTheirs );
    BOOST_CHECK( dialog->GetResolvedPlan().actions[1].kind == KICAD_DIFF::ITEM_RES::TAKE_THEIRS );
    BOOST_CHECK( !dialog->m_sdbSizerApply->IsEnabled() );

    dialog->SelectConflict( 2 );
    dialog->Choose( dialog->m_radioAncestor );
    BOOST_CHECK( dialog->GetResolvedPlan().actions[2].kind == KICAD_DIFF::ITEM_RES::TAKE_ANCESTOR );
    BOOST_CHECK( dialog->m_sdbSizerApply->IsEnabled() );

    dialog->SelectConflict( 0 );
    dialog->Choose( dialog->m_radioUnresolved );
    BOOST_CHECK( dialog->GetResolvedPlan().actions[0].kind == KICAD_DIFF::ITEM_RES::KEEP );
    BOOST_CHECK( !dialog->m_sdbSizerApply->IsEnabled() );

    dialog->Choose( dialog->m_radioOurs );
    BOOST_CHECK( dialog->m_sdbSizerApply->IsEnabled() );
}


BOOST_AUTO_TEST_SUITE_END()

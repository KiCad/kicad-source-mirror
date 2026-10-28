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
 * WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <optional>
#include <widgets/paged_dialog.h>
#include <wx/app.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/grid.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/srchctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/treectrl.h>


namespace
{
bool canRunDisplayTests( boost::unit_test::test_unit_id )
{
    return KI_TEST::CanDoDisplayTests();
}


class TEST_SEARCH_DIALOG : public PAGED_DIALOG
{
public:
    explicit TEST_SEARCH_DIALOG( bool aExcludeHeading ) :
            PAGED_DIALOG( nullptr, wxS( "Search regression test" ), false, false )
    {
        SetEvtHandlerEnabled( false );
        m_treebook->AddPage( new wxPanel( m_treebook ), wxS( "General" ) );
        m_treebook->AddLazyPage(
                [this, aExcludeHeading]( wxWindow* aParent ) -> wxWindow*
                {
                    wxPanel* panel = new wxPanel( aParent );
                    m_heading = new wxStaticText( panel, wxID_ANY, wxS( "Member Nets: DDR4_DQ0" ) );
                    new wxStaticText( panel, wxID_ANY, wxS( "Static setting" ) );

                    if( aExcludeHeading )
                        ExcludeHeading();

                    return panel;
                },
                wxS( "Chains" ) );
        SetEvtHandlerEnabled( true );
    }

    void SetLiveTerm( const wxString& aTerm ) { m_liveTerm = aTerm; }

    void SetTermOnPageChange( const wxString& aTerm ) { m_termOnPageChange = aTerm; }

    void ExcludeHeading() { ExcludeFromSearchIndex( m_heading ); }

    wxNotebook* AddNotebook()
    {
        m_notebook = new wxNotebook( m_treebook->ResolvePageForSearch( 1 ), wxID_ANY );

        wxPanel* first = new wxPanel( m_notebook );
        new wxStaticText( first, wxID_ANY, wxS( "Target impedance" ) );
        m_notebook->AddPage( first, wxS( "USB3Profile" ) );

        wxPanel* second = new wxPanel( m_notebook );
        new wxStaticText( second, wxID_ANY, wxS( "Propagation delay" ) );
        m_notebook->AddPage( second, wxS( "SecondProfile" ) );

        return m_notebook;
    }

    bool SearchMatches( const wxString& aQuery )
    {
        wxSearchCtrl* search = dynamic_cast<wxSearchCtrl*>( FindWindowByName( wxSearchCtrlNameStr, this ) );
        BOOST_REQUIRE( search );
        search->SetValue( aQuery );
        return PageMatches();
    }

    bool PageMatches() const { return m_treebook->GetTreeCtrl()->IsBold( m_treebook->GetPageTreeItem( 1 ) ); }

private:
    void onPageChanged( wxBookCtrlEvent& aEvent ) override
    {
        PAGED_DIALOG::onPageChanged( aEvent );

        if( m_termOnPageChange )
        {
            m_liveTerm = *m_termOnPageChange;
            m_termOnPageChange.reset();
        }
    }

    void AppendSearchTerms( size_t aPage, std::vector<wxString>& aTerms ) const override
    {
        PAGED_DIALOG::AppendSearchTerms( aPage, aTerms );

        if( aPage != 1 )
            return;

        aTerms.push_back( m_liveTerm );

        if( m_notebook )
        {
            for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
                aTerms.push_back( m_notebook->GetPageText( i ) );
        }
    }

    wxStaticText* m_heading = nullptr;
    wxString      m_liveTerm;
    wxNotebook*   m_notebook = nullptr;

    std::optional<wxString> m_termOnPageChange;
};
} // namespace


BOOST_AUTO_TEST_SUITE( PagedDialogSearch, *boost::unit_test::precondition( canRunDisplayTests ) )


BOOST_AUTO_TEST_CASE( RenamedAndRemovedTermsDoNotMatchCachedHeading )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    dialog->SetLiveTerm( wxS( "DDR4_DQ0" ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "DDR4_DQ0" ) ) );

    // Leave the heading unchanged: only the edit buffer supplies current names.
    dialog->SetLiveTerm( wxS( "PCIE_TX0" ) );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "DDR4_DQ0" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "PCIE_TX0" ) ) );

    dialog->SetLiveTerm( wxEmptyString );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "PCIE_TX0" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Static setting" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( WordsSplitAcrossLabelsStillMatchTheirPage )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( false );

    // "DDR4_DQ0" is in the heading and "Static" in another label, so no single label holds
    // both. The page shows both, so it has to match.
    BOOST_CHECK( dialog->SearchMatches( wxS( "DDR4_DQ0 Static" ) ) );

    // The page title is indexed separately again, so a word from it pairs with a label too.
    BOOST_CHECK( dialog->SearchMatches( wxS( "Chains DDR4_DQ0" ) ) );

    // A word that is nowhere on the page still rules it out.
    BOOST_CHECK( !dialog->SearchMatches( wxS( "DDR4_DQ0 Impedance" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( WordsSplitBetweenLabelsAndCurrentValuesMatch )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );

    // The heading is excluded, so "PCIE_TX0" can only come from the live value while
    // "Static" comes from the cached labels.
    dialog->SetLiveTerm( wxS( "PCIE_TX0" ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "PCIE_TX0 Static" ) ) );

    dialog->SetLiveTerm( wxEmptyString );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "PCIE_TX0 Static" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( ExcludingControlInvalidatesExistingIndex )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( false );
    BOOST_CHECK( dialog->SearchMatches( wxS( "DDR4_DQ0" ) ) );

    dialog->ExcludeHeading();
    BOOST_CHECK( !dialog->SearchMatches( wxS( "DDR4_DQ0" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Static setting" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( ExcludingBookTitlesKeepsPageLabelsAndCurrentNames )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    wxNotebook*         book = dialog->AddNotebook();
    BOOST_CHECK( dialog->SearchMatches( wxS( "USB3Profile" ) ) );

    dialog->ExcludeBookTitlesFromSearchIndex( book );
    book->SetPageText( 0, wxS( "HdmiProfile" ) );

    BOOST_CHECK( !dialog->SearchMatches( wxS( "USB3Profile" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "HdmiProfile" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Target impedance" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Propagation delay" ) ) );

    book->DeletePage( 0 );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "HdmiProfile" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "SecondProfile" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( PageChangesRefreshMatchesWithoutNavigatingAgain )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    dialog->SetLiveTerm( wxS( "DDR4_DQ0" ) );
    dialog->SetTermOnPageChange( wxS( "PCIE_TX0" ) );

    BOOST_CHECK( dialog->SearchMatches( wxS( "DDR4_DQ0" ) ) );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( !dialog->PageMatches() );
    BOOST_CHECK_EQUAL( dialog->GetTreebook()->GetSelection(), 1 );
    BOOST_CHECK( dialog->SearchMatches( wxS( "PCIE_TX0" ) ) );

    dialog->SetTermOnPageChange( wxEmptyString );
    dialog->GetTreebook()->SetSelection( 0 );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( !dialog->PageMatches() );
    BOOST_CHECK_EQUAL( dialog->GetTreebook()->GetSelection(), 0 );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( RevealedControlsRefreshMatchesWithoutChangingPages )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    wxPanel*            details = new wxPanel( dialog->GetTreebook()->ResolvePageForSearch( 1 ) );
    new wxStaticText( details, wxID_ANY, wxS( "Comment:" ) );
    wxStaticText* background = new wxStaticText( details, wxID_ANY, wxS( "Sheet backgrounds" ) );
    details->Hide();

    BOOST_CHECK( !dialog->SearchMatches( wxS( "comment" ) ) );
    details->Show();
    dialog->InvalidateSearchIndex( details );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( dialog->PageMatches() );
    BOOST_CHECK_EQUAL( dialog->GetTreebook()->GetSelection(), 0 );
    BOOST_CHECK( !dialog->GetTreebook()->GetResolvedPage( 1 ) );

    BOOST_CHECK( dialog->SearchMatches( wxS( "sheet backgrounds" ) ) );
    background->Hide();
    dialog->InvalidateSearchIndex( background );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( !dialog->PageMatches() );

    background->Show();
    dialog->InvalidateSearchIndex( background );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( dialog->PageMatches() );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Static setting" ) ) );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "DDR4_DQ0" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( AddedAndRemovedNotebookPagesRefreshSearchLabels )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    wxNotebook*         book = new wxNotebook( dialog->GetTreebook()->ResolvePageForSearch( 1 ), wxID_ANY );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "target impedance" ) ) );

    wxPanel* profile = new wxPanel( book );
    new wxStaticText( profile, wxID_ANY, wxS( "Target impedance:" ) );
    book->AddPage( profile, wxS( "USB3Profile" ) );
    dialog->InvalidateSearchIndex( book );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( dialog->PageMatches() );
    BOOST_CHECK_EQUAL( dialog->GetTreebook()->GetSelection(), 0 );

    BOOST_CHECK( dialog->SearchMatches( wxS( "USB3Profile" ) ) );
    dialog->ExcludeBookTitlesFromSearchIndex( book );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "USB3Profile" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "target impedance" ) ) );

    book->RemovePage( 0 );
    dialog->InvalidateSearchIndex( book );
    wxTheApp->ProcessPendingEvents();
    BOOST_CHECK( !dialog->PageMatches() );
    profile->Destroy();
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( TextValuesReflectUnsavedEditsAndNewControls )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    wxWindow*           page = dialog->GetTreebook()->ResolvePageForSearch( 1 );
    wxTextCtrl*         name = new wxTextCtrl( page, wxID_ANY, wxS( "Isola370HR" ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Isola370HR" ) ) );

    name->ChangeValue( wxS( "Rogers4350B" ) );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "Isola370HR" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "Rogers4350B" ) ) );
    name->Clear();
    BOOST_CHECK( !dialog->SearchMatches( wxS( "Rogers4350B" ) ) );

    wxTextCtrl* added = new wxTextCtrl( page, wxID_ANY, wxS( "NewComponentClass" ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "NewComponentClass" ) ) );
    added->Hide();
    BOOST_CHECK( !dialog->SearchMatches( wxS( "NewComponentClass" ) ) );

    new wxTextCtrl( page, wxID_ANY, wxS( "HiddenPassword" ), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "HiddenPassword" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( ChoiceEntriesAndEditableCombosStayCurrent )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    wxWindow*           page = dialog->GetTreebook()->ResolvePageForSearch( 1 );
    wxChoice*           choice = new wxChoice( page, wxID_ANY );
    choice->Append( wxS( "SelectedFinish" ) );
    choice->Append( wxS( "UnselectedFinish" ) );
    choice->SetSelection( 0 );
    BOOST_CHECK( dialog->SearchMatches( wxS( "UnselectedFinish" ) ) );

    choice->SetString( 1, wxS( "ReplacementFinish" ) );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "UnselectedFinish" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "ReplacementFinish" ) ) );
    choice->Delete( 1 );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "ReplacementFinish" ) ) );

    wxComboBox* combo = new wxComboBox( page, wxID_ANY, wxS( "TypedExpression" ) );
    combo->Append( wxS( "SuggestedExpression" ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "TypedExpression" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "SuggestedExpression" ) ) );
    combo->ChangeValue( wxS( "UpdatedExpression" ) );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "TypedExpression" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "UpdatedExpression" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_CASE( GridSearchReadsTheEditorWithoutCommittingIt )
{
    TEST_SEARCH_DIALOG* dialog = new TEST_SEARCH_DIALOG( true );
    wxGrid*             grid = new wxGrid( dialog->GetTreebook()->ResolvePageForSearch( 1 ), wxID_ANY );
    grid->CreateGrid( 1, 2 );
    grid->SetCellValue( 0, 0, wxS( "OldVariableName" ) );
    grid->SetCellValue( 0, 1, wxS( "VariableContent" ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "OldVariableName" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "VariableContent" ) ) );

    dialog->Show( true );
    grid->SetGridCursor( 0, 0 );
    grid->EnableCellEditControl();
    BOOST_REQUIRE( grid->IsCellEditControlEnabled() );
    wxGridCellEditor* editor = grid->GetCellEditor( 0, 0 );
    wxTextEntryBase*  text = dynamic_cast<wxTextEntryBase*>( editor->GetControl() );
    BOOST_REQUIRE( text );
    text->ChangeValue( wxS( "DraftVariableName" ) );
    editor->DecRef();

    BOOST_CHECK( !dialog->SearchMatches( wxS( "OldVariableName" ) ) );
    BOOST_CHECK( dialog->SearchMatches( wxS( "DraftVariableName" ) ) );
    BOOST_CHECK_EQUAL( grid->GetCellValue( 0, 0 ), wxS( "OldVariableName" ) );
    BOOST_CHECK( grid->IsCellEditControlEnabled() );

    grid->DisableCellEditControl();
    grid->DeleteRows( 0 );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "DraftVariableName" ) ) );
    BOOST_CHECK( !dialog->SearchMatches( wxS( "VariableContent" ) ) );
    dialog->Destroy();
}


BOOST_AUTO_TEST_SUITE_END()

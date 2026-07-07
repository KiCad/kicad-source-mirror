/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23743
 *
 * Exercises LIB_SYMBOL::SyncFieldsFromParent, the non-GUI core driven by the symbol
 * editor's "Update Symbol Fields..." action for derived symbols.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <sch_field.h>
#include <template_fieldnames.h>

BOOST_AUTO_TEST_SUITE( SyncDerivedFields )


/**
 * A parent value change and a brand-new parent field both propagate to the derived child.
 */
BOOST_AUTO_TEST_CASE( PullsValueAndNewFieldFromParent )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );
    std::unique_ptr<LIB_SYMBOL> child  = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    BOOST_REQUIRE( child->IsDerived() );

    // Parent gains a value and a manufacturer field after the child already exists.
    parent->GetValueField().SetText( "10k" );

    SCH_FIELD* mfg = new SCH_FIELD( parent.get(), FIELD_T::USER, "Manufacturer" );
    mfg->SetText( "ACME" );
    parent->AddField( mfg );

    BOOST_CHECK( child->GetValueField().GetText().IsEmpty() );
    BOOST_CHECK_EQUAL( child->GetField( wxString( "Manufacturer" ) ), nullptr );

    LIB_FIELD_SYNC_OPTIONS options;
    options.m_resetText = true;

    child->SyncFieldsFromParent( options );

    BOOST_CHECK_EQUAL( child->GetValueField().GetText(), wxString( "10k" ) );

    SCH_FIELD* childMfg = child->GetField( wxString( "Manufacturer" ) );
    BOOST_REQUIRE( childMfg );
    BOOST_CHECK_EQUAL( childMfg->GetText(), wxString( "ACME" ) );
}


/**
 * Only the field names in m_updateFields are touched when m_updateAllFields is false.
 */
BOOST_AUTO_TEST_CASE( RespectsSelectedFieldSubset )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );

    SCH_FIELD* mfg = new SCH_FIELD( parent.get(), FIELD_T::USER, "Manufacturer" );
    mfg->SetText( "ACME" );
    parent->AddField( mfg );

    SCH_FIELD* mpn = new SCH_FIELD( parent.get(), FIELD_T::USER, "MPN" );
    mpn->SetText( "12345" );
    parent->AddField( mpn );

    std::unique_ptr<LIB_SYMBOL> child = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    LIB_FIELD_SYNC_OPTIONS options;
    options.m_resetText = true;
    options.m_updateAllFields = false;
    options.m_updateFields = { wxString( "Manufacturer" ) };

    child->SyncFieldsFromParent( options );

    BOOST_REQUIRE( child->GetField( wxString( "Manufacturer" ) ) );
    BOOST_CHECK_EQUAL( child->GetField( wxString( "Manufacturer" ) )->GetText(), wxString( "ACME" ) );

    // MPN was not selected, so it must not have been pulled in.
    BOOST_CHECK_EQUAL( child->GetField( wxString( "MPN" ) ), nullptr );
}


/**
 * A mandatory field is selected by its canonical name.  The dialog populates the listbox
 * with translated mandatory names, so it must feed canonical names into m_updateFields; a
 * non-English UI would otherwise never update mandatory fields (e.g. Value).
 */
BOOST_AUTO_TEST_CASE( MandatoryFieldSelectedByCanonicalName )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );
    parent->GetValueField().SetText( "10k" );

    std::unique_ptr<LIB_SYMBOL> child = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    LIB_FIELD_SYNC_OPTIONS options;
    options.m_resetText = true;
    options.m_updateAllFields = false;
    options.m_updateFields = { GetCanonicalFieldName( FIELD_T::VALUE ) };

    child->SyncFieldsFromParent( options );

    BOOST_CHECK_EQUAL( child->GetValueField().GetText(), wxString( "10k" ) );
}


/**
 * An empty selection (dialog "Select None") touches nothing, even with reset flags set.
 */
BOOST_AUTO_TEST_CASE( EmptySelectionUpdatesNothing )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );
    parent->GetValueField().SetText( "10k" );

    SCH_FIELD* mfg = new SCH_FIELD( parent.get(), FIELD_T::USER, "Manufacturer" );
    mfg->SetText( "ACME" );
    parent->AddField( mfg );

    std::unique_ptr<LIB_SYMBOL> child = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    LIB_FIELD_SYNC_OPTIONS options;
    options.m_updateAllFields = false;
    options.m_resetText = true;

    child->SyncFieldsFromParent( options );

    BOOST_CHECK( child->GetValueField().GetText().IsEmpty() );
    BOOST_CHECK_EQUAL( child->GetField( wxString( "Manufacturer" ) ), nullptr );
}


/**
 * A child-only field is dropped only when removeExtraFields is requested.
 */
BOOST_AUTO_TEST_CASE( RemoveExtraFieldsOptional )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );
    std::unique_ptr<LIB_SYMBOL> child  = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    SCH_FIELD* extra = new SCH_FIELD( child.get(), FIELD_T::USER, "ChildOnly" );
    extra->SetText( "keep-me" );
    child->AddField( extra );

    // Without removeExtraFields the child-only field survives.
    LIB_FIELD_SYNC_OPTIONS keep;
    child->SyncFieldsFromParent( keep );
    BOOST_REQUIRE( child->GetField( wxString( "ChildOnly" ) ) );

    // With removeExtraFields it is dropped, since the parent has no such field.
    LIB_FIELD_SYNC_OPTIONS strip;
    strip.m_removeExtraFields = true;
    child->SyncFieldsFromParent( strip );
    BOOST_CHECK_EQUAL( child->GetField( wxString( "ChildOnly" ) ), nullptr );
}


/**
 * Visibility flags are reconciled from the parent only when requested.
 */
BOOST_AUTO_TEST_CASE( ResetVisibilityFromParent )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );

    SCH_FIELD* mfg = new SCH_FIELD( parent.get(), FIELD_T::USER, "Manufacturer" );
    mfg->SetText( "ACME" );
    mfg->SetVisible( true );
    parent->AddField( mfg );

    std::unique_ptr<LIB_SYMBOL> child = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    // Pull the field in first (hidden by default for a fresh user field).
    LIB_FIELD_SYNC_OPTIONS add;
    add.m_resetText = true;
    child->SyncFieldsFromParent( add );

    SCH_FIELD* childMfg = child->GetField( wxString( "Manufacturer" ) );
    BOOST_REQUIRE( childMfg );
    childMfg->SetVisible( false );

    LIB_FIELD_SYNC_OPTIONS vis;
    vis.m_resetVisibility = true;
    child->SyncFieldsFromParent( vis );

    BOOST_CHECK( child->GetField( wxString( "Manufacturer" ) )->IsVisible() );
}


/**
 * A non-derived symbol is left untouched (the action is meaningless without a parent).
 */
BOOST_AUTO_TEST_CASE( RootSymbolUnchanged )
{
    std::unique_ptr<LIB_SYMBOL> root = std::make_unique<LIB_SYMBOL>( "root" );
    root->GetValueField().SetText( "orig" );

    BOOST_REQUIRE( root->IsRoot() );

    LIB_FIELD_SYNC_OPTIONS options;
    options.m_resetText = true;
    root->SyncFieldsFromParent( options );

    BOOST_CHECK_EQUAL( root->GetValueField().GetText(), wxString( "orig" ) );
}


/**
 * The "Update Symbol Fields" action gate (and its tool handler) both key off
 * CanUpdateFieldsFromParent(); it must be true for a derived symbol and false for a root.
 * This is the predicate whose mis-wiring was issue 23743.
 */
BOOST_AUTO_TEST_CASE( CanUpdateFieldsGate )
{
    std::unique_ptr<LIB_SYMBOL> parent = std::make_unique<LIB_SYMBOL>( "parent" );
    std::unique_ptr<LIB_SYMBOL> child  = std::make_unique<LIB_SYMBOL>( "child", parent.get() );

    BOOST_CHECK( !parent->CanUpdateFieldsFromParent() );
    BOOST_CHECK( child->CanUpdateFieldsFromParent() );
}


BOOST_AUTO_TEST_SUITE_END()

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
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <template_fieldnames.h>


BOOST_AUTO_TEST_SUITE( TemplateFieldNames )


/**
 * Mandatory field names collide with any case variant (the s-expression parser folds case
 * variants of mandatory fields onto the canonical mandatory field).
 */
BOOST_AUTO_TEST_CASE( MandatoryFieldsAreCaseInsensitive )
{
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Value" ), wxS( "VALUE" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "VALUE" ), wxS( "value" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Reference" ), wxS( "REFERENCE" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Footprint" ), wxS( "footprint" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Datasheet" ), wxS( "DATASHEET" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Description" ), wxS( "DESCRIPTION" ) ) );
}


/**
 * User-defined field names are case-sensitive in storage, so two user fields that differ
 * only in case (e.g. "Manufacturer" vs "MANUFACTURER") must be treated as distinct.  See
 * issue #24021.
 */
BOOST_AUTO_TEST_CASE( UserFieldsAreCaseSensitive )
{
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Manufacturer" ), wxS( "MANUFACTURER" ) ) );
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "MPN" ), wxS( "mpn" ) ) );
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Supplier" ), wxS( "supplier" ) ) );
}


/**
 * Identical names always match (regardless of case).
 */
BOOST_AUTO_TEST_CASE( IdenticalNamesMatch )
{
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Manufacturer" ), wxS( "Manufacturer" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Value" ), wxS( "Value" ) ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "" ), wxS( "" ) ) );
}


/**
 * Different user-defined names never match.
 */
BOOST_AUTO_TEST_CASE( DifferentUserNamesDoNotMatch )
{
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Manufacturer" ), wxS( "Supplier" ) ) );
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "MPN" ), wxS( "Description" ) ) );
}


/**
 * A user field whose name only superficially resembles a mandatory canonical name is not
 * treated as a duplicate of an unrelated user field.
 */
BOOST_AUTO_TEST_CASE( MandatoryNameDoesNotPullInUnrelatedNames )
{
    // "Value" matches "VALUE" (mandatory case-insensitive) but does not match "MyValue".
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Value" ), wxS( "MyValue" ) ) );
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "VALUE" ), wxS( "ValueAdded" ) ) );
}


/**
 * Sheet mandatory fields (Sheetname, Sheetfile) are folded case-insensitively by the parser
 * for SCH_SHEET_T parents, so the helper must collide their case variants when given the
 * SHEET_MANDATORY_FIELDS set.
 */
BOOST_AUTO_TEST_CASE( SheetMandatoryFieldsAreCaseInsensitive )
{
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Sheetname" ), wxS( "SHEETNAME" ),
                                          SHEET_MANDATORY_FIELDS ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Sheetfile" ), wxS( "SHEETFILE" ),
                                          SHEET_MANDATORY_FIELDS ) );

    // Symbol mandatory names ("Value") are NOT mandatory in a sheet context, so they remain
    // case-sensitive there.
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Value" ), wxS( "VALUE" ),
                                           SHEET_MANDATORY_FIELDS ) );
}


/**
 * Global label mandatory fields (Intersheetrefs) are folded case-insensitively by the parser
 * for SCH_GLOBAL_LABEL_T parents.
 */
BOOST_AUTO_TEST_CASE( GlobalLabelMandatoryFieldsAreCaseInsensitive )
{
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Intersheetrefs" ), wxS( "INTERSHEETREFS" ),
                                          GLOBALLABEL_MANDATORY_FIELDS ) );

    // Symbol mandatory names are NOT mandatory in a global-label context.
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Value" ), wxS( "VALUE" ),
                                           GLOBALLABEL_MANDATORY_FIELDS ) );
}


/**
 * An empty mandatory-field set (used by hierarchical / regular / directive labels which have
 * no mandatory user-visible fields) makes every comparison case-sensitive.
 */
BOOST_AUTO_TEST_CASE( EmptyMandatorySetIsAlwaysCaseSensitive )
{
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Value" ), wxS( "VALUE" ), {} ) );
    BOOST_CHECK( !FieldNamesAreDuplicates( wxS( "Manufacturer" ), wxS( "MANUFACTURER" ), {} ) );
    BOOST_CHECK( FieldNamesAreDuplicates( wxS( "Same" ), wxS( "Same" ), {} ) );
}


static bool hasTemplate( TEMPLATES& aTemplates, const wxString& aName )
{
    for( const TEMPLATE_FIELDNAME& tfn : aTemplates.GetTemplateFieldNames() )
    {
        if( tfn.m_Name == aName )
            return true;
    }

    return false;
}


/**
 * A field-name template that only differs in case from a mandatory canonical field (e.g.
 * "VALUE" vs "Value") must be rejected, because the s-expression parser folds it onto the
 * mandatory field and it can never exist as a distinct user field.  See issue #24021.
 */
BOOST_AUTO_TEST_CASE( TemplateRejectsMandatoryCaseVariant )
{
    TEMPLATES templates;

    templates.AddTemplateFieldName( TEMPLATE_FIELDNAME( wxS( "VALUE" ) ), false );
    templates.AddTemplateFieldName( TEMPLATE_FIELDNAME( wxS( "reference" ) ), false );

    BOOST_CHECK( !hasTemplate( templates, wxS( "VALUE" ) ) );
    BOOST_CHECK( !hasTemplate( templates, wxS( "reference" ) ) );
}


/**
 * Two user field-name templates that differ only in case (e.g. "Manufacturer" vs
 * "MANUFACTURER") are distinct and both retained.  See issue #24021.
 */
BOOST_AUTO_TEST_CASE( TemplateKeepsUserCaseVariantsDistinct )
{
    TEMPLATES templates;

    templates.AddTemplateFieldName( TEMPLATE_FIELDNAME( wxS( "Manufacturer" ) ), false );
    templates.AddTemplateFieldName( TEMPLATE_FIELDNAME( wxS( "MANUFACTURER" ) ), false );

    BOOST_CHECK( hasTemplate( templates, wxS( "Manufacturer" ) ) );
    BOOST_CHECK( hasTemplate( templates, wxS( "MANUFACTURER" ) ) );
    BOOST_CHECK_EQUAL( templates.GetTemplateFieldNames().size(), 2 );
}


BOOST_AUTO_TEST_SUITE_END()

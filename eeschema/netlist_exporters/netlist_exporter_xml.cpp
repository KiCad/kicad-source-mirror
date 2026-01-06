/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include "netlist_exporter_xml.h"

#include <build_version.h>
#include <common.h>     // for ExpandTextVars
#include <sch_base_frame.h>
#include <sch_group.h>
#include <string_utils.h>
#include <connection_graph.h>
#include <pgm_base.h>
#include <core/kicad_algo.h>
#include <wx/wfstream.h>
#include <xnode.h>      // also nests: <wx/xml/xml.h>
#include <json_common.h>
#include <project_sch.h>
#include <sch_rule_area.h>
#include <trace_helpers.h>

#include <set>
#include <libraries/symbol_library_adapter.h>

static bool sortPinsByNumber( SCH_PIN* aPin1, SCH_PIN* aPin2 );

bool NETLIST_EXPORTER_XML::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions,
                                         REPORTER& aReporter )
{
    // output the XML format netlist.

    // declare the stream ourselves to use the buffered FILE api
    // instead of letting wx use the syscall variant
    wxFFileOutputStream stream( aOutFileName );

    if( !stream.IsOk() )
        return false;

    wxXmlDocument xdoc;
    xdoc.SetRoot( makeRoot( GNL_ALL | aNetlistOptions ) );

    return xdoc.Save( stream, 2 /* indent bug, today was ignored by wxXml lib */ );
}


XNODE* NETLIST_EXPORTER_XML::makeRoot( unsigned aCtl )
{
    XNODE*      xroot = node( wxT( "export" ) );

    xroot->AddAttribute( wxT( "version" ), wxT( "E" ) );

    if( aCtl & GNL_HEADER )
        // add the "design" header
        xroot->AddChild( makeDesignHeader() );

    if( aCtl & GNL_SYMBOLS )
    {
        xroot->AddChild( makeSymbols( aCtl ) );

        if( aCtl & GNL_OPT_KICAD )
        {
            xroot->AddChild( makeGroups() );
            xroot->AddChild( makeVariants() );
        }
    }

    if( aCtl & GNL_PARTS )
        xroot->AddChild( makeLibParts() );

    if( aCtl & GNL_LIBRARIES )
        // must follow makeGenericLibParts()
        xroot->AddChild( makeLibraries() );

    if( aCtl & GNL_NETS )
        xroot->AddChild( makeListOfNets( aCtl ) );

    return xroot;
}


/// Holder for multi-unit symbol fields


void NETLIST_EXPORTER_XML::addSymbolFields( XNODE* aNode, SCH_SYMBOL* aSymbol,
                                            const SCH_SHEET_PATH& aSheet,
                                            const SCH_SHEET_LIST& aSheetList )
{
    wxString                     value;
    wxString                     footprint;
    wxString                     datasheet;
    wxString                     description;
    wxString                     candidate;
    nlohmann::ordered_map<wxString, wxString> fields;

    if( aSymbol->GetUnitCount() > 1 )
    {
        // Sadly, each unit of a symbol can have its own unique fields. This
        // block finds the unit with the lowest number having a non blank field
        // value and records it.  Therefore user is best off setting fields
        // into only the first unit.  But this scavenger algorithm will find
        // any non blank fields in all units and use the first non-blank field
        // for each unique field name.

        wxString ref = aSymbol->GetRef( &aSheet );

        int minUnit = aSymbol->GetUnitSelection( &aSheet );

        for( const SCH_SHEET_PATH& sheet : aSheetList )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol2 = static_cast<SCH_SYMBOL*>( item );

                wxString ref2 = symbol2->GetRef( &sheet );

                if( ref2.CmpNoCase( ref ) != 0 )
                    continue;

                int unit = symbol2->GetUnitSelection( &aSheet );

                // The lowest unit number wins.  User should only set fields in any one unit.

                // Value
                candidate = symbol2->GetValue( m_resolveTextVars, &sheet, false );

                if( !candidate.IsEmpty() && ( unit < minUnit || value.IsEmpty() ) )
                    value = candidate;

                // Footprint
                candidate = symbol2->GetFootprintFieldText( m_resolveTextVars, &sheet, false );

                if( !candidate.IsEmpty() && ( unit < minUnit || footprint.IsEmpty() ) )
                    footprint = candidate;

                // Datasheet
                candidate = m_resolveTextVars
                                ? symbol2->GetField( FIELD_T::DATASHEET )->GetShownText( &sheet, false )
                                : symbol2->GetField( FIELD_T::DATASHEET )->GetText();

                if( !candidate.IsEmpty() && ( unit < minUnit || datasheet.IsEmpty() ) )
                    datasheet = candidate;

                // Description
                candidate = m_resolveTextVars
                                ? symbol2->GetField( FIELD_T::DESCRIPTION )->GetShownText( &sheet, false )
                                : symbol2->GetField( FIELD_T::DESCRIPTION )->GetText();

                if( !candidate.IsEmpty() && ( unit < minUnit || description.IsEmpty() ) )
                    description = candidate;

                // All non-mandatory fields
                for( SCH_FIELD& field : symbol2->GetFields() )
                {
                    if( field.IsMandatory() || field.IsPrivate() )
                        continue;

                    if( unit < minUnit || fields.count( field.GetName() ) == 0 )
                    {
                        if( m_resolveTextVars )
                            fields[field.GetName()] = field.GetShownText( &aSheet, false );
                        else
                            fields[field.GetName()] = field.GetText();
                    }
                }

                minUnit = std::min( unit, minUnit );
            }
        }
    }
    else
    {
        value = aSymbol->GetValue( m_resolveTextVars, &aSheet, false );
        footprint = aSymbol->GetFootprintFieldText( m_resolveTextVars, &aSheet, false );

        SCH_FIELD* datasheetField = aSymbol->GetField( FIELD_T::DATASHEET );
        SCH_FIELD* descriptionField = aSymbol->GetField( FIELD_T::DESCRIPTION );

        // Datasheet
        if( m_resolveTextVars )
            datasheet = datasheetField->GetShownText( &aSheet, false );
        else
            datasheet = datasheetField->GetText();

        // Description
        if( m_resolveTextVars )
            description = descriptionField->GetShownText( &aSheet, false );
        else
            description = descriptionField->GetText();

        for( SCH_FIELD& field : aSymbol->GetFields() )
        {
            if( field.IsMandatory() || field.IsPrivate() )
                continue;

            if( m_resolveTextVars )
                fields[field.GetName()] = field.GetShownText( &aSheet, false );
            else
                fields[field.GetName()] = field.GetText();
        }
    }

    fields[GetCanonicalFieldName( FIELD_T::FOOTPRINT )] = footprint;
    fields[GetCanonicalFieldName( FIELD_T::DATASHEET )] = datasheet;
    fields[GetCanonicalFieldName( FIELD_T::DESCRIPTION )] = description;

    // Do not output field values blank in netlist:
    if( value.size() )
        aNode->AddChild( node( wxT( "value" ), UnescapeString( value ) ) );
    else    // value field always written in netlist
        aNode->AddChild( node( wxT( "value" ), wxT( "~" ) ) );

    if( footprint.size() )
        aNode->AddChild( node( wxT( "footprint" ), UnescapeString( footprint ) ) );

    if( datasheet.size() )
        aNode->AddChild( node( wxT( "datasheet" ), UnescapeString( datasheet ) ) );

    if( description.size() )
        aNode->AddChild( node( wxT( "description" ), UnescapeString( description ) ) );

    XNODE* xfields;
    aNode->AddChild( xfields = node( wxT( "fields" ) ) );

    for( const auto& [ fieldName, fieldValue ] : fields )
    {
        XNODE* xfield = node( wxT( "field" ), UnescapeString( fieldValue ) );
        xfield->AddAttribute( wxT( "name" ), UnescapeString( fieldName ) );
        xfields->AddChild( xfield );
    }
}


XNODE* NETLIST_EXPORTER_XML::makeSymbols( unsigned aCtl )
{
    XNODE* xcomps = node( wxT( "components" ) );

    m_referencesAlreadyFound.Clear();
    m_libParts.clear();
    getSheetComponentClasses();

    SCH_SHEET_PATH currentSheet = m_schematic->CurrentSheet();
    SCH_SHEET_LIST sheetList = m_schematic->Hierarchy();

    // Output is xml, so there is no reason to remove spaces from the field values.
    // And XML element names need not be translated to various languages.

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        // Change schematic CurrentSheet in each iteration to allow hierarchical
        // resolution of text variables in sheet fields.
        m_schematic->SetCurrentSheet( sheet );

        auto cmp =
                [&sheet]( SCH_SYMBOL* a, SCH_SYMBOL* b )
                {
                    return ( StrNumCmp( a->GetRef( &sheet, false ),
                                        b->GetRef( &sheet, false ), true ) < 0 );
                };

        std::set<SCH_SYMBOL*, decltype( cmp )> ordered_symbols( cmp );
        std::multiset<SCH_SYMBOL*, decltype( cmp )> extra_units( cmp );

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            auto        test = ordered_symbols.insert( symbol );

            if( !test.second )
            {
                if( ( *( test.first ) )->m_Uuid > symbol->m_Uuid )
                {
                    extra_units.insert( *( test.first ) );
                    ordered_symbols.erase( test.first );
                    ordered_symbols.insert( symbol );
                }
                else
                {
                    extra_units.insert( symbol );
                }
            }
        }

        for( EDA_ITEM* item : ordered_symbols )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, sheet );
            bool        forBOM = aCtl & GNL_OPT_BOM;
            bool        forBoard = aCtl & GNL_OPT_KICAD;

            if( !symbol )
                continue;

            if( forBOM && ( sheet.GetExcludedFromBOM() || symbol->ResolveExcludedFromBOM() ) )
                continue;

            if( forBoard && ( sheet.GetExcludedFromBoard() || symbol->ResolveExcludedFromBoard() ) )
                continue;

            // Output the symbol's elements in order of expected access frequency. This may
            // not always look best, but it will allow faster execution under XSL processing
            // systems which do sequential searching within an element.

            XNODE* xcomp;  // current symbol being constructed
            xcomps->AddChild( xcomp = node( wxT( "comp" ) ) );

            xcomp->AddAttribute( wxT( "ref" ), symbol->GetRef( &sheet ) );
            addSymbolFields( xcomp, symbol, sheet, sheetList );

            XNODE*  xlibsource;
            xcomp->AddChild( xlibsource = node( wxT( "libsource" ) ) );

            // "logical" library name, which is in anticipation of a better search algorithm
            // for parts based on "logical_lib.part" and where logical_lib is merely the library
            // name minus path and extension.
            wxString libName;
            wxString partName;

            if( symbol->UseLibIdLookup() )
            {
                libName = symbol->GetLibId().GetUniStringLibNickname();
                partName = symbol->GetLibId().GetUniStringLibItemName();
            }
            else
            {
                partName = symbol->GetSchSymbolLibraryName();
            }

            xlibsource->AddAttribute( wxT( "lib" ), libName );

            // We only want the symbol name, not the full LIB_ID.
            xlibsource->AddAttribute( wxT( "part" ), partName );

            if( m_resolveTextVars )
                xlibsource->AddAttribute( wxT( "description" ), symbol->GetShownDescription() );
            else
                xlibsource->AddAttribute( wxT( "description" ), symbol->GetDescription() );

            /* Add the symbol properties. */
            XNODE* xproperty;

            std::vector<SCH_FIELD>& fields = symbol->GetFields();

            for( SCH_FIELD& field : fields )
            {
                if( field.IsMandatory() || field.IsPrivate() )
                    continue;

                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), field.GetCanonicalName() );

                if( m_resolveTextVars )
                    xproperty->AddAttribute( wxT( "value" ), field.GetShownText( &sheet, false ) );
                else
                    xproperty->AddAttribute( wxT( "value" ), field.GetText() );
            }

            for( const SCH_FIELD& sheetField : sheet.Last()->GetFields() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), sheetField.GetCanonicalName() );

                if( m_resolveTextVars )
                    // do not allow GetShownText() to add any prefix useful only when displaying
                    // the field on screen
                    xproperty->AddAttribute( wxT( "value" ), sheetField.GetShownText( &sheet, false ) );
                else
                    xproperty->AddAttribute( wxT( "value" ), sheetField.GetText() );
            }

            if( symbol->ResolveExcludedFromBOM() || sheet.GetExcludedFromBOM() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "exclude_from_bom" ) );
            }

            if( symbol->ResolveExcludedFromBoard() || sheet.GetExcludedFromBoard() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "exclude_from_board" ) );
            }

            if( symbol->ResolveExcludedFromPosFiles() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "exclude_from_pos_files" ) );
            }

            if( symbol->ResolveDNP() || sheet.GetDNP() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "dnp" ) );
            }

            SCH_SYMBOL_INSTANCE instance;

            if( symbol->GetInstance( instance, sheet.Path() ) && !instance.m_Variants.empty() )
            {
                const bool baseDnp = symbol->GetDNP( &sheet );
                const bool baseExcludedFromBOM = symbol->GetExcludedFromBOM( &sheet );
                const bool baseExcludedFromSim = symbol->GetExcludedFromSim( &sheet );
                const bool baseExcludedFromPosFiles = symbol->GetExcludedFromPosFiles( &sheet );
                XNODE*     xvariants = nullptr;

                for( const auto& [variantName, variant] : instance.m_Variants )
                {
                    XNODE* xvariant = node( wxT( "variant" ) );
                    bool   hasVariantData = false;

                    xvariant->AddAttribute( wxT( "name" ), variantName );

                    if( variant.m_DNP != baseDnp )
                    {
                        XNODE* xvarprop = node( wxT( "property" ) );
                        xvarprop->AddAttribute( wxT( "name" ), wxT( "dnp" ) );
                        xvarprop->AddAttribute( wxT( "value" ), variant.m_DNP ? wxT( "1" ) : wxT( "0" ) );
                        xvariant->AddChild( xvarprop );
                        hasVariantData = true;
                    }

                    if( variant.m_ExcludedFromBOM != baseExcludedFromBOM )
                    {
                        XNODE* xvarprop = node( wxT( "property" ) );
                        xvarprop->AddAttribute( wxT( "name" ), wxT( "exclude_from_bom" ) );
                        xvarprop->AddAttribute( wxT( "value" ), variant.m_ExcludedFromBOM ? wxT( "1" ) : wxT( "0" ) );
                        xvariant->AddChild( xvarprop );
                        hasVariantData = true;
                    }

                    if( variant.m_ExcludedFromSim != baseExcludedFromSim )
                    {
                        XNODE* xvarprop = node( wxT( "property" ) );
                        xvarprop->AddAttribute( wxT( "name" ), wxT( "exclude_from_sim" ) );
                        xvarprop->AddAttribute( wxT( "value" ), variant.m_ExcludedFromSim ? wxT( "1" ) : wxT( "0" ) );
                        xvariant->AddChild( xvarprop );
                        hasVariantData = true;
                    }

                    if( variant.m_ExcludedFromPosFiles != baseExcludedFromPosFiles )
                    {
                        XNODE* xvarprop = node( wxT( "property" ) );
                        xvarprop->AddAttribute( wxT( "name" ), wxT( "exclude_from_pos_files" ) );
                        xvarprop->AddAttribute( wxT( "value" ), variant.m_ExcludedFromPosFiles ? wxT( "1" ) : wxT( "0" ) );
                        xvariant->AddChild( xvarprop );
                        hasVariantData = true;
                    }

                    if( !variant.m_Fields.empty() )
                    {
                        XNODE* xfields = nullptr;

                        for( const auto& [fieldName, fieldValue] : variant.m_Fields )
                        {
                            const wxString baseValue =
                                    symbol->GetFieldText( fieldName, &sheet, wxEmptyString );

                            if( fieldValue == baseValue )
                                continue;

                            if( !xfields )
                                xfields = node( wxT( "fields" ) );

                            wxString resolvedValue = fieldValue;

                            if( m_resolveTextVars )
                                resolvedValue = symbol->ResolveText( fieldValue, &sheet );

                            XNODE* xfield = node( wxT( "field" ), UnescapeString( resolvedValue ) );
                            xfield->AddAttribute( wxT( "name" ), UnescapeString( fieldName ) );
                            xfields->AddChild( xfield );
                            hasVariantData = true;
                        }

                        if( xfields )
                            xvariant->AddChild( xfields );
                    }

                    if( hasVariantData )
                    {
                        if( !xvariants )
                            xvariants = node( wxT( "variants" ) );

                        xvariants->AddChild( xvariant );
                    }
                    else
                    {
                        delete xvariant;
                    }
                }

                if( xvariants )
                    xcomp->AddChild( xvariants );
            }

            if( const std::unique_ptr<LIB_SYMBOL>& part = symbol->GetLibSymbolRef() )
            {
                if( part->GetKeyWords().size() )
                {
                    xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                    xproperty->AddAttribute( wxT( "name" ), wxT( "ki_keywords" ) );
                    xproperty->AddAttribute( wxT( "value" ), part->GetKeyWords() );
                }

                if( !part->GetFPFilters().IsEmpty() )
                {
                    wxString filters;

                    for( const wxString& filter : part->GetFPFilters() )
                        filters += ' ' + filter;

                    xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                    xproperty->AddAttribute( wxT( "name" ), wxT( "ki_fp_filters" ) );
                    xproperty->AddAttribute( wxT( "value" ), filters.Trim( false ) );
                }

                if( part->GetDuplicatePinNumbersAreJumpers() )
                    xcomp->AddChild( node( wxT( "duplicate_pin_numbers_are_jumpers" ), wxT( "1" ) ) );

                const std::vector<std::set<wxString>>& jumperGroups = part->JumperPinGroups();

                if( !jumperGroups.empty() )
                {
                    XNODE* groupNode;
                    xcomp->AddChild( xproperty = node( wxT( "jumper_pin_groups" ) ) );

                    for( const std::set<wxString>& group : jumperGroups )
                    {
                        xproperty->AddChild( groupNode = node( wxT( "group" ) ) );

                        for( const wxString& pinName : group )
                            groupNode->AddChild( node( wxT( "pin" ), pinName ) );
                    }
                }
            }

            XNODE* xsheetpath;
            xcomp->AddChild( xsheetpath = node( wxT( "sheetpath" ) ) );

            xsheetpath->AddAttribute( wxT( "names" ), sheet.PathHumanReadable() );
            xsheetpath->AddAttribute( wxT( "tstamps" ), sheet.PathAsString() );

            // Node for component class
            std::vector<wxString> compClassNames =
                    getComponentClassNamesForAllSymbolUnits( symbol, sheet, sheetList );

            if( compClassNames.size() > 0 )
            {
                XNODE* xcompclasslist;
                xcomp->AddChild( xcompclasslist = node( wxT( "component_classes" ) ) );

                for( const wxString& compClass : compClassNames )
                {
                    xcompclasslist->AddChild( node( wxT( "class" ), UnescapeString( compClass ) ) );
                }
            }

            XNODE* xunits; // Node for extra units
            xcomp->AddChild( xunits = node( wxT( "tstamps" ) ) );

            auto     range = extra_units.equal_range( symbol );
            wxString uuid;

            // Output a series of children with all UUIDs associated with the REFDES
            for( auto it = range.first; it != range.second; ++it )
            {
                uuid = ( *it )->m_Uuid.AsString();

                // Add a space between UUIDs, if not in KICAD mode (i.e.
                // using wxXmlDocument::Save()).  KICAD MODE has its own XNODE::Format function.
                if( !( aCtl & GNL_OPT_KICAD ) )     // i.e. for .xml format
                    uuid += ' ';

                xunits->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, uuid ) );
            }

            // Output the primary UUID
            uuid = symbol->m_Uuid.AsString();
            xunits->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, uuid ) );

            // Emit unit information (per-unit name and pins) after tstamps
            XNODE* xunitInfo;
            xcomp->AddChild( xunitInfo = node( wxT( "units" ) ) );

            // Emit all units defined by the library symbol, independent of placement
            const std::unique_ptr<LIB_SYMBOL>& libSym = symbol->GetLibSymbolRef();

            if( libSym )
            {
                for( const LIB_SYMBOL::UNIT_PIN_INFO& unitInfo : libSym->GetUnitPinInfo() )
                {
                    XNODE* xunit;
                    xunitInfo->AddChild( xunit = node( wxT( "unit" ) ) );
                    xunit->AddAttribute( wxT( "name" ), unitInfo.m_unitName );

                    XNODE* xpins;
                    xunit->AddChild( xpins = node( wxT( "pins" ) ) );

                    for( const wxString& number : unitInfo.m_pinNumbers )
                    {
                        XNODE* xpin;
                        xpins->AddChild( xpin = node( wxT( "pin" ) ) );
                        xpin->AddAttribute( wxT( "num" ), number );
                    }
                }
            }
        }
    }

    m_schematic->SetCurrentSheet( currentSheet );

    return xcomps;
}


XNODE* NETLIST_EXPORTER_XML::makeGroups()
{
    XNODE* xcomps = node( wxT( "groups" ) );

    m_referencesAlreadyFound.Clear();
    // Do not clear m_libParts here: it is populated in makeSymbols() and used later by
    // makeLibParts() to emit the libparts section for CvPcb and other consumers.

    SCH_SHEET_PATH currentSheet = m_schematic->CurrentSheet();
    SCH_SHEET_LIST sheetList = m_schematic->Hierarchy();

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        // Change schematic CurrentSheet in each iteration to allow hierarchical
        // resolution of text variables in sheet fields.
        m_schematic->SetCurrentSheet( sheet );

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_GROUP_T ) )
        {
            SCH_GROUP* group = static_cast<SCH_GROUP*>( item );

            XNODE* xgroup;  // current symbol being constructed
            xcomps->AddChild( xgroup = node( wxT( "group" ) ) );

            xgroup->AddAttribute( wxT( "name" ), group->GetName() );
            xgroup->AddAttribute( wxT( "uuid" ), group->m_Uuid.AsString() );
            xgroup->AddAttribute( wxT( "lib_id" ), group->GetDesignBlockLibId().Format() );

            XNODE* xmembers;
            xgroup->AddChild( xmembers = node( wxT( "members" ) ) );

            for( EDA_ITEM* member : group->GetItems() )
            {
                if( member->Type() == SCH_SYMBOL_T )
                {
                    XNODE* xmember;
                    xmembers->AddChild( xmember = node( wxT( "member" ) ) );
                    xmember->AddAttribute( wxT( "uuid" ), member->m_Uuid.AsString() );
                }
            }
        }
    }

    m_schematic->SetCurrentSheet( currentSheet );

    return xcomps;
}


XNODE* NETLIST_EXPORTER_XML::makeVariants()
{
    XNODE* xvariants = node( wxT( "variants" ) );

    std::set<wxString> variantNames = m_schematic->GetVariantNames();

    for( const wxString& variantName : variantNames )
    {
        XNODE* xvariant;
        xvariants->AddChild( xvariant = node( wxT( "variant" ) ) );
        xvariant->AddAttribute( wxT( "name" ), variantName );

        wxString description = m_schematic->GetVariantDescription( variantName );

        if( !description.IsEmpty() )
            xvariant->AddAttribute( wxT( "description" ), description );
    }

    return xvariants;
}


std::vector<wxString> NETLIST_EXPORTER_XML::getComponentClassNamesForAllSymbolUnits(
        SCH_SYMBOL* aSymbol, const SCH_SHEET_PATH& aSymbolSheet, const SCH_SHEET_LIST& aSheetList )
{
    std::vector<SCH_SHEET_PATH> symbolSheets;
    symbolSheets.push_back( aSymbolSheet );

    std::unordered_set<wxString> compClassNames = aSymbol->GetComponentClassNames( &aSymbolSheet );
    int                          primaryUnit = aSymbol->GetUnitSelection( &aSymbolSheet );

    if( aSymbol->GetUnitCount() > 1 )
    {
        const wxString ref = aSymbol->GetRef( &aSymbolSheet );

        for( const SCH_SHEET_PATH& sheet : aSheetList )
        {
            for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                const SCH_SYMBOL* symbol2 = static_cast<SCH_SYMBOL*>( item );

                wxString  ref2 = symbol2->GetRef( &sheet );
                const int otherUnit = symbol2->GetUnitSelection( &sheet );

                if( ref2.CmpNoCase( ref ) != 0 )
                    continue;

                if( otherUnit == primaryUnit )
                    continue;

                symbolSheets.push_back( sheet );

                std::unordered_set<wxString> otherClassNames =
                        symbol2->GetComponentClassNames( &sheet );
                compClassNames.insert( otherClassNames.begin(), otherClassNames.end() );
            }
        }
    }

    // Add sheet-level component classes
    for( auto& [sheetPath, sheetCompClasses] : m_sheetComponentClasses )
    {
        for( SCH_SHEET_PATH& symbolSheetPath : symbolSheets )
        {
            if( symbolSheetPath.IsContainedWithin( sheetPath ) )
            {
                compClassNames.insert( sheetCompClasses.begin(), sheetCompClasses.end() );
            }
        }
    }

    std::vector<wxString> sortedCompClassNames( compClassNames.begin(), compClassNames.end() );
    std::sort( sortedCompClassNames.begin(), sortedCompClassNames.end(),
               []( const wxString& str1, const wxString& str2 )
               {
                   return str1.Cmp( str2 ) < 0;
               } );

    return sortedCompClassNames;
}


XNODE* NETLIST_EXPORTER_XML::makeDesignHeader()
{
    SCH_SCREEN* screen;
    XNODE*      xdesign = node( wxT( "design" ) );
    XNODE*      xtitleBlock;
    XNODE*      xsheet;
    XNODE*      xcomment;
    XNODE*      xtextvar;
    wxString    sheetTxt;
    wxFileName  sourceFileName;

    // the root sheet is a special sheet, call it source
    xdesign->AddChild( node( wxT( "source" ), m_schematic->GetFileName() ) );

    xdesign->AddChild( node( wxT( "date" ), GetISO8601CurrentDateTime() ) );

    // which Eeschema tool
    xdesign->AddChild( node( wxT( "tool" ), wxT( "Eeschema " ) + GetBuildVersion() ) );

    const std::map<wxString, wxString>& properties = m_schematic->Project().GetTextVars();

    for( const std::pair<const wxString, wxString>& prop : properties )
    {
        xdesign->AddChild( xtextvar = node( wxT( "textvar" ), prop.second ) );
        xtextvar->AddAttribute( wxT( "name" ), prop.first );
    }

    /*
     *  Export the sheets information
     */
    unsigned sheetIndex = 1;     // Human readable index

    for( const SCH_SHEET_PATH& sheet : m_schematic->Hierarchy() )
    {
        screen = sheet.LastScreen();

        xdesign->AddChild( xsheet = node( wxT( "sheet" ) ) );

        // get the string representation of the sheet index number.
        sheetTxt.Printf( wxT( "%u" ), sheetIndex++ );
        xsheet->AddAttribute( wxT( "number" ), sheetTxt );
        xsheet->AddAttribute( wxT( "name" ), sheet.PathHumanReadable() );
        xsheet->AddAttribute( wxT( "tstamps" ), sheet.PathAsString() );

        TITLE_BLOCK tb = screen->GetTitleBlock();
        PROJECT*    prj = &m_schematic->Project();

        xsheet->AddChild( xtitleBlock = node( wxT( "title_block" ) ) );

        xtitleBlock->AddChild( node( wxT( "title" ), ExpandTextVars( tb.GetTitle(), prj ) ) );
        xtitleBlock->AddChild( node( wxT( "company" ), ExpandTextVars( tb.GetCompany(), prj ) ) );
        xtitleBlock->AddChild( node( wxT( "rev" ), ExpandTextVars( tb.GetRevision(), prj ) ) );
        xtitleBlock->AddChild( node( wxT( "date" ), ExpandTextVars( tb.GetDate(), prj ) ) );

        // We are going to remove the fileName directories.
        sourceFileName = wxFileName( screen->GetFileName() );
        xtitleBlock->AddChild( node( wxT( "source" ), sourceFileName.GetFullName() ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "1" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 0 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "2" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 1 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "3" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 2 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "4" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 3 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "5" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 4 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "6" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 5 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "7" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 6 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "8" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 7 ), prj ) );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT( "number" ), wxT( "9" ) );
        xcomment->AddAttribute( wxT( "value" ), ExpandTextVars( tb.GetComment( 8 ), prj ) );
    }

    return xdesign;
}


XNODE* NETLIST_EXPORTER_XML::makeLibraries()
{
    XNODE*            xlibs = node( wxT( "libraries" ) );     // auto_ptr
    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    for( std::set<wxString>::iterator it = m_libraries.begin(); it!=m_libraries.end();  ++it )
    {
        wxString    libNickname = *it;
        XNODE*      xlibrary;

        std::optional<wxString> uri = manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, libNickname );

        if( uri )
        {
            xlibs->AddChild( xlibrary = node( wxT( "library" ) ) );
            xlibrary->AddAttribute( wxT( "logical" ), libNickname );
            xlibrary->AddChild( node( wxT( "uri" ), *uri  ) );
        }

        // @todo: add more fun stuff here
    }

    return xlibs;
}


XNODE* NETLIST_EXPORTER_XML::makeLibParts()
{
    XNODE*                  xlibparts = node( wxT( "libparts" ) );   // auto_ptr
    std::vector<SCH_FIELD*> fieldList;

    m_libraries.clear();

    for( LIB_SYMBOL* lcomp : m_libParts )
    {
        wxString libNickname = lcomp->GetLibId().GetLibNickname();;

        // The library nickname will be empty if the cache library is used.
        if( !libNickname.IsEmpty() )
            m_libraries.insert( libNickname );  // inserts symbol's library if unique

        XNODE* xlibpart;
        xlibparts->AddChild( xlibpart = node( wxT( "libpart" ) ) );
        xlibpart->AddAttribute( wxT( "lib" ), libNickname );
        xlibpart->AddAttribute( wxT( "part" ), lcomp->GetName()  );

        //----- show the important properties -------------------------
        if( !lcomp->GetDescription().IsEmpty() )
            xlibpart->AddChild( node( wxT( "description" ), lcomp->GetDescription() ) );

        if( !lcomp->GetDatasheetField().GetText().IsEmpty() )
            xlibpart->AddChild( node( wxT( "docs" ),  lcomp->GetDatasheetField().GetText() ) );

        // Write the footprint list
        if( lcomp->GetFPFilters().GetCount() )
        {
            XNODE*  xfootprints;
            xlibpart->AddChild( xfootprints = node( wxT( "footprints" ) ) );

            for( unsigned i = 0; i < lcomp->GetFPFilters().GetCount(); ++i )
            {
                if( !lcomp->GetFPFilters()[i].IsEmpty() )
                    xfootprints->AddChild( node( wxT( "fp" ), lcomp->GetFPFilters()[i] ) );
            }
        }

        //----- show the fields here ----------------------------------
        fieldList.clear();
        lcomp->GetFields( fieldList );

        XNODE*     xfields;
        xlibpart->AddChild( xfields = node( "fields" ) );

        for( const SCH_FIELD* field : fieldList )
        {
            XNODE* xfield;
            xfields->AddChild( xfield = node( wxT( "field" ), field->GetText() ) );
            xfield->AddAttribute( wxT( "name" ), field->GetCanonicalName() );
        }

    //----- show the pins here ------------------------------------
    // NOTE: Expand stacked-pin notation into individual pins so downstream
    // tools (e.g. CvPcb) see the actual number of footprint pins.
    std::vector<SCH_PIN*> pinList = lcomp->GetGraphicalPins( 0, 0 );

        /*
         * We must erase redundant Pins references in pinList
         * These redundant pins exist because some pins are found more than one time when a
         * symbol has multiple parts per package or has 2 representations (DeMorgan conversion).
         * For instance, a 74ls00 has DeMorgan conversion, with different pin shapes, and
         * therefore each pin  appears 2 times in the list. Common pins (VCC, GND) can also be
         * found more than once.
         */
        sort( pinList.begin(), pinList.end(), sortPinsByNumber );

        for( int ii = 0; ii < (int)pinList.size()-1; ii++ )
        {
            if( pinList[ii]->GetNumber() == pinList[ii+1]->GetNumber() )
            {   // 2 pins have the same number, remove the redundant pin at index i+1
                pinList.erase(pinList.begin() + ii + 1);
                ii--;
            }
        }

        wxLogTrace( "CVPCB_PINCOUNT",
                wxString::Format( "makeLibParts: lib='%s' part='%s' pinList(size)=%zu",
                          libNickname, lcomp->GetName(), pinList.size() ) );

        if( pinList.size() )
        {
            XNODE*     pins;

            xlibpart->AddChild( pins = node( wxT( "pins" ) ) );

            for( unsigned i=0; i<pinList.size();  ++i )
            {
                SCH_PIN* basePin = pinList[i];

                bool                     stackedValid = false;
                std::vector<wxString>    expandedNums = basePin->GetStackedPinNumbers( &stackedValid );

                // If stacked notation detected and valid, emit one libparts pin per expanded number.
                if( stackedValid && !expandedNums.empty() )
                {
                    for( const wxString& num : expandedNums )
                    {
                        XNODE* pin;
                        pins->AddChild( pin = node( wxT( "pin" ) ) );
                        pin->AddAttribute( wxT( "num" ), num );
                        pin->AddAttribute( wxT( "name" ), basePin->GetShownName() );
                        pin->AddAttribute( wxT( "type" ), basePin->GetCanonicalElectricalTypeName() );

                        wxLogTrace( "CVPCB_PINCOUNT",
                                    wxString::Format( "makeLibParts: -> pin num='%s' name='%s' (expanded)",
                                                      num, basePin->GetShownName() ) );
                    }
                }
                else
                {
                    XNODE* pin;
                    pins->AddChild( pin = node( wxT( "pin" ) ) );
                    pin->AddAttribute( wxT( "num" ), basePin->GetShownNumber() );
                    pin->AddAttribute( wxT( "name" ), basePin->GetShownName() );
                    pin->AddAttribute( wxT( "type" ), basePin->GetCanonicalElectricalTypeName() );

                    wxLogTrace( "CVPCB_PINCOUNT",
                                wxString::Format( "makeLibParts: -> pin num='%s' name='%s'",
                                                  basePin->GetShownNumber(),
                                                  basePin->GetShownName() ) );
                }

                // caution: construction work site here, drive slowly
            }
        }
    }

    return xlibparts;
}


XNODE* NETLIST_EXPORTER_XML::makeListOfNets( unsigned aCtl )
{
    wxString netCodeTxt;
    XNODE*   xnets = node( wxT( "nets" ) ); // auto_ptr if exceptions ever get used.
    XNODE*   xnet = nullptr;

    /*  output:
        <net code="123" name="/cfcard.sch/WAIT#" class="signal">
            <node ref="R23" pin="1"/>
            <node ref="U18" pin="12"/>
        </net>
    */

    struct NET_NODE
    {
        NET_NODE( SCH_PIN* aPin, const SCH_SHEET_PATH& aSheet ) :
                m_Pin( aPin ),
                m_Sheet( aSheet )
        {}

        SCH_PIN*       m_Pin;
        SCH_SHEET_PATH m_Sheet;
    };

    struct NET_RECORD
    {
        NET_RECORD( const wxString& aName ) :
                m_Name( aName ),
                m_HasNoConnect( false )
        {};

        wxString              m_Name;
        wxString              m_Class;
        bool                  m_HasNoConnect;
        std::vector<NET_NODE> m_Nodes;
    };

    std::vector<NET_RECORD*> nets;

    for( const auto& [ key, subgraphs ] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        wxString    net_name = key.Name;
        NET_RECORD* net_record = nullptr;

        if( !( aCtl & GNL_OPT_KICAD ) )
            net_name = UnescapeString( net_name );

        if( subgraphs.empty() )
            continue;

        nets.emplace_back( new NET_RECORD( net_name ) );
        net_record = nets.back();

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            bool nc = subgraph->GetNoConnect() && subgraph->GetNoConnect()->Type() == SCH_NO_CONNECT_T;
            const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

            if( net_record->m_Class.IsEmpty() && subgraph->GetDriver() )
            {
                if( subgraph->GetDriver()->GetEffectiveNetClass() )
                {
                    net_record->m_Class = subgraph->GetDriver()->GetEffectiveNetClass()->GetName();
                    net_record->m_Class = UnescapeString( net_record->m_Class );
                }
            }

            if( nc )
                net_record->m_HasNoConnect = true;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                    SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );
                    bool        forBOM = aCtl & GNL_OPT_BOM;
                    bool        forBoard = aCtl & GNL_OPT_KICAD;

                    if( !symbol )
                        continue;

                    if( forBOM && ( sheet.GetExcludedFromBOM() || symbol->ResolveExcludedFromBOM() ) )
                        continue;

                    if( forBoard && ( sheet.GetExcludedFromBoard() || symbol->ResolveExcludedFromBoard() ) )
                        continue;

                    net_record->m_Nodes.emplace_back( pin, sheet );
                }
            }
        }
    }

    // Netlist ordering: Net name, then ref des, then pin name
    std::sort( nets.begin(), nets.end(),
               []( const NET_RECORD* a, const NET_RECORD*b )
               {
                   return StrNumCmp( a->m_Name, b->m_Name ) < 0;
               } );

    for( int i = 0; i < (int) nets.size(); ++i )
    {
        NET_RECORD* net_record = nets[i];
        bool        added = false;
        XNODE*      xnode;

        // Netlist ordering: Net name, then ref des, then pin name
        std::sort( net_record->m_Nodes.begin(), net_record->m_Nodes.end(),
                []( const NET_NODE& a, const NET_NODE& b )
                {
                    wxString refA = a.m_Pin->GetParentSymbol()->GetRef( &a.m_Sheet );
                    wxString refB = b.m_Pin->GetParentSymbol()->GetRef( &b.m_Sheet );

                    if( refA == refB )
                        return a.m_Pin->GetShownNumber() < b.m_Pin->GetShownNumber();

                    return refA < refB;
                } );

        // Some duplicates can exist, for example on multi-unit parts with duplicated pins across
        // units.  If the user connects the pins on each unit, they will appear on separate
        // subgraphs.  Remove those here:
        alg::remove_duplicates( net_record->m_Nodes,
                []( const NET_NODE& a, const NET_NODE& b )
                {
                    wxString refA = a.m_Pin->GetParentSymbol()->GetRef( &a.m_Sheet );
                    wxString refB = b.m_Pin->GetParentSymbol()->GetRef( &b.m_Sheet );

                    return refA == refB && a.m_Pin->GetShownNumber() == b.m_Pin->GetShownNumber();
                } );

        // Determine if all pins in the net are stacked (nets with only one pin are implicitly
        // taken to be stacked)
        bool allNetPinsStacked = true;

        if( net_record->m_Nodes.size() > 1 )
        {
            SCH_PIN* firstPin = net_record->m_Nodes.begin()->m_Pin;
            allNetPinsStacked =
                    std::all_of( net_record->m_Nodes.begin() + 1, net_record->m_Nodes.end(),
                                 [=]( auto& node )
                                 {
                                     return firstPin->GetParent() == node.m_Pin->GetParent()
                                            && firstPin->GetPosition() == node.m_Pin->GetPosition()
                                            && firstPin->GetName() == node.m_Pin->GetName();
                                 } );
        }

    for( const NET_NODE& netNode : net_record->m_Nodes )
        {
            wxString refText = netNode.m_Pin->GetParentSymbol()->GetRef( &netNode.m_Sheet );

            // Skip power symbols and virtual symbols
            if( refText[0] == wxChar( '#' ) )
                continue;

            if( !added )
            {
                netCodeTxt.Printf( wxT( "%d" ), i + 1 );

                xnets->AddChild( xnet = node( wxT( "net" ) ) );
                xnet->AddAttribute( wxT( "code" ), netCodeTxt );
                xnet->AddAttribute( wxT( "name" ), net_record->m_Name );
                xnet->AddAttribute( wxT( "class" ), net_record->m_Class );

                added = true;
            }

            std::vector<wxString> nums = netNode.m_Pin->GetStackedPinNumbers();
            wxString              baseName = netNode.m_Pin->GetShownName();
            wxString              pinType = netNode.m_Pin->GetCanonicalElectricalTypeName();

            wxLogTrace( traceStackedPins,
                        wxString::Format( "XML: net='%s' ref='%s' base='%s' shownNum='%s' expand=%zu",
                                          net_record->m_Name, refText, baseName,
                                          netNode.m_Pin->GetShownNumber(), nums.size() ) );

            for( const wxString& num : nums )
            {
                xnet->AddChild( xnode = node( wxT( "node" ) ) );
                xnode->AddAttribute( wxT( "ref" ), refText );
                xnode->AddAttribute( wxT( "pin" ), num );

                wxString fullName = baseName.IsEmpty() ? num : baseName + wxT( "_" ) + num;

                if( !baseName.IsEmpty() || nums.size() > 1 )
                    xnode->AddAttribute( wxT( "pinfunction" ), fullName );

                wxString typeAttr = pinType;

                if( net_record->m_HasNoConnect
                    && ( net_record->m_Nodes.size() == 1 || allNetPinsStacked ) )
                {
                    typeAttr += wxT( "+no_connect" );
                    wxLogTrace( traceStackedPins,
                                wxString::Format( "XML: marking node ref='%s' pin='%s' as no_connect",
                                                  refText, num ) );
                }

                xnode->AddAttribute( wxT( "pintype" ), typeAttr );
            }
        }
    }

    for( NET_RECORD* record : nets )
        delete record;

    return xnets;
}


XNODE* NETLIST_EXPORTER_XML::node( const wxString& aName,
                                   const wxString& aTextualContent /* = wxEmptyString*/ )
{
    XNODE* n = new XNODE( wxXML_ELEMENT_NODE, aName );

    if( aTextualContent.Len() > 0 )     // excludes wxEmptyString, the parameter's default value
        n->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, aTextualContent ) );

    return n;
}


static bool sortPinsByNumber( SCH_PIN* aPin1, SCH_PIN* aPin2 )
{
    // return "lhs < rhs"
    return StrNumCmp( aPin1->GetShownNumber(), aPin2->GetShownNumber(), true ) < 0;
}
void NETLIST_EXPORTER_XML::getSheetComponentClasses()
{
    m_sheetComponentClasses.clear();

    SCH_SHEET_LIST sheetList = m_schematic->Hierarchy();

    auto getComponentClassFields = [&]( const std::vector<SCH_FIELD>& fields, const SCH_SHEET_PATH* sheetPath )
    {
        std::unordered_set<wxString> componentClasses;

        for( const SCH_FIELD& field : fields )
        {
            if( field.GetCanonicalName() == wxT( "Component Class" ) )
            {
                if( field.GetShownText( sheetPath, false ) != wxEmptyString )
                    componentClasses.insert( field.GetShownText( sheetPath, false ) );
            }
        }

        return componentClasses;
    };

    for( const SCH_SHEET_PATH& sheet : sheetList )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SHEET_T ) )
        {
            SCH_SHEET*                                sheetItem = static_cast<SCH_SHEET*>( item );
            std::unordered_set<wxString>              sheetComponentClasses;
            const std::unordered_set<SCH_RULE_AREA*>& sheetRuleAreas = sheetItem->GetRuleAreaCache();

            for( const SCH_RULE_AREA* ruleArea : sheetRuleAreas )
            {
                for( const SCH_DIRECTIVE_LABEL* label : ruleArea->GetDirectives() )
                {
                    std::unordered_set<wxString> ruleAreaComponentClasses =
                            getComponentClassFields( label->GetFields(), &sheet );
                    sheetComponentClasses.insert( ruleAreaComponentClasses.begin(), ruleAreaComponentClasses.end() );
                }
            }

            SCH_SHEET_PATH newPath = sheet;
            newPath.push_back( sheetItem );
            wxASSERT( !m_sheetComponentClasses.contains( newPath ) );

            m_sheetComponentClasses[newPath] = sheetComponentClasses;
        }
    }
}

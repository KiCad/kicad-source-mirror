/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_library.h>
#include <string_utils.h>
#include <connection_graph.h>
#include <core/kicad_algo.h>
#include <wx/wfstream.h>
#include <xnode.h>      // also nests: <wx/xml/xml.h>

#include <symbol_lib_table.h>

#include <set>

static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 );

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
        xroot->AddChild( makeSymbols( aCtl ) );

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
                                            SCH_SHEET_PATH* aSheet )
{
    wxString                     value;
    wxString                     datasheet;
    wxString                     footprint;
    wxString                     candidate;
    std::map<wxString, wxString> userFields;

    if( aSymbol->GetUnitCount() > 1 )
    {
        // Sadly, each unit of a symbol can have its own unique fields. This
        // block finds the unit with the lowest number having a non blank field
        // value and records it.  Therefore user is best off setting fields
        // into only the first unit.  But this scavenger algorithm will find
        // any non blank fields in all units and use the first non-blank field
        // for each unique field name.

        wxString ref = aSymbol->GetRef( aSheet );

        SCH_SHEET_LIST sheetList = m_schematic->GetSheets();
        int minUnit = aSymbol->GetUnitSelection( aSheet );

        for( unsigned i = 0;  i < sheetList.size();  i++ )
        {
            for( SCH_ITEM* item : sheetList[i].LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                SCH_SYMBOL* symbol2 = (SCH_SYMBOL*) item;

                wxString ref2 = symbol2->GetRef( &sheetList[i] );

                if( ref2.CmpNoCase( ref ) != 0 )
                    continue;

                int unit = symbol2->GetUnitSelection( aSheet );

                // The lowest unit number wins.  User should only set fields in any one unit.
                candidate = symbol2->GetValueFieldText( m_resolveTextVars, &sheetList[i], false );

                if( !candidate.IsEmpty() && ( unit < minUnit || value.IsEmpty() ) )
                    value = candidate;

                candidate = symbol2->GetFootprintFieldText( m_resolveTextVars, &sheetList[i], false );

                if( !candidate.IsEmpty() && ( unit < minUnit || footprint.IsEmpty() ) )
                    footprint = candidate;

                candidate = m_resolveTextVars
                                ? symbol2->GetField( DATASHEET_FIELD )->GetShownText( &sheetList[i], false )
                                : symbol2->GetField( DATASHEET_FIELD )->GetText();

                if( !candidate.IsEmpty() && ( unit < minUnit || datasheet.IsEmpty() ) )
                    datasheet = candidate;

                for( int ii = MANDATORY_FIELDS; ii < symbol2->GetFieldCount(); ++ii )
                {
                    const SCH_FIELD& f = symbol2->GetFields()[ ii ];

                    if( f.GetText().size()
                        && ( unit < minUnit || userFields.count( f.GetName() ) == 0 ) )
                    {
                        if( m_resolveTextVars )
                            userFields[ f.GetName() ] = f.GetShownText( aSheet, false );
                        else
                            userFields[ f.GetName() ] = f.GetText();
                    }
                }

                minUnit = std::min( unit, minUnit );
            }
        }
    }
    else
    {
        value = aSymbol->GetValueFieldText( m_resolveTextVars, aSheet, false );
        footprint = aSymbol->GetFootprintFieldText( m_resolveTextVars, aSheet, false );

        if( m_resolveTextVars )
            datasheet = aSymbol->GetField( DATASHEET_FIELD )->GetShownText( aSheet, false );
        else
            datasheet = aSymbol->GetField( DATASHEET_FIELD )->GetText();

        for( int ii = MANDATORY_FIELDS; ii < aSymbol->GetFieldCount(); ++ii )
        {
            const SCH_FIELD& f = aSymbol->GetFields()[ ii ];

            if( f.GetText().size() )
            {
                if( m_resolveTextVars )
                    userFields[ f.GetName() ] = f.GetShownText( aSheet, false );
                else
                    userFields[ f.GetName() ] = f.GetText();
            }
        }
    }

    // Do not output field values blank in netlist:
    if( value.size() )
        aNode->AddChild( node( wxT( "value" ), UnescapeString( value ) ) );
    else    // value field always written in netlist
        aNode->AddChild( node( wxT( "value" ), wxT( "~" ) ) );

    if( footprint.size() )
        aNode->AddChild( node( wxT( "footprint" ), UnescapeString( footprint ) ) );

    if( datasheet.size() )
        aNode->AddChild( node( wxT( "datasheet" ), UnescapeString( datasheet ) ) );

    if( userFields.size() )
    {
        XNODE* xfields;
        aNode->AddChild( xfields = node( wxT( "fields" ) ) );

        // non MANDATORY fields are output alphabetically
        for( const std::pair<const wxString, wxString>& f : userFields )
        {
            XNODE* xfield = node( wxT( "field" ), UnescapeString( f.second ) );
            xfield->AddAttribute( wxT( "name" ), UnescapeString( f.first ) );
            xfields->AddChild( xfield );
        }
    }
}


XNODE* NETLIST_EXPORTER_XML::makeSymbols( unsigned aCtl )
{
    XNODE* xcomps = node( wxT( "components" ) );

    m_referencesAlreadyFound.Clear();
    m_libParts.clear();

    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();
    SCH_SHEET_PATH currentSheet = m_schematic->CurrentSheet();

    // Output is xml, so there is no reason to remove spaces from the field values.
    // And XML element names need not be translated to various languages.

    for( unsigned ii = 0; ii < sheetList.size(); ii++ )
    {
        SCH_SHEET_PATH sheet = sheetList[ii];

        // Change schematic CurrentSheet in each iteration to allow hierarchical
        // resolution of text variables in sheet fields.
        m_schematic->SetCurrentSheet( sheet );

        auto cmp = [sheet]( SCH_SYMBOL* a, SCH_SYMBOL* b )
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
            SCH_SYMBOL* symbol = findNextSymbol( item, &sheet );

            if( !symbol
               || ( ( aCtl & GNL_OPT_BOM ) && symbol->GetExcludedFromBOM() )
               || ( ( aCtl & GNL_OPT_KICAD ) && symbol->GetExcludedFromBoard() ) )
            {
                continue;
            }

            // Output the symbol's elements in order of expected access frequency. This may
            // not always look best, but it will allow faster execution under XSL processing
            // systems which do sequential searching within an element.

            XNODE* xcomp;  // current symbol being constructed
            xcomps->AddChild( xcomp = node( wxT( "comp" ) ) );

            xcomp->AddAttribute( wxT( "ref" ), symbol->GetRef( &sheet ) );
            addSymbolFields( xcomp, symbol, &sheet );

            XNODE*  xlibsource;
            xcomp->AddChild( xlibsource = node( wxT( "libsource" ) ) );

            // "logical" library name, which is in anticipation of a better search algorithm
            // for parts based on "logical_lib.part" and where logical_lib is merely the library
            // name minus path and extension.
            wxString libName;
            wxString partName;

            if( symbol->UseLibIdLookup() )
            {
                libName = symbol->GetLibId().GetLibNickname();
                partName = symbol->GetLibId().GetLibItemName();
            }
            else
            {
                partName = symbol->GetSchSymbolLibraryName();
            }

            xlibsource->AddAttribute( wxT( "lib" ), libName );

            // We only want the symbol name, not the full LIB_ID.
            xlibsource->AddAttribute( wxT( "part" ), partName );

            xlibsource->AddAttribute( wxT( "description" ), symbol->GetDescription() );

            /* Add the symbol properties. */
            XNODE* xproperty;

            std::vector<SCH_FIELD>& fields = symbol->GetFields();

            for( size_t jj = MANDATORY_FIELDS; jj < fields.size(); ++jj )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), fields[jj].GetCanonicalName() );

                if( m_resolveTextVars )
                    xproperty->AddAttribute( wxT( "value" ), fields[jj].GetShownText( &sheet, false ) );
                else
                    xproperty->AddAttribute( wxT( "value" ), fields[jj].GetText() );
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

            if( symbol->GetExcludedFromBOM() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "exclude_from_bom" ) );
            }

            if( symbol->GetExcludedFromBoard() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "exclude_from_board" ) );
            }

            if( symbol->GetDNP() )
            {
                xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                xproperty->AddAttribute( wxT( "name" ), wxT( "dnp" ) );
            }

            if( const std::unique_ptr<LIB_SYMBOL>& part = symbol->GetLibSymbolRef() )
            {
                if( part->GetKeyWords().size() )
                {
                    xcomp->AddChild( xproperty = node( wxT( "property" ) ) );
                    xproperty->AddAttribute( wxT( "name" ), wxT( "ki_keywords" ) );
                    xproperty->AddAttribute( wxT( "value" ), part->GetKeyWords() );
                }
            }

            XNODE* xsheetpath;
            xcomp->AddChild( xsheetpath = node( wxT( "sheetpath" ) ) );

            xsheetpath->AddAttribute( wxT( "names" ), sheet.PathHumanReadable() );
            xsheetpath->AddAttribute( wxT( "tstamps" ), sheet.PathAsString() );

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
        }
    }

    m_schematic->SetCurrentSheet( currentSheet );

    return xcomps;
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

    const std::map<wxString, wxString>& properties = m_schematic->Prj().GetTextVars();

    for( const std::pair<const wxString, wxString>& prop : properties )
    {
        xdesign->AddChild( xtextvar = node( wxT( "textvar" ), prop.second ) );
        xtextvar->AddAttribute( wxT( "name" ), prop.first );
    }

    /*
     *  Export the sheets information
     */
    SCH_SHEET_LIST sheetList = m_schematic->GetSheets();

    for( unsigned i = 0;  i < sheetList.size();  i++ )
    {
        screen = sheetList[i].LastScreen();

        xdesign->AddChild( xsheet = node( wxT( "sheet" ) ) );

        // get the string representation of the sheet index number.
        // Note that sheet->GetIndex() is zero index base and we need to increment the
        // number by one to make it human readable
        sheetTxt.Printf( wxT( "%u" ), i + 1 );
        xsheet->AddAttribute( wxT( "number" ), sheetTxt );
        xsheet->AddAttribute( wxT( "name" ), sheetList[i].PathHumanReadable() );
        xsheet->AddAttribute( wxT( "tstamps" ), sheetList[i].PathAsString() );

        TITLE_BLOCK tb = screen->GetTitleBlock();
        PROJECT*    prj = &m_schematic->Prj();

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
    SYMBOL_LIB_TABLE* symbolLibTable = m_schematic->Prj().SchSymbolLibTable();

    for( std::set<wxString>::iterator it = m_libraries.begin(); it!=m_libraries.end();  ++it )
    {
        wxString    libNickname = *it;
        XNODE*      xlibrary;

        if( symbolLibTable->HasLibrary( libNickname ) )
        {
            xlibs->AddChild( xlibrary = node( wxT( "library" ) ) );
            xlibrary->AddAttribute( wxT( "logical" ), libNickname );
            xlibrary->AddChild( node( wxT( "uri" ), symbolLibTable->GetFullURI( libNickname ) ) );
        }

        // @todo: add more fun stuff here
    }

    return xlibs;
}


XNODE* NETLIST_EXPORTER_XML::makeLibParts()
{
    XNODE*                  xlibparts = node( wxT( "libparts" ) );   // auto_ptr

    LIB_PINS                pinList;
    std::vector<LIB_FIELD*> fieldList;

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
                xfootprints->AddChild( node( wxT( "fp" ), lcomp->GetFPFilters()[i] ) );
        }

        //----- show the fields here ----------------------------------
        fieldList.clear();
        lcomp->GetFields( fieldList );

        XNODE*     xfields;
        xlibpart->AddChild( xfields = node( "fields" ) );

        for( const LIB_FIELD* field : fieldList )
        {
            if( !field->GetText().IsEmpty() )
            {
                XNODE*     xfield;
                xfields->AddChild( xfield = node( wxT( "field" ), field->GetText() ) );
                xfield->AddAttribute( wxT( "name" ), field->GetCanonicalName() );
            }
        }

        //----- show the pins here ------------------------------------
        pinList.clear();
        lcomp->GetPins( pinList, 0, 0 );

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

        if( pinList.size() )
        {
            XNODE*     pins;

            xlibpart->AddChild( pins = node( wxT( "pins" ) ) );
            for( unsigned i=0; i<pinList.size();  ++i )
            {
                XNODE*     pin;

                pins->AddChild( pin = node( wxT( "pin" ) ) );
                pin->AddAttribute( wxT( "num" ), pinList[i]->GetShownNumber() );
                pin->AddAttribute( wxT( "name" ), pinList[i]->GetShownName() );
                pin->AddAttribute( wxT( "type" ), pinList[i]->GetCanonicalElectricalTypeName() );

                // caution: construction work site here, drive slowly
            }
        }
    }

    return xlibparts;
}


XNODE* NETLIST_EXPORTER_XML::makeListOfNets( unsigned aCtl )
{
    XNODE*      xnets = node( wxT( "nets" ) );      // auto_ptr if exceptions ever get used.
    wxString    netCodeTxt;
    wxString    netName;
    wxString    ref;

    XNODE*      xnet = nullptr;

    /*  output:
        <net code="123" name="/cfcard.sch/WAIT#">
            <node ref="R23" pin="1"/>
            <node ref="U18" pin="12"/>
        </net>
    */

    struct NET_NODE
    {
        NET_NODE( SCH_PIN* aPin, const SCH_SHEET_PATH& aSheet ) : m_Pin( aPin ), m_Sheet( aSheet )
        {}

        SCH_PIN*       m_Pin;
        SCH_SHEET_PATH m_Sheet;
    };

    struct NET_RECORD
    {
        NET_RECORD( const wxString& aName ) : m_Name( aName ), m_HasNoConnect( false ){};

        wxString              m_Name;
        bool                  m_HasNoConnect;
        std::vector<NET_NODE> m_Nodes;
    };

    std::vector<NET_RECORD*> nets;

    for( const auto& [ key, subgraphs ] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        wxString    net_name  = key.Name;
        NET_RECORD* net_record = nullptr;

        if( subgraphs.empty() )
            continue;

        nets.emplace_back( new NET_RECORD( net_name ) );
        net_record = nets.back();

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            bool nc = subgraph->GetNoConnect() && subgraph->GetNoConnect()->Type() == SCH_NO_CONNECT_T;
            const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

            if( nc )
                net_record->m_HasNoConnect = true;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                    SCH_SYMBOL* symbol = pin->GetParentSymbol();

                    if( !symbol
                       || ( ( aCtl & GNL_OPT_BOM ) && symbol->GetExcludedFromBOM() )
                       || ( ( aCtl & GNL_OPT_KICAD ) && symbol->GetExcludedFromBoard() ) )
                    {
                        continue;
                    }

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
            wxString pinText = netNode.m_Pin->GetShownNumber();

            // Skip power symbols and virtual symbols
            if( refText[0] == wxChar( '#' ) )
                continue;

            if( !added )
            {
                netCodeTxt.Printf( wxT( "%d" ), i + 1 );

                xnets->AddChild( xnet = node( wxT( "net" ) ) );
                xnet->AddAttribute( wxT( "code" ), netCodeTxt );
                xnet->AddAttribute( wxT( "name" ), net_record->m_Name );

                added = true;
            }

            xnet->AddChild( xnode = node( wxT( "node" ) ) );
            xnode->AddAttribute( wxT( "ref" ), refText );
            xnode->AddAttribute( wxT( "pin" ), pinText );

            wxString pinName = netNode.m_Pin->GetShownName();
            wxString pinType = netNode.m_Pin->GetCanonicalElectricalTypeName();

            if( !pinName.IsEmpty() )
                xnode->AddAttribute( wxT( "pinfunction" ), pinName );

            if( net_record->m_HasNoConnect
                && ( net_record->m_Nodes.size() == 1 || allNetPinsStacked ) )
                pinType += wxT( "+no_connect" );

            xnode->AddAttribute( wxT( "pintype" ), pinType );
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


static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 )
{
    // return "lhs < rhs"
    return StrNumCmp( aPin1->GetShownNumber(), aPin2->GetShownNumber(), true ) < 0;
}

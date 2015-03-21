/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <build_version.h>
#include <sch_base_frame.h>
#include <class_library.h>

#include <schframe.h>
#include "netlist_exporter_generic.h"

static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 );

bool NETLIST_EXPORTER_GENERIC::Write( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // output the XML format netlist.
    wxXmlDocument   xdoc;

    xdoc.SetRoot( makeRoot() );

    return xdoc.Save( aOutFileName, 2 /* indent bug, today was ignored by wxXml lib */ );
}


XNODE* NETLIST_EXPORTER_GENERIC::makeRoot()
{
    XNODE*      xroot = node( wxT( "export" ) );

    xroot->AddAttribute( wxT( "version" ), wxT( "D" ) );

    // add the "design" header
    xroot->AddChild( makeDesignHeader() );

    xroot->AddChild( makeComponents() );

    xroot->AddChild( makeLibParts() );

    // must follow makeGenericLibParts()
    xroot->AddChild( makeLibraries() );

    xroot->AddChild( makeListOfNets() );

    return xroot;
}


XNODE* NETLIST_EXPORTER_GENERIC::makeComponents()
{
    XNODE*      xcomps = node( wxT( "components" ) );

    wxString    timeStamp;

    // some strings we need many times, but don't want to construct more
    // than once for performance.  These are used within loops so the
    // enclosing wxString constructor would fire on each loop iteration if
    // they were in a nested scope.

    // these are actually constructor invocations, not assignments as it appears:
    wxString    sFields     = wxT( "fields" );
    wxString    sField      = wxT( "field" );
    wxString    sComponent  = wxT( "comp" );          // use "part" ?
    wxString    sName       = wxT( "name" );
    wxString    sRef        = wxT( "ref" );
    wxString    sPins       = wxT( "pins" );
    wxString    sPin        = wxT( "pin" );
    wxString    sValue      = wxT( "value" );
    wxString    sSheetPath  = wxT( "sheetpath" );
    wxString    sFootprint  = wxT( "footprint" );
    wxString    sDatasheet  = wxT( "datasheet" );
    wxString    sTStamp     = wxT( "tstamp" );
    wxString    sTStamps    = wxT( "tstamps" );
    wxString    sTSFmt      = wxT( "%8.8lX" );        // comp->m_TimeStamp
    wxString    sLibSource  = wxT( "libsource" );
    wxString    sLibPart    = wxT( "libpart" );
    wxString    sLib        = wxT( "lib" );
    wxString    sPart       = wxT( "part" );
    wxString    sNames      = wxT( "names" );

    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST sheetList;

    // Output is xml, so there is no reason to remove spaces from the field values.
    // And XML element names need not be translated to various languages.

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_ITEM* schItem = path->LastDrawList();  schItem;  schItem = schItem->Next() )
        {
            SCH_COMPONENT*  comp = findNextComponentAndCreatePinList( schItem, path );
            if( !comp )
                break;  // No component left

            schItem = comp;

            XNODE* xcomp;  // current component being constructed

            // Output the component's elements in order of expected access frequency.
            // This may not always look best, but it will allow faster execution
            // under XSL processing systems which do sequential searching within
            // an element.

            xcomps->AddChild( xcomp = node( sComponent ) );
            xcomp->AddAttribute( sRef, comp->GetRef( path ) );

            xcomp->AddChild( node( sValue, comp->GetField( VALUE )->GetText() ) );

            if( !comp->GetField( FOOTPRINT )->IsVoid() )
                xcomp->AddChild( node( sFootprint, comp->GetField( FOOTPRINT )->GetText() ) );

            if( !comp->GetField( DATASHEET )->IsVoid() )
                xcomp->AddChild( node( sDatasheet, comp->GetField( DATASHEET )->GetText() ) );

            // Export all user defined fields within the component,
            // which start at field index MANDATORY_FIELDS.  Only output the <fields>
            // container element if there are any <field>s.
            if( comp->GetFieldCount() > MANDATORY_FIELDS )
            {
                XNODE* xfields;
                xcomp->AddChild( xfields = node( sFields ) );

                for( int fldNdx = MANDATORY_FIELDS; fldNdx < comp->GetFieldCount(); ++fldNdx )
                {
                    SCH_FIELD*  f = comp->GetField( fldNdx );

                    // only output a field if non empty and not just "~"
                    if( !f->IsVoid() )
                    {
                        XNODE*  xfield;
                        xfields->AddChild( xfield = node( sField, f->GetText() ) );
                        xfield->AddAttribute( sName, f->GetName() );
                    }
                }
            }

            XNODE*  xlibsource;
            xcomp->AddChild( xlibsource = node( sLibSource ) );

            // "logical" library name, which is in anticipation of a better search
            // algorithm for parts based on "logical_lib.part" and where logical_lib
            // is merely the library name minus path and extension.
            LIB_PART* part = m_libs->FindLibPart( comp->GetPartName() );
            if( part )
                xlibsource->AddAttribute( sLib, part->GetLib()->GetLogicalName() );

            xlibsource->AddAttribute( sPart, comp->GetPartName() );

            XNODE* xsheetpath;

            xcomp->AddChild( xsheetpath = node( sSheetPath ) );
            xsheetpath->AddAttribute( sNames, path->PathHumanReadable() );
            xsheetpath->AddAttribute( sTStamps, path->Path() );

            timeStamp.Printf( sTSFmt, comp->GetTimeStamp() );
            xcomp->AddChild( node( sTStamp, timeStamp ) );
        }
    }

    return xcomps;
}


XNODE* NETLIST_EXPORTER_GENERIC::makeDesignHeader()
{
    SCH_SCREEN* screen;
    XNODE*     xdesign = node( wxT("design") );
    XNODE*     xtitleBlock;
    XNODE*     xsheet;
    XNODE*     xcomment;
    wxString   sheetTxt;
    wxFileName sourceFileName;

    // the root sheet is a special sheet, call it source
    xdesign->AddChild( node( wxT( "source" ), g_RootSheet->GetScreen()->GetFileName() ) );

    xdesign->AddChild( node( wxT( "date" ), DateAndTime() ) );

    // which Eeschema tool
    xdesign->AddChild( node( wxT( "tool" ), wxT( "Eeschema " ) + GetBuildVersion() ) );

    /*
        Export the sheets information
    */
    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
    	screen = sheet->LastScreen();

        xdesign->AddChild( xsheet = node( wxT( "sheet" ) ) );

        // get the string representation of the sheet index number. 
        // Note that sheet->GetIndex() is zero index base and we need to increment the number by one to make 
        // human readable
		sheetTxt.Printf( wxT( "%d" ), ( sheetList.GetIndex() + 1 ) );
        xsheet->AddAttribute( wxT( "number" ), sheetTxt );
        xsheet->AddAttribute( wxT( "name" ), sheet->PathHumanReadable() );
        xsheet->AddAttribute( wxT( "tstamps" ), sheet->Path() );

    
        TITLE_BLOCK tb = screen->GetTitleBlock();

        xsheet->AddChild( xtitleBlock = node( wxT( "title_block" ) ) );
    
        xtitleBlock->AddChild( node( wxT( "title" ), tb.GetTitle() ) );
        xtitleBlock->AddChild( node( wxT( "company" ), tb.GetCompany() ) );
        xtitleBlock->AddChild( node( wxT( "rev" ), tb.GetRevision() ) );
        xtitleBlock->AddChild( node( wxT( "date" ), tb.GetDate() ) );

        // We are going to remove the fileName directories.
        sourceFileName = wxFileName( screen->GetFileName() );
        xtitleBlock->AddChild( node( wxT( "source" ), sourceFileName.GetFullName() ) );
   
        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT("number"), wxT("1") );
        xcomment->AddAttribute( wxT( "value" ), tb.GetComment1() );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT("number"), wxT("2") );
        xcomment->AddAttribute( wxT( "value" ), tb.GetComment2() );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT("number"), wxT("3") );
        xcomment->AddAttribute( wxT( "value" ), tb.GetComment3() );

        xtitleBlock->AddChild( xcomment = node( wxT( "comment" ) ) );
        xcomment->AddAttribute( wxT("number"), wxT("4") );
        xcomment->AddAttribute( wxT( "value" ), tb.GetComment4() );
	}

    return xdesign;
}


XNODE* NETLIST_EXPORTER_GENERIC::makeLibraries()
{
    XNODE*  xlibs = node( wxT( "libraries" ) );     // auto_ptr

    for( std::set<void*>::iterator it = m_Libraries.begin(); it!=m_Libraries.end();  ++it )
    {
        PART_LIB*    lib = (PART_LIB*) *it;
        XNODE*      xlibrary;

        xlibs->AddChild( xlibrary = node( wxT( "library" ) ) );
        xlibrary->AddAttribute( wxT( "logical" ), lib->GetLogicalName() );
        xlibrary->AddChild( node( wxT( "uri" ),  lib->GetFullFileName() ) );

        // @todo: add more fun stuff here
    }

    return xlibs;
}


XNODE* NETLIST_EXPORTER_GENERIC::makeLibParts()
{
    XNODE*      xlibparts = node( wxT( "libparts" ) );   // auto_ptr
    wxString    sLibpart  = wxT( "libpart" );
    wxString    sLib      = wxT( "lib" );
    wxString    sPart     = wxT( "part" );
    wxString    sAliases  = wxT( "aliases" );
    wxString    sAlias    = wxT( "alias" );
    wxString    sPins     = wxT( "pins" );      // key for library component pins list
    wxString    sPin      = wxT( "pin" );       // key for one library component pin descr
    wxString    sPinNum   = wxT( "num" );       // key for one library component pin num
    wxString    sPinName  = wxT( "name" );      // key for one library component pin name
    wxString    sPinType  = wxT( "type" );      // key for one library component pin electrical type
    wxString    sName     = wxT( "name" );
    wxString    sField    = wxT( "field" );
    wxString    sFields   = wxT( "fields" );
    wxString    sDescr    = wxT( "description" );
    wxString    sDocs     = wxT( "docs" );
    wxString    sFprints  = wxT( "footprints" );
    wxString    sFp       = wxT( "fp" );

    LIB_PINS    pinList;
    LIB_FIELDS  fieldList;

    m_Libraries.clear();

    for( std::set<void*>::iterator it = m_LibParts.begin(); it!=m_LibParts.end();  ++it )
    {
        LIB_PART*       lcomp = (LIB_PART*     ) *it;
        PART_LIB*    library = lcomp->GetLib();

        m_Libraries.insert( library );  // inserts component's library if unique

        XNODE*      xlibpart;
        xlibparts->AddChild( xlibpart = node( sLibpart ) );
        xlibpart->AddAttribute( sLib, library->GetLogicalName() );
        xlibpart->AddAttribute( sPart, lcomp->GetName()  );

        if( lcomp->GetAliasCount() )
        {
            wxArrayString aliases = lcomp->GetAliasNames( false );
            if( aliases.GetCount() )
            {
                XNODE*  xaliases = node( sAliases );
                xlibpart->AddChild( xaliases );
                for( unsigned i=0;  i<aliases.GetCount();  ++i )
                {
                    xaliases->AddChild( node( sAlias, aliases[i] ) );
                }
            }
        }

        //----- show the important properties -------------------------
        if( !lcomp->GetAlias( 0 )->GetDescription().IsEmpty() )
            xlibpart->AddChild( node( sDescr, lcomp->GetAlias( 0 )->GetDescription() ) );

        if( !lcomp->GetAlias( 0 )->GetDocFileName().IsEmpty() )
            xlibpart->AddChild( node( sDocs,  lcomp->GetAlias( 0 )->GetDocFileName() ) );

        // Write the footprint list
        if( lcomp->GetFootPrints().GetCount() )
        {
            XNODE*  xfootprints;
            xlibpart->AddChild( xfootprints = node( sFprints ) );

            for( unsigned i=0; i<lcomp->GetFootPrints().GetCount(); ++i )
            {
                xfootprints->AddChild( node( sFp, lcomp->GetFootPrints()[i] ) );
            }
        }

        //----- show the fields here ----------------------------------
        fieldList.clear();
        lcomp->GetFields( fieldList );

        XNODE*     xfields;
        xlibpart->AddChild( xfields = node( sFields ) );

        for( unsigned i=0;  i<fieldList.size();  ++i )
        {
            if( !fieldList[i].GetText().IsEmpty() )
            {
                XNODE*     xfield;
                xfields->AddChild( xfield = node( sField, fieldList[i].GetText() ) );
                xfield->AddAttribute( sName, fieldList[i].GetName(false) );
            }
        }

        //----- show the pins here ------------------------------------
        pinList.clear();
        lcomp->GetPins( pinList, 0, 0 );

        /* we must erase redundant Pins references in pinList
         * These redundant pins exist because some pins
         * are found more than one time when a component has
         * multiple parts per package or has 2 representations (DeMorgan conversion)
         * For instance, a 74ls00 has DeMorgan conversion, with different pin shapes,
         * and therefore each pin  appears 2 times in the list.
         * Common pins (VCC, GND) can also be found more than once.
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

            xlibpart->AddChild( pins = node( sPins ) );
            for( unsigned i=0; i<pinList.size();  ++i )
            {
                XNODE*     pin;

                pins->AddChild( pin = node( sPin ) );
                pin->AddAttribute( sPinNum, pinList[i]->GetNumberString() );
                pin->AddAttribute( sPinName, pinList[i]->GetName() );
                pin->AddAttribute( sPinType, pinList[i]->GetTypeString() );

                // caution: construction work site here, drive slowly
            }
        }
    }

    return xlibparts;
}


XNODE* NETLIST_EXPORTER_GENERIC::makeListOfNets()
{
    XNODE*      xnets = node( wxT( "nets" ) );      // auto_ptr if exceptions ever get used.
    wxString    netCodeTxt;
    wxString    netName;
    wxString    ref;

    wxString    sNet  = wxT( "net" );
    wxString    sName = wxT( "name" );
    wxString    sCode = wxT( "code" );
    wxString    sRef  = wxT( "ref" );
    wxString    sPin  = wxT( "pin" );
    wxString    sNode = wxT( "node" );
    wxString    sFmtd = wxT( "%d" );

    XNODE*      xnet = 0;
    int         netCode;
    int         lastNetCode = -1;
    int         sameNetcodeCount = 0;


    /*  output:
        <net code="123" name="/cfcard.sch/WAIT#">
            <node ref="R23" pin="1"/>
            <node ref="U18" pin="12"/>
        </net>
    */

    m_LibParts.clear();     // must call this function before using m_LibParts.

    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
    {
        NETLIST_OBJECT* nitem = m_masterList->GetItem( ii );
        SCH_COMPONENT*  comp;

        // New net found, write net id;
        if( ( netCode = nitem->GetNet() ) != lastNetCode )
        {
            sameNetcodeCount = 0;   // item count for this net
            netName = nitem->GetNetName();
            lastNetCode  = netCode;
        }

        if( nitem->m_Type != NET_PIN )
            continue;

        if( nitem->m_Flag != 0 )     // Redundant pin, skip it
            continue;

        comp = nitem->GetComponentParent();

        // Get the reference for the net name and the main parent component
        ref = comp->GetRef( &nitem->m_SheetPath );
        if( ref[0] == wxChar( '#' ) )
            continue;

        if( ++sameNetcodeCount == 1 )
        {
            xnets->AddChild( xnet = node( sNet ) );
            netCodeTxt.Printf( sFmtd, netCode );
            xnet->AddAttribute( sCode, netCodeTxt );
            xnet->AddAttribute( sName, netName );
        }

        XNODE*      xnode;
        xnet->AddChild( xnode = node( sNode ) );
        xnode->AddAttribute( sRef, ref );
        xnode->AddAttribute( sPin,  nitem->GetPinNumText() );
    }

    return xnets;
}


bool NETLIST_EXPORTER_GENERIC::writeListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
{
    int         ret = 0;
    int         netCode;
    int         lastNetCode = -1;
    int         sameNetcodeCount = 0;
    wxString    netName;
    wxString    ref;
    wxString    netcodeName;
    char        firstItemInNet[256];

    for( unsigned ii = 0; ii < aObjectsList.size(); ii++ )
    {
        SCH_COMPONENT*  comp;
        NETLIST_OBJECT* nitem = aObjectsList[ii];

        // New net found, write net id;
        if( ( netCode = nitem->GetNet() ) != lastNetCode )
        {
            sameNetcodeCount = 0;              // Items count for this net
            netName = nitem->GetNetName();

            netcodeName.Printf( wxT( "Net %d " ), netCode );
            netcodeName << wxT( "\"" ) << netName << wxT( "\"" );

            // Add the netname without prefix, in cases we need only the
            // "short" netname
            netcodeName += wxT( " \"" ) + nitem->GetShortNetName() + wxT( "\"" );
            lastNetCode  = netCode;
        }

        if( nitem->m_Type != NET_PIN )
            continue;

        if( nitem->m_Flag != 0 )     // Redundant pin, skip it
            continue;

        comp = nitem->GetComponentParent();

        // Get the reference for the net name and the main parent component
        ref = comp->GetRef( &nitem->m_SheetPath );
        if( ref[0] == wxChar( '#' ) )
            continue;                 // Pseudo component (Like Power symbol)

        // Print the pin list for this net, use special handling if
        // 2 or more items are connected:

        // if first item for this net found, defer printing this connection
        // until a second item will is found
        if( ++sameNetcodeCount == 1 )
        {
            snprintf( firstItemInNet, sizeof(firstItemInNet), " %s %.4s\n",
                      TO_UTF8( ref ),
                      (const char*) &aObjectsList[ii]->m_PinNum );
        }

        // Second item for this net found, print the Net name, and the
        // first item
        if( sameNetcodeCount == 2 )
        {
            ret |= fprintf( f, "%s\n", TO_UTF8( netcodeName ) );
            ret |= fputs( firstItemInNet, f );
        }

        if( sameNetcodeCount >= 2 )
            ret |= fprintf( f, " %s %.4s\n", TO_UTF8( ref ),
                     (const char*) &nitem->m_PinNum );
    }

    return ret >= 0;
}


XNODE* NETLIST_EXPORTER_GENERIC::node( const wxString& aName, const wxString& aTextualContent /* = wxEmptyString*/ )
{
    XNODE* n = new XNODE( wxXML_ELEMENT_NODE, aName );

    if( aTextualContent.Len() > 0 )     // excludes wxEmptyString, the parameter's default value
        n->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, aTextualContent ) );

    return n;
}


static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 )
{
    // return "lhs < rhs"
    return RefDesStringCompare( aPin1->GetNumberString(), aPin2->GetNumberString() ) < 0;
}

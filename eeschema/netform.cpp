/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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
 * @file eeschema/netform.cpp
 * @brief Net list generation code.
 */

#include <fctsys.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <schframe.h>

#include <netlist.h>
#include <sch_reference_list.h>
#include <class_netlist_object.h>
#include <class_library.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_text.h>
#include <sch_sheet.h>

#include <wx/tokenzr.h>
#include <xnode.h>      // also nests: <wx/xml/xml.h>
#include <build_version.h>
#include <set>
#include <sch_base_frame.h>

#define INTERMEDIATE_NETLIST_EXT wxT( "xml" )

/**
 * Class UNIQUE_STRINGS
 * tracks unique wxStrings and is useful in telling if a string
 * has been seen before.
 */
class UNIQUE_STRINGS
{
    std::set<wxString>      m_set;    ///< set of wxStrings already found

    typedef std::set<wxString>::iterator us_iterator;

public:
    /**
     * Function Clear
     * erases the record.
     */
    void Clear()  {  m_set.clear();  }

    /**
     * Function Lookup
     * returns true if \a aString already exists in the set, otherwise returns
     * false and adds \a aString to the set for next time.
     */
    bool Lookup( const wxString& aString );
};

bool UNIQUE_STRINGS::Lookup( const wxString& aString )
{
    std::pair<us_iterator, bool> pair = m_set.insert( aString );

    return !pair.second;
}


/**
 * Class NETLIST_EXPORT_TOOL
 * is a private implementation class used in this source file to keep track
 * of and recycle data structures used in the generation of various exported netlist
 * files.  Since it is private it is not in a header file.
 */
class NETLIST_EXPORT_TOOL
{
    NETLIST_OBJECT_LIST* m_masterList;      /// The main connected items flat list

    PART_LIBS*          m_libs;             /// no ownership

    /// Used to temporary store and filter the list of pins of a schematic component
    /// when generating schematic component data in netlist (comp section)
    NETLIST_OBJECT_LIST m_SortedComponentPinList;

    /// Used for "multi parts per package" components,
    /// avoids processing a lib component more than once.
    UNIQUE_STRINGS      m_ReferencesAlreadyFound;

    // share a code generated std::set<void*> to reduce code volume

    std::set<void*>     m_LibParts;     ///< unique library parts used

    std::set<void*>     m_Libraries;    ///< unique libraries used


    /**
     * Function sprintPinNetName
     * formats the net name for \a aPin using \a aNetNameFormat into \a aResult.
     * <p>
     *  Net name is:
     *  <ul>
     * <li> "?" if pin not connected
     * <li> "netname" for global net (like gnd, vcc ..
     * <li> "/path/netname" for the usual nets
     * </ul>
     * if aUseNetcodeAsNetName is true, the net name is just the net code (SPICE only)
     */
    static void sprintPinNetName( wxString& aResult, const wxString& aNetNameFormat,
                                  NETLIST_OBJECT* aPin, bool aUseNetcodeAsNetName = false );

    /**
     * Function findNextComponentAndCreatePinList
     * finds a component from the DrawList and builds
     * its pin list in m_SortedComponentPinList. This list is sorted by pin num.
     * the component is the next actual component after aItem
     * (power symbols and virtual components that have their reference starting by '#'are skipped).
     */
    SCH_COMPONENT* findNextComponentAndCreatePinList( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath );

    SCH_COMPONENT* findNextComponent( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath );

    /**
     * Function eraseDuplicatePins
     * erase duplicate Pins from m_SortedComponentPinList (i.e. set pointer in this list to NULL).
     * (This is a list of pins found in the whole schematic, for a single
     * component.) These duplicate pins were put in list because some pins (powers... )
     * are found more than one time when we have a multiple parts per package
     * component. For instance, a 74ls00 has 4 parts, and therefore the VCC pin
     * and GND pin appears 4 times in the list.
     * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
     * Also set the m_Flag member of "removed" NETLIST_OBJECT pin item to 1
     */
    void eraseDuplicatePins( );

    /**
     * Function addPinToComponentPinList
     * adds a new pin description to the pin list m_SortedComponentPinList.
     * A pin description is a pointer to the corresponding structure
     * created by BuildNetList() in the table g_NetObjectslist.
     */
    bool addPinToComponentPinList( SCH_COMPONENT*  Component,
                                   SCH_SHEET_PATH* sheet,
                                   LIB_PIN*        PinEntry );

    /**
     * Function findAllInstancesOfComponent
     * is used for "multiple parts per package" components.
     * <p>
     * Search the entire design for all instances of \a aComponent based on
     * matching reference designator, and for each part, add all its pins
     * to the temporary sorted pin list.
     */
    void findAllInstancesOfComponent( SCH_COMPONENT*  aComponent,
                                      LIB_PART*       aEntry,
                                      SCH_SHEET_PATH* aSheetPath );

    /**
     * Function writeGENERICListOfNets
     * writes out nets (ranked by Netcode), and elements that are
     * connected as part of that net.
     */
    bool writeGENERICListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );

    /**
     * Function writeListOfNetsCADSTAR
     * writes a net list (ranked by Netcode), and pins connected to it.
     * <p>
     * Format:
     *   - ADD_TER RR2 6 \"$42\"
     *   - B U1 100
     *   - 6 CA
     * </p>
     */
    bool writeListOfNetsCADSTAR( FILE* f );

    /**
     * Function makeGenericRoot
     * builds the entire document tree for the generic export.  This is factored
     * out here so we can write the tree in either S-expression file format
     * or in XML if we put the tree built here into a wxXmlDocument.
     * @return XNODE* - the root nodes
     */
    XNODE* makeGenericRoot();

    /**
     * Function makeGenericComponents
     * @return XNODE* - returns a sub-tree holding all the schematic components.
     */
    XNODE* makeGenericComponents();

    /**
     * Function makeGenericDesignHeader
     * fills out a project "design" header into an XML node.
     * @return XNODE* - the design header
     */
    XNODE* makeGenericDesignHeader();

    /**
     * Function makeGenericLibParts
     * fills out an XML node with the unique library parts and returns it.
     * @return XNODE* - the library parts nodes
     */
    XNODE* makeGenericLibParts();

    /**
     * Function makeGenericListOfNets
     * fills out an XML node with a list of nets and returns it.
     * @return XNODE* - the list of nets nodes
     */
    XNODE* makeGenericListOfNets();

    /**
     * Function makeGenericLibraries
     * fills out an XML node with a list of used libraries and returns it.
     * Must have called makeGenericLibParts() before this function.
     * @return XNODE* - the library nodes
     */
    XNODE* makeGenericLibraries();

public:
    NETLIST_EXPORT_TOOL( NETLIST_OBJECT_LIST* aMasterList, PART_LIBS* aLibs )
    {
        m_masterList = aMasterList;
        m_libs = aLibs;
    }

    /**
     * Function WriteKiCadNetList
     * creates a netlist, using the S expressions.
     * the netlist creates the same data as the generic XML netlist,
     * but using SWEET (or S expression), more easy to read and faster to parse
     * @param aOutFileName = the full filename of the file to create
     * @return bool - true if there were no errors, else false.
     */
    bool WriteKiCadNetList( const wxString& aOutFileName );

    /**
     * Function WriteGENERICNetList
     * creates a generic netlist, now in XML.
     * @param aOutFileName = the full filename of the file to create
     * @return bool - true if there were no errors, else false.
     */
    bool WriteGENERICNetList( const wxString& aOutFileName );

    /**
     * Function WriteNetListPCBNEW
     * generates a net list file (Format 2 improves ORCAD PCB)
     *
     * @param f = the file to write to
     * @param with_pcbnew if true, then use Pcbnew format (OrcadPcb2 + a list of net),<p>
     *                    else use ORCADPCB2 basic format.
     */
    bool WriteNetListPCBNEW( FILE* f, bool with_pcbnew );

    /**
     * Function WriteNetListCADSTAR
     * generates a netlist file in CADSTAR Format.
     * Header:
     * HEA ..
     * TIM .. 2004 07 29 16 22 17
     * APA .. "Cadstar RINF Output - Version 6.0.2.3"
     * INCH UNI .. 1000.0 in
     * FULL TYP ..
     *
     * List of components:
     * .. ADD_COM X1 "CNT D41612 (48pts CONTOUR TM)"
     * .. ADD_COM U2 "74HCT245D" "74HCT245D"
     *
     * Connections:
     *  .. ADD_TER RR2 * 6 "$ 42"
     * .. B U1 100
     * 6 CA
     *
     * ADD_TER .. U2 * 6 "$ 59"
     * .. B * U7 39
     * U6 17
     * U1 * 122
     *
     * .. ADD_TER P2 * 1 "$ 9"
     * .. B * T3 1
     *U1 * 14
     */
    bool WriteNetListCADSTAR( FILE* f );

    /**
     * Function WriteNetListPspice
     * generates a netlist file in PSPICE format.
     * <p>
     * All graphics text starting by  [.-+] PSpice or [.-+] gnucap
     * are seen as spice directives and put in netlist
     * .-PSpice or .-gnucap put at beginning of the netlist
     * .+PSpice or .-genucap are put at end of the netList
     * @param f = the file to write to
     * @param aUsePrefix = true, adds an 'X' prefix to any reference designator starting
     *                     with "U" or "IC", false to leave reference designator unchanged.
     * @param aUseNetcodeAsNetName = true to use numbers (net codes) as net names.
     *                                false to use net names from schematic.
     */
    bool WriteNetListPspice( FILE* f, bool aUsePrefix, bool aUseNetcodeAsNetName );

    /**
     * Function MakeCommandLine
     * builds up a string that describes a command line for
     * executing a child process. The input and output file names
     * along with any options to the executable are all possibly
     * in the returned string.
     *
     * @param aFormatString holds:
     *   <ul>
     *   <li>the name of the external program
     *   <li>any options needed by that program
     *   <li>formatting sequences, see below.
     *   </ul>
     *
     * @param aTempfile is the name of an input file to the
     *  external program.
     * @param aFinalFile is the name of an output file that
     *  the user expects.
     *
     *  <p> Supported formatting sequences and their meaning:
     *  <ul>
     *  <li> %B => base filename of selected output file, minus
     *       path and extension.
     *  <li> %I => complete filename and path of the temporary
     *       input file.
     *  <li> %O => complete filename and path of the user chosen
     *       output file.
     *  </ul>
     */
    static wxString MakeCommandLine( const wxString& aFormatString,
            const wxString& aTempfile, const wxString& aFinalFile );
};


wxString NETLIST_EXPORT_TOOL::MakeCommandLine( const wxString& aFormatString,
            const wxString& aTempfile, const wxString& aFinalFile )
{
    wxString    ret  = aFormatString;
    wxFileName  in   = aTempfile;
    wxFileName  out  = aFinalFile;

    ret.Replace( wxT( "%B" ), out.GetName().GetData(), true );
    ret.Replace( wxT( "%I" ), in.GetFullPath().GetData(), true );
    ret.Replace( wxT( "%O" ), out.GetFullPath().GetData(), true );

    return ret;
}


bool SCH_EDIT_FRAME::WriteNetListFile( NETLIST_OBJECT_LIST * aConnectedItemsList,
                                       int aFormat, const wxString& aFullFileName,
                                       unsigned aNetlistOptions )
{
    bool        ret = true;
    FILE*       f = NULL;

    NETLIST_EXPORT_TOOL helper( aConnectedItemsList, Prj().SchLibs() );

    bool open_file = (aFormat < NET_TYPE_CUSTOM1) && (aFormat >= 0);

    if( (aFormat == NET_TYPE_PCBNEW) && (aNetlistOptions & NET_PCBNEW_USE_NEW_FORMAT ) )
        open_file = false;

    if( open_file )
    {
        if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
        {
            wxString msg;
            msg.Printf( _( "Failed to create file '%s'" ),
                        GetChars( aFullFileName ) );
            DisplayError( this, msg );
            return false;
        }
    }

    wxBusyCursor Busy;

    switch( aFormat )
    {
    case NET_TYPE_PCBNEW:
        if( (aNetlistOptions & NET_PCBNEW_USE_NEW_FORMAT ) )
            ret = helper.WriteKiCadNetList( aFullFileName );
        else
            ret = helper.WriteNetListPCBNEW( f, true );
        break;

    case NET_TYPE_ORCADPCB2:
        ret = helper.WriteNetListPCBNEW( f, false );
        break;

    case NET_TYPE_CADSTAR:
        ret = helper.WriteNetListCADSTAR( f );
        break;

    case NET_TYPE_SPICE:
        ret = helper.WriteNetListPspice( f, aNetlistOptions & NET_USE_X_PREFIX,
                                         aNetlistOptions & NET_USE_NETCODES_AS_NETNAMES );
        break;

    default:
    {
        wxFileName  tmpFile = aFullFileName;
        tmpFile.SetExt( INTERMEDIATE_NETLIST_EXT );

        ret = helper.WriteGENERICNetList( tmpFile.GetFullPath() );

        if( !ret )
            break;

        // If user provided no plugin command line, return now.
        if( m_netListerCommand.IsEmpty() )
            break;

        // build full command line from user's format string, e.g.:
        // "xsltproc -o %O /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl %I"
        // becomes, after the user selects /tmp/s1.net as the output file from the file dialog:
        // "xsltproc -o /tmp/s1.net /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl /tmp/s1.xml"
        wxString commandLine = NETLIST_EXPORT_TOOL::MakeCommandLine( m_netListerCommand,
                                                                     tmpFile.GetFullPath(),
                                                                     aFullFileName );

        ProcessExecute( commandLine, wxEXEC_SYNC );
        break;
    }
    }

    if( f != NULL )
        fclose( f );

    return ret;
}


/// Comparison routine for sorting by pin numbers.
static bool sortPinsByNum( NETLIST_OBJECT* aPin1, NETLIST_OBJECT* aPin2 )
{
    // return "lhs < rhs"
    return RefDesStringCompare( aPin1->GetPinNumText(), aPin2->GetPinNumText() ) < 0;
}

static bool sortPinsByNumber( LIB_PIN* aPin1, LIB_PIN* aPin2 )
{
    // return "lhs < rhs"
    return RefDesStringCompare( aPin1->GetNumberString(), aPin2->GetNumberString() ) < 0;
}


void NETLIST_EXPORT_TOOL::sprintPinNetName( wxString& aResult,
                                    const wxString& aNetNameFormat, NETLIST_OBJECT* aPin,
                                    bool aUseNetcodeAsNetName )
{
    int netcode = aPin->GetNet();

    // Not wxString::Clear(), which would free memory.  We want the worst
    // case wxString memory to grow to avoid reallocation from within the
    // caller's loop.
    aResult.Empty();

    if( netcode != 0 && aPin->GetConnectionType() == PAD_CONNECT )
    {
        if( aUseNetcodeAsNetName )
        {
            aResult.Printf( wxT("%d"), netcode );
        }
        else
        {
        aResult = aPin->GetNetName();

        if( aResult.IsEmpty() )     // No net name: give a name from net code
            aResult.Printf( aNetNameFormat.GetData(), netcode );
        }
    }
}


SCH_COMPONENT* NETLIST_EXPORT_TOOL::findNextComponent( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref;

    // continue searching from the middle of a linked list (the draw list)
    for(  ; aItem;  aItem = aItem->Next() )
    {
        if( aItem->Type() != SCH_COMPONENT_T )
            continue;

        // found next component
        SCH_COMPONENT* comp = (SCH_COMPONENT*) aItem;

        // Power symbols and other components which have the reference starting
        // with "#" are not included in netlist (pseudo or virtual components)
        ref = comp->GetRef( aSheetPath );
        if( ref[0] == wxChar( '#' ) )
            continue;

        // if( Component->m_FlagControlMulti == 1 )
        //    continue;                                      /* yes */
        // removed because with multiple instances of one schematic
        // (several sheets pointing to 1 screen), this will be erroneously be
        // toggled.

        LIB_PART* part = m_libs->FindLibPart( comp->GetPartName() );
        if( !part )
            continue;

        // If component is a "multi parts per package" type
        if( part->GetUnitCount() > 1 )
        {
            // test if this reference has already been processed, and if so skip
            if( m_ReferencesAlreadyFound.Lookup( ref ) )
                continue;
        }

        // record the usage of this library component entry.
        m_LibParts.insert( part );     // rejects non-unique pointers

        return comp;
    }

    return NULL;
}


SCH_COMPONENT* NETLIST_EXPORT_TOOL::findNextComponentAndCreatePinList( EDA_ITEM*       aItem,
                                                              SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref;

    m_SortedComponentPinList.clear();

    // continue searching from the middle of a linked list (the draw list)
    for(  ; aItem;  aItem = aItem->Next() )
    {
        if( aItem->Type() != SCH_COMPONENT_T )
            continue;

        // found next component
        SCH_COMPONENT* comp = (SCH_COMPONENT*) aItem;

        // Power symbols and other components which have the reference starting
        // with "#" are not included in netlist (pseudo or virtual components)
        ref = comp->GetRef( aSheetPath );

        if( ref[0] == wxChar( '#' ) )
            continue;

        // if( Component->m_FlagControlMulti == 1 )
        //    continue;                                      /* yes */
        // removed because with multiple instances of one schematic
        // (several sheets pointing to 1 screen), this will be erroneously be
        // toggled.

        LIB_PART* part = m_libs->FindLibPart( comp->GetPartName() );

        if( !part )
            continue;

        // If component is a "multi parts per package" type
        if( part->GetUnitCount() > 1 )
        {
            // test if this reference has already been processed, and if so skip
            if( m_ReferencesAlreadyFound.Lookup( ref ) )
                continue;

            // Collect all pins for this reference designator by searching
            // the entire design for other parts with the same reference designator.
            // This is only done once, it would be too expensive otherwise.
            findAllInstancesOfComponent( comp, part, aSheetPath );
        }

        else    // entry->GetUnitCount() <= 1 means one part per package
        {
            LIB_PINS pins;      // constructed once here

            part->GetPins( pins, comp->GetUnitSelection( aSheetPath ), comp->GetConvert() );

            for( size_t i = 0; i < pins.size(); i++ )
            {
                LIB_PIN* pin = pins[i];

                wxASSERT( pin->Type() == LIB_PIN_T );

                addPinToComponentPinList( comp, aSheetPath, pin );
            }
        }

        // Sort pins in m_SortedComponentPinList by pin number
        sort( m_SortedComponentPinList.begin(),
              m_SortedComponentPinList.end(), sortPinsByNum );

        // Remove duplicate Pins in m_SortedComponentPinList
        eraseDuplicatePins( );

        // record the usage of this library component entry.
        m_LibParts.insert( part );     // rejects non-unique pointers

        return comp;
    }

    return NULL;
}


/**
 * Function node
 * is a convenience function that creates a new XNODE with an optional textual child.
 * It also provides some insulation from a possible change in XML library.
 *
 * @param aName is the name to associate with a new node of type wxXML_ELEMENT_NODE.
 * @param aTextualContent is optional, and if given is the text to include in a child
 *   of the returned node, and has type wxXML_TEXT_NODE.
 */
static XNODE* node( const wxString& aName, const wxString& aTextualContent = wxEmptyString )
{
    XNODE* n = new XNODE( wxXML_ELEMENT_NODE, aName );

    if( aTextualContent.Len() > 0 )     // excludes wxEmptyString, the parameter's default value
        n->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, aTextualContent ) );

    return n;
}


XNODE* NETLIST_EXPORT_TOOL::makeGenericDesignHeader()
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


XNODE* NETLIST_EXPORT_TOOL::makeGenericLibraries()
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


XNODE* NETLIST_EXPORT_TOOL::makeGenericLibParts()
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


XNODE* NETLIST_EXPORT_TOOL::makeGenericListOfNets()
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


XNODE* NETLIST_EXPORT_TOOL::makeGenericRoot()
{
    XNODE*      xroot = node( wxT( "export" ) );

    xroot->AddAttribute( wxT( "version" ), wxT( "D" ) );

    // add the "design" header
    xroot->AddChild( makeGenericDesignHeader() );

    xroot->AddChild( makeGenericComponents() );

    xroot->AddChild( makeGenericLibParts() );

    // must follow makeGenericLibParts()
    xroot->AddChild( makeGenericLibraries() );

    xroot->AddChild( makeGenericListOfNets() );

    return xroot;
}


XNODE* NETLIST_EXPORT_TOOL::makeGenericComponents()
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


bool NETLIST_EXPORT_TOOL::WriteKiCadNetList( const wxString& aOutFileName )
{
    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    std::auto_ptr<XNODE>    xroot( makeGenericRoot() );

    try
    {
        FILE_OUTPUTFORMATTER    formatter( aOutFileName );

        xroot->Format( &formatter, 0 );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( NULL, ioe.errorText );
        return false;
    }

    return true;
}

bool NETLIST_EXPORT_TOOL::WriteGENERICNetList( const wxString& aOutFileName )
{
    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // output the XML format netlist.
    wxXmlDocument   xdoc;

    xdoc.SetRoot( makeGenericRoot() );

    return xdoc.Save( aOutFileName, 2 /* indent bug, today was ignored by wxXml lib */ );
}


bool NETLIST_EXPORT_TOOL::WriteNetListPspice( FILE* f, bool aUsePrefix, bool aUseNetcodeAsNetName )
{
    int                 ret = 0;
    int                 nbitems;
    wxString            text;
    wxArrayString       spiceCommandAtBeginFile;
    wxArrayString       spiceCommandAtEndFile;
    wxString            msg;
    wxString            netName;

    #define BUFYPOS_LEN 4
    wxChar              bufnum[BUFYPOS_LEN + 1];
    std::vector<int>    pinSequence;                    // numeric indices into m_SortedComponentPinList
    wxArrayString       stdPinNameArray;                // Array containing Standard Pin Names
    wxString            delimeters = wxT( "{:,; }" );
    wxString            disableStr = wxT( "N" );

    ret |= fprintf( f, "* %s (Spice format) creation date: %s\n\n",
                    NETLIST_HEAD_STRING, TO_UTF8( DateAndTime() ) );

    // Prepare list of nets generation (not used here, but...
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    ret |= fprintf( f, "* To exclude a component from the Spice Netlist add [Spice_Netlist_Enabled] user FIELD set to: N\n" );
    ret |= fprintf( f, "* To reorder the component spice node sequence add [Spice_Node_Sequence] user FIELD and define sequence: 2,1,0\n" );

    // Create text list starting by [.-]pspice , or [.-]gnucap (simulator
    // commands) and create text list starting by [+]pspice , or [+]gnucap
    // (simulator commands)
    bufnum[BUFYPOS_LEN] = 0;
    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst(); sheet; sheet = sheetList.GetNext() )
    {
        for( EDA_ITEM* item = sheet->LastDrawList(); item; item = item->Next() )
        {
            wxChar ident;
            if( item->Type() != SCH_TEXT_T )
                continue;

            SCH_TEXT*   drawText = (SCH_TEXT*) item;

            text = drawText->GetText();

            if( text.IsEmpty() )
                continue;

            ident = text.GetChar( 0 );

            if( ident != '.' && ident != '-' && ident != '+' )
                continue;

            text.Remove( 0, 1 );    // Remove the first char.
            text.Remove( 6 );       // text contains 6 char.
            text.MakeLower();

            if( ( text == wxT( "pspice" ) ) || ( text == wxT( "gnucap" ) ) )
            {
                // Put the Y position as an ascii string, for sort by vertical
                // position, using usual sort string by alphabetic value
                int ypos = drawText->GetPosition().y;

                for( int ii = 0; ii < BUFYPOS_LEN; ii++ )
                {
                    bufnum[BUFYPOS_LEN - 1 - ii] = (ypos & 63) + ' ';
                    ypos >>= 6;
                }

                text = drawText->GetText().AfterFirst( ' ' );

                // First BUFYPOS_LEN char are the Y position.
                msg.Printf( wxT( "%s %s" ), bufnum, text.GetData() );
                if( ident == '+' )
                    spiceCommandAtEndFile.Add( msg );
                else
                    spiceCommandAtBeginFile.Add( msg );
            }
        }
    }

    // Print texts starting by [.-]pspice , ou [.-]gnucap (of course, without
    // the Y position string)
    nbitems = spiceCommandAtBeginFile.GetCount();
    if( nbitems )
    {
        spiceCommandAtBeginFile.Sort();
        for( int ii = 0; ii < nbitems; ii++ )
        {
            spiceCommandAtBeginFile[ii].Remove( 0, BUFYPOS_LEN );
            spiceCommandAtBeginFile[ii].Trim( true );
            spiceCommandAtBeginFile[ii].Trim( false );
            ret |= fprintf( f, "%s\n", TO_UTF8( spiceCommandAtBeginFile[ii] ) );
        }
    }
    ret |= fprintf( f, "\n" );

    //  Create component list

    m_ReferencesAlreadyFound.Clear();

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
        ret |= fprintf( f, "*Sheet Name:%s\n", TO_UTF8( sheet->PathHumanReadable() ) );

        for( EDA_ITEM* item = sheet->LastDrawList();  item;  item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, sheet );
            if( !comp )
                break;

            item = comp;

            // Reset NodeSeqIndex Count:
            pinSequence.clear();

            // Check to see if component should be removed from Spice Netlist:
            SCH_FIELD*  netlistEnabledField = comp->FindField( wxT( "Spice_Netlist_Enabled" ) );
            if( netlistEnabledField )
            {
                wxString netlistEnabled = netlistEnabledField->GetText();

                if( netlistEnabled.IsEmpty() )
                    break;

                if( netlistEnabled.CmpNoCase( disableStr ) == 0 )
                    continue;
            }

            // Check if Alternative Pin Sequence is Available:
            SCH_FIELD*  spiceSeqField = comp->FindField( wxT( "Spice_Node_Sequence" ) );
            if( spiceSeqField )
            {
                // Get String containing the Sequence of Nodes:
                wxString nodeSeqIndexLineStr = spiceSeqField->GetText();

                // Verify Field Exists and is not empty:
                if( nodeSeqIndexLineStr.IsEmpty() )
                    break;

                // Create an Array of Standard Pin Names from part definition:
                stdPinNameArray.Clear();
                for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
                {
                    NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];
                    if( !pin )
                        continue;
                    stdPinNameArray.Add( pin->GetPinNumText() );
                }

                // Get Alt Pin Name Array From User:
                wxStringTokenizer tkz( nodeSeqIndexLineStr, delimeters );
                while( tkz.HasMoreTokens() )
                {
                    wxString    pinIndex = tkz.GetNextToken();
                    int         seq;

                    // Find PinName In Standard List assign Standard List Index to Name:
                    seq = stdPinNameArray.Index(pinIndex);

                    if( seq != wxNOT_FOUND )
                    {
                        pinSequence.push_back( seq );
                    }
                }
            }

            //Get Standard Reference Designator:
            wxString RefName = comp->GetRef( sheet );

            //Conditionally add Prefix only for devices that begin with U or IC:
            if( aUsePrefix )
            {
                if( RefName.StartsWith( wxT( "U" ) ) || RefName.StartsWith( wxT( "IC" ) ) )
                    RefName = wxT( "X" ) + RefName;
            }

            ret |= fprintf( f, "%s ", TO_UTF8( RefName) );

            // Write pin list:
            int activePinIndex = 0;

            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                // Case of Alt Sequence definition with Unused/Invalid Node index:
                // Valid used Node Indexes are in the set
                // {0,1,2,...m_SortedComponentPinList.size()-1}
                if( pinSequence.size() )
                {
                    // All Vector values must be less <= max package size
                    // And Total Vector size should be <= package size
                    if( ( (unsigned) pinSequence[ii] < m_SortedComponentPinList.size() )
                      && ( ii < pinSequence.size() ) )
                    {
                        // Case of Alt Pin Sequence in control good Index:
                        activePinIndex = pinSequence[ii];
                    }
                    else
                    {
                        // Case of Alt Pin Sequence in control Bad Index or not using all
                        // pins for simulation:
                        continue;
                    }
                }
                // Case of Standard Pin Sequence in control:
                else
                {
                    activePinIndex = ii;
                }

                NETLIST_OBJECT* pin = m_SortedComponentPinList[activePinIndex];

                if( !pin )
                    continue;

                sprintPinNetName( netName , wxT( "N-%.6d" ), pin, aUseNetcodeAsNetName );

                //Replace parenthesis with underscore to prevent parse issues with Simulators:
                netName.Replace(wxT("("),wxT("_"));
                netName.Replace(wxT(")"),wxT("_"));

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                ret |= fprintf( f, " %s", TO_UTF8( netName ) );

            }

            // Get Component Value Name:
            wxString CompValue = comp->GetField( VALUE )->GetText();

            // Check if Override Model Name is Provided:
            SCH_FIELD* spiceModelField = comp->FindField( wxT( "spice_model" ) );

            if( spiceModelField )
            {
                // Get Model Name String:
                wxString ModelNameStr = spiceModelField->GetText();

                // Verify Field Exists and is not empty:
                if( !ModelNameStr.IsEmpty() )
                    CompValue = ModelNameStr;
            }

            // Print Component Value:
            ret |= fprintf( f, " %s\t\t",TO_UTF8( CompValue ) );

            // Show Seq Spec on same line as component using line-comment ";":
            for( unsigned i = 0;  i < pinSequence.size();  ++i )
            {
                if( i==0 )
                    ret |= fprintf( f, ";Node Sequence Spec.<" );

                ret |= fprintf( f, "%s", TO_UTF8( stdPinNameArray.Item( pinSequence[i] ) ) );

                if( i < pinSequence.size()-1 )
                    ret |= fprintf( f, "," );
                else
                    ret |= fprintf( f, ">" );
            }

            // Next Netlist line record:
            ret |= fprintf( f, "\n" );
        }
    }

    m_SortedComponentPinList.clear();

    // Print texts starting with [+]pspice or [+]gnucap
    nbitems = spiceCommandAtEndFile.GetCount();

    if( nbitems )
    {
        ret |= fprintf( f, "\n" );
        spiceCommandAtEndFile.Sort();

        for( int ii = 0; ii < nbitems; ii++ )
        {
            spiceCommandAtEndFile[ii].Remove( 0, +BUFYPOS_LEN );
            spiceCommandAtEndFile[ii].Trim( true );
            spiceCommandAtEndFile[ii].Trim( false );
            ret |= fprintf( f, "%s\n", TO_UTF8( spiceCommandAtEndFile[ii] ) );
        }
    }

    ret |= fprintf( f, "\n.end\n" );

    return ret >= 0;
}


bool NETLIST_EXPORT_TOOL::WriteNetListPCBNEW( FILE* f, bool with_pcbnew )
{
    wxString    field;
    wxString    footprint;
    int         ret = 0;        // zero now, OR in the sign bit on error
    wxString    netName;

    std::vector< SCH_REFERENCE > cmpList;

    if( with_pcbnew )
        ret |= fprintf( f, "# %s created  %s\n(\n",
                        NETLIST_HEAD_STRING, TO_UTF8( DateAndTime() ) );
    else
        ret |= fprintf( f, "( { %s created  %s }\n",
                        NETLIST_HEAD_STRING, TO_UTF8( DateAndTime() ) );

    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // Create netlist module section
    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_ITEM* item = path->LastDrawList();  item;  item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, path );

            if( !comp )
                break;

            item = comp;

            // Get the Component FootprintFilter and put the component in
            // cmpList if filter is present
            LIB_PART* part = m_libs->FindLibPart( comp->GetPartName() );

            if( part )
            {
                if( part->GetFootPrints().GetCount() != 0 )    // Put in list
                {
                    cmpList.push_back( SCH_REFERENCE( comp, part, *path ) );
                }
            }

            if( !comp->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = comp->GetField( FOOTPRINT )->GetText();
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                footprint = wxT( "$noname" );

            field = comp->GetRef( path );

            ret |= fprintf( f, " ( %s %s",
                            TO_UTF8( comp->GetPath( path ) ),
                            TO_UTF8( footprint ) );

            ret |= fprintf( f, "  %s", TO_UTF8( field ) );

            field = comp->GetField( VALUE )->GetText();
            field.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( f, " %s", TO_UTF8( field ) );

            if( with_pcbnew )  // Add the lib name for this component
            {
                field = comp->GetPartName();
                field.Replace( wxT( " " ), wxT( "_" ) );
                ret |= fprintf( f, " {Lib=%s}", TO_UTF8( field ) );
            }

            ret |= fprintf( f, "\n" );

            // Write pin list:
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];

                if( !pin )
                    continue;

                sprintPinNetName( netName, wxT( "N-%.6d" ), pin );

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                netName.Replace( wxT( " " ), wxT( "_" ) );

                ret |= fprintf( f, "  ( %4.4s %s )\n", (char*) &pin->m_PinNum,
                                TO_UTF8( netName ) );
            }

            ret |= fprintf( f, " )\n" );
        }
    }

    ret |= fprintf( f, ")\n*\n" );

    m_SortedComponentPinList.clear();

    // Write the allowed footprint list for each component
    if( with_pcbnew && cmpList.size() )
    {
        wxString    ref;

        ret |= fprintf( f, "{ Allowed footprints by component:\n" );

        for( unsigned ii = 0; ii < cmpList.size(); ii++ )
        {
            LIB_PART*      entry = cmpList[ii].GetLibComponent();

            ref = cmpList[ii].GetRef();

            ref.Replace( wxT( " " ), wxT( "_" ) );

            ret |= fprintf( f, "$component %s\n", TO_UTF8( ref ) );

            // Write the footprint list
            for( unsigned jj = 0; jj < entry->GetFootPrints().GetCount(); jj++ )
            {
                ret |= fprintf( f, " %s\n", TO_UTF8( entry->GetFootPrints()[jj] ) );
            }

            ret |= fprintf( f, "$endlist\n" );
        }

        ret |= fprintf( f, "$endfootprintlist\n}\n" );
    }

    if( with_pcbnew )
    {
        ret |= fprintf( f, "{ Pin List by Nets\n" );

        if( !writeGENERICListOfNets( f, *m_masterList ) )
            ret = -1;

        ret |= fprintf( f, "}\n" );
        ret |= fprintf( f, "#End\n" );
    }

    return ret >= 0;
}


bool NETLIST_EXPORT_TOOL::addPinToComponentPinList( SCH_COMPONENT* aComponent,
                                                    SCH_SHEET_PATH* aSheetPath, LIB_PIN* aPin )
{
    // Search the PIN description for Pin in g_NetObjectslist
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
    {
        NETLIST_OBJECT* pin = m_masterList->GetItem( ii );

        if( pin->m_Type != NET_PIN )
            continue;

        if( pin->m_Link != aComponent )
            continue;

        if( pin->m_PinNum != aPin->GetNumber() )
            continue;

        // most expensive test at the end.
        if( pin->m_SheetPath != *aSheetPath )
            continue;

        m_SortedComponentPinList.push_back( pin );

        if( m_SortedComponentPinList.size() >= MAXPIN )
        {
            // Log message for Internal error
            DisplayError( NULL, wxT( "addPinToComponentPinList err: MAXPIN reached" ) );
        }

        return true;  // we're done, we appended.
    }

    return false;
}


void NETLIST_EXPORT_TOOL::eraseDuplicatePins( )
{
    for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
    {
        if( m_SortedComponentPinList[ii] == NULL ) /* already deleted */
            continue;

        /* Search for duplicated pins
         * If found, remove duplicates. The priority is to keep connected pins
         * and remove unconnected
         * - So this allows (for instance when using multi op amps per package
         * - to connect only one op amp to power
         * Because the pin list is sorted by m_PinNum value, duplicated pins
         * are necessary successive in list
         */
        int idxref = ii;

        for( unsigned jj = ii + 1; jj < m_SortedComponentPinList.size(); jj++ )
        {
            if(  m_SortedComponentPinList[jj] == NULL )   // Already removed
                continue;

            // if other pin num, stop search,
            // because all pins having the same number are consecutive in list.
            if( m_SortedComponentPinList[idxref]->m_PinNum != m_SortedComponentPinList[jj]->m_PinNum )
                break;

            if( m_SortedComponentPinList[idxref]->GetConnectionType() == PAD_CONNECT )
            {
                m_SortedComponentPinList[jj]->m_Flag = 1;
                m_SortedComponentPinList[jj] = NULL;
            }
            else /* the reference pin is not connected: remove this pin if the
                  * other pin is connected */
            {
                if( m_SortedComponentPinList[jj]->GetConnectionType() == PAD_CONNECT )
                {
                    m_SortedComponentPinList[idxref]->m_Flag = 1;
                    m_SortedComponentPinList[idxref] = NULL;
                    idxref = jj;
                }
                else    // the 2 pins are not connected: remove the tested pin,
                {       // and continue ...
                    m_SortedComponentPinList[jj]->m_Flag = 1;
                    m_SortedComponentPinList[jj] = NULL;
                }
            }
        }
    }
}


void NETLIST_EXPORT_TOOL::findAllInstancesOfComponent( SCH_COMPONENT*  aComponent,
                                                       LIB_PART*       aEntry,
                                                       SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref = aComponent->GetRef( aSheetPath );
    wxString    ref2;

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
        for( EDA_ITEM* item = sheet->LastDrawList();  item;  item = item->Next() )
        {
            if( item->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT*  comp2 = (SCH_COMPONENT*) item;

            ref2 = comp2->GetRef( sheet );
            if( ref2.CmpNoCase( ref ) != 0 )
                continue;

            int unit2 = comp2->GetUnitSelection( sheet );  // slow

            for( LIB_PIN* pin = aEntry->GetNextPin();  pin;  pin = aEntry->GetNextPin( pin ) )
            {
                wxASSERT( pin->Type() == LIB_PIN_T );

                if( pin->GetUnit() && pin->GetUnit() != unit2 )
                    continue;

                if( pin->GetConvert() && pin->GetConvert() != comp2->GetConvert() )
                    continue;

                // A suitable pin is found: add it to the current list
                addPinToComponentPinList( comp2, sheet, pin );
            }
        }
    }
}


bool NETLIST_EXPORT_TOOL::writeGENERICListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
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


/* Generate CADSTAR net list. */
static wxString StartLine( wxT( "." ) );


bool NETLIST_EXPORT_TOOL::WriteNetListCADSTAR( FILE* f )
{
    int ret = 0;
    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    wxString footprint;
    SCH_SHEET_PATH* sheet;
    EDA_ITEM* DrawList;
    SCH_COMPONENT* component;
    wxString title = wxT( "Eeschema " ) + GetBuildVersion();

    ret |= fprintf( f, "%sHEA\n", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "%sTIM %s\n", TO_UTF8( StartLine ), TO_UTF8( DateAndTime() ) );
    ret |= fprintf( f, "%sAPP ", TO_UTF8( StartLine ) );
    ret |= fprintf( f, "\"%s\"\n", TO_UTF8( title ) );
    ret |= fprintf( f, "\n" );

    // Prepare list of nets generation
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    // Create netlist module section
    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = component = findNextComponentAndCreatePinList( DrawList, sheet );

            if( component == NULL )
                break;

            /*
            doing nothing with footprint
            if( !component->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = component->GetField( FOOTPRINT )->m_Text;
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                footprint = wxT( "$noname" );
            */

            msg = component->GetRef( sheet );
            ret |= fprintf( f, "%s     ", TO_UTF8( StartCmpDesc ) );
            ret |= fprintf( f, "%s", TO_UTF8( msg ) );

            msg = component->GetField( VALUE )->GetText();
            msg.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( f, "     \"%s\"", TO_UTF8( msg ) );
            ret |= fprintf( f, "\n" );
        }
    }

    ret |= fprintf( f, "\n" );

    m_SortedComponentPinList.clear();

    if( ! writeListOfNetsCADSTAR( f ) )
        ret = -1;   // set error

    ret |= fprintf( f, "\n%sEND\n", TO_UTF8( StartLine ) );

    return ret >= 0;
}


bool NETLIST_EXPORT_TOOL::writeListOfNetsCADSTAR( FILE* f )
{
    int ret = 0;
    wxString InitNetDesc  = StartLine + wxT( "ADD_TER" );
    wxString StartNetDesc = StartLine + wxT( "TER" );
    wxString netcodeName, InitNetDescLine;
    unsigned ii;
    int print_ter = 0;
    int NetCode, lastNetCode = -1;
    SCH_COMPONENT* Cmp;
    wxString netName;

    for( ii = 0; ii < m_masterList->size(); ii++ )
    {
        NETLIST_OBJECT* nitem = m_masterList->GetItem( ii );

        // Get the NetName of the current net :
        if( ( NetCode = nitem->GetNet() ) != lastNetCode )
        {
            netName = nitem->GetNetName();
            netcodeName = wxT( "\"" );

            if( !netName.IsEmpty() )
                netcodeName << netName;
            else  // this net has no name: create a default name $<net number>
                netcodeName << wxT( "$" ) << NetCode;

            netcodeName += wxT( "\"" );
            lastNetCode  = NetCode;
            print_ter    = 0;
        }


        if( nitem->m_Type != NET_PIN )
            continue;

        if( nitem->m_Flag != 0 )
            continue;

        Cmp = nitem->GetComponentParent();
        wxString refstr = Cmp->GetRef( &nitem->m_SheetPath );

        if( refstr[0] == '#' )
            continue;  // Power supply symbols.

        switch( print_ter )
        {
        case 0:
            {
                char buf[5];
                wxString str_pinnum;
                strncpy( buf, (char*) &nitem->m_PinNum, 4 );
                buf[4]     = 0;
                str_pinnum = FROM_UTF8( buf );
                InitNetDescLine.Printf( wxT( "\n%s   %s   %.4s     %s" ),
                                        GetChars( InitNetDesc ),
                                        GetChars( refstr ),
                                        GetChars( str_pinnum ),
                                        GetChars( netcodeName ) );
            }
            print_ter++;
            break;

        case 1:
            ret |= fprintf( f, "%s\n", TO_UTF8( InitNetDescLine ) );
            ret |= fprintf( f, "%s       %s   %.4s\n",
                            TO_UTF8( StartNetDesc ),
                            TO_UTF8( refstr ),
                            (char*) &nitem->m_PinNum );
            print_ter++;
            break;

        default:
            ret |= fprintf( f, "            %s   %.4s\n",
                            TO_UTF8( refstr ),
                            (char*) &nitem->m_PinNum );
            break;
        }

        nitem->m_Flag = 1;
    }

    return ret >= 0;
}

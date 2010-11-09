/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2009 jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

/*****************************/
/* Net list generation code. */
/*****************************/


#include "fctsys.h"


#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "kicad_string.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "program.h"
#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "class_library.h"
#include "lib_pin.h"

#include "xnode.h"      // also nests: <wx/xml/xml.h>

#include "build_version.h"

#include <set>

/**
 * @bug - Every place in this file where fprintf() is used and the return
 *        is not checked is a bug.  The fprintf() function can fail and
 *        returns a value less than 0 when it does.
 */


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
 * Class EXPORT_HELP
 * is a private implementation class used in this source file to keep track
 * of and recycle datastructures used in the generation of various exported netlist
 * files.  Since it is private it is not in a header file.
 */
class EXPORT_HELP
{
    NETLIST_OBJECT_LIST m_SortedComponentPinList;

    /// Used for "multi parts per package" components, avoids processing a lib component more than once.
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
     * <li> "netname_sheetnumber" for the usual nets
     * </ul>
     */
    static void sprintPinNetName( wxString* aResult, const wxString& aNetNameFormat, NETLIST_OBJECT* aPin );

    /**
     * Function findNextComponentAndCreatePinList
     * finds a "suitable" component from the DrawList and optionally builds
     * its pin list int m_SortedComponentPinList. The list is sorted by pin num.
     * A suitable component is a "new" real component (power symbols are not
     * considered).
     */
    SCH_COMPONENT* findNextComponentAndCreatPinList( EDA_BaseStruct* aItem, SCH_SHEET_PATH* aSheetPath );

    SCH_COMPONENT* findNextComponent( EDA_BaseStruct* aItem, SCH_SHEET_PATH* aSheetPath );

    /**
     * Function eraseDuplicatePins
     * removes duplicate Pins fromt the pin list, m_SortedComponentPinList.
     * (This is a list of pins found in the whole schematic, for a single
     * component.) These duplicate pins were put in list because some pins (powers... )
     * are found more than one time when we have a multiple parts per package
     * component. For instance, a 74ls00 has 4 parts, and therefore the VCC pin
     * and GND pin appears 4 times in the list.
     * @param aPinList = a NETLIST_OBJECT_LIST that contains the list of pins for a
     * given component.
     * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
     */
    void eraseDuplicatePins( NETLIST_OBJECT_LIST& aPinList );

    /**
     * Function addPintToComponentPinList
     * adds a new pin description to the pin list m_SortedComponentPinList.
     * A.a pin description is a pointer to the corresponding structure
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
    void findAllInstancesOfComponent(  SCH_COMPONENT*  aComponent,
                                              LIB_COMPONENT*  aEntry,
                                              SCH_SHEET_PATH* aSheetPath );

    /**
     * Function writeGENERICListOfNets
     * writes out nets (ranked by Netcode), and elements that are
     * connected as part of that net.
     */
    bool writeGENERICListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );

    /**
     * Function writeListOfNetsCADSTAR
     * writes a net list (ranked by Netcode), and
     * Pins connected to it
     * Format:
     *. ADD_TER RR2 6 "$ 42"
     *. B U1 100
     * 6 CA
     */
    void writeListOfNetsCADSTAR( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );

    /**
     * Function makeGenericRoot
     * builds the entire document tree for the generic export.  This is factored
     * out here so we can write the tree in either S-expression file format
     * or in XML if we put the tree built here into a wxXmlDocument.
     */
    XNODE* makeGenericRoot();

    /**
     * Function makeGenericComponents
     * returns a sub-tree holding all the schematic components.
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
     */
    XNODE* makeGenericLibParts();

    /**
     * Function makeGenericListOfNets
     * fills out an XML node with a list of nets and returns it.
     */
    XNODE* makeGenericListOfNets();

    /**
     * Function makeGenericLibraries
     * fills out an XML node with a list of used libraries and returns it.
     * Must have called makeGenericLibParts() before this function.
     */
    XNODE* makeGenericLibraries();

public:

    /**
     * Function WriteGENERICNetList
     * creates a generic netlist, now in XML.
     * @return bool - true if there were no errors, else false.
     */
    bool WriteGENERICNetList( WinEDA_SchematicFrame* frame, const wxString& aOutFileName );

    /**
     * Function WriteNetListPCBNEW
     * generates a net list file (Format 2 improves ORCAD PCB)
     *
     * @param with_pcbnew if true, then format Pcbnew (OrcadPcb2 + reviews and lists of net),<p>
     *                    else output ORCADPCB2 strict format.
     */
    bool WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f,
                                    bool with_pcbnew );

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
    void WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f );

    /**
     * Function WriteNetListPspice
     * generates a netlist file in PSPICE format.
     * <p>
     * All graphics text commentary by a [.-+] PSpice or [.-+] gnucap
     * Are considered in placing orders in the netlist
     * [.-] Or PSpice gnucap are beginning
     * + + Gnucap and PSpice are ultimately NetList
     * @param use_netnames if true, then nodes are identified by the netname,
     *          else by net number.
     */
    bool WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f,
                                    bool use_netnames );

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


wxString EXPORT_HELP::MakeCommandLine( const wxString& aFormatString,
            const wxString& aTempfile, const wxString& aFinalFile )
{
    wxString    ret  = aFormatString;
    wxFileName  in   = aTempfile;
    wxFileName  out  = aFinalFile;

    ret.Replace( wxT("%B"), out.GetName().GetData(), true );
    ret.Replace( wxT("%I"), in.GetFullPath().GetData(), true );
    ret.Replace( wxT("%O"), out.GetFullPath().GetData(), true );

    return ret;
}


/**
 * Function  WriteNetListFile
 * creates the netlist file. Netlist info must be existing
 * @param aFormat = netlist format (NET_TYPE_PCBNEW ...)
 * @param aFullFileName = full netlist file name
 * @param aUse_netnames = bool. if true, use net names from labels in schematic
 *                              if false, use net numbers (net codes)
 *   bool aUse_netnames is used only for Spice netlist
 * @return true if success.
 */
bool WinEDA_SchematicFrame::WriteNetListFile( int aFormat, const wxString& aFullFileName,
                   bool aUse_netnames )
{
    bool        ret = true;
    FILE*       f = NULL;
    EXPORT_HELP helper;

    if( aFormat < NET_TYPE_CUSTOM1 )
    {
        if( ( f = wxFopen( aFullFileName, wxT( "wt" ) ) ) == NULL )
        {
            wxString msg = _( "Failed to create file " ) + aFullFileName;
            DisplayError( this, msg );
            return false;
        }
    }

    wxBusyCursor Busy;

    switch( aFormat )
    {
    case NET_TYPE_PCBNEW:
        ret = helper.WriteNetListPCBNEW( this, f, true );
        fclose( f );
        break;

    case NET_TYPE_ORCADPCB2:
        ret = helper.WriteNetListPCBNEW( this, f, false );
        fclose( f );
        break;

    case NET_TYPE_CADSTAR:
        helper.WriteNetListCADSTAR( this, f );
        fclose( f );
        break;

    case NET_TYPE_SPICE:
        ret = helper.WriteNetListPspice( this, f, aUse_netnames );
        fclose( f );
        break;

    default:
        {
            wxFileName  tmpFile = aFullFileName;
            tmpFile.SetExt( wxT( "tmp" ) );

            D(printf("tmpFile:'%s'\n", CONV_TO_UTF8( tmpFile.GetFullPath() ) );)

            ret = helper.WriteGENERICNetList( this, tmpFile.GetFullPath() );
            if( !ret )
                break;

            // If user provided no plugin command line, return now.
            if( g_NetListerCommandLine.IsEmpty() )
                break;

            // build full command line from user's format string, e.g.:
            // "xsltproc -o %O /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl %I"
            // becomes, after the user selects /tmp/s1.net as the output file from the file dialog:
            // "xsltproc -o /tmp/s1.net /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl /tmp/s1.tmp"
            wxString commandLine = EXPORT_HELP::MakeCommandLine(
                                        g_NetListerCommandLine,
                                        tmpFile.GetFullPath(),
                                        aFullFileName );

            D(printf("commandLine:'%s'\n", CONV_TO_UTF8( commandLine ) );)

            ProcessExecute( commandLine, wxEXEC_SYNC );

            // ::wxRemoveFile( tmpFile.GetFullPath() );
        }
        break;
    }

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
    return RefDesStringCompare( aPin1->GetNumber(), aPin2->GetNumber() ) < 0;
}


void EXPORT_HELP::sprintPinNetName( wxString* aResult,
                        const wxString& aNetNameFormat, NETLIST_OBJECT* aPin )
{
    int netcode = aPin->GetNet();

    // Not wxString::Clear(), which would free memory.  We want the worst
    // case wxString memory to grow to avoid reallocation from within the
    // caller's loop.
    aResult->Empty();

    if( netcode != 0 && aPin->m_FlagOfConnection == PAD_CONNECT )
    {
        NETLIST_OBJECT* netref = aPin->m_NetNameCandidate;
        if( netref )
            *aResult = netref->m_Label;

        if( !aResult->IsEmpty() )
        {
            // prefix non global label names with the sheet path, to avoid name collisions
            if( netref->m_Type != NET_PINLABEL && netref->m_Type != NET_GLOBLABEL )
            {
                wxString lnet = *aResult;

                *aResult = netref->m_SheetList.PathHumanReadable();

                // If sheet path is too long, use the time stamp name instead
                if( aResult->Length() > 32 )
                    *aResult = netref->m_SheetList.Path();

                *aResult += lnet;
            }
        }
        else
        {
            aResult->Printf( aNetNameFormat.GetData(), netcode );
        }
    }
}


SCH_COMPONENT* EXPORT_HELP::findNextComponent( EDA_BaseStruct* aItem, SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref;

    // continue searching from the middle of a linked list (the draw list)
    for(  ; aItem;  aItem = aItem->Next() )
    {
        if( aItem->Type() != TYPE_SCH_COMPONENT )
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

        LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );
        if( !entry )
            continue;

        // If component is a "multi parts per package" type
        if( entry->GetPartCount() > 1 )
        {
            // test if this reference has already been processed, and if so skip
            if( m_ReferencesAlreadyFound.Lookup( ref ) )
                continue;
        }

        // record the usage of this library component entry.
        m_LibParts.insert( entry );     // rejects non-unique pointers

        return comp;
    }

    return NULL;
}


SCH_COMPONENT* EXPORT_HELP::findNextComponentAndCreatPinList(
        EDA_BaseStruct* aItem, SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref;

    m_SortedComponentPinList.clear();

    // continue searching from the middle of a linked list (the draw list)
    for(  ; aItem;  aItem = aItem->Next() )
    {
        if( aItem->Type() != TYPE_SCH_COMPONENT )
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

        LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );
        if( !entry )
            continue;

        // If component is a "multi parts per package" type
        if( entry->GetPartCount() > 1 )
        {
            // test if this reference has already been processed, and if so skip
            if( m_ReferencesAlreadyFound.Lookup( ref ) )
                continue;

            // Collect all pins for this reference designator by searching
            // the entire design for other parts with the same reference designator.
            // This is only done once, it would be too expensive otherwise.
            findAllInstancesOfComponent( comp, entry, aSheetPath );
        }

        else    // entry->GetPartCount() <= 1 means one part per package
        {
            LIB_PIN_LIST pins;      // constructed once here

            entry->GetPins( pins, comp->GetUnitSelection( aSheetPath ), comp->m_Convert );

            for( size_t i = 0; i < pins.size(); i++ )
            {
                LIB_PIN* pin = pins[i];

                wxASSERT( pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                addPinToComponentPinList( comp, aSheetPath, pin );
            }
        }

        // Sort pins in m_SortedComponentPinList by pin number
        sort( m_SortedComponentPinList.begin(),
              m_SortedComponentPinList.end(), sortPinsByNum );

        // Remove duplicate Pins in m_SortedComponentPinList
        eraseDuplicatePins( m_SortedComponentPinList );

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
 * @param aContent is optional, and if given is the text to include in a child
 *   of the returned node, and has type wxXML_TEXT_NODE.
 */
static XNODE* node( const wxString& aName, const wxString& aTextualContent = wxEmptyString )
{
    XNODE* n = new XNODE( wxXML_ELEMENT_NODE, aName );

    if( aTextualContent.Len() > 0 )     // excludes wxEmptyString, the parameter's default value
        n->AddChild( new XNODE( wxXML_TEXT_NODE, wxEmptyString, aTextualContent ) );

    return n;
}


XNODE* EXPORT_HELP::makeGenericDesignHeader()
{
    XNODE*  xdesign = node( wxT("design") );
    char    date[128];

    DateAndTime( date );

    // the root sheet is a special sheet, call it source
    xdesign->AddChild( node( wxT( "source" ), g_RootSheet->m_AssociatedScreen->m_FileName ) );

    xdesign->AddChild( node( wxT( "date" ), CONV_FROM_UTF8( date )) );

    // which eeschema tool
    xdesign->AddChild( node( wxT( "tool" ), wxGetApp().GetAppName() + wxChar(' ') + GetBuildVersion() ) );

    /*  @todo might do a list of schematic pages

        <page name="">
            <title/>
            <revision/>
            <company/>
            <comments>
                <comment>blah</comment> <!-- comment1 -->
                <comment>blah</comment> <!-- comment2 -->
            </comments>
            <pagesize/>
        </page>
        :

        and a sheet hierarchy report here
        <sheets>
            <sheet name="sheetname1" page="pagenameA">
                <sheet name="sheetname2" page="pagenameB"/>  use recursion to output?
            </sheet>
        </sheets>
    */

    return xdesign;
}


XNODE* EXPORT_HELP::makeGenericLibraries()
{
    XNODE*  xlibs = node( wxT( "libraries" ) );     // auto_ptr

    for( std::set<void*>::iterator it = m_Libraries.begin(); it!=m_Libraries.end();  ++it )
    {
        CMP_LIBRARY*    lib = (CMP_LIBRARY*) *it;
        XNODE*      xlibrary;

        xlibs->AddChild( xlibrary = node( wxT( "library" ) ) );
        xlibrary->AddAttribute( wxT( "logical" ), lib->GetLogicalName() );
        xlibrary->AddChild( node( wxT( "uri" ),  lib->GetFullFileName() ) );

        // @todo: add more fun stuff here
    }

    return xlibs;
}


XNODE* EXPORT_HELP::makeGenericLibParts()
{
    XNODE*      xlibparts = node( wxT( "libparts" ) );   // auto_ptr
    wxString    sLibpart  = wxT( "libpart" );
    wxString    sLib      = wxT( "lib" );
    wxString    sPart     = wxT( "part" );
    wxString    sPins     = wxT( "pins" );
    wxString    sPin      = wxT( "pin" );
    wxString    sNum      = wxT( "num" );
    wxString    sName     = wxT( "name" );
    wxString    sField    = wxT( "field" );
    wxString    sFields   = wxT( "fields" );
    wxString    sDescr    = wxT( "description" );
    wxString    sDocs     = wxT( "docs" );
    wxString    sFprints  = wxT( "footprints" );
    wxString    sFp       = wxT( "fp" );

    LIB_PIN_LIST    pinList;
    LIB_FIELD_LIST  fieldList;

    m_Libraries.clear();

    for( std::set<void*>::iterator it = m_LibParts.begin(); it!=m_LibParts.end();  ++it )
    {
        LIB_COMPONENT*  lcomp = (LIB_COMPONENT*) *it;
        CMP_LIBRARY*    library = lcomp->GetLibrary();

        m_Libraries.insert( library );  // inserts component's library iff unique

        XNODE*      xlibpart;
        xlibparts->AddChild( xlibpart = node( sLibpart ) );
        xlibpart->AddAttribute( sLib, library->GetLogicalName() );
        xlibpart->AddAttribute( sPart, lcomp->GetName()  );

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
            if( !fieldList[i].m_Text.IsEmpty() )
            {
                XNODE*     xfield;
                xfields->AddChild( xfield = node( sField, fieldList[i].m_Text ) );
                xfield->AddAttribute( sName, fieldList[i].m_Name );
            }
        }

        //----- show the pins here ------------------------------------
        pinList.clear();
        lcomp->GetPins( pinList, 0, 0 );

        sort( pinList.begin(), pinList.end(), sortPinsByNumber );

        if( pinList.size() )
        {
            XNODE*     pins;

            xlibpart->AddChild( pins = node( sPins ) );
            for( unsigned i=0; i<pinList.size();  ++i )
            {
                XNODE*     pin;

                pins->AddChild( pin = node( sPin ) );
                pin->AddAttribute( sNum, pinList[i]->GetNumber() );

                // caution: construction work site here, drive slowly
            }
        }
    }

    return xlibparts;
}


XNODE* EXPORT_HELP::makeGenericListOfNets()
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

    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        NETLIST_OBJECT* nitem = g_NetObjectslist[ii];
        SCH_COMPONENT*  comp;

        // New net found, write net id;
        if( ( netCode = nitem->GetNet() ) != lastNetCode )
        {
            sameNetcodeCount = 0;   // item count for this net

            netName.Empty();

            // Find a label for this net, if it exists.
            NETLIST_OBJECT* netref = nitem->m_NetNameCandidate;
            if( netref )
            {
                if( netref->m_Type != NET_PINLABEL && netref->m_Type != NET_GLOBLABEL )
                {
                    // usual net name, prefix it by the sheet path
                    netName = netref->m_SheetList.PathHumanReadable();
                }

                netName += netref->m_Label;
            }

            lastNetCode  = netCode;
        }

        if( nitem->m_Type != NET_PIN )
            continue;

        comp = (SCH_COMPONENT*) nitem->m_Link;

        // Get the reference for the net name and the main parent component
        ref = comp->GetRef( &nitem->m_SheetList );
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


XNODE* EXPORT_HELP::makeGenericRoot()
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


XNODE* EXPORT_HELP::makeGenericComponents()
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
    wxString    sPinNum     = wxT( "num" );
    wxString    sPinNetname = wxT( "netname" );
    wxString    sPinNetcode = wxT( "netcode" );


    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST sheetList;

    // Output is xml, so there is no reason to remove spaces from the field values.
    // And XML element names need not be translated to various languages.

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* schItem = path->LastDrawList();  schItem;  schItem = schItem->Next() )
        {
            SCH_COMPONENT*  comp = findNextComponentAndCreatPinList( schItem, path );
            if( !comp )
                break;  // No component left

            schItem = comp;

            XNODE* xcomp;  // current component being constructed

            // Output the component's elments in order of expected access frequency.
            // This may not always look best, but it will allow faster execution
            // under XSL processing systems which do sequential searching within
            // an element.

            xcomps->AddChild( xcomp = node( sComponent ) );
            xcomp->AddAttribute( sRef, comp->GetRef( path ) );

            xcomp->AddChild( node( sValue, comp->GetField( VALUE )->m_Text ) );

            if( !comp->GetField( FOOTPRINT )->m_Text.IsEmpty() )
                xcomp->AddChild( node( sFootprint, comp->GetField( FOOTPRINT )->m_Text ) );

            if( !comp->GetField( DATASHEET )->m_Text.IsEmpty() )
                xcomp->AddChild( node( sDatasheet, comp->GetField( DATASHEET )->m_Text ) );

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

                    // only output a field if non empty
                    if( !f->m_Text.IsEmpty() )
                    {
                        XNODE*  xfield;
                        xfields->AddChild( xfield = node( sField, f->m_Text ) );
                        xfield->AddAttribute( sName, f->m_Name );
                    }
                }
            }

            XNODE*  xlibsource;
            xcomp->AddChild( xlibsource = node( sLibSource ) );

            // "logical" library name, which is in anticipation of a better search
            // algorithm for parts based on "logical_lib.part" and where logical_lib
            // is merely the library name minus path and extension.
            LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );
            if( entry )
                xlibsource->AddAttribute( sLib, entry->GetLibrary()->GetLogicalName() );
            xlibsource->AddAttribute( sPart, comp->m_ChipName );

            XNODE* xsheetpath;
            xcomp->AddChild( xsheetpath = node( sSheetPath ) );
            xsheetpath->AddAttribute( sNames, path->PathHumanReadable() );
            xsheetpath->AddAttribute( sTStamps, path->Path() );

            timeStamp.Printf( sTSFmt, comp->m_TimeStamp );
            xcomp->AddChild( node( sTStamp, timeStamp ) );

            // Add pins list for this cmponent.
            // Usedful to build netlist which have pads connection inside the footprint description
            // (Spice, OrcadPCB2 ...)
            XNODE* xpinslist;
            xcomp->AddChild( xpinslist = node( sPins ) );
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* Pin = m_SortedComponentPinList[ii];
                if( !Pin )
                    continue;
                XNODE* xpin;
                xpinslist->AddChild( xpin = node( sPin ) );
                wxString text;
                xpin->AddAttribute( sPinNum, Pin->GetPinNumText() );
                sprintPinNetName( &text, wxT( "N-%.6d" ), Pin );
                xpin->AddAttribute( sPinNetname, text );
                text.Printf( wxT( "%d" ), Pin->GetNet() );
                xpin->AddAttribute( sPinNetcode, text );
            }
        }
    }

    return xcomps;
}

#include <wx/wfstream.h>        // wxFFileOutputStream

bool EXPORT_HELP::WriteGENERICNetList( WinEDA_SchematicFrame* frame, const wxString& aOutFileName )
{
#if 0

    // this code seems to work now, for S-expression support.

    bool rc = false;
    wxFFileOutputStream os( aOutFileName, wxT( "wt" ) );

    if( !os.IsOk() )
    {
    L_error:
        wxString msg = _( "Failed to create file " ) + aOutFileName;
        DisplayError( frame, msg );
    }
    else
    {
        XNODE*  xroot = makeGenericRoot();

        try
        {
            STREAM_OUTPUTFORMATTER  outputFormatter( os );
            xroot->Format( &outputFormatter, 0 );
        }
        catch( IO_ERROR ioe )
        {
            delete xroot;
            goto L_error;
        }

        delete xroot;
        rc = true;
    }

    return rc;

#elif 1
    // output the XML format netlist.
    wxXmlDocument   xdoc;

    xdoc.SetRoot( makeGenericRoot() );

    return xdoc.Save( aOutFileName, 2 /* indent bug, today was ignored by wxXml lib */ );

#else   // ouput the well established/old generic net list format which was not XML.

    wxString        field;
    wxString        footprint;
    wxString        netname;
    FILE*           out;
    int             ret = 0;    // OR on each call, test sign bit at very end.

    if( ( out = wxFopen( aOutFileName, wxT( "wt" ) ) ) == NULL )
    {
        wxString msg = _( "Failed to create file " ) + aOutFileName;
        DisplayError( frame, msg );
        return false;
    }

    m_ReferencesAlreadyFound.Clear();

    ret |= fprintf( out, "$BeginNetlist\n" );

    // Create netlist module section
    ret |= fprintf( out, "$BeginComponentList\n" );

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* schItem = path->LastDrawList();  schItem;  schItem = schItem->Next() )
        {
            SCH_COMPONENT*  comp = findNextComponentAndCreatPinList( schItem, path );
            if( !comp )
                break;  // No component left

            schItem = comp;

            footprint.Empty();
            if( !comp->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = comp->GetField( FOOTPRINT )->m_Text;
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }

            ret |= fprintf( out, "\n$BeginComponent\n" );
            ret |= fprintf( out, "TimeStamp=%8.8lX\n", comp->m_TimeStamp );
            ret |= fprintf( out, "Footprint=%s\n", CONV_TO_UTF8( footprint ) );

            field = wxT( "Reference=" ) + comp->GetRef( path ) + wxT( "\n" );
            field.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fputs( CONV_TO_UTF8( field ), out );

            field = comp->GetField( VALUE )->m_Text;
            field.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( out, "Value=%s\n", CONV_TO_UTF8( field ) );

            field = comp->m_ChipName;
            field.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( out, "Libref=%s\n", CONV_TO_UTF8( field ) );

            // Write pin list:
            ret |= fprintf( out, "$BeginPinList\n" );
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* Pin = m_SortedComponentPinList[ii];
                if( !Pin )
                    continue;

                sprintPinNetName( &netname, wxT( "$-%.6d" ), Pin );
                if( netname.IsEmpty() )
                    netname = wxT( "?" );

                ret |= fprintf( out, "%.4s=%s\n", (char*) &Pin->m_PinNum, CONV_TO_UTF8( netname ) );
            }

            ret |= fprintf( out, "$EndPinList\n" );
            ret |= fprintf( out, "$EndComponent\n" );
        }
    }

    ret |= fprintf( out, "$EndComponentList\n" );

    ret |= fprintf( out, "\n$BeginNets\n" );

    if( !writeGENERICListOfNets( out, g_NetObjectslist ) )
        ret = -1;

    ret |= fprintf( out, "$EndNets\n" );

    ret |= fprintf( out, "\n$EndNetlist\n" );
    ret |= fclose( out );

    return ret >= 0;
#endif
}


bool EXPORT_HELP::WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f, bool use_netnames )
{
    int             ret = 0;
    char            Line[1024];
    int             nbitems;
    wxString        text;
    wxArrayString   spiceCommandAtBeginFile;
    wxArrayString   spiceCommandAtEndFile;
    wxString        msg;
    wxString        netName;

#define BUFYPOS_LEN 4
    wxChar          bufnum[BUFYPOS_LEN + 1];

    DateAndTime( Line );

    ret |= fprintf( f, "* %s (Spice format) creation date: %s\n\n",
                    NETLIST_HEAD_STRING, Line );

    // Create text list starting by [.-]pspice , or [.-]gnucap (simulator
    // commands) and create text list starting by [+]pspice , or [+]gnucap
    // (simulator commands)
    bufnum[BUFYPOS_LEN] = 0;
    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst(); sheet; sheet = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* item = sheet->LastDrawList(); item; item = item->Next() )
        {
            wxChar ident;
            if( item->Type() != TYPE_SCH_TEXT )
                continue;

            SCH_TEXT*   drawText = (SCH_TEXT*) item;

            text = drawText->m_Text;
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
                int ypos = drawText->m_Pos.y;
                for( int ii = 0; ii < BUFYPOS_LEN; ii++ )
                {
                    bufnum[BUFYPOS_LEN - 1 - ii] = (ypos & 63) + ' ';
                    ypos >>= 6;
                }

                text = drawText->m_Text.AfterFirst( ' ' );

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
            ret |= fprintf( f, "%s\n", CONV_TO_UTF8( spiceCommandAtBeginFile[ii] ) );
        }
    }
    ret |= fprintf( f, "\n" );

    //  Create component list

    m_ReferencesAlreadyFound.Clear();

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* item = sheet->LastDrawList();  item;  item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatPinList( item, sheet );
            if( !comp )
                break;

            item = comp;

            ret |= fprintf( f, "%s ", CONV_TO_UTF8( comp->GetRef( sheet ) ) );

            // Write pin list:
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];
                if( !pin )
                    continue;

                sprintPinNetName( &netName , wxT( "N-%.6d" ), pin );

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                if( use_netnames )
                    ret |= fprintf( f, " %s", CONV_TO_UTF8( netName ) );

                else    // Use number for net names (net number = 0 for "GND")
                {
                    // NetName = "0" is "GND" net for Spice
                    if( netName == wxT( "0" ) || netName == wxT( "GND" ) )
                        ret |= fprintf( f, " 0" );
                    else
                        ret |= fprintf( f, " %d", pin->GetNet() );
                }
            }

            ret |= fprintf( f, " %s\n",
                    CONV_TO_UTF8( comp->GetField( VALUE )->m_Text ) );
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
            ret |= fprintf( f, "%s\n", CONV_TO_UTF8( spiceCommandAtEndFile[ii] ) );
        }
    }

    ret |= fprintf( f, "\n.end\n" );

    return ret >= 0;
}


bool EXPORT_HELP::WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f, bool with_pcbnew )
{
    wxString    field;
    wxString    footprint;
    char        dateBuf[256];
    int         ret = 0;        // zero now, OR in the sign bit on error
    wxString    netName;

    std::vector<OBJ_CMP_TO_LIST> cmpList;

    DateAndTime( dateBuf );

    if( with_pcbnew )
        ret |= fprintf( f, "# %s created  %s\n(\n", NETLIST_HEAD_STRING, dateBuf );
    else
        ret |= fprintf( f, "( { %s created  %s }\n", NETLIST_HEAD_STRING, dateBuf );

    // Create netlist module section

    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* item = path->LastDrawList();  item;  item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatPinList( item, path );
            if( !comp )
                break;

            item = comp;

            // Get the Component FootprintFilter and put the component in
            // cmpList if filter is present
            LIB_COMPONENT* entry =
                CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );

            if( entry )
            {
                if( entry->GetFootPrints().GetCount() != 0 )    // Put in list
                {
                    cmpList.push_back( OBJ_CMP_TO_LIST() );

                    cmpList.back().m_RootCmp = comp;
                    cmpList.back().SetRef( comp->GetRef( path ) );
                }
            }

            if( !comp->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = comp->GetField( FOOTPRINT )->m_Text;
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                footprint = wxT( "$noname" );

            field = comp->GetRef( path );

            ret |= fprintf( f, " ( %s %s",
                            CONV_TO_UTF8( comp->GetPath( path ) ),
                            CONV_TO_UTF8( footprint ) );

            ret |= fprintf( f, "  %s", CONV_TO_UTF8( field ) );

            field = comp->GetField( VALUE )->m_Text;
            field.Replace( wxT( " " ), wxT( "_" ) );
            ret |= fprintf( f, " %s", CONV_TO_UTF8( field ) );

            if( with_pcbnew )  // Add the lib name for this component
            {
                field = comp->m_ChipName;
                field.Replace( wxT( " " ), wxT( "_" ) );
                ret |= fprintf( f, " {Lib=%s}", CONV_TO_UTF8( field ) );
            }
            ret |= fprintf( f, "\n" );

            // Write pin list:
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];
                if( !pin )
                    continue;

                sprintPinNetName( &netName, wxT( "N-%.6d" ), pin );
                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                netName.Replace( wxT( " " ), wxT( "_" ) );

                ret |= fprintf( f, "  ( %4.4s %s )\n", (char*) &pin->m_PinNum,
                                CONV_TO_UTF8( netName ) );
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
            SCH_COMPONENT* comp = cmpList[ii].m_RootCmp;

            LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );

            ref = cmpList[ii].GetRef();

            ref.Replace( wxT( " " ), wxT( "_" ) );

            ret |= fprintf( f, "$component %s\n", CONV_TO_UTF8( ref ) );

            // Write the footprint list
            for( unsigned jj = 0; jj < entry->GetFootPrints().GetCount(); jj++ )
            {
                ret |= fprintf( f, " %s\n", CONV_TO_UTF8( entry->GetFootPrints()[jj] ) );
            }

            ret |= fprintf( f, "$endlist\n" );
        }

        ret |= fprintf( f, "$endfootprintlist\n}\n" );
    }

    if( with_pcbnew )
    {
        ret |= fprintf( f, "{ Pin List by Nets\n" );

        if( !writeGENERICListOfNets( f, g_NetObjectslist ) )
            ret = -1;

        ret |= fprintf( f, "}\n" );
        ret |= fprintf( f, "#End\n" );
    }

    return ret >= 0;
}


bool EXPORT_HELP::addPinToComponentPinList( SCH_COMPONENT* aComponent,
                                      SCH_SHEET_PATH* aSheetPath, LIB_PIN* aPin )
{
    // Search the PIN description for Pin in g_NetObjectslist
    for( unsigned ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        NETLIST_OBJECT* pin = g_NetObjectslist[ii];

        if( pin->m_Type != NET_PIN )
            continue;

        if( pin->m_Link != aComponent )
            continue;

        if( pin->m_PinNum != aPin->m_PinNum )
            continue;

        // most expensive test at the end.
        if( pin->m_SheetList != *aSheetPath )
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


void EXPORT_HELP::eraseDuplicatePins( NETLIST_OBJECT_LIST& aPinList )
{
    if( aPinList.size() == 0 )  // Trivial case: component with no pin
        return;

    for( unsigned ii = 0; ii < aPinList.size(); ii++ )
    {
        if( aPinList[ii] == NULL ) /* already deleted */
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
        for( unsigned jj = ii + 1; jj < aPinList.size(); jj++ )
        {
            if(  aPinList[jj] == NULL )   // Already removed
                continue;

            // other pin num end of duplicate list.
            if( aPinList[idxref]->m_PinNum != aPinList[jj]->m_PinNum )
                break;

            if( aPinList[idxref]->m_FlagOfConnection == PAD_CONNECT )
                aPinList[jj] = NULL;
            else /* the reference pin is not connected: remove this pin if the
                  * other pin is connected */
            {
                if( aPinList[jj]->m_FlagOfConnection == PAD_CONNECT )
                {
                    aPinList[idxref] = NULL;
                    idxref = jj;
                }
                else    // the 2 pins are not connected: remove the tested pin,
                        // and continue ...
                    aPinList[jj] = NULL;
            }
        }
    }
}


void EXPORT_HELP::findAllInstancesOfComponent( SCH_COMPONENT*  aComponent,
                                         LIB_COMPONENT*  aEntry,
                                         SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref = aComponent->GetRef( aSheetPath );
    wxString    ref2;

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* item = sheet->LastDrawList();  item;  item = item->Next() )
        {
            if( item->Type() != TYPE_SCH_COMPONENT )
                continue;

            SCH_COMPONENT*  comp2 = (SCH_COMPONENT*) item;

            ref2 = comp2->GetRef( sheet );
            if( ref2.CmpNoCase( ref ) != 0 )
                continue;

            int unit2 = comp2->GetUnitSelection( aSheetPath );  // slow

            for( LIB_PIN* pin = aEntry->GetNextPin();  pin;  pin = aEntry->GetNextPin( pin ) )
            {
                wxASSERT( pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                if( pin->GetUnit() && pin->GetUnit() != unit2 )
                    continue;

                if( pin->GetConvert() && pin->GetConvert() != comp2->m_Convert )
                    continue;

                // A suitable pin is found: add it to the current list
                addPinToComponentPinList( comp2, sheet, pin );
            }
        }
    }
}


bool EXPORT_HELP::writeGENERICListOfNets( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
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

        // New net found, write net id;
        if( ( netCode = aObjectsList[ii]->GetNet() ) != lastNetCode )
        {
            sameNetcodeCount = 0;              // Items count for this net
            netName.Empty();

            // Find a label (if exists) for this net.
            NETLIST_OBJECT* netref;
            netref = aObjectsList[ii]->m_NetNameCandidate;
            if( netref )
                netName = netref->m_Label;

            netcodeName.Printf( wxT( "Net %d " ), netCode );
            netcodeName += wxT( "\"" );
            if( !netName.IsEmpty() )
            {
                if( ( netref->m_Type != NET_PINLABEL )
                   && ( netref->m_Type != NET_GLOBLABEL ) )
                {
                    // usual net name, prefix it by the sheet path
                    netcodeName += netref->m_SheetList.PathHumanReadable();
                }
                netcodeName += netName;
            }
            netcodeName += wxT( "\"" );

            // Add the netname without prefix, in cases we need only the
            // "short" netname
            netcodeName += wxT( " \"" ) + netName + wxT( "\"" );
            lastNetCode  = netCode;
        }

        if( aObjectsList[ii]->m_Type != NET_PIN )
            continue;

        comp = (SCH_COMPONENT*) aObjectsList[ii]->m_Link;

        // Get the reference for the net name and the main parent component
        ref = comp->GetRef( &aObjectsList[ii]->m_SheetList );
        if( ref[0] == wxChar( '#' ) )
            continue;                 // Pseudo component (Like Power symbol)

        // Print the pin list for this net, use special handling if
        // 2 or more items are connected:

        // if first item for this net found, defer printing this connection
        // until a second item will is found
        if( ++sameNetcodeCount == 1 )
        {
            snprintf( firstItemInNet, sizeof(firstItemInNet), " %s %.4s\n",
                      CONV_TO_UTF8( ref ),
                      (const char*) &aObjectsList[ii]->m_PinNum );
        }

        // Second item for this net found, print the Net name, and the
        // first item
        if( sameNetcodeCount == 2 )
        {
            ret |= fprintf( f, "%s\n", CONV_TO_UTF8( netcodeName ) );
            ret |= fputs( firstItemInNet, f );
        }

        if( sameNetcodeCount >= 2 )
            ret |= fprintf( f, " %s %.4s\n", CONV_TO_UTF8( ref ),
                     (const char*) &aObjectsList[ii]->m_PinNum );
    }

    return ret >= 0;
}


/* Generate CADSTAR net list. */
static wxString StartLine( wxT( "." ) );


void EXPORT_HELP::WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f )
{
    wxString StartCmpDesc = StartLine + wxT( "ADD_COM" );
    wxString msg;
    wxString footprint;
    char Line[1024];
    SCH_SHEET_PATH* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT* Component;
    wxString Title = wxGetApp().GetAppName() + wxT( " " ) + GetBuildVersion();

    fprintf( f, "%sHEA\n", CONV_TO_UTF8( StartLine ) );
    DateAndTime( Line );
    fprintf( f, "%sTIM %s\n", CONV_TO_UTF8( StartLine ), Line );
    fprintf( f, "%sAPP ", CONV_TO_UTF8( StartLine ) );
    fprintf( f, "\"%s\"\n", CONV_TO_UTF8( Title ) );
    fprintf( f, "\n" );

    // Create netlist module section
    m_ReferencesAlreadyFound.Clear();

    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = Component = findNextComponentAndCreatPinList( DrawList, sheet );
            if( Component == NULL )
                break;

            /*
            doing nothing with footprint
            if( !Component->GetField( FOOTPRINT )->IsVoid() )
            {
                footprint = Component->GetField( FOOTPRINT )->m_Text;
                footprint.Replace( wxT( " " ), wxT( "_" ) );
            }
            else
                footprint = wxT( "$noname" );
            */

            msg = Component->GetRef( sheet );
            fprintf( f, "%s     ", CONV_TO_UTF8( StartCmpDesc ) );
            fprintf( f, "%s", CONV_TO_UTF8( msg ) );

            msg = Component->GetField( VALUE )->m_Text;
            msg.Replace( wxT( " " ), wxT( "_" ) );
            fprintf( f, "     \"%s\"", CONV_TO_UTF8( msg ) );
            fprintf( f, "\n" );
        }
    }

    fprintf( f, "\n" );

    m_SortedComponentPinList.clear();

    writeListOfNetsCADSTAR( f, g_NetObjectslist );

    fprintf( f, "\n%sEND\n", CONV_TO_UTF8( StartLine ) );
}


void EXPORT_HELP::writeListOfNetsCADSTAR( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
{
    wxString InitNetDesc  = StartLine + wxT( "ADD_TER" );
    wxString StartNetDesc = StartLine + wxT( "TER" );
    wxString netcodeName, InitNetDescLine;
    unsigned ii;
    int print_ter = 0;
    int NetCode, lastNetCode = -1;
    SCH_COMPONENT* Cmp;
    wxString NetName;

    for( ii = 0; ii < aObjectsList.size(); ii++ )
        aObjectsList[ii]->m_Flag = 0;

    for( ii = 0; ii < g_NetObjectslist.size(); ii++ )
    {
        // Get the NetName of the current net :
        if( ( NetCode = aObjectsList[ii]->GetNet() ) != lastNetCode )
        {
            NetName.Empty();

            NETLIST_OBJECT* netref;
            netref = aObjectsList[ii]->m_NetNameCandidate;
            if( netref )
                NetName = netref->m_Label;

            netcodeName = wxT( "\"" );
            if( !NetName.IsEmpty() )
            {
                if( ( netref->m_Type != NET_PINLABEL )
                   && ( netref->m_Type != NET_GLOBLABEL ) )
                {
                    // usual net name, prefix it by the sheet path
                    netcodeName +=
                        netref->m_SheetList.PathHumanReadable();
                }
                netcodeName += NetName;
            }
            else  // this net has no name: create a default name $<net number>
                netcodeName << wxT( "$" ) << NetCode;
            netcodeName += wxT( "\"" );
            lastNetCode  = NetCode;
            print_ter    = 0;
        }


        if( aObjectsList[ii]->m_Type != NET_PIN )
            continue;

        if( aObjectsList[ii]->m_Flag != 0 )
            continue;

        Cmp = (SCH_COMPONENT*) aObjectsList[ii]->m_Link;
        wxString refstr = Cmp->GetRef( &(aObjectsList[ii]->m_SheetList) );
        if( refstr[0] == '#' )
            continue;  // Power supply symbols.

        switch( print_ter )
        {
        case 0:
        {
            char buf[5];
            wxString str_pinnum;
            strncpy( buf, (char*) &aObjectsList[ii]->m_PinNum, 4 );
            buf[4]     = 0;
            str_pinnum = CONV_FROM_UTF8( buf );
            InitNetDescLine.Printf( wxT( "\n%s   %s   %.4s     %s" ),
                                   GetChars( InitNetDesc ),
                                   GetChars( refstr ),
                                   GetChars( str_pinnum ),
                                   GetChars( netcodeName ) );
        }
            print_ter++;
            break;

        case 1:
            fprintf( f, "%s\n", CONV_TO_UTF8( InitNetDescLine ) );
            fprintf( f, "%s       %s   %.4s\n",
                     CONV_TO_UTF8( StartNetDesc ),
                     CONV_TO_UTF8( refstr ),
                     (char*) &aObjectsList[ii]->m_PinNum );
            print_ter++;
            break;

        default:
            fprintf( f, "            %s   %.4s\n",
                     CONV_TO_UTF8( refstr ),
                     (char*) &aObjectsList[ii]->m_PinNum );
            break;
        }

        aObjectsList[ii]->m_Flag = 1;

        // Search for redundant pins to avoid generation of the same connection
        // more than once.
        for( unsigned jj = ii + 1; jj < aObjectsList.size(); jj++ )
        {
            if( aObjectsList[jj]->GetNet() != NetCode )
                break;
            if( aObjectsList[jj]->m_Type != NET_PIN )
                continue;
            SCH_COMPONENT* tstcmp =
                (SCH_COMPONENT*) aObjectsList[jj]->m_Link;
            wxString p    = Cmp->GetPath( &( aObjectsList[ii]->m_SheetList ) );
            wxString tstp = tstcmp->GetPath( &( aObjectsList[jj]->m_SheetList ) );
            if( p.Cmp( tstp ) != 0 )
                continue;

            if( aObjectsList[jj]->m_PinNum == aObjectsList[ii]->m_PinNum )
                aObjectsList[jj]->m_Flag = 1;
        }
    }
}

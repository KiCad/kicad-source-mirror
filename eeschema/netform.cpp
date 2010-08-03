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

#include <wx/xml/xml.h>

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
#include "class_pin.h"

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
    bool ret = ( m_set.find( aString ) != m_set.end() );
    if( !ret )
        m_set.insert( aString );

    return ret;
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
    SCH_COMPONENT* findNextComponentAndCreatPinList( EDA_BaseStruct* aItem,
                                                        SCH_SHEET_PATH* aSheetPath );

public:
    bool Write_GENERIC_NetList( WinEDA_SchematicFrame* frame, const wxString& aOutFileName );

    /**
     * Function WriteNetListPCBNEW
     * generates a net list file (Format 2 improves ORCAD PCB)
     * = TRUE if with_pcbnew
     * Format Pcbnew (OrcadPcb2 + reviews and lists of net)
     * = FALSE if with_pcbnew
     * Format ORCADPCB2 strict
     */
    bool WriteNetListPCBNEW( WinEDA_SchematicFrame* frame, FILE* f,
                                    bool with_pcbnew );

    void WriteNetListCADSTAR( WinEDA_SchematicFrame* frame, FILE* f );
    void WriteListOfNetsCADSTAR( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );

    void WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f,
                                    bool use_netnames );

    bool WriteGENERICListOfNetsTxt( FILE* f, NETLIST_OBJECT_LIST& aObjectsList );
    bool WriteGENERICListOfNets( wxXmlNode* aNode, NETLIST_OBJECT_LIST& aObjectsList );

    bool AddPinToComponentPinList( SCH_COMPONENT*  Component,
                                          SCH_SHEET_PATH* sheet,
                                          LIB_PIN*        PinEntry );

    void FindAllInstancesOfComponent(  SCH_COMPONENT*  Component,
                                              LIB_COMPONENT*  aEntry,
                                              SCH_SHEET_PATH* Sheet_in );

    void EraseDuplicatePins( NETLIST_OBJECT_LIST& aPinList );
};


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
    bool            ret = true;
    FILE*           f = NULL;
    EXPORT_HELP    helper;

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
        ret = helper.WriteNetListPCBNEW( this, f, TRUE );
        fclose( f );
        break;

    case NET_TYPE_ORCADPCB2:
        ret = helper.WriteNetListPCBNEW( this, f, FALSE );
        fclose( f );
        break;

    case NET_TYPE_CADSTAR:
        helper.WriteNetListCADSTAR( this, f );
        fclose( f );
        break;

    case NET_TYPE_SPICE:
        helper.WriteNetListPspice( this, f, aUse_netnames );
        fclose( f );
        break;

    default:
        {
            wxFileName  tmpFile = aFullFileName;
            tmpFile.SetExt( wxT( "tmp" ) );

            ret = helper.Write_GENERIC_NetList( this, tmpFile.GetFullPath() );
            if( !ret )
                break;

            // Call the external module (plug in )
            if( g_NetListerCommandLine.IsEmpty() )
                break;

            wxString commandLine;

            if( wxIsAbsolutePath( g_NetListerCommandLine ) )
                commandLine = g_NetListerCommandLine;
            else
                commandLine = FindKicadFile( g_NetListerCommandLine );

            // this is the input file to the plugin
            commandLine += wxT( " " ) + tmpFile.GetFullPath();

            // this is the output file to the plugin
            commandLine += wxT( " " ) + aFullFileName;

            ProcessExecute( commandLine, wxEXEC_SYNC );

            // ::wxRemoveFile( tmpFile.GetFullPath() );
        }
        break;
    }

    return ret;
}


/*
 * Comparison routine for sorting by pin numbers.
 */
static bool sortPinsByNum( NETLIST_OBJECT* Pin1, NETLIST_OBJECT* Pin2 )
{
    // return "lhs < rhs"
    return RefDesStringCompare( Pin1->GetPinNumText(), Pin2->GetPinNumText() ) < 0;
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


SCH_COMPONENT* EXPORT_HELP::findNextComponentAndCreatPinList( EDA_BaseStruct* aItem,
                                                        SCH_SHEET_PATH* aSheetPath )
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
            // test if already visited, and if so skip
            if( m_ReferencesAlreadyFound.Lookup( ref ) )
                continue;

            // Collect all parts and pins for this first occurance of reference.
            // This is only done once, it would be too expensive otherwise.
            FindAllInstancesOfComponent( comp, entry, aSheetPath );
        }

        else    // entry->GetPartCount() <= 1 means one part per package
        {
            LIB_PIN_LIST pins;      // constructed once here

            entry->GetPins( pins, comp->GetUnitSelection( aSheetPath ), comp->m_Convert );

            for( size_t i = 0; i < pins.size(); i++ )
            {
                LIB_PIN* pin = pins[i];

                wxASSERT( pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                AddPinToComponentPinList( comp, aSheetPath, pin );
            }
        }

        // Sort pins in m_SortedComponentPinList by pin number
        sort( m_SortedComponentPinList.begin(),
              m_SortedComponentPinList.end(), sortPinsByNum );

        // Remove duplicate Pins in m_SortedComponentPinList
        EraseDuplicatePins( m_SortedComponentPinList );

        return comp;
    }

    return NULL;
}


/**
 * Function Node
 * is a convenience function that creates a new wxXmlNode with an optional textual child.
 * @param aName is the name to associate with a new node of type wxXML_ELEMENT_NODE.
 * @param aContent is optional, and if given is the text to include in a child
 *   of the returned node, and has type wxXML_TEXT_NODE.
 */
static wxXmlNode* Node( const wxString& aName, const wxString& aTextualContent = wxEmptyString )
{
    wxXmlNode* n = new wxXmlNode( 0, wxXML_ELEMENT_NODE, aName );

    if( aTextualContent.Len() > 0 )     // excludes wxEmptyString, the default textual content
        n->AddChild( new wxXmlNode( 0, wxXML_TEXT_NODE, wxEmptyString, aTextualContent ) );

    return n;
}


/**
 * Function Write_GENERIC_NetList
 * creates a generic netlist, now in XML.
 * @return bool - true if there were no errors, else false.
 */
bool EXPORT_HELP::Write_GENERIC_NetList( WinEDA_SchematicFrame* frame, const wxString& aOutFileName )
{
#if 1
    // output the XML format netlist.
    wxXmlDocument   xdoc;
                                // tree markers or walkers
    wxXmlNode*      xroot;      // root node
    wxXmlNode*      xcomps;     // start of components

    // some strings we need many times, but don't want to construct more
    // than once for performance.  These are used within loops so the
    // enclosing wxString constructor would fire on each loop iteration if
    // they were in a nested scope.

    wxString        timeStamp;
    wxString        logicalLibName;

    // these are actually constructor invocations, not assignments as it appears:
    const wxString  sFields     = wxT( "fields" );
    const wxString  sField      = wxT( "field" );
    const wxString  sComponent  = wxT( "comp" );          // use "part" ?
    const wxString  sName       = wxT( "name" );
    const wxString  sRef        = wxT( "ref" );
    const wxString  sPins       = wxT( "pins" );
    const wxString  sPin        = wxT( "pin" );
    const wxString  sValue      = wxT( "value" );
    const wxString  sSheetPath  = wxT( "sheetpath" );
    const wxString  sFootprint  = wxT( "footprint" );
    const wxString  sDatasheet  = wxT( "datasheet" );
    const wxString  sTStamp     = wxT( "tstamp" );
    const wxString  sTStamps    = wxT( "tstamps" );
    const wxString  sTSFmt      = wxT( "%8.8lX" );        // comp->m_TimeStamp
    const wxString  sLibSource  = wxT( "libsource" );
    const wxString  sLibPart    = wxT( "libpart" );
    const wxString  sLib        = wxT( "lib" );
    const wxString  sPart       = wxT( "part" );
    const wxString  sNames      = wxT( "names" );

    m_ReferencesAlreadyFound.Clear();

    xdoc.SetRoot( xroot = Node( wxT( "netlist" ) ) );
    xroot->AddProperty( wxT( "version" ), wxT( "B" ) );

    xroot->AddChild( xcomps = Node( wxT( "components" ) ) );

    SCH_SHEET_LIST sheetList;

    // output is xml, so there is no reason to remove spaces from the field values.

    for( SCH_SHEET_PATH* path = sheetList.GetFirst();  path;  path = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* schItem = path->LastDrawList();  schItem;  schItem = schItem->Next() )
        {
            SCH_COMPONENT*  comp = findNextComponentAndCreatPinList( schItem, path );
            if( !comp )
                break;  // No component left

            schItem = comp;

            wxXmlNode* xcomp;  // current component being constructed

            // Output the component's elments in order of expected access frequency.
            // This may not always look best, but it will allow faster execution
            // under XSL processing systems which do sequential searching within
            // an element.

            xcomps->AddChild( xcomp = Node( sComponent ) );
            xcomp->AddProperty( sRef, comp->GetRef( path ) );

            xcomp->AddChild( Node( sValue, comp->GetField( VALUE )->m_Text ) );

            if( !comp->GetField( FOOTPRINT )->m_Text.IsEmpty() )
                xcomp->AddChild( Node( sFootprint, comp->GetField( FOOTPRINT )->m_Text ) );

            if( !comp->GetField( DATASHEET )->m_Text.IsEmpty() )
                xcomp->AddChild( Node( sDatasheet, comp->GetField( DATASHEET )->m_Text ) );

            // Export all user defined fields within the component,
            // which start at field index MANDATORY_FIELDS.  Only output the <fields>
            // container element if there are any <field>s.
            if( comp->GetFieldCount() > MANDATORY_FIELDS )
            {
                wxXmlNode* xfields;
                xcomp->AddChild( xfields = Node( sFields ) );

                for( int fldNdx = MANDATORY_FIELDS; fldNdx < comp->GetFieldCount(); ++fldNdx )
                {
                    SCH_FIELD*  f = comp->GetField( fldNdx );

                    // only output a field if non empty
                    if( !f->m_Text.IsEmpty() )
                    {
                        wxXmlNode*  xfield;
                        xfields->AddChild( xfield = Node( sField, f->m_Text ) );
                        xfield->AddProperty( sName, f->m_Name );
                    }
                }
            }

            wxXmlNode*  xlibsource;
            xcomp->AddChild( xlibsource = Node( sLibSource ) );

            // "logical" library name, which is in anticipation of a better search
            // algorithm for parts based on "logical_lib.part" and where logical_lib
            // is merely the library name minus path and extension.
            LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( comp->m_ChipName );
            if( entry )
                xlibsource->AddProperty( sLib, entry->GetLibrary()->GetLogicalName()  );
            xlibsource->AddProperty( sPart, comp->m_ChipName );

            wxXmlNode* xsheetpath;
            xcomp->AddChild( xsheetpath = Node( sSheetPath ) );
            xsheetpath->AddProperty( sTStamps, path->Path() );
            xsheetpath->AddProperty( sNames, path->PathHumanReadable() );

            timeStamp.Printf( sTSFmt, comp->m_TimeStamp );
            xcomp->AddChild( Node( sTStamp, timeStamp ) );
        }
    }

    // @todo generate the nested <libpart> s
    xroot->AddChild( Node( wxT( "libparts" ) ) );

    wxXmlNode* xnets;
    xroot->AddChild( xnets = Node( wxT( "nets" ) ) );
    WriteGENERICListOfNets( xnets, g_NetObjectslist );

    return xdoc.Save( aOutFileName, 2 /* indent bug, today was ignored by wxXml lib */ );

#else   // ouput the well established/old net list format which was not XML.

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

                netname = ReturnPinNetName( Pin, wxT( "$-%.6d" ) );
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

    if( !WriteGENERICListOfNetsTxt( out, g_NetObjectslist ) )
        ret = -1;

    ret |= fprintf( out, "$EndNets\n" );

    ret |= fprintf( out, "\n$EndNetlist\n" );
    ret |= fclose( out );

    return ret >= 0;
#endif
}


/* Routine generation of the netlist file (Format PSPICE)
 * = TRUE if use_netnames
 * Nodes are identified by the netname
 * If the nodes are identified by the netnumber
 *
 * All graphics text commentary by a [.-+] PSpice or [.-+] gnucap
 * Are considered in placing orders in the netlist
 * [.-] Or PSpice gnucap are beginning
 * + + Gnucap and PSpice are ultimately NetList
 */
void EXPORT_HELP::WriteNetListPspice( WinEDA_SchematicFrame* frame, FILE* f, bool use_netnames )
{
    char            Line[1024];
    SCH_SHEET_PATH* sheet;
    EDA_BaseStruct* DrawList;
    SCH_COMPONENT*  Component;
    int             nbitems;
    wxString        text;
    wxArrayString   SpiceCommandAtBeginFile, SpiceCommandAtEndFile;
    wxString        msg;
    wxString        netName;

#define BUFYPOS_LEN 4
    wxChar          bufnum[BUFYPOS_LEN + 1];

    DateAndTime( Line );
    fprintf( f,
             "* %s (Spice format) creation date: %s\n\n",
             NETLIST_HEAD_STRING,
             Line );

    /* Create text list starting by [.-]pspice , or [.-]gnucap (simulator
     * commands) and create text list starting by [+]pspice , or [+]gnucap
     * (simulator commands) */
    bufnum[BUFYPOS_LEN] = 0;
    SCH_SHEET_LIST SheetList;

    for( sheet = SheetList.GetFirst();
        sheet != NULL;
        sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList();
            DrawList != NULL;
            DrawList = DrawList->Next() )
        {
            wxChar ident;
            if( DrawList->Type() != TYPE_SCH_TEXT )
                continue;

            #define DRAWTEXT ( (SCH_TEXT*) DrawList )
            text = DRAWTEXT->m_Text; if( text.IsEmpty() )
                continue;

            ident = text.GetChar( 0 );
            if( ident != '.' && ident != '-' && ident != '+' )
                continue;

            text.Remove( 0, 1 );    // Remove the first char.
            text.Remove( 6 );       // text contains 6 char.
            text.MakeLower();
            if( ( text == wxT( "pspice" ) ) || ( text == wxT( "gnucap" ) ) )
            {
                /* Put the Y position as an ascii string, for sort by vertical
                 * position, using usual sort string by alphabetic value */
                int ypos = DRAWTEXT->m_Pos.y;
                for( int ii = 0; ii < BUFYPOS_LEN; ii++ )
                {
                    bufnum[BUFYPOS_LEN - 1 -
                           ii] = (ypos & 63) + ' '; ypos >>= 6;
                }

                text = DRAWTEXT->m_Text.AfterFirst( ' ' );

                // First BUFYPOS_LEN char are the Y position.
                msg.Printf( wxT( "%s %s" ), bufnum, text.GetData() );
                if( ident == '+' )
                    SpiceCommandAtEndFile.Add( msg );
                else
                    SpiceCommandAtBeginFile.Add( msg );
            }
        }
    }

    /* Print texts starting by [.-]pspice , ou [.-]gnucap (of course, without
     * the Y position string)*/
    nbitems = SpiceCommandAtBeginFile.GetCount();
    if( nbitems )
    {
        SpiceCommandAtBeginFile.Sort();
        for( int ii = 0; ii < nbitems; ii++ )
        {
            SpiceCommandAtBeginFile[ii].Remove( 0, BUFYPOS_LEN );
            SpiceCommandAtBeginFile[ii].Trim( TRUE );
            SpiceCommandAtBeginFile[ii].Trim( FALSE );
            fprintf( f, "%s\n", CONV_TO_UTF8( SpiceCommandAtBeginFile[ii] ) );
        }
    }
    fprintf( f, "\n" );

    //  Create component list

    m_ReferencesAlreadyFound.Clear();

    for( sheet = SheetList.GetFirst(); sheet != NULL; sheet = SheetList.GetNext() )
    {
        for( DrawList = sheet->LastDrawList(); DrawList != NULL; DrawList = DrawList->Next() )
        {
            DrawList = Component = findNextComponentAndCreatPinList( DrawList, sheet );
            if( Component == NULL )
                break;

            fprintf( f, "%s ", CONV_TO_UTF8( Component->GetRef( sheet ) ) );

            // Write pin list:
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* Pin = m_SortedComponentPinList[ii];
                if( !Pin )
                    continue;

                sprintPinNetName( &netName , wxT( "N-%.6d" ), Pin );

                if( netName.IsEmpty() )
                    netName = wxT( "?" );

                if( use_netnames )
                    fprintf( f, " %s", CONV_TO_UTF8( netName ) );
                else    // Use number for net names (with net number = 0 for
                        // "GND"
                {
                    // NetName = "0" is "GND" net for Spice
                    if( netName == wxT( "0" ) || netName == wxT( "GND" ) )
                        fprintf( f, " 0" );
                    else
                        fprintf( f, " %d", Pin->GetNet() );
                }
            }

            fprintf( f, " %s\n",
                    CONV_TO_UTF8( Component->GetField( VALUE )->m_Text ) );
        }
    }

    m_SortedComponentPinList.clear();

    /* Print texts starting by [+]pspice , ou [+]gnucap */
    nbitems = SpiceCommandAtEndFile.GetCount();
    if( nbitems )
    {
        fprintf( f, "\n" );
        SpiceCommandAtEndFile.Sort();
        for( int ii = 0; ii < nbitems; ii++ )
        {
            SpiceCommandAtEndFile[ii].Remove( 0, +BUFYPOS_LEN );
            SpiceCommandAtEndFile[ii].Trim( TRUE );
            SpiceCommandAtEndFile[ii].Trim( FALSE );
            fprintf( f, "%s\n", CONV_TO_UTF8( SpiceCommandAtEndFile[ii] ) );
        }
    }

    fprintf( f, "\n.end\n" );
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
                if( entry->m_FootprintList.GetCount() != 0 )    // Put in list
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
            for( unsigned jj = 0; jj < entry->m_FootprintList.GetCount(); jj++ )
            {
                ret |= fprintf( f, " %s\n",
                        CONV_TO_UTF8( entry->m_FootprintList[jj] ) );
            }

            ret |= fprintf( f, "$endlist\n" );
        }

        ret |= fprintf( f, "$endfootprintlist\n}\n" );
    }

    if( with_pcbnew )
    {
        ret |= fprintf( f, "{ Pin List by Nets\n" );

        if( !WriteGENERICListOfNetsTxt( f, g_NetObjectslist ) )
            ret = -1;

        ret |= fprintf( f, "}\n" );
        ret |= fprintf( f, "#End\n" );
    }

    return ret >= 0;
}


/*
 * Add a new pin description in the pin list m_SortedComponentPinList
 * a pin description is a pointer to the corresponding structure
 * created by BuildNetList() in the table g_NetObjectslist
 */
bool EXPORT_HELP::AddPinToComponentPinList( SCH_COMPONENT* aComponent,
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
            DisplayError( NULL, wxT( "AddPinToComponentPinList err: MAXPIN reached" ) );
        }

        return true;  // we're done, we appended.
    }

    return false;
}


/** Function EraseDuplicatePins
 *  Function to remove duplicate Pins in the TabPin pin list
 *  (This is a list of pins found in the whole schematic, for a given
 * component)
 *  These duplicate pins were put in list because some pins (powers... )
 *  are found more than one time when we have a multiple parts per package
 * component
 *  for instance, a 74ls00 has 4 parts, and therefore the VCC pin and GND pin
 * appears 4 times
 *  in the list.
 * @param aPinList = a NETLIST_OBJECT_LIST that contains the list of pins for a
 * given component.
 * Note: this list *MUST* be sorted by pin number (.m_PinNum member value)
 */
void EXPORT_HELP::EraseDuplicatePins( NETLIST_OBJECT_LIST& aPinList )
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


/**
 * Function FindAllInstancesOfComponent
 * is used for "multiple parts per package" components.
 *
 * Search all instances of Component_in,
 * Calls AddPinToComponentPinList() to and pins founds to the current
 * component pin list
 */
void EXPORT_HELP::FindAllInstancesOfComponent( SCH_COMPONENT*  aComponent,
                                         LIB_COMPONENT*  aEntry,
                                         SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref = aComponent->GetRef( aSheetPath );
    wxString    ref2;

    SCH_SHEET_LIST sheetList;

    for( SCH_SHEET_PATH* sheet = sheetList.GetFirst();  sheet;  sheet = sheetList.GetNext() )
    {
        for( EDA_BaseStruct* schItem = sheet->LastDrawList();  schItem;  schItem = schItem->Next() )
        {
            if( schItem->Type() != TYPE_SCH_COMPONENT )
                continue;

            SCH_COMPONENT*  comp2 = (SCH_COMPONENT*) schItem;

            ref2 = comp2->GetRef( sheet );
            if( ref2.CmpNoCase( ref ) != 0 )
                continue;

            if( aEntry == NULL )
                continue;

            for( LIB_PIN* pin = aEntry->GetNextPin();  pin;  pin = aEntry->GetNextPin( pin ) )
            {
                wxASSERT( pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                if( pin->m_Unit && pin->m_Unit != comp2->GetUnitSelection( aSheetPath ) )
                    continue;

                if( pin->m_Convert && pin->m_Convert != comp2->m_Convert )
                    continue;

                // A suitable pin is found: add it to the current list
                AddPinToComponentPinList( comp2, sheet, pin );
            }
        }
    }
}


/* Written in the file / net list (ranked by Netcode), and elements that are
 * connected
 */
bool EXPORT_HELP::WriteGENERICListOfNetsTxt( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
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


/**
 * Function WriteGENERICListOfNets
 * saves a netlist in xml format.
 */
bool EXPORT_HELP::WriteGENERICListOfNets( wxXmlNode* aNode, NETLIST_OBJECT_LIST& aObjectsList )
{
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

    wxXmlNode*  xnet = 0;
    int         netCode;
    int         lastNetCode = -1;
    int         sameNetcodeCount = 0;

    /*  output:
        <net code="123" name="/cfcard.sch/WAIT#">
            <node ref="R23" pin="1"/>
            <node ref="U18" pin="12"/>
        </net>
    */

    for( unsigned ii = 0; ii < aObjectsList.size(); ii++ )
    {
        SCH_COMPONENT*  comp;

        // New net found, write net id;
        if( ( netCode = aObjectsList[ii]->GetNet() ) != lastNetCode )
        {
            sameNetcodeCount = 0;   // item count for this net

            netName.Empty();

            // Find a label for this net, if it exists.
            NETLIST_OBJECT* netref = aObjectsList[ii]->m_NetNameCandidate;
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

        if( aObjectsList[ii]->m_Type != NET_PIN )
            continue;

        comp = (SCH_COMPONENT*) aObjectsList[ii]->m_Link;

        // Get the reference for the net name and the main parent component
        ref = comp->GetRef( &aObjectsList[ii]->m_SheetList );
        if( ref[0] == wxChar( '#' ) )
            continue;

        if( ++sameNetcodeCount == 1 )
        {
            aNode->AddChild( xnet = Node( sNet ) );
            netCodeTxt.Printf( sFmtd, netCode );
            xnet->AddProperty( sCode, netCodeTxt );
            xnet->AddProperty( sName, netName );
        }

        wxXmlNode*  xnode;
        xnet->AddChild( xnode = Node( sNode ) );
        xnode->AddProperty( sRef, ref );
        xnode->AddProperty( sPin,  LIB_PIN::ReturnPinStringNum( aObjectsList[ii]->m_PinNum ) );
    }

    return true;
}


/* Generate CADSTAR net list. */
wxString StartLine( wxT( "." ) );


/* Routine generation of the netlist file (CADSTAR Format)
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

    WriteListOfNetsCADSTAR( f, g_NetObjectslist );

    fprintf( f, "\n%sEND\n", CONV_TO_UTF8( StartLine ) );
}


/*
 * Written in the file / net list (ranked by Netcode), and
 * Pins connected to it
 * Format:
 *. ADD_TER RR2 6 "$ 42"
 *. B U1 100
 * 6 CA
 */
void EXPORT_HELP::WriteListOfNetsCADSTAR( FILE* f, NETLIST_OBJECT_LIST& aObjectsList )
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

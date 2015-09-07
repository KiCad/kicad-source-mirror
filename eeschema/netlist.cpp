/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file eeschema/netlist.cpp
 */

#include <fctsys.h>
#include <schframe.h>
#include <confirm.h>
#include <netlist_exporter_kicad.h>
#include <kiway.h>

#include <netlist.h>
#include <class_netlist_object.h>
#include <class_library.h>
#include <lib_pin.h>
#include <sch_junction.h>
#include <sch_component.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <algorithm>
#include <invoke_sch_dialog.h>
#include <boost/foreach.hpp>

#define IS_WIRE false
#define IS_BUS true

//Imported function:
int TestDuplicateSheetNames( bool aCreateMarker );


bool SCH_EDIT_FRAME::prepareForNetlist()
{
    SCH_SHEET_LIST sheets;

    sheets.AnnotatePowerSymbols( Prj().SchLibs() );

    // Performs some controls:
    if( CheckAnnotate( NULL, 0 ) )
    {
        // Schematic must be annotated: call Annotate dialog and tell
        // the user why that is.
        InvokeDialogAnnotate( this,  _( "Exporting the netlist requires a "
                                        "completely\nannotated schematic." ) );

        if( CheckAnnotate( NULL, 0 ) )
            return false;
    }

    // Test duplicate sheet names:
    if( TestDuplicateSheetNames( false ) > 0 )
    {
        if( !IsOK( NULL, _( "Error: duplicate sheet names. Continue?" ) ) )
            return false;
    }

    // Cleanup the entire hierarchy
    SCH_SCREENS screens;

    screens.SchematicCleanUp();

    return true;
}


void SCH_EDIT_FRAME::sendNetlist()
{
    NETLIST_OBJECT_LIST* net_atoms = BuildNetListBase();

    NETLIST_EXPORTER_KICAD exporter( net_atoms, Prj().SchLibs() );

    STRING_FORMATTER    formatter;

    // @todo : trim GNL_ALL down to minimum for CVPCB
    exporter.Format( &formatter, GNL_ALL );

    Kiway().ExpressMail( FRAME_CVPCB,
        MAIL_EESCHEMA_NETLIST,
        formatter.GetString(),  // an abbreviated "kicad" (s-expr) netlist
        this
        );
}


bool SCH_EDIT_FRAME::CreateNetlist( int aFormat, const wxString& aFullFileName,
                                    unsigned aNetlistOptions, REPORTER* aReporter )
{
    if( !prepareForNetlist() )
        return false;

    std::auto_ptr<NETLIST_OBJECT_LIST> connectedItemsList( BuildNetListBase() );

    bool success = WriteNetListFile( connectedItemsList.release(), aFormat,
                                     aFullFileName, aNetlistOptions, aReporter );

    return success;
}


//#define NETLIST_DEBUG

NETLIST_OBJECT_LIST::~NETLIST_OBJECT_LIST()
{
    Clear();
}


void NETLIST_OBJECT_LIST::Clear()
{
    NETLIST_OBJECTS::iterator iter;

    for( iter = begin(); iter != end(); iter++ )
    {
        NETLIST_OBJECT* item = *iter;
        delete item;
    }

    clear();
}


void NETLIST_OBJECT_LIST::SortListbyNetcode()
{
    sort( this->begin(), this->end(), NETLIST_OBJECT_LIST::sortItemsbyNetcode );
}


void NETLIST_OBJECT_LIST::SortListbySheet()
{
    sort( this->begin(), this->end(), NETLIST_OBJECT_LIST::sortItemsBySheet );
}


NETLIST_OBJECT_LIST* SCH_EDIT_FRAME::BuildNetListBase()
{
    // I own this list until I return it to the new owner.
    std::auto_ptr<NETLIST_OBJECT_LIST> ret( new NETLIST_OBJECT_LIST() );

    // Creates the flattened sheet list:
    SCH_SHEET_LIST aSheets;

    // Build netlist info
    bool success = ret->BuildNetListInfo( aSheets );

    if( !success )
    {
        SetStatusText( _( "No Objects" ) );
        return ret.release();
    }

    wxString msg = wxString::Format( _( "Net count = %d" ), int( ret->size() ) );

    SetStatusText( msg );

    return ret.release();
}


bool NETLIST_OBJECT_LIST::BuildNetListInfo( SCH_SHEET_LIST& aSheets )
{
    SCH_SHEET_PATH* sheet;

    // Fill list with connected items from the flattened sheet list
    for( sheet = aSheets.GetFirst(); sheet != NULL;
         sheet = aSheets.GetNext() )
    {
        for( SCH_ITEM* item = sheet->LastScreen()->GetDrawItems(); item; item = item->Next() )
        {
            item->GetNetListItem( *this, sheet );
        }
    }

    if( size() == 0 )
        return false;

    // Sort objects by Sheet
    SortListbySheet();

    sheet = &(GetItem( 0 )->m_SheetPath);
    m_lastNetCode = m_lastBusNetCode = 1;

    for( unsigned ii = 0, istart = 0; ii < size(); ii++ )
    {
        NETLIST_OBJECT* net_item = GetItem( ii );

        if( net_item->m_SheetPath != *sheet )   // Sheet change
        {
            sheet  = &(net_item->m_SheetPath);
            istart = ii;
        }

        switch( net_item->m_Type )
        {
        case NET_ITEM_UNSPECIFIED:
            wxMessageBox( wxT( "BuildNetListBase() error" ) );
            break;

        case NET_PIN:
        case NET_PINLABEL:
        case NET_SHEETLABEL:
        case NET_NOCONNECT:
            if( net_item->GetNet() != 0 )
                break;

        case NET_SEGMENT:
            // Test connections point to point type without bus.
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( m_lastNetCode );
                m_lastNetCode++;
            }

            pointToPointConnect( net_item, IS_WIRE, istart );
            break;

        case NET_JUNCTION:
            // Control of the junction outside BUS.
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( m_lastNetCode );
                m_lastNetCode++;
            }

            segmentToPointConnect( net_item, IS_WIRE, istart );

            // Control of the junction, on BUS.
            if( net_item->m_BusNetCode == 0 )
            {
                net_item->m_BusNetCode = m_lastBusNetCode;
                m_lastBusNetCode++;
            }

            segmentToPointConnect( net_item, IS_BUS, istart );
            break;

        case NET_LABEL:
        case NET_HIERLABEL:
        case NET_GLOBLABEL:
            // Test connections type junction without bus.
            if( net_item->GetNet() == 0 )
            {
                net_item->SetNet( m_lastNetCode );
                m_lastNetCode++;
            }

            segmentToPointConnect( net_item, IS_WIRE, istart );
            break;

        case NET_SHEETBUSLABELMEMBER:
            if( net_item->m_BusNetCode != 0 )
                break;

        case NET_BUS:
            // Control type connections point to point mode bus
            if( net_item->m_BusNetCode == 0 )
            {
                net_item->m_BusNetCode = m_lastBusNetCode;
                m_lastBusNetCode++;
            }

            pointToPointConnect( net_item, IS_BUS, istart );
            break;

        case NET_BUSLABELMEMBER:
        case NET_HIERBUSLABELMEMBER:
        case NET_GLOBBUSLABELMEMBER:
            // Control connections similar has on BUS
            if( net_item->GetNet() == 0 )
            {
                net_item->m_BusNetCode = m_lastBusNetCode;
                m_lastBusNetCode++;
            }

            segmentToPointConnect( net_item, IS_BUS, istart );
            break;
        }
    }

#if defined(NETLIST_DEBUG) && defined(DEBUG)
    std::cout << "\n\nafter sheet local\n\n";
    DumpNetTable();
#endif

    // Updating the Bus Labels Netcode connected by Bus
    connectBusLabels();

    // Group objects by label.
    for( unsigned ii = 0; ii < size(); ii++ )
    {
        switch( GetItem( ii )->m_Type )
        {
        case NET_PIN:
        case NET_SHEETLABEL:
        case NET_SEGMENT:
        case NET_JUNCTION:
        case NET_BUS:
        case NET_NOCONNECT:
            break;

        case NET_LABEL:
        case NET_GLOBLABEL:
        case NET_PINLABEL:
        case NET_BUSLABELMEMBER:
        case NET_GLOBBUSLABELMEMBER:
            labelConnect( GetItem( ii ) );
            break;

        case NET_SHEETBUSLABELMEMBER:
        case NET_HIERLABEL:
        case NET_HIERBUSLABELMEMBER:
            break;

        case NET_ITEM_UNSPECIFIED:
            break;
        }
    }

#if defined(NETLIST_DEBUG) && defined(DEBUG)
    std::cout << "\n\nafter sheet global\n\n";
    DumpNetTable();
#endif

    // Connection between hierarchy sheets
    for( unsigned ii = 0; ii < size(); ii++ )
    {
        if( GetItem( ii )->m_Type == NET_SHEETLABEL
            || GetItem( ii )->m_Type == NET_SHEETBUSLABELMEMBER )
            sheetLabelConnect( GetItem( ii ) );
    }

    // Sort objects by NetCode
    SortListbyNetcode();

#if defined(NETLIST_DEBUG) && defined(DEBUG)
    std::cout << "\n\nafter qsort()\n";
    DumpNetTable();
#endif

    // Compress numbers of Netcode having consecutive values.
    int NetCode = 0;
    m_lastNetCode = 0;

    for( unsigned ii = 0; ii < size(); ii++ )
    {
        if( GetItem( ii )->GetNet() != m_lastNetCode )
        {
            NetCode++;
            m_lastNetCode = GetItem( ii )->GetNet();
        }

        GetItem( ii )->SetNet( NetCode );
    }

    // Set the minimal connection info:
    setUnconnectedFlag();

    // find the best label object to give the best net name to each net
    findBestNetNameForEachNet();

    return true;
}

// Helper function to give a priority to sort labels:
// NET_PINLABEL and NET_GLOBLABEL are global labels
// and the priority is hight
static int getPriority( const NETLIST_OBJECT* Objet )
{
    switch( Objet->m_Type )
    {
        case NET_PIN: return 1;
        case NET_LABEL: return 2;
        case NET_HIERLABEL: return 3;
        case NET_PINLABEL: return 4;
        case NET_GLOBLABEL: return 5;
        default: break;
    }

    return 0;
}


/* function evalLabelsPriority used by findBestNetNameForEachNet()
 * evalLabelsPriority calculates the priority of alabel1 and aLabel2
 * return true if alabel1 has a highter priority than aLabel2
 */
static bool evalLabelsPriority( const NETLIST_OBJECT* aLabel1,
                                 const NETLIST_OBJECT* aLabel2 )
{
    int priority1 = getPriority( aLabel1 );
    int priority2 = getPriority( aLabel2 );

    if( priority1 != priority2 )
        return priority1 > priority2;

    // Objects have here the same priority, therefore they have the same type.

    // for global labels, we select the best candidate by alphabetic order
    // because they have no sheetpath as prefix name
    // for other labels, we select them before by sheet deep order
    // because the actual name is /sheetpath/label
    // and for a given path length, by alphabetic order

    if( aLabel1->m_Type == NET_PINLABEL || aLabel1->m_Type == NET_GLOBLABEL )
        return aLabel1->m_Label.Cmp( aLabel2->m_Label ) < 0;

    // not global: names are prefixed by their sheetpath
    // use name defined in higher hierarchical sheet
    // (i.e. shorter path because paths are /<timestamp1>/<timestamp2>/...
    // and timestamp = 8 letters.
    if( aLabel1->m_SheetPath.Path().Length() != aLabel2->m_SheetPath.Path().Length() )
        return aLabel1->m_SheetPath.Path().Length() < aLabel2->m_SheetPath.Path().Length();

    // Sheet paths have the same length: use alphabetic label name order
    // For labels on sheets having an equivalent deep in hierarchy, use
    // alphabetic label name order:
    if( aLabel1->m_Label.Cmp( aLabel2->m_Label ) != 0 )
        return aLabel1->m_Label.Cmp( aLabel2->m_Label ) < 0;

    return aLabel1->m_SheetPath.PathHumanReadable().Cmp(
                aLabel2->m_SheetPath.PathHumanReadable() ) < 0;
}


void NETLIST_OBJECT_LIST::findBestNetNameForEachNet()
{
    int netcode = 0;            // current netcode for tested items
    unsigned idxstart = 0;      // index of the first item of this net
    NETLIST_OBJECT* item;
    NETLIST_OBJECT* candidate;

    // Pass 1: find the best name for labelled nets:
    candidate = NULL;
    for( unsigned ii = 0; ii <= size(); ii++ )
    {
        if( ii == size() ) // last item already tested
            item = NULL;
        else
            item = GetItem( ii );

        if( !item || netcode != item->GetNet() )     // End of net found
        {
            if( candidate )         // One or more labels exists, find the best
            {
                for (unsigned jj = idxstart; jj < ii; jj++ )
                    GetItem( jj )->SetNetNameCandidate( candidate );
            }

            if( item == NULL )
                break;

            netcode = item->GetNet();
            candidate = NULL;
            idxstart = ii;
        }

        switch( item->m_Type )
        {
        case NET_HIERLABEL:
        case NET_LABEL:
        case NET_PINLABEL:
        case NET_GLOBLABEL:
            // A candidate is found: select the better between the previous
            // and this one
            if( candidate == NULL )
                candidate = item;
            else
            {
                if( evalLabelsPriority( item, candidate ) )
                    // item has a highter priority than candidate
                    // so update the best candidate
                    candidate = item;
            }
            break;

        default:
            break;
        }
    }

    // Pass 2: find the best name for not labelled nets:
    // The "default" net name is Net-<<Ref cmp>_Pad<num pad>>
    // (see NETLIST_OBJECT::GetShortNetName())
    // therefore the "best" is the short net name alphabetically classed first
    // (to avoid net names changes when the net is not modified,
    // even if components are moved or deleted and undelete or replaced, as long
    // the reference is kept)

    // Build a list of items with no net names
    NETLIST_OBJECTS    list;   // no ownership of elements being pointed at

    for( unsigned ii = 0; ii < size(); ii++ )
    {
        item = GetItem( ii );
        if( !item->HasNetNameCandidate() )
            list.push_back( item );
    }

    if( list.size() == 0 )
        return;

    idxstart = 0;
    candidate = NULL;
    netcode = list[0]->GetNet();

    for( unsigned ii = 0; ii <= list.size(); ii++ )
    {
        if( ii < list.size() )
            item = list[ii];
        else
            item = NULL;

        if( !item || netcode != item->GetNet() )     // End of net found
        {
            if( candidate )
            {
                for (unsigned jj = idxstart; jj < ii; jj++ )
                {
                    NETLIST_OBJECT* obj = list[jj];
                    obj->SetNetNameCandidate( candidate );
                }
            }

            if( !item )
                break;

            netcode = item->GetNet();
            candidate = NULL;
            idxstart = ii;
        }

        // Examine all pins of the net to find the best candidate,
        // i.e. the first net name candidate, by alphabetic order
        // the net names are names bu_ilt by GetShortNetName
        // (Net-<{reference}-Pad{pad number}> like Net-<U3-Pad5>
        // Not named nets do not have usually a lot of members.
        // Many have only 2 members(a pad and a non connection symbol)
        if( item->m_Type == NET_PIN )
        {
            // A candidate is found, however components which are not in
            // netlist are not candidate because some have their reference
            // changed each time the netlist is built (power components)
            // and anyway obviously they are not a good candidate
            SCH_COMPONENT* link = item->GetComponentParent();
            if( link && link->IsInNetlist() )
            {
                // select the better between the previous and this one
                item->SetNetNameCandidate( item );  // Needed to calculate GetShortNetName

                if( candidate == NULL )
                    candidate = item;
                else
                {
                    if( item->GetShortNetName().Cmp( candidate->GetShortNetName() ) < 0 )
                        candidate = item;
                }
            }
        }
    }
}


void NETLIST_OBJECT_LIST::sheetLabelConnect( NETLIST_OBJECT* SheetLabel )
{
    if( SheetLabel->GetNet() == 0 )
        return;

    for( unsigned ii = 0; ii < size(); ii++ )
    {
        NETLIST_OBJECT* ObjetNet = GetItem( ii );

        if( ObjetNet->m_SheetPath != SheetLabel->m_SheetPathInclude )
            continue;  //use SheetInclude, not the sheet!!

        if( (ObjetNet->m_Type != NET_HIERLABEL ) && (ObjetNet->m_Type != NET_HIERBUSLABELMEMBER ) )
            continue;

        if( ObjetNet->GetNet() == SheetLabel->GetNet() )
            continue;  //already connected.

        if( ObjetNet->m_Label.CmpNoCase( SheetLabel->m_Label ) != 0 )
            continue;  //different names.

        // Propagate Netcode having all the objects of the same Netcode.
        if( ObjetNet->GetNet() )
            propageNetCode( ObjetNet->GetNet(), SheetLabel->GetNet(), IS_WIRE );
        else
            ObjetNet->SetNet( SheetLabel->GetNet() );
    }
}


void NETLIST_OBJECT_LIST::connectBusLabels()
{
    for( unsigned ii = 0; ii < size(); ii++ )
    {
        NETLIST_OBJECT* Label = GetItem( ii );

        if(  (Label->m_Type == NET_SHEETBUSLABELMEMBER)
          || (Label->m_Type == NET_BUSLABELMEMBER)
          || (Label->m_Type == NET_HIERBUSLABELMEMBER) )
        {
            if( Label->GetNet() == 0 )
            {
                Label->SetNet( m_lastNetCode );
                m_lastNetCode++;
            }

            for( unsigned jj = ii + 1; jj < size(); jj++ )
            {
                NETLIST_OBJECT* LabelInTst =  GetItem( jj );
                if( (LabelInTst->m_Type == NET_SHEETBUSLABELMEMBER)
                   || (LabelInTst->m_Type == NET_BUSLABELMEMBER)
                   || (LabelInTst->m_Type == NET_HIERBUSLABELMEMBER) )
                {
                    if( LabelInTst->m_BusNetCode != Label->m_BusNetCode )
                        continue;

                    if( LabelInTst->m_Member != Label->m_Member )
                        continue;

                    if( LabelInTst->GetNet() == 0 )
                        LabelInTst->SetNet( Label->GetNet() );
                    else
                        propageNetCode( LabelInTst->GetNet(), Label->GetNet(), IS_WIRE );
                }
            }
        }
    }
}


void NETLIST_OBJECT_LIST::propageNetCode( int aOldNetCode, int aNewNetCode, bool aIsBus )
{
    if( aOldNetCode == aNewNetCode )
        return;

    if( aIsBus == false )    // Propagate NetCode
    {
        for( unsigned jj = 0; jj < size(); jj++ )
        {
            NETLIST_OBJECT* object = GetItem( jj );

            if( object->GetNet() == aOldNetCode )
                object->SetNet( aNewNetCode );
        }
    }
    else               // Propagate BusNetCode
    {
        for( unsigned jj = 0; jj < size(); jj++ )
        {
            NETLIST_OBJECT* object = GetItem( jj );

            if( object->m_BusNetCode == aOldNetCode )
                object->m_BusNetCode = aNewNetCode;
        }
    }
}


void NETLIST_OBJECT_LIST::pointToPointConnect( NETLIST_OBJECT* aRef, bool aIsBus,
                                               int start )
{
    int netCode;

    if( aIsBus == false )    // Objects other than BUS and BUSLABELS
    {
        netCode = aRef->GetNet();

        for( unsigned i = start; i < size(); i++ )
        {
            NETLIST_OBJECT* item = GetItem( i );

            if( item->m_SheetPath != aRef->m_SheetPath )  //used to be > (why?)
                continue;

            switch( item->m_Type )
            {
            case NET_SEGMENT:
            case NET_PIN:
            case NET_LABEL:
            case NET_HIERLABEL:
            case NET_GLOBLABEL:
            case NET_SHEETLABEL:
            case NET_PINLABEL:
            case NET_JUNCTION:
            case NET_NOCONNECT:
                if( aRef->m_Start == item->m_Start
                    || aRef->m_Start == item->m_End
                    || aRef->m_End   == item->m_Start
                    || aRef->m_End   == item->m_End )
                {
                    if( item->GetNet() == 0 )
                        item->SetNet( netCode );
                    else
                        propageNetCode( item->GetNet(), netCode, IS_WIRE );
                }
                break;

            case NET_BUS:
            case NET_BUSLABELMEMBER:
            case NET_SHEETBUSLABELMEMBER:
            case NET_HIERBUSLABELMEMBER:
            case NET_GLOBBUSLABELMEMBER:
            case NET_ITEM_UNSPECIFIED:
                break;
            }
        }
    }
    else    // Object type BUS, BUSLABELS, and junctions.
    {
        netCode = aRef->m_BusNetCode;

        for( unsigned i = start; i < size(); i++ )
        {
            NETLIST_OBJECT* item = GetItem( i );

            if( item->m_SheetPath != aRef->m_SheetPath )
                continue;

            switch( item->m_Type )
            {
            case NET_ITEM_UNSPECIFIED:
            case NET_SEGMENT:
            case NET_PIN:
            case NET_LABEL:
            case NET_HIERLABEL:
            case NET_GLOBLABEL:
            case NET_SHEETLABEL:
            case NET_PINLABEL:
            case NET_NOCONNECT:
                break;

            case NET_BUS:
            case NET_BUSLABELMEMBER:
            case NET_SHEETBUSLABELMEMBER:
            case NET_HIERBUSLABELMEMBER:
            case NET_GLOBBUSLABELMEMBER:
            case NET_JUNCTION:
                if(  aRef->m_Start == item->m_Start
                  || aRef->m_Start == item->m_End
                  || aRef->m_End   == item->m_Start
                  || aRef->m_End   == item->m_End )
                {
                    if( item->m_BusNetCode == 0 )
                        item->m_BusNetCode = netCode;
                    else
                        propageNetCode( item->m_BusNetCode, netCode, IS_BUS );
                }
                break;
            }
        }
    }
}


void NETLIST_OBJECT_LIST::segmentToPointConnect( NETLIST_OBJECT* aJonction,
                                                bool aIsBus, int aIdxStart )
{
    for( unsigned i = aIdxStart; i < size(); i++ )
    {
        NETLIST_OBJECT* segment = GetItem( i );

        // if different sheets, obviously no physical connection between elements.
        if( segment->m_SheetPath != aJonction->m_SheetPath )
            continue;

        if( aIsBus == IS_WIRE )
        {
            if( segment->m_Type != NET_SEGMENT )
                continue;
        }
        else
        {
            if( segment->m_Type != NET_BUS )
                continue;
        }

        if( IsPointOnSegment( segment->m_Start, segment->m_End, aJonction->m_Start ) )
        {
            // Propagation Netcode has all the objects of the same Netcode.
            if( aIsBus == IS_WIRE )
            {
                if( segment->GetNet() )
                    propageNetCode( segment->GetNet(), aJonction->GetNet(), aIsBus );
                else
                    segment->SetNet( aJonction->GetNet() );
            }
            else
            {
                if( segment->m_BusNetCode )
                    propageNetCode( segment->m_BusNetCode, aJonction->m_BusNetCode, aIsBus );
                else
                    segment->m_BusNetCode = aJonction->m_BusNetCode;
            }
        }
    }
}


void NETLIST_OBJECT_LIST::labelConnect( NETLIST_OBJECT* aLabelRef )
{
    if( aLabelRef->GetNet() == 0 )
        return;

    for( unsigned i = 0; i < size(); i++ )
    {
        NETLIST_OBJECT* item = GetItem( i );

        if( item->GetNet() == aLabelRef->GetNet() )
            continue;

        if( item->m_SheetPath != aLabelRef->m_SheetPath )
        {
            if( item->m_Type != NET_PINLABEL && item->m_Type != NET_GLOBLABEL
                && item->m_Type != NET_GLOBBUSLABELMEMBER )
                continue;

            if( (item->m_Type == NET_GLOBLABEL
                 || item->m_Type == NET_GLOBBUSLABELMEMBER)
               && item->m_Type != aLabelRef->m_Type )
                //global labels only connect other global labels.
                continue;
        }

        // NET_HIERLABEL are used to connect sheets.
        // NET_LABEL are local to a sheet
        // NET_GLOBLABEL are global.
        // NET_PINLABEL is a kind of global label (generated by a power pin invisible)
        if( item->IsLabelType() )
        {
            if( item->m_Label.CmpNoCase( aLabelRef->m_Label ) != 0 )
                continue;

            if( item->GetNet() )
                propageNetCode( item->GetNet(), aLabelRef->GetNet(), IS_WIRE );
            else
                item->SetNet( aLabelRef->GetNet() );
        }
    }
}


void NETLIST_OBJECT_LIST::setUnconnectedFlag()
{
    NETLIST_OBJECT* NetItemRef;
    unsigned NetStart, NetEnd;
    NET_CONNECTION_T StateFlag;

    NetStart  = NetEnd = 0;
    StateFlag = UNCONNECTED;
    for( unsigned ii = 0; ii < size(); ii++ )
    {
        NetItemRef = GetItem( ii );
        if( NetItemRef->m_Type == NET_NOCONNECT && StateFlag != PAD_CONNECT )
            StateFlag = NOCONNECT_SYMBOL_PRESENT;

        // Analysis of current net.
        unsigned idxtoTest = ii + 1;

        if( ( idxtoTest >= size() )
           || ( NetItemRef->GetNet() != GetItem( idxtoTest )->GetNet() ) )
        {
            // Net analysis to update m_ConnectionType
            NetEnd = idxtoTest;

            /* set m_ConnectionType member to StateFlag for all items of
             * this net: */
            for( unsigned kk = NetStart; kk < NetEnd; kk++ )
                GetItem( kk )->m_ConnectionType = StateFlag;

            if( idxtoTest >= size() )
                return;

            // Start Analysis next Net
            StateFlag = UNCONNECTED;
            NetStart  = idxtoTest;
            continue;
        }

        /* test the current item: if this is a pin and if the reference item
         * is also a pin, then 2 pins are connected, so set StateFlag to
         * PAD_CONNECT (can be already done)  Of course, if the current
         * item is a no connect symbol, set StateFlag to
         * NOCONNECT_SYMBOL_PRESENT to inhibit error diags. However if
         * StateFlag is already set to PAD_CONNECT this state is kept (the
         * no connect symbol was surely an error and an ERC will report this)
         */
       for( ; ; idxtoTest++ )
        {
            if( ( idxtoTest >= size() )
               || ( NetItemRef->GetNet() != GetItem( idxtoTest )->GetNet() ) )
                break;

            switch( GetItem( idxtoTest )->m_Type )
            {
            case NET_ITEM_UNSPECIFIED:
                wxMessageBox( wxT( "BuildNetListBase() error" ) );
                break;

            case NET_SEGMENT:
            case NET_LABEL:
            case NET_HIERLABEL:
            case NET_GLOBLABEL:
            case NET_SHEETLABEL:
            case NET_PINLABEL:
            case NET_BUS:
            case NET_BUSLABELMEMBER:
            case NET_SHEETBUSLABELMEMBER:
            case NET_HIERBUSLABELMEMBER:
            case NET_GLOBBUSLABELMEMBER:
            case NET_JUNCTION:
                break;

            case NET_PIN:
                if( NetItemRef->m_Type == NET_PIN )
                    StateFlag = PAD_CONNECT;

                break;

            case NET_NOCONNECT:
                if( StateFlag != PAD_CONNECT )
                    StateFlag = NOCONNECT_SYMBOL_PRESENT;

                break;
            }
        }
    }
}

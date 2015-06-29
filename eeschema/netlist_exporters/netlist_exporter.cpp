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


/**
 * @file netlist_exporter.cpp
 */

#include <fctsys.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <pgm_base.h>

#include <sch_reference_list.h>
#include <class_netlist_object.h>
#include <class_library.h>
#include <lib_pin.h>
#include <sch_component.h>
#include <sch_text.h>
#include <sch_sheet.h>

#include <netlist.h>
#include <netlist_exporter.h>



wxString NETLIST_EXPORTER::MakeCommandLine( const wxString& aFormatString,
            const wxString& aTempfile, const wxString& aFinalFile, const wxString& aProjectPath )
{
    // Expand format symbols in the command line:
    // %B => base filename of selected output file, minus path and extension.
    // %P => project directory name, without trailing '/' or '\'.
    // %I => full filename of the input file (the intermediate net file).
    // %O => complete filename and path (but without extension) of the user chosen output file.

    wxString    ret  = aFormatString;
    wxFileName  in   = aTempfile;
    wxFileName  out  = aFinalFile;

    ret.Replace( wxT( "%P" ), aProjectPath.GetData(), true );
    ret.Replace( wxT( "%B" ), out.GetName().GetData(), true );
    ret.Replace( wxT( "%I" ), in.GetFullPath().GetData(), true );
    ret.Replace( wxT( "%O" ), out.GetFullPath().GetData(), true );

    return ret;
}


void NETLIST_EXPORTER::sprintPinNetName( wxString& aResult,
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


SCH_COMPONENT* NETLIST_EXPORTER::findNextComponent( EDA_ITEM* aItem, SCH_SHEET_PATH* aSheetPath )
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


/// Comparison routine for sorting by pin numbers.
static bool sortPinsByNum( NETLIST_OBJECT* aPin1, NETLIST_OBJECT* aPin2 )
{
    // return "lhs < rhs"
    return RefDesStringCompare( aPin1->GetPinNumText(), aPin2->GetPinNumText() ) < 0;
}


SCH_COMPONENT* NETLIST_EXPORTER::findNextComponentAndCreatePinList( EDA_ITEM*       aItem,
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

bool NETLIST_EXPORTER::addPinToComponentPinList( SCH_COMPONENT* aComponent,
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


void NETLIST_EXPORTER::eraseDuplicatePins()
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


void NETLIST_EXPORTER::findAllInstancesOfComponent( SCH_COMPONENT*  aComponent,
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


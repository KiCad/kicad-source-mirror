/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 jp.charras at wanadoo.fr
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <netlist_exporter.h>

#include <confirm.h>
#include <fctsys.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <refdes_utils.h>

#include <class_library.h>
#include <netlist.h>
#include <sch_reference_list.h>


wxString NETLIST_EXPORTER::MakeCommandLine( const wxString& aFormatString,
            const wxString& aNetlistFile, const wxString& aFinalFile, const wxString& aProjectPath )
{
    // Expand format symbols in the command line:
    // %B => base filename of selected output file, minus path and extension.
    // %P => project directory name, without trailing '/' or '\'.
    // %I => full filename of the input file (the intermediate net file).
    // %O => complete filename and path (but without extension) of the user chosen output file.

    wxString   ret  = aFormatString;
    wxFileName in   = aNetlistFile;
    wxFileName out  = aFinalFile;
    wxString str_out  = out.GetFullPath();

    ret.Replace( "%P", aProjectPath, true );
    ret.Replace( "%B", out.GetName(), true );
    ret.Replace( "%I", in.GetFullPath(), true );

#ifdef __WINDOWS__
    // A ugly hack to run xsltproc that has a serious bug on Window since a long time:
    // the filename given after -o option (output filename) cannot use '\' in filename
    // so replace if by '/' if possible (I mean if the filename does not start by "\\"
    // that is a filename on a Windows server)

    if( !str_out.StartsWith( "\\\\" ) )
        str_out.Replace( "\\", "/" );
#endif

    ret.Replace( "%O", str_out, true );

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

    if( netcode != 0 && aPin->GetConnectionType() == NET_CONNECTION::PAD_CONNECT )
    {
        if( aUseNetcodeAsNetName )
        {
            aResult.Printf( "%d", netcode );
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

    if( aItem->Type() != SCH_COMPONENT_T )
        return nullptr;

    // found next component
    SCH_COMPONENT* comp = (SCH_COMPONENT*) aItem;

    // Power symbols and other components which have the reference starting
    // with "#" are not included in netlist (pseudo or virtual components)
    ref = comp->GetRef( aSheetPath );

    if( ref[0] == wxChar( '#' ) )
        return nullptr;

    // if( Component->m_FlagControlMulti == 1 )
    //    continue;                                      /* yes */
    // removed because with multiple instances of one schematic
    // (several sheets pointing to 1 screen), this will be erroneously be
    // toggled.

    if( !comp->GetPartRef() )
        return nullptr;

    // If component is a "multi parts per package" type
    if( comp->GetPartRef()->GetUnitCount() > 1 )
    {
        // test if this reference has already been processed, and if so skip
        if( m_ReferencesAlreadyFound.Lookup( ref ) )
            return nullptr;
    }

    // record the usage of this library component entry.
    m_LibParts.insert( comp->GetPartRef().get() ); // rejects non-unique pointers

    return comp;
}


/// Comparison routine for sorting by pin numbers.
static bool sortPinsByNum( NETLIST_OBJECT* aPin1, NETLIST_OBJECT* aPin2 )
{
    // return "lhs < rhs"
    return UTIL::RefDesStringCompare( aPin1->GetPinNumText(), aPin2->GetPinNumText() ) < 0;
}


void NETLIST_EXPORTER::CreatePinList( SCH_COMPONENT* comp, SCH_SHEET_PATH* aSheetPath )
{
    wxString ref( comp->GetRef( aSheetPath ) );

    // Power symbols and other components which have the reference starting
    // with "#" are not included in netlist (pseudo or virtual components)

    if( ref[0] == wxChar( '#' ) )
        return;

    // if( Component->m_FlagControlMulti == 1 )
    //    continue;                                      /* yes */
    // removed because with multiple instances of one schematic
    // (several sheets pointing to 1 screen), this will be erroneously be
    // toggled.

    if( !comp->GetPartRef() )
        return;

    m_SortedComponentPinList.clear();

    // If component is a "multi parts per package" type
    if( comp->GetPartRef()->GetUnitCount() > 1 )
    {
        // test if this reference has already been processed, and if so skip
        if( m_ReferencesAlreadyFound.Lookup( ref ) )
            return;

        // Collect all pins for this reference designator by searching
        // the entire design for other parts with the same reference designator.
        // This is only done once, it would be too expensive otherwise.
        findAllUnitsOfComponent( comp, comp->GetPartRef().get(), aSheetPath );
    }

    else // entry->GetUnitCount() <= 1 means one part per package
    {
        LIB_PINS pins; // constructed once here

        comp->GetPartRef()->GetPins(
                pins, comp->GetUnitSelection( aSheetPath ), comp->GetConvert() );

        for( size_t i = 0; i < pins.size(); i++ )
        {
            LIB_PIN* pin = pins[i];

            wxASSERT( pin->Type() == LIB_PIN_T );

            addPinToComponentPinList( comp, aSheetPath, pin );
        }
    }

    // Sort pins in m_SortedComponentPinList by pin number
    sort( m_SortedComponentPinList.begin(), m_SortedComponentPinList.end(), sortPinsByNum );

    // Remove duplicate Pins in m_SortedComponentPinList
    eraseDuplicatePins();

    // record the usage of this library component entry.
    m_LibParts.insert( comp->GetPartRef().get() ); // rejects non-unique pointers
}


bool NETLIST_EXPORTER::addPinToComponentPinList( SCH_COMPONENT* aComponent,
   SCH_SHEET_PATH* aSheetPath, LIB_PIN* aPin )
{
    // Search the PIN description for Pin in g_NetObjectslist
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
    {
        NETLIST_OBJECT* pin = m_masterList->GetItem( ii );

        if( pin->m_Type != NETLIST_ITEM::PIN )
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

            if( m_SortedComponentPinList[idxref]->GetConnectionType() == NET_CONNECTION::PAD_CONNECT )
            {
                m_SortedComponentPinList[jj]->m_Flag = 1;
                m_SortedComponentPinList[jj] = NULL;
            }
            else /* the reference pin is not connected: remove this pin if the
                  * other pin is connected */
            {
                if( m_SortedComponentPinList[jj]->GetConnectionType() == NET_CONNECTION::PAD_CONNECT )
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


void NETLIST_EXPORTER::findAllUnitsOfComponent( SCH_COMPONENT* aComponent,
        LIB_PART* aEntry, SCH_SHEET_PATH* aSheetPath )
{
    wxString    ref = aComponent->GetRef( aSheetPath );
    wxString    ref2;

    SCH_SHEET_LIST sheetList( g_RootSheet );

    for( unsigned i = 0;  i < sheetList.size();  i++ )
    {
        for( auto item : sheetList[i].LastScreen()->Items().OfType( SCH_COMPONENT_T ) )
        {
            SCH_COMPONENT* comp2 = static_cast<SCH_COMPONENT*>( item );

            ref2 = comp2->GetRef( &sheetList[i] );

            if( ref2.CmpNoCase( ref ) != 0 )
                continue;

            int unit2 = comp2->GetUnitSelection( &sheetList[i] );  // slow

            for( LIB_PIN* pin = aEntry->GetNextPin();  pin;  pin = aEntry->GetNextPin( pin ) )
            {
                wxASSERT( pin->Type() == LIB_PIN_T );

                if( pin->GetUnit() && pin->GetUnit() != unit2 )
                    continue;

                if( pin->GetConvert() && pin->GetConvert() != comp2->GetConvert() )
                    continue;

                // A suitable pin is found: add it to the current list
                addPinToComponentPinList( comp2, &sheetList[i], pin );
            }
        }
    }
}


/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <sch_symbol.h>
#include <eda_draw_frame.h>
#include <lib_shape.h>
#include <macros.h>

// helper function to sort pins by pin num
static bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst );

static void CheckLibSymbolGraphics( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                                     EDA_DRAW_FRAME* aUnitsProvider );

/**
 * Check a lib symbol to find incorrect settings
 * Pins not on a valid grid
 * Pins duplicated
 * Conflict with pins at same location
 * Incorrect Power Symbols
 * illegal reference prefix (cannot ends by a digit or a '?')
 * @param aSymbol is the library symbol to check
 * @param aMessages is a room to store error messages
 * @param aGridForPins (in IU) is the grid to test pin positions ( >= 25 mils )
 * should be 25, 50 or 100 mils (convered to IUs)
 * @param aUnitsProvider a frame to format coordinates in messages
 */
void CheckLibSymbol( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                    int aGridForPins, EDA_DRAW_FRAME* aUnitsProvider )
{
    if( !aSymbol )
        return;

    wxString msg;

    // Test reference prefix validity:
    // if the symbol is saved in a library, the prefix should not ends by a digit or a '?'
    // but it is acceptable if the symbol is saved to aschematic
    wxString reference_base = aSymbol->GetReferenceField().GetText();
    wxString illegal_end( wxT( "0123456789?" ) );
    wxUniChar last_char = reference_base.Last();

    if( illegal_end.Find( last_char ) != wxNOT_FOUND )
    {
        msg.Printf( _( "<b>Warning: reference prefix</b><br>prefix ending by '%s' can create"
                       " issues if saved in a symbol library" ),
                    illegal_end );
        msg += wxT( "<br><br>" );
        aMessages.push_back( msg );
    }

    std::vector<LIB_PIN*> pinList;
    aSymbol->GetPins( pinList );

    // Test for duplicates:
    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( pinList.begin(), pinList.end(), sort_by_pin_number );

    // The minimal grid size allowed to place a pin is 25 mils
    // the best grid size is 50 mils, but 25 mils is still usable
    // this is because all aSymbols are using a 50 mils grid to place pins, and therefore
    // the wires must be on the 50 mils grid
    // So raise an error if a pin is not on a 25 (or bigger :50 or 100) mils grid
    const int min_grid_size = schIUScale.MilsToIU( 25 );
    const int clamped_grid_size = ( aGridForPins < min_grid_size ) ? min_grid_size : aGridForPins;

    for( unsigned ii = 1; ii < pinList.size(); ii++ )
    {
        LIB_PIN* pin  = pinList[ii - 1];
        LIB_PIN* next = pinList[ii];

        if( pin->GetNumber() != next->GetNumber() )
            continue;

        // Pins are not duplicated only if they are in different body styles
        // (but GetBodyStyle() == 0 means commun to all body styles)
        if( pin->GetBodyStyle() != 0 && next->GetBodyStyle() != 0 )
        {
            if( pin->GetBodyStyle() != next->GetBodyStyle() )
                continue;
        }

        wxString pinName;
        wxString nextName;

        if( pin->GetName() != "~"  && !pin->GetName().IsEmpty() )
            pinName = " '" + pin->GetName() + "'";

        if( next->GetName() != "~"  && !next->GetName().IsEmpty() )
            nextName = " '" + next->GetName() + "'";

        if( aSymbol->HasAlternateBodyStyle() && next->GetBodyStyle() )
        {
            if( pin->GetUnit() == 0 || next->GetUnit() == 0 )
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%s, %s)</b>"
                               " conflicts with pin %s%s at location <b>(%s, %s)</b>"
                               " in %s body style." ),
                            next->GetNumber(),
                            nextName,
                            aUnitsProvider->MessageTextFromValue( next->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -next->GetPosition().y ),
                            pin->GetNumber(),
                            pin->GetName(),
                            aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                            SCH_ITEM::GetBodyStyleDescription( pin->GetBodyStyle() ).Lower() );
            }
            else
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%s, %s)</b>"
                               " conflicts with pin %s%s at location <b>(%s, %s)</b>"
                               " in units %s and %s of %s body style." ),
                            next->GetNumber(),
                            nextName,
                            aUnitsProvider->MessageTextFromValue( next->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -next->GetPosition().y ),
                            pin->GetNumber(),
                            pinName,
                            aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                            aSymbol->GetUnitReference( next->GetUnit() ),
                            aSymbol->GetUnitReference( pin->GetUnit() ),
                            SCH_ITEM::GetBodyStyleDescription( pin->GetBodyStyle() ).Lower() );
            }
        }
        else
        {
            if( pin->GetUnit() == 0 || next->GetUnit() == 0 )
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%s, %s)</b>"
                               " conflicts with pin %s%s at location <b>(%s, %s)</b>." ),
                            next->GetNumber(),
                            nextName,
                            aUnitsProvider->MessageTextFromValue( next->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -next->GetPosition().y ),
                            pin->GetNumber(),
                            pinName,
                            aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ) );
            }
            else
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%s, %s)</b>"
                               " conflicts with pin %s%s at location <b>(%s, %s)</b>"
                               " in units %s and %s." ),
                            next->GetNumber(),
                            nextName,
                            aUnitsProvider->MessageTextFromValue( next->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -next->GetPosition().y ),
                            pin->GetNumber(),
                            pinName,
                            aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                            aSymbol->GetUnitReference( next->GetUnit() ),
                            aSymbol->GetUnitReference( pin->GetUnit() ) );
            }
        }

        msg += wxT( "<br><br>" );
        aMessages.push_back( msg );
    }

    // Test for a valid power aSymbol.
    // A valid power aSymbol has only one unit, no alternate body styles and one pin.
    // And this pin should be PT_POWER_IN (invisible to be automatically connected)
    // or PT_POWER_OUT for a power flag
    if( aSymbol->IsPower() )
    {
        if( aSymbol->GetUnitCount() != 1 )
        {
            msg.Printf( _( "<b>A Power Symbol should have only one unit</b><br><br>" ) );
            aMessages.push_back( msg );
        }

        if( aSymbol->HasAlternateBodyStyle() )
        {
            msg.Printf( _( "<b>A Power Symbol should not have DeMorgan variants</b><br><br>" ) );
            aMessages.push_back( msg );
        }

        if( pinList.size() != 1 )
        {
            msg.Printf( _( "<b>A Power Symbol should have only one pin</b><br><br>" ) );
            aMessages.push_back( msg );
        }

        LIB_PIN* pin = pinList[0];

        if( pin->GetType() != ELECTRICAL_PINTYPE::PT_POWER_IN
                && pin->GetType() != ELECTRICAL_PINTYPE::PT_POWER_OUT )
        {
            msg.Printf( _( "<b>Suspicious Power Symbol</b><br>"
                           "Only an input or output power pin has meaning<br><br>" ) );
            aMessages.push_back( msg );
        }

        if( pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN && !pin->IsVisible() )
        {
            msg.Printf( _( "<b>Suspicious Power Symbol</b><br>"
                           "Invisible input power pins are no longer required<br><br>" ) );
            aMessages.push_back( msg );
        }
    }


    for( LIB_PIN* pin : pinList )
    {
        wxString pinName = pin->GetName();

        if( pinName.IsEmpty() || pinName == "~" )
            pinName = "";
        else
            pinName = "'" + pinName + "'";

        if( !aSymbol->IsPower()
                && pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN
                && !pin->IsVisible() )
        {
            // hidden power pin
            if( aSymbol->HasAlternateBodyStyle() && pin->GetBodyStyle() )
            {
                if( aSymbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in %s body style." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                                SCH_ITEM::GetBodyStyleDescription( pin->GetBodyStyle() ).Lower() );
                }
                else
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c of %s body style." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1,
                                SCH_ITEM::GetBodyStyleDescription( pin->GetBodyStyle() ).Lower() );
                }
            }
            else
            {
                if( aSymbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ) );
                }
                else
                {
                    msg.Printf( _( "Info: <b>Hidden power pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1 );
                }
            }

            msg += wxT( "<br>" );
            msg += _( "(Hidden power pins will drive their pin names on to any connected nets.)" );
            msg += wxT( "<br><br>" );
            aMessages.push_back( msg );
        }

        if( ( (pin->GetPosition().x % clamped_grid_size) != 0 )
                || ( (pin->GetPosition().y % clamped_grid_size) != 0 ) )
        {
            // pin is off grid
            msg.Empty();

            if( aSymbol->HasAlternateBodyStyle() && pin->GetBodyStyle() )
            {
                if( aSymbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " of %s body style." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                                SCH_ITEM::GetBodyStyleDescription( pin->GetBodyStyle() ).Lower() );
                }
                else
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c of %s body style." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1,
                                SCH_ITEM::GetBodyStyleDescription( pin->GetBodyStyle() ).Lower() );
                 }
            }
            else
            {
                if( aSymbol->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ) );
                }
                else
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%s, %s)</b>"
                                   " in unit %c." ),
                                pin->GetNumber(),
                                pinName,
                                aUnitsProvider->MessageTextFromValue( pin->GetPosition().x ),
                                aUnitsProvider->MessageTextFromValue( -pin->GetPosition().y ),
                                'A' + pin->GetUnit() - 1 );
                }
            }

            msg += wxT( "<br><br>" );
            aMessages.push_back( msg );
        }
    }

    CheckLibSymbolGraphics( aSymbol, aMessages,  aUnitsProvider );
}


void CheckLibSymbolGraphics( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                             EDA_DRAW_FRAME* aUnitsProvider )
{
    if( !aSymbol )
        return;

    wxString msg;

    for( const SCH_ITEM& item : aSymbol->GetDrawItems() )
    {
        if( item.Type() != LIB_SHAPE_T )
            continue;

        const LIB_SHAPE* shape = static_cast<const LIB_SHAPE*>( &item );

        switch( shape->GetShape() )
        {
        case SHAPE_T::ARC:
            break;

        case SHAPE_T::CIRCLE:
            if( shape->GetRadius() <= 0 )
            {
                msg.Printf( _( "<b>Graphic circle has radius = 0</b> at location <b>(%s, %s)</b>." ),
                            aUnitsProvider->MessageTextFromValue(shape->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -shape->GetPosition().y ) );
                msg += wxT( "<br>" );
                aMessages.push_back( msg );
            }
            break;

        case SHAPE_T::RECTANGLE:
            if( shape->GetPosition() == shape->GetEnd() )
            {
                msg.Printf( _( "<b>Graphic rectangle has size 0</b> at location <b>(%s, %s)</b>." ),
                            aUnitsProvider->MessageTextFromValue(shape->GetPosition().x ),
                            aUnitsProvider->MessageTextFromValue( -shape->GetPosition().y ) );
                msg += wxT( "<br>" );
                aMessages.push_back( msg );
            }
            break;

        case SHAPE_T::POLY:
            break;

        case SHAPE_T::BEZIER:
            break;

        default:
            UNIMPLEMENTED_FOR( shape->SHAPE_T_asString() );
        }
    }
}


bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    // Use number as primary key
    int test = ref->GetNumber().Cmp( tst->GetNumber() );

    // Use DeMorgan variant as secondary key
    if( test == 0 )
        test = ref->GetBodyStyle() - tst->GetBodyStyle();

    // Use unit as tertiary key
    if( test == 0 )
        test = ref->GetUnit() - tst->GetUnit();

    return test < 0;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef SELECTION_CONDITIONS_H_
#define SELECTION_CONDITIONS_H_

#include <functional>
#include <core/typeinfo.h>
#include <vector>

#include <tool/selection.h>

///> Functor type that checks a specific condition for selected items.
typedef std::function<bool (const SELECTION&)> SELECTION_CONDITION;

SELECTION_CONDITION operator||( const SELECTION_CONDITION& aConditionA,
                                const SELECTION_CONDITION& aConditionB );

SELECTION_CONDITION operator&&( const SELECTION_CONDITION& aConditionA,
                                const SELECTION_CONDITION& aConditionB );


/**
 * Class that groups generic conditions for selected items.
 */
class SELECTION_CONDITIONS
{
public:
    /**
     * Function ShowAlways
     * The default condition function (always returns true).
     * @param aSelection is the selection to be tested.
     * @return Always true;
     */
    static bool ShowAlways( const SELECTION& aSelection )
    {
        return true;
    }

    /**
     * Function NotEmpty
     * Tests if there are any items selected.
     * @param aSelection is the selection to be tested.
     * @return True if there is at least one item selected.
     */
    static bool NotEmpty( const SELECTION& aSelection );

    /**
     * Function HasType
     * Creates a functor that tests if among the selected items there is at least one of a given type.
     * @param aType is the type that is searched.
     * @return Functor testing for presence of items of a given type.
     */
    static SELECTION_CONDITION HasType( KICAD_T aType );

    /**
     * Function OnlyType
     * Creates a functor that tests if the selected items are *only* of given type.
     * @param aType is the type that is searched.
     * @return Functor testing if selected items are exclusively of one type.
     */
    static SELECTION_CONDITION OnlyType( KICAD_T aType );

    /**
     * Function OnlyTypes
     * Creates a functor that tests if the selected items are *only* of given types.
     * @param aType is a vector containing types that are searched.
     * @return Functor testing if selected items are exclusively of the requested types.
     */
    static SELECTION_CONDITION OnlyTypes( const std::vector<KICAD_T>& aTypes );

    /**
     * Function OnlyTypes
     * Creates a functor that tests if the selected items are *only* of given types.
     * @param aType is an array containing types that are searched. It has to be ended with
     * KICAD_T::EOT as end marker.
     * @return Functor testing if selected items are exclusively of the requested types.
     */
    static SELECTION_CONDITION OnlyTypes( const KICAD_T aTypes[] );

    /**
     * Function Count
     * Creates a functor that tests if the number of selected items is equal to the value given as
     * parameter.
     * @param aNumber is the number of expected items.
     * @return Functor testing if the number of selected items is equal aNumber.
     */
    static SELECTION_CONDITION Count( int aNumber );

    /**
     * Function MoreThan
     * Creates a functor that tests if the number of selected items is greater than the value given
     * as parameter.
     * @param aNumber is the number used for comparison.
     * @return Functor testing if the number of selected items is greater than aNumber.
     */
    static SELECTION_CONDITION MoreThan( int aNumber );

    /**
     * Function LessThan
     * Creates a functor that tests if the number of selected items is smaller than the value given
     * as parameter.
     * @param aNumber is the number used for comparison.
     * @return Functor testing if the number of selected items is smaller than aNumber.
     */
    static SELECTION_CONDITION LessThan( int aNumber );

private:
    ///> Helper function used by HasType()
    static bool hasTypeFunc( const SELECTION& aSelection, KICAD_T aType );

    ///> Helper function used by OnlyType()
    static bool onlyTypeFunc( const SELECTION& aSelection, KICAD_T aType );

    ///> Helper function used by OnlyTypes()
    static bool onlyTypesFunc( const SELECTION& aSelection, const std::vector<KICAD_T>& aTypes );
    static bool onlyTypesFuncArr( const SELECTION& aSelection, const KICAD_T aTypes[] );

    ///> Helper function used by Count()
    static bool countFunc( const SELECTION& aSelection, int aNumber );

    ///> Helper function used by MoreThan()
    static bool moreThanFunc( const SELECTION& aSelection, int aNumber );

    ///> Helper function used by LessThan()
    static bool lessThanFunc( const SELECTION& aSelection, int aNumber );

    ///> Helper function used by operator||
    static bool orFunc( const SELECTION_CONDITION& aConditionA,
                        const SELECTION_CONDITION& aConditionB, const SELECTION& aSelection )
    {
        return aConditionA( aSelection ) || aConditionB( aSelection );
    }

    ///> Helper function used by operator&&
    static bool andFunc( const SELECTION_CONDITION& aConditionA,
                         const SELECTION_CONDITION& aConditionB, const SELECTION& aSelection )
    {
        return aConditionA( aSelection ) && aConditionB( aSelection );
    }

    friend SELECTION_CONDITION operator||( const SELECTION_CONDITION& aConditionA,
                                           const SELECTION_CONDITION& aConditionB );

    friend SELECTION_CONDITION operator&&( const SELECTION_CONDITION& aConditionA,
                                           const SELECTION_CONDITION& aConditionB );
};

#endif /* SELECTION_CONDITIONS_H_ */

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

#include "selection_conditions.h"
#include "selection_tool.h"

#include <boost/bind.hpp>


bool SELECTION_CONDITIONS::NotEmpty( const SELECTION& aSelection )
{
    return !aSelection.Empty();
}


SELECTION_CONDITION SELECTION_CONDITIONS::HasType( KICAD_T aType )
{
    return boost::bind( &SELECTION_CONDITIONS::hasTypeFunc, _1, aType );
}


SELECTION_CONDITION SELECTION_CONDITIONS::OnlyType( KICAD_T aType )
{
    return boost::bind( &SELECTION_CONDITIONS::onlyTypeFunc, _1, aType );
}


SELECTION_CONDITION SELECTION_CONDITIONS::Count( int aNumber )
{
    return boost::bind( &SELECTION_CONDITIONS::countFunc, _1, aNumber );
}


SELECTION_CONDITION SELECTION_CONDITIONS::MoreThan( int aNumber )
{
    return boost::bind( &SELECTION_CONDITIONS::moreThanFunc, _1, aNumber );
}


SELECTION_CONDITION SELECTION_CONDITIONS::LessThan( int aNumber )
{
    return boost::bind( &SELECTION_CONDITIONS::lessThanFunc, _1, aNumber );
}


bool SELECTION_CONDITIONS::hasTypeFunc( const SELECTION& aSelection, KICAD_T aType )
{
    for( int i = 0; i < aSelection.Size(); ++i )
    {
        if( aSelection.Item<EDA_ITEM>( i )->Type() == aType )
            return true;
    }

    return false;
}


bool SELECTION_CONDITIONS::onlyTypeFunc( const SELECTION& aSelection, KICAD_T aType )
{
    for( int i = 0; i < aSelection.Size(); ++i )
    {
        if( aSelection.Item<EDA_ITEM>( i )->Type() != aType )
            return false;
    }

    return true;
}


bool SELECTION_CONDITIONS::countFunc( const SELECTION& aSelection, int aNumber )
{
    return aSelection.Size() == aNumber;
}


bool SELECTION_CONDITIONS::moreThanFunc( const SELECTION& aSelection, int aNumber )
{
    return aSelection.Size() > aNumber;
}


bool SELECTION_CONDITIONS::lessThanFunc( const SELECTION& aSelection, int aNumber )
{
    return aSelection.Size() > aNumber;
}


SELECTION_CONDITION operator||( const SELECTION_CONDITION& aConditionA,
                                const SELECTION_CONDITION& aConditionB )
{
    return boost::bind( &SELECTION_CONDITIONS::orFunc, aConditionA, aConditionB, _1 );
}


SELECTION_CONDITION operator&&( const SELECTION_CONDITION& aConditionA,
                                const SELECTION_CONDITION& aConditionB )
{
    return boost::bind( &SELECTION_CONDITIONS::andFunc, aConditionA, aConditionB, _1 );
}

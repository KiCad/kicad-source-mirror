/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2008 Kicad Developers, see change_log.txt for contributors.
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


#include "fctsys.h"
#include "dlist.h"
#include "base_struct.h"


/* Implement the class DHEAD from dlist.h */


DHEAD::~DHEAD()
{
    if( meOwner )
        DeleteAll();
}


void DHEAD::DeleteAll()
{
    EDA_ITEM* next;
    EDA_ITEM* item = first;

    while( item )
    {
        next = item->Next();
        delete item;            // virtual destructor, class specific
        item = next;
    }

    first = 0;
    last  = 0;
    count = 0;
}


void DHEAD::append( EDA_ITEM* aNewElement )
{
    wxASSERT( aNewElement != NULL );

    if( first )        // list is not empty, first is not touched
    {
        wxASSERT( last != NULL );

        aNewElement->SetNext( 0 );
        aNewElement->SetBack( last );

        last->SetNext( aNewElement );
        last = aNewElement;
    }
    else        // list is empty, first and last are changed
    {
        aNewElement->SetNext( 0 );
        aNewElement->SetBack( 0 );

        first = aNewElement;
        last  = aNewElement;
    }

    aNewElement->SetList( this );

    ++count;
}


void DHEAD::insert( EDA_ITEM* aNewElement, EDA_ITEM* aAfterMe )
{
    wxASSERT( aNewElement != NULL );

    if( !aAfterMe )
        append( aNewElement );
    else
    {
        wxASSERT( aAfterMe->GetList() == this );

        // the list cannot be empty if aAfterMe is supposedly on the list
        wxASSERT( first && last );

        if( first == aAfterMe )
        {
            aAfterMe->SetBack( aNewElement );

            aNewElement->SetBack( 0 );  // first in list does not point back
            aNewElement->SetNext( aAfterMe );

            first = aNewElement;
        }
        else
        {
            EDA_ITEM* oldBack = aAfterMe->Back();

            aAfterMe->SetBack( aNewElement );

            aNewElement->SetBack( oldBack );
            aNewElement->SetNext( aAfterMe );

            oldBack->SetNext( aNewElement );
        }

        aNewElement->SetList( this );

        ++count;
    }
}


void DHEAD::remove( EDA_ITEM* aElement )
{
    wxASSERT( aElement );
    wxASSERT( aElement->GetList() == this );

    if( aElement->Next() )
    {
        aElement->Next()->SetBack( aElement->Back() );
    }
    else  // element being removed is last
    {
        wxASSERT( last == aElement );
        last = aElement->Back();
    }

    if( aElement->Back() )
    {
        aElement->Back()->SetNext( aElement->Next() );
    }
    else    // element being removed is first
    {
        wxASSERT( first == aElement );
        first = aElement->Next();
    }

    aElement->SetBack( 0 );
    aElement->SetNext( 0 );
    aElement->SetList( 0 );

    --count;
}

#if defined(DEBUG)

void DHEAD::VerifyListIntegrity()
{
    EDA_ITEM* item;
    unsigned i = 0;

    for( item = first;  item && i<count;  ++i, item = item->Next() )
    {
        if( i < count-1 )
        {
            wxASSERT( item->Next() );
        }

        wxASSERT( item->GetList() == this );
    }

    wxASSERT( item == NULL );
    wxASSERT( i == count );

    i = 0;
    for( item = last;  item && i<count;  ++i, item = item->Back() )
    {
        if( i < count-1 )
        {
            wxASSERT( item->Back() );
        }
    }

    wxASSERT( item == NULL );
    wxASSERT( i == count );

    // printf("list %p has %d items.\n", this, count );
}

#endif


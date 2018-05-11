/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2008 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <dlist.h>
#include <base_struct.h>

// verifies conditions for setting a parent list for an element, i.e.
// element either does not belong to any list or it already belongs to the same list
#define CHECK_OWNERSHIP(a) wxASSERT( !a->GetList() || a->GetList() == this );

/* Implement the class DHEAD from dlist.h */


DHEAD::~DHEAD()
{
    if( meOwner )
        DeleteAll();
}


void DHEAD::DeleteAll()
{
    wxCHECK( meOwner, /*void*/ );   // only owning lists may delete the contents
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
    wxCHECK( aNewElement, /*void*/ );

    if( first )        // list is not empty, first is not touched
    {
        wxASSERT( count > 0 );
        wxCHECK( last, /*void*/ );      // 'first' is non-null, so 'last' should be non-null too

        aNewElement->SetNext( 0 );
        aNewElement->SetBack( last );

        wxASSERT( !last->Next() );      // the last element should point to nullptr
        last->SetNext( aNewElement );
        last = aNewElement;
    }
    else        // list is empty, first and last are changed
    {
        wxASSERT( count == 0 );
        wxASSERT( !last );              // 'first' is null, then 'last' should be too
        aNewElement->SetNext( 0 );
        aNewElement->SetBack( 0 );

        first = aNewElement;
        last  = aNewElement;
    }

    CHECK_OWNERSHIP( aNewElement );
    aNewElement->SetList( this );

    ++count;
}


void DHEAD::append( DHEAD& aList )
{
    if( aList.first )
    {
        // Change the item's list to me.
        for( EDA_ITEM* item = aList.first;  item;  item = item->Next() )
        {
            wxASSERT( item->GetList() == &aList );
            item->SetList( this );
        }

        if( first )       // this list is not empty, set last item's next to the first item in aList
        {
            wxCHECK_RET( last != NULL, wxT( "Last list element not set." ) );

            last->SetNext( aList.first );
            aList.first->SetBack( last );
            last = aList.last;
        }
        else              // this list is empty, first and last are same as aList
        {
            first = aList.first;
            last  = aList.last;
        }

        count += aList.count;

        aList.count = 0;
        aList.first = NULL;
        aList.last  = NULL;
    }
}


void DHEAD::insert( EDA_ITEM* aNewElement, EDA_ITEM* aAfterMe )
{
    wxCHECK( aNewElement, /*void*/ );

    if( !aAfterMe )
        append( aNewElement );
    else
    {
        wxCHECK( aAfterMe->GetList() == this, /*void*/ );

        // the list cannot be empty if aAfterMe is supposedly on the list
        wxASSERT( first && last && count > 0 );

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

        CHECK_OWNERSHIP( aNewElement );
        aNewElement->SetList( this );

        ++count;
    }
}


void DHEAD::remove( EDA_ITEM* aElement )
{
    wxCHECK( aElement && aElement->GetList() == this, /*void*/ );

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
    wxASSERT( ( first && last ) || count == 0 );
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


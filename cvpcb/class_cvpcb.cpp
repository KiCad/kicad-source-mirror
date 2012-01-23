/**
 * @file class_cvpcb.cpp
 */

#include <fctsys.h>
#include <kicad_string.h>

#include <cvpcb.h>
#include <footprint_info.h>


PIN::PIN()
{
    m_Index = 0;         /* Variable used by types of netlists. */
    m_Type = 0;          /* Electrical type. */
}


bool operator<( const PIN& item1, const PIN& item2 )
{
    return StrNumCmp( item1.m_Number, item2.m_Number, 4, true ) < 0;
}


bool operator==( const PIN& item1, const PIN& item2 )
{
    return ( item1.m_Number == item2.m_Number );
}


bool same_pin_number( const PIN* item1, const PIN* item2 )
{
    wxASSERT( item1 != NULL && item2 != NULL );

    return ( item1->m_Number == item2->m_Number );
}


bool same_pin_net( const PIN* item1, const PIN* item2 )
{
    wxASSERT( item1 != NULL && item2 != NULL );

    return ( item1->m_Net == item2->m_Net );
}


COMPONENT::COMPONENT()
{
    m_Num = 0;
    m_Multi = 0;
}


COMPONENT::~COMPONENT()
{
}


bool operator<( const COMPONENT& item1, const COMPONENT& item2 )
{
    return StrNumCmp( item1.m_Reference, item2.m_Reference, INT_MAX, true ) < 0;
}

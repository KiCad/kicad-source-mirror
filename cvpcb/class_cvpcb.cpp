/*******************/
/* class_cvpcb.cpp */
/*******************/

#include "fctsys.h"
#include "kicad_string.h"

#include "cvpcb.h"


PIN::PIN()
{
    m_Index = 0;         /* variable utilisee selon types de netlistes */
    m_Type = 0;          /* code type electrique ( Entree Sortie Passive..) */
}

bool operator<( const PIN& item1, const PIN& item2 )
{
    return ( StrLenNumICmp( item1.m_Number.GetData(),
                            item2.m_Number.GetData(), 4 ) < 0 );
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
    return ( StrNumICmp( item1.m_Reference.GetData(),
                         item2.m_Reference.GetData() ) < 0 );
}


FOOTPRINT::FOOTPRINT()
{
    m_Num = 0;
}

bool operator<( const FOOTPRINT& item1, const FOOTPRINT& item2 )
{
    return ( StrNumICmp( item1.m_Module.GetData(),
                         item2.m_Module.GetData() ) < 0 );
}

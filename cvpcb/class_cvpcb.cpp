/*******************/
/* class_cvpcb.cpp */
/*******************/

#include "fctsys.h"
#include "kicad_string.h"

#include "cvpcb.h"

#include <wx/listimpl.cpp>


WX_DEFINE_LIST( PIN_LIST );

PIN::PIN()
{
    m_Index = 0;         /* variable utilisee selon types de netlistes */
    m_PinType = 0;       /* code type electrique ( Entree Sortie Passive..) */
}

int compare( const PIN** item1, const PIN** item2 )
{
    return StrLenNumICmp( (*item1)->m_PinNum.GetData(),
                          (*item2)->m_PinNum.GetData(), 4 );
}

bool same_pin_number( const PIN* item1, const PIN* item2 )
{
    wxASSERT( item1 != NULL && item2 != NULL );

    return ( item1->m_PinNum == item2->m_PinNum );
}

bool same_pin_net( const PIN* item1, const PIN* item2 )
{
    wxASSERT( item1 != NULL && item2 != NULL );

    return ( item1->m_PinNet == item2->m_PinNet );
}


WX_DEFINE_LIST( COMPONENT_LIST );

COMPONENT::COMPONENT()
{
    m_Num = 0;
    m_Multi = 0;
}

COMPONENT::~COMPONENT()
{
    m_Pins.DeleteContents( true );
    m_Pins.Clear();
}

int compare( const COMPONENT** item1, const COMPONENT** item2 )
{
    return StrNumICmp( (*item1)->m_Reference.GetData(),
                       (*item2)->m_Reference.GetData() );
}


WX_DEFINE_LIST( FOOTPRINT_LIST );

FOOTPRINT::FOOTPRINT()
{
    m_Num = 0;
}

int compare( const FOOTPRINT** item1, const FOOTPRINT** item2 )
{
    return StrNumICmp( (*item1)->m_Module.GetData(),
                       (*item2)->m_Module.GetData() );
}

/**
 * @file class_cvpcb.cpp
 */

#include <fctsys.h>
#include <kicad_string.h>

#include <cvpcb.h>
#include <footprint_info.h>


bool operator<( const PIN& item1, const PIN& item2 )
{
    return StrNumCmp( item1.m_Number, item2.m_Number, 4, true ) < 0;
}


bool operator<( const COMPONENT& item1, const COMPONENT& item2 )
{
    return StrNumCmp( item1.m_Reference, item2.m_Reference, INT_MAX, true ) < 0;
}

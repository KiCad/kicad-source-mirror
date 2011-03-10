/******************************************************/
/* Routines for locating an element of a schematic.   */
/******************************************************/

#include "fctsys.h"
#include "common.h"
#include "trigo.h"
#include "macros.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_junction.h"
#include "sch_component.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_polyline.h"
#include "sch_sheet.h"
#include "lib_pin.h"
#include "template_fieldnames.h"


/**
 * Search the smaller (considering its area) component under the mouse
 * cursor or the pcb cursor
 *
 * If more than 1 component is found, a pointer to the smaller component is
 * returned
 */
SCH_COMPONENT* LocateSmallestComponent( SCH_SCREEN* Screen )
{
    double area;
    EDA_Rect rect;
    PICKED_ITEMS_LIST itemList;
    SCH_COMPONENT* component = NULL;
    SCH_COMPONENT* lastcomponent = NULL;

    if( Screen->GetItems( Screen->RefPos( true ), itemList, COMPONENT_T ) == 0 )
    {
        if( Screen->GetItems( Screen->GetCrossHairPosition(), itemList, COMPONENT_T ) == 0 )
            return NULL;
    }

    if( itemList.GetCount() == 1 )
        return (SCH_COMPONENT*) itemList.GetPickedItem( 0 );

    for( size_t i = 0;  i < itemList.GetCount();  i++ )
    {
        component = (SCH_COMPONENT*) itemList.GetPickedItem( i );

        if( lastcomponent == NULL )  // First component
        {
            lastcomponent = component;
            rect = lastcomponent->GetBoundingBox();
            area = ABS( (double) rect.GetWidth() * (double) rect.GetHeight() );
        }
        else
        {
            rect = component->GetBoundingBox();
            double tmp = ABS( (double) rect.GetWidth() * (double) rect.GetHeight() );

            if( area > tmp )         // a smaller component is found
            {
                area = tmp;
                lastcomponent = component;
            }
        }
    }

    return lastcomponent;
}

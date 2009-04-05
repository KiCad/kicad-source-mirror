/****************************************************/
/*	class_drawpickedstruct.cpp */
/****************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpickedstruct.h"


/* This class has only one useful member: .m_PickedStruct, used as a link.
 *  It does not describe really an item.
 *  It is used to create a linked list of selected items (in block selection).
 *  Each DrawPickedStruct item has is member: .m_PickedStruct pointing the
 *  real selected item
 */

/*******************************************************************/
DrawPickedStruct::DrawPickedStruct( EDA_BaseStruct* pickedstruct ) :
    EDA_BaseStruct( NULL, DRAW_PICK_ITEM_STRUCT_TYPE )
/*******************************************************************/
{
    m_PickedStruct = pickedstruct;
}


DrawPickedStruct::~DrawPickedStruct()
{
}

#if defined(DEBUG)
void DrawPickedStruct::Show( int nestLevel, std::ostream& os )
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << "/>\n";
}
#endif


EDA_Rect DrawPickedStruct::GetBoundingBox()
{
    if( m_PickedStruct )
        return m_PickedStruct->GetBoundingBox();
    else
    {
        return EDA_Rect();      // empty rectangle
    }
}


EDA_Rect DrawPickedStruct::GetBoundingBoxUnion()
{
    EDA_Rect          ret;
    EDA_BaseStruct*   item;
    DrawPickedStruct* cur = this;

    while( cur && (item = cur->m_PickedStruct) != NULL )
    {
        ret.Merge( item->GetBoundingBox() );

        cur = cur->Next();
    }

    return ret;
}


/*********************************************/
void DrawPickedStruct::DeleteWrapperList()
/*********************************************/

/* Delete this item all the items of the linked list
 *  Free the wrapper, but DOES NOT delete the picked items linked by .m_PickedStruct
 */
{
    DrawPickedStruct* wrapp_struct, * next_struct;

    for( wrapp_struct = Next(); wrapp_struct != NULL; wrapp_struct = next_struct )
    {
        next_struct = wrapp_struct->Next();
        delete wrapp_struct;
    }
}

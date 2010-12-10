/*****************************************************/
/*                  libarch.cpp                      */
/* Module for generation of component archive files. */
/*****************************************************/
#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxstruct.h"
#include "sch_item_struct.h"

#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "class_library.h"
#include "sch_component.h"


/*
 *  Creates a library that contains all components used in the schematic.
 *
 *  return  true if success
 */
bool LibArchive( wxWindow* frame, const wxString& ArchFullFileName )
{
    wxString   msg;
    LIB_COMPONENT* Entry;
    CMP_LIBRARY* libCache;
    SCH_SCREENS ScreenList;

    libCache = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, ArchFullFileName );
    libCache->SetCache();

    /* examine all screens (not sheets) used and build the list of components
     * found in lib complex hierarchies are not a problem because we just want
     * to know used components in libraries
     */
    for( SCH_SCREEN* screen = ScreenList.GetFirst(); screen != NULL;
         screen = ScreenList.GetNext() )
    {
        for( SCH_ITEM* SchItem = screen->GetDrawItems(); SchItem; SchItem = SchItem->Next() )
        {
            if( SchItem->Type() != SCH_COMPONENT_T )
                continue;

            SCH_COMPONENT* component = (SCH_COMPONENT*) SchItem;
            // If not already saved in the new cache, put it:
            if( libCache->FindEntry( component->m_ChipName) == NULL )
            {
                Entry = CMP_LIBRARY::FindLibraryComponent( component->m_ChipName );

                if( Entry )    // if NULL : component not found, cannot be stored
                    libCache->AddComponent( Entry );
            }
        }
    }

    if( !libCache->Save( ArchFullFileName ) )
    {
        msg.Printf( _( "An error occurred attempting to save component \
library <%s>." ), GetChars( ArchFullFileName ) );
        DisplayError( frame, msg );
        return false;
    }

    return true;
}

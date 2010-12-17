/*****************************************************************/
/* class_DCodeSelectionbox.cpp: class for displaying DCodes list */
/*****************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "gerbview.h"

#include "class_DCodeSelectionbox.h"

/***************************************/
/* ListBox handling the footprint list */
/***************************************/

DCODE_SELECTION_BOX::DCODE_SELECTION_BOX( WinEDA_Toolbar* aParent, wxWindowID aId,
                             const wxPoint& aLocation, const wxSize& aSize,
                             const wxArrayString& aChoices  ) :
    wxChoice( aParent, aId, aLocation, aSize, aChoices )
{
    m_dcodeList  = &aChoices;
}


DCODE_SELECTION_BOX::~DCODE_SELECTION_BOX()
{
}


int DCODE_SELECTION_BOX::GetSelectedDCodeId()
{
    int ii = GetSelection();

    if( ii > 0 )
    {
        wxString msg = (*m_dcodeList)[ii].AfterFirst( wxChar( ' ' ) );
        long id;
        msg.ToLong(&id);
        return id;
    }

    return -1;
}


/* SetDCodeSelection
 * aDCodeId = the DCode Id to select or -1 to select "no dcode"
 */
void DCODE_SELECTION_BOX::SetDCodeSelection( int aDCodeId )
{
    if( aDCodeId > LAST_DCODE )
        aDCodeId = LAST_DCODE;

    int index = 0;
    if( aDCodeId >= FIRST_DCODE )
        index = aDCodeId - FIRST_DCODE + 1;

    SetSelection(index);
}

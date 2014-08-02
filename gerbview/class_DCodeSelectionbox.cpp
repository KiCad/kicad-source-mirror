/*****************************************************************/
/* class_DCodeSelectionbox.cpp: class for displaying DCodes list */
/*****************************************************************/

#include <fctsys.h>
#include <pgm_base.h>
#include <wxstruct.h>
#include <class_drawpanel.h>
#include <gerbview.h>
#include <dcode.h>

#include <class_DCodeSelectionbox.h>

/*******************************************/
/* Helper class for displaying DCodes list */
/*******************************************/

DCODE_SELECTION_BOX::DCODE_SELECTION_BOX( wxAuiToolBar* aParent, wxWindowID aId,
                                          const wxPoint& aLocation, const wxSize& aSize,
                                          const wxArrayString& aChoices  ) :
    wxComboBox( aParent, aId, wxEmptyString, aLocation, aSize, 0, NULL, wxCB_READONLY )
{
    m_dcodeList  = &aChoices;
    // Append aChoices here is by far faster than use aChoices inside
    // the wxComboBox constructor
    Append(aChoices);
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

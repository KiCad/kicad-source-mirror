/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_gendrill.cpp
// Author:      jean-pierre Charras
// Licence:     GPL
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "gendrill.h"

#include "dialog_gendrill.h"


DIALOG_GENDRILL::DIALOG_GENDRILL( WinEDA_PcbFrame* parent )
    : DIALOG_GENDRILL_BASE( parent )
{
    m_Parent = parent;

    SetReturnCode( 1 );
    initDialog();
    GetSizer()->SetSizeHints( this );
    Centre( );
}


/*!
 * DIALOG_GENDRILL destructor
 */

DIALOG_GENDRILL::~DIALOG_GENDRILL()
{
}


/*!
 * Member initialisation
 */

void DIALOG_GENDRILL::initDialog()
{
    SetFocus(); // Under wxGTK: mandatory to close dialog by the ESC key
	InitDisplayParams();
}


/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_RADIOBOX
 */

void DIALOG_GENDRILL::OnSelDrillUnitsSelected( wxCommandEvent& event )
{
	UpdatePrecisionOptions(event);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
 */

void DIALOG_GENDRILL::OnOkClick( wxCommandEvent& event )
{
	GenDrillFiles(event);
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */

void DIALOG_GENDRILL::OnCancelClick( wxCommandEvent& event )
{
    UpdateConfig();     /* Save drill options: */
    event.Skip();       // Process the default cancel event (close dialog)
}

/*!
 * wxEVT_COMMAND_RADIOBOX_SELECTED event handler for ID_SEL_ZEROS_FMT
 */

void DIALOG_GENDRILL::OnSelZerosFmtSelected( wxCommandEvent& event )
{
	UpdatePrecisionOptions(event);
}


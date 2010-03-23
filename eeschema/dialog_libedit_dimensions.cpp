/*
 * dialog_libedit_dimensions.cpp
 * Handles the dialog so set current texts and pins sizes in LibEdit
 */
#include "fctsys.h"
//#include "appl_wxstruct.h"
#include "common.h"
//#include "class_drawpanel.h"

#include "program.h"
#include "general.h"
#include "libeditframe.h"


#include "dialog_libedit_dimensions_base.h"


class DIALOG_LIBEDIT_DIMENSIONS : public DIALOG_LIBEDIT_DIMENSIONS_BASE
{
	private:
	public:
		DIALOG_LIBEDIT_DIMENSIONS( WinEDA_LibeditFrame* parent);
        ~DIALOG_LIBEDIT_DIMENSIONS();
    private:
        void initDialog( );

};

void WinEDA_LibeditFrame::InstallDimensionsDialog( wxCommandEvent& event )
{
    DIALOG_LIBEDIT_DIMENSIONS dlg(this);
    dlg.ShowModal( );
}

DIALOG_LIBEDIT_DIMENSIONS::DIALOG_LIBEDIT_DIMENSIONS( WinEDA_LibeditFrame* parent )
    : DIALOG_LIBEDIT_DIMENSIONS_BASE( parent )
{
    GetSizer()->SetSizeHints(this);
	Centre( wxBOTH );
}

DIALOG_LIBEDIT_DIMENSIONS::~DIALOG_LIBEDIT_DIMENSIONS()
{
}

void DIALOG_LIBEDIT_DIMENSIONS::initDialog()
{
    SetFocus( );
}


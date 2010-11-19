/*
 * dialog_libedit_dimensions.cpp
 * Handles the dialog so set current texts and pins sizes in LibEdit
 */
#include "fctsys.h"
#include "common.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "libeditframe.h"

#include "dialog_libedit_dimensions.h"


DIALOG_LIBEDIT_DIMENSIONS::DIALOG_LIBEDIT_DIMENSIONS( LIB_EDIT_FRAME* parent )
    : DIALOG_LIBEDIT_DIMENSIONS_BASE( parent )
{
    GetSizer()->SetSizeHints( this );
	Centre( wxBOTH );
}


DIALOG_LIBEDIT_DIMENSIONS::~DIALOG_LIBEDIT_DIMENSIONS()
{
}


void DIALOG_LIBEDIT_DIMENSIONS::initDialog()
{
    SetFocus();
}

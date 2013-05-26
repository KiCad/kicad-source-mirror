
#ifndef _DIALOG_LIBEDIT_DIMENSIONS_H_
#define _DIALOG_LIBEDIT_DIMENSIONS_H_


#include <dialog_libedit_dimensions_base.h>


class LIB_EDIT_FRAME;


class DIALOG_LIBEDIT_DIMENSIONS : public DIALOG_LIBEDIT_DIMENSIONS_BASE
{
public:
    DIALOG_LIBEDIT_DIMENSIONS( LIB_EDIT_FRAME* parent );
    ~DIALOG_LIBEDIT_DIMENSIONS();

private:
    void initDialog( );
};


#endif    // _DIALOG_LIBEDIT_DIMENSIONS_H_

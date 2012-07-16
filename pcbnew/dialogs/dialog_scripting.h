/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_scripting.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_SCRIPTING_H_
#define _DIALOG_SCRIPTING_H_

#include <dialog_scripting_base.h>

class DIALOG_SCRIPTING: public DIALOG_SCRIPTING_BASE
{
private:
    wxDialog * m_Parent;

public:
    DIALOG_SCRIPTING(wxWindow * parent );

private:
    void OnRunButtonClick( wxCommandEvent& event );
};

#endif  // _DIALOG_SCRIPTING_H_

/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_global_deletion.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_GLOBAL_DELETION_H_
#define _DIALOG_GLOBAL_DELETION_H_

#include "dialog_global_deletion_base.h"

class DIALOG_GLOBAL_DELETION: public DIALOG_GLOBAL_DELETION_BASE
{
private:
	WinEDA_PcbFrame * m_Parent;

public:
    DIALOG_GLOBAL_DELETION( WinEDA_PcbFrame* parent );

private:
    void OnOkClick( wxCommandEvent& event )
    {
        AcceptPcbDelete();
        EndModal(wxID_OK);
    }
    void OnCancelClick( wxCommandEvent& event )
    {
        EndModal(wxID_CANCEL);
    }

	void AcceptPcbDelete();

};

#endif
    // _DIALOG_GLOBAL_DELETION_H_

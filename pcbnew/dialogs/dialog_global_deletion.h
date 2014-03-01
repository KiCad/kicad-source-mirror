/////////////////////////////////////////////////////////////////////////////
// Name:        dialog_global_deletion.h
/////////////////////////////////////////////////////////////////////////////

#ifndef _DIALOG_GLOBAL_DELETION_H_
#define _DIALOG_GLOBAL_DELETION_H_

#include <dialog_global_deletion_base.h>

class DIALOG_GLOBAL_DELETION: public DIALOG_GLOBAL_DELETION_BASE
{
private:
    PCB_EDIT_FRAME * m_Parent;
    LAYER_NUM m_currentLayer;

public:
    DIALOG_GLOBAL_DELETION( PCB_EDIT_FRAME* parent );
    void SetCurrentLayer( LAYER_NUM aLayer );

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
    void OnCheckDeleteTracks( wxCommandEvent& event );
    void OnCheckDeleteModules( wxCommandEvent& event );
};

#endif  // _DIALOG_GLOBAL_DELETION_H_

#ifndef DIALOG_PCB_TEXT_PROPERTIES_H
#define DIALOG_PCB_TEXT_PROPERTIES_H

#include <vector>
#include <wx/wx.h>
#include "dialog_pcb_text_properties_base.h"

class PCB_EDIT_FRAME;
class TEXTE_PCB;

/** Implementing DIALOG_PCB_TEXT_PROPERTIES_BASE */
class DIALOG_PCB_TEXT_PROPERTIES : public DIALOG_PCB_TEXT_PROPERTIES_BASE
{
private:
    PCB_EDIT_FRAME*      m_Parent;
    wxDC*                m_DC;
    TEXTE_PCB*           m_SelectedPCBText;
    std::vector<int>     layerList;

    void MyInit();

protected:
    // Handlers for DIALOG_PCB_TEXT_PROPERTIES_BASE events.
    void OnClose( wxCloseEvent& event );
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );

public:
    DIALOG_PCB_TEXT_PROPERTIES( PCB_EDIT_FRAME* parent, TEXTE_PCB* passedTextPCB, wxDC* DC );
};

#endif // DIALOG_PCB_TEXT_PROPERTIES_H

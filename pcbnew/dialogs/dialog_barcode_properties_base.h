///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 20 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <pcb_base_frame.h>
#include <pcb_draw_panel_gal.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BARCODE_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BARCODE_PROPERTIES_BASE : public DIALOG_SHIM
{
private:

protected:
    enum
    {
        wxID_DIALOG_EDIT_PAD = 1000
    };

    wxStaticText* m_staticText28;
    wxTextCtrl* m_textCtrl11;
    wxStaticLine* m_staticline2;
    wxStaticText* m_staticText44;
    wxChoice* m_PadType;
    wxStaticText* m_posXLabel;
    wxTextCtrl* m_posXCtrl;
    wxStaticText* m_posXUnits;
    wxStaticText* m_posYLabel;
    wxTextCtrl* m_posYCtrl;
    wxStaticText* m_posYUnits;
    wxStaticText* m_sizeXLabel;
    wxTextCtrl* m_sizeXCtrl;
    wxStaticText* m_sizeXUnits;
    wxStaticText* m_sizeYLabel;
    wxTextCtrl* m_sizeYCtrl;
    wxStaticText* m_sizeYUnits;
    wxStaticText* m_PadOrientText;
    wxComboBox* m_orientation;
    wxStaticText* m_staticText491;
    wxStaticText* m_offsetXLabel;
    wxTextCtrl* m_offsetXCtrl;
    wxStaticText* m_offsetXUnits;
    wxStaticText* m_offsetYLabel;
    wxTextCtrl* m_offsetYCtrl;
    wxStaticText* m_offsetYUnits;
    wxBoxSizer* m_middleBoxSizer;
    wxCheckBox* m_checkBox1;
    wxRadioBox* m_barcode;
    wxRadioBox* m_errorCorrection;
    PCB_DRAW_PANEL_GAL* m_panelShowBarcodeGal;
    KIGFX::GAL_DISPLAY_OPTIONS m_galOptions;
    wxStaticLine* m_staticline13;
    wxStdDialogButtonSizer* m_sdbSizer;
    wxButton* m_sdbSizerOK;
    wxButton* m_sdbSizerCancel;

    // Virtual event handlers, overide them in your derived class
    virtual void OnInitDialog( wxInitDialogEvent& event ) { event.Skip(); }
    virtual void PadTypeSelected( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
    virtual void PadOrientEvent( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


public:

    DIALOG_BARCODE_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Barcode Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 764,581 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
    ~DIALOG_BARCODE_PROPERTIES_BASE();

};


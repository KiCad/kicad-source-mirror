///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class FONT_CHOICE;
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/textctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TABLECELL_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TABLECELL_PROPERTIES_BASE : public DIALOG_SHIM
{
private:
protected:
    WX_INFOBAR*             m_infoBar;
    wxStaticText*           m_cellTextLabel;
    wxStyledTextCtrl*       m_cellTextCtrl;
    wxStaticText*           m_fontLabel;
    FONT_CHOICE*            m_fontCtrl;
    BITMAP_BUTTON*          m_bold;
    BITMAP_BUTTON*          m_italic;
    BITMAP_BUTTON*          m_separator0;
    BITMAP_BUTTON*          m_hAlignLeft;
    BITMAP_BUTTON*          m_hAlignCenter;
    BITMAP_BUTTON*          m_hAlignRight;
    BITMAP_BUTTON*          m_separator1;
    BITMAP_BUTTON*          m_vAlignTop;
    BITMAP_BUTTON*          m_vAlignCenter;
    BITMAP_BUTTON*          m_vAlignBottom;
    wxStaticText*           m_SizeXLabel;
    wxTextCtrl*             m_SizeXCtrl;
    wxStaticText*           m_SizeXUnits;
    wxStaticText*           m_SizeYLabel;
    wxTextCtrl*             m_SizeYCtrl;
    wxStaticText*           m_SizeYUnits;
    wxStaticText*           m_ThicknessLabel;
    wxTextCtrl*             m_ThicknessCtrl;
    wxStaticText*           m_ThicknessUnits;
    BITMAP_BUTTON*          m_autoTextThickness;
    wxTextCtrl*             m_marginTopCtrl;
    wxStaticText*           m_marginTopUnits;
    wxTextCtrl*             m_marginLeftCtrl;
    wxTextCtrl*             m_marginRightCtrl;
    wxTextCtrl*             m_marginBottomCtrl;
    wxButton*               m_editTable;
    wxHyperlinkCtrl*        m_syntaxHelp;
    wxStdDialogButtonSizer* m_sdbSizer1;
    wxButton*               m_sdbSizer1OK;
    wxButton*               m_sdbSizer1Cancel;

    // Virtual event handlers, override them in your derived class
    virtual void onBoldToggle( wxCommandEvent& event ) { event.Skip(); }
    virtual void onTextSize( wxCommandEvent& event ) { event.Skip(); }
    virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
    virtual void onThickness( wxCommandEvent& event ) { event.Skip(); }
    virtual void onAutoTextThickness( wxCommandEvent& event ) { event.Skip(); }
    virtual void onEditTable( wxCommandEvent& event ) { event.Skip(); }
    virtual void onSyntaxHelp( wxHyperlinkEvent& event ) { event.Skip(); }


public:
    DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY,
                                      const wxString& title = _( "Table Cell Properties" ),
                                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1, -1 ),
                                      long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    ~DIALOG_TABLECELL_PROPERTIES_BASE();
};

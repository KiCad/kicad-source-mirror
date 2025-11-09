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
class COLOR_SWATCH;
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
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/panel.h>
#include <wx/simplebook.h>
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
    wxHyperlinkCtrl*        m_syntaxHelp;
    BITMAP_BUTTON*          m_hAlignLeft;
    BITMAP_BUTTON*          m_hAlignCenter;
    BITMAP_BUTTON*          m_hAlignRight;
    wxStaticText*           vAlignLabel;
    BITMAP_BUTTON*          m_vAlignTop;
    BITMAP_BUTTON*          m_vAlignCenter;
    BITMAP_BUTTON*          m_vAlignBottom;
    wxStaticText*           m_styleLabel;
    wxCheckBox*             m_bold;
    wxCheckBox*             m_italic;
    wxStaticText*           m_fontLabel;
    FONT_CHOICE*            m_fontCtrl;
    wxStaticText*           m_textSizeLabel;
    wxTextCtrl*             m_textSizeCtrl;
    wxStaticText*           m_textSizeUnits;
    wxStaticText*           m_textColorLabel;
    wxSimplebook*           m_textColorBook;
    wxChoice*               m_textColorPopup;
    wxPanel*                textColorSwatchPanel;
    wxPanel*                m_panelTextColor;
    COLOR_SWATCH*           m_textColorSwatch;
    wxStaticText*           m_fillColorLabel;
    wxSimplebook*           m_fillColorBook;
    wxChoice*               m_fillColorPopup;
    wxPanel*                fillColorSwatchPanel;
    wxPanel*                m_panelFillColor;
    COLOR_SWATCH*           m_fillColorSwatch;
    wxTextCtrl*             m_marginTopCtrl;
    wxStaticText*           m_marginTopUnits;
    wxTextCtrl*             m_marginLeftCtrl;
    wxTextCtrl*             m_marginRightCtrl;
    wxTextCtrl*             m_marginBottomCtrl;
    wxButton*               m_editTable;
    wxStdDialogButtonSizer* m_sdbSizer1;
    wxButton*               m_sdbSizer1OK;
    wxButton*               m_sdbSizer1Cancel;

    // Virtual event handlers, override them in your derived class
    virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
    virtual void OnFormattingHelp( wxHyperlinkEvent& event ) { event.Skip(); }
    virtual void onTextColorPopup( wxCommandEvent& event ) { event.Skip(); }
    virtual void onFillColorPopup( wxCommandEvent& event ) { event.Skip(); }
    virtual void onEditTable( wxCommandEvent& event ) { event.Skip(); }


public:
    DIALOG_TABLECELL_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY,
                                      const wxString& title = _( "Table Cell Properties" ),
                                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1, -1 ),
                                      long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    ~DIALOG_TABLECELL_PROPERTIES_BASE();
};

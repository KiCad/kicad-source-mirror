///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 25 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include "dialog_shim.h"
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/grid.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOARD_STATISTICS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOARD_STATISTICS_BASE : public DIALOG_SHIM
{
    private:

    protected:
        wxGrid* m_gridComponents;
        wxGrid* m_gridPads;
        wxGrid* m_gridBoard;
        wxCheckBox* m_checkBoxSubtractHoles;
        wxCheckBox* m_checkBoxExcludeComponentsNoPins;
        wxStdDialogButtonSizer* m_sdbControlSizer;
        wxButton* m_sdbControlSizerOK;

        // Virtual event handlers, overide them in your derived class
        virtual void checkboxClicked( wxCommandEvent& event ) { event.Skip(); }


    public:

        DIALOG_BOARD_STATISTICS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Board Statistics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );
        ~DIALOG_BOARD_STATISTICS_BASE();

};


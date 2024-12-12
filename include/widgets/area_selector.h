#pragma once

#include <widgets/filter_combobox.h>

class BOARD;

class AREA_SELECTOR_POPUP;

class AREA_SELECTOR : public FILTER_COMBOBOX
{
public:
    AREA_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize, long style = 0 );

    void SetBoard( BOARD* aBoard );
    void SetSelectedArea( const wxString& aName );
    wxString GetSelectedArea();

private:
    AREA_SELECTOR_POPUP* m_popup;
};


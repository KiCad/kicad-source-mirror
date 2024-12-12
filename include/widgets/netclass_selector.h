#pragma once

#include <widgets/filter_combobox.h>

class BOARD;

class NETCLASS_SELECTOR_POPUP;

class NETCLASS_SELECTOR : public FILTER_COMBOBOX
{
public:
    NETCLASS_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize, long style = 0 );

    void SetBoard( BOARD* aBoard );

    void SetSelectedNetclass( const wxString& aName );
    wxString GetSelectedNetclass();

private:
    NETCLASS_SELECTOR_POPUP* m_popup;
};


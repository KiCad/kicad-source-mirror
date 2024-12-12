#include "widgets/netclass_selector.h"

#include <algorithm>

#include <board.h>
#include <board_design_settings.h>
#include <netclass.h>
#include <project/net_settings.h>

class NETCLASS_SELECTOR_POPUP : public FILTER_COMBOPOPUP
{
public:
    NETCLASS_SELECTOR_POPUP() : m_board( nullptr ) {}

    void SetBoard( BOARD* aBoard )
    {
        m_board = aBoard;
        rebuildList();
    }

protected:
    void getListContent( wxArrayString& aList ) override
    {
        aList.clear();

        if( !m_board )
            return;

        auto netclasses = m_board->GetDesignSettings().m_NetSettings->GetNetclasses();

        for( const auto& [name, cls] : netclasses )
            aList.push_back( name );

        aList.Sort();
    }

private:
    BOARD* m_board;
};

NETCLASS_SELECTOR::NETCLASS_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                      const wxSize& size, long style )
        : FILTER_COMBOBOX( parent, id, pos, size, style|wxCB_READONLY )
{
    m_popup = new NETCLASS_SELECTOR_POPUP();
    setFilterPopup( m_popup );
}

void NETCLASS_SELECTOR::SetBoard( BOARD* aBoard )
{
    m_popup->SetBoard( aBoard );
}

void NETCLASS_SELECTOR::SetSelectedNetclass( const wxString& aName )
{
    m_popup->SetSelectedString( aName );
}

wxString NETCLASS_SELECTOR::GetSelectedNetclass()
{
    return m_popup->GetStringValue();
}


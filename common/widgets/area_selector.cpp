#include "widgets/area_selector.h"

#include <algorithm>

#include <board.h>
#include <zone.h>

class AREA_SELECTOR_POPUP : public FILTER_COMBOPOPUP
{
public:
    AREA_SELECTOR_POPUP() : m_board( nullptr ) {}

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

        for( ZONE* zone : m_board->Zones() )
        {
            if( zone->GetIsRuleArea() && !zone->GetZoneName().IsEmpty() )
                aList.push_back( zone->GetZoneName() );
        }

        aList.Sort();
    }

private:
    BOARD* m_board;
};

AREA_SELECTOR::AREA_SELECTOR( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                              const wxSize& size, long style )
        : FILTER_COMBOBOX( parent, id, pos, size, style|wxCB_READONLY )
{
    m_popup = new AREA_SELECTOR_POPUP();
    setFilterPopup( m_popup );
}

void AREA_SELECTOR::SetBoard( BOARD* aBoard )
{
    m_popup->SetBoard( aBoard );
}

void AREA_SELECTOR::SetSelectedArea( const wxString& aName )
{
    m_popup->SetSelectedString( aName );
}

wxString AREA_SELECTOR::GetSelectedArea()
{
    return m_popup->GetStringValue();
}


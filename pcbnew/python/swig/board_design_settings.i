%ignore BOARD_DESIGN_SETTINGS::m_Pad_Master;
%ignore BOARD_DESIGN_SETTINGS::m_DRCEngine;
%ignore NET_SETTINGS::m_netClassPatternAssignments;
%ignore NET_SETTINGS::m_netClassLabelAssignments;

%shared_ptr(NET_SETTINGS)
%template(netclasses_map) std::map<wxString, std::shared_ptr<NETCLASS>>;

%{
#include <board_design_settings.h>
#include <project/net_settings.h>
%}

%include <board_design_settings.h>
%include <project/net_settings.h>

%extend BOARD_DESIGN_SETTINGS
{
    void CloneFrom( BOARD_DESIGN_SETTINGS* aOther )
    {
        *$self = *aOther;
    }
}

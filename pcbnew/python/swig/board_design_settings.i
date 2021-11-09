%ignore BOARD_DESIGN_SETTINGS::m_Pad_Master;
%ignore BOARD_DESIGN_SETTINGS::m_DRCEngine;

%{
#include <board_design_settings.h>
%}

%include <board_design_settings.h>

%extend BOARD_DESIGN_SETTINGS
{
    void CloneFrom( BOARD_DESIGN_SETTINGS* aOther )
    {
        *$self = *aOther;
    }
}

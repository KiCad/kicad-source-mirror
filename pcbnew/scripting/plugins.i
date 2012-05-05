
%{
#include <scripting/pcbnew_footprint_wizards.h>
%}

class FOOTPRINT_WIZARDS 
{
public:
    static void register_wizard(PyObject *wizard);

};

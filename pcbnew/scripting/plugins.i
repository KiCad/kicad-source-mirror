
%{
#include <scripting/pcbnew_footprint_wizards.h>
%}

class PYTHON_FOOTPRINT_WIZARDS 
{
public:
    static void register_wizard(PyObject *wizard);

};

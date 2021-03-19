
%warnfilter(325) NETINFO_MAPPING::iterator;
%ignore NETINFO_MAPPING;        // no code generation for this class

%feature("notabstract")     NETINFO_ITEM;

%include netinfo.h

%{
#include <netinfo.h>
%}

// http://swig.10945.n7.nabble.com/std-containers-and-pointers-td3728.html
%{
    namespace swig {
        template <>  struct traits<NETINFO_ITEM> {
            typedef pointer_category category;
            static const char* type_name() { return "NETINFO_ITEM"; }
        };
    }
%}

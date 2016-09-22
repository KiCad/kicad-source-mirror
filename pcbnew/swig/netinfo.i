
%warnfilter(325) NETINFO_MAPPING::iterator;
%ignore NETINFO_MAPPING;        // no code generation for this class

%include class_netinfo.h

%{
#include <class_netinfo.h>
%}

%feature("notabstract")     NETINFO_ITEM;

// http://swig.10945.n7.nabble.com/std-containers-and-pointers-td3728.html
%{
    namespace swig {
        template <>  struct traits<NETINFO_ITEM> {
            typedef pointer_category category;
            static const char* type_name() { return "NETINFO_ITEM"; }
        };
    }
%}

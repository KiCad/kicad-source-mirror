// Do not permit default ZONE_FILLER ctor since commits are not supported from Python
%ignore ZONE_FILLER::ZONE_FILLER(BOARD*, COMMIT*);

%include zone.h
%include zones.h

%{
#include <zone.h>
#include <zones.h>
#include <zone_filler.h>
%}

// Provide a compatiblity ctor for ZONE_FILLER that doesn't need a COMMIT
%include zone_filler.h
%extend ZONE_FILLER
{
    ZONE_FILLER( BOARD* aBoard )
    {
        return new ZONE_FILLER( aBoard, nullptr );
    }
}

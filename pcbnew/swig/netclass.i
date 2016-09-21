
/*

NETCLASS is not exposed/generated via SWIG but rather class NETCLASSPTR is. This
is because we can never have a simple pointer to a NETCLASS, only can we have a
NETCLASSPTR. So we cannot build a python wrapper without using NETCLASSPTR to do
the pointing.

*/

%ignore NETCLASS;               // no code generation for it
%include class_netclass.h


/*

Propagate important member functions from NETCLASS into SWIG's NETCLASSPTR. Do
this by dereferencing the pointer to the NETCLASSPTR proxy, then dereference the
std::share_ptr to get to the actual NETCLASS::member. Complete the member
function set if you see something missing from class NETCLASS that you need.

*/

//%extend NETCLASSPTR   // would have thought the typedef in header above was golden here.
%extend std::shared_ptr<NETCLASS>
{
public:

    STRINGSET&  NetNames()                  { return (*self)->NetNames(); }

    const wxString& GetName()               { return (*self)->GetName(); }

    unsigned GetCount() const               { return (*self)->GetCount(); }

    const wxString& GetDescription() const          { return (*self)->GetDescription(); }
    void    SetDescription( const wxString& aDesc ) { (*self)->SetDescription( aDesc ); }

    int     GetClearance() const            { return (*self)->GetClearance(); }
    void    SetClearance( int aClearance )  { (*self)->SetClearance( aClearance ); }

    int     GetTrackWidth() const           { return (*self)->GetTrackWidth(); }
    void    SetTrackWidth( int aWidth )     { (*self)->SetTrackWidth( aWidth ); }

    int     GetViaDiameter() const          { return (*self)->GetViaDiameter(); }
    void    SetViaDiameter( int aDia )      { (*self)->SetViaDiameter( aDia ); }

    int     GetViaDrill() const             { return (*self)->GetViaDrill(); }
    void    SetViaDrill( int aSize )        { (*self)->SetViaDrill( aSize ); }

    int     GetuViaDiameter() const         { return (*self)->GetuViaDiameter(); }
    void    SetuViaDiameter( int aSize )    { (*self)->SetuViaDiameter( aSize ); }

    int     GetuViaDrill() const            { return (*self)->GetuViaDrill(); }
    void    SetuViaDrill( int aSize )       { (*self)->SetuViaDrill( aSize ); }

    int     GetDiffPairWidth() const        { return (*self)->GetDiffPairWidth(); }
    void    SetDiffPairWidth( int aSize )   { (*self)->SetDiffPairWidth( aSize ); }

    int     GetDiffPairGap() const          { return (*self)->GetDiffPairGap(); }
    void    SetDiffPairGap( int aSize )     { (*self)->SetDiffPairGap( aSize ); }
};


%{
#include <class_netclass.h>
%}

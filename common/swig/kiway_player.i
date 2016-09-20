
// Swig interface to classes KIWAY_PLAYER and KIWAY_HOLDER

class wxFrame
{
};

class KIWAY_PLAYER : public wxFrame
{
};

%ignore wxFrame;
%ignore KIWAY_PLAYER;

//%include kiway_player.h

%{
#include <kiway_player.h>
%}


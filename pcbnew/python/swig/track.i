

%include track.h
%rename(Get) operator   TRACK*;
%{
#include <track.h>
%}


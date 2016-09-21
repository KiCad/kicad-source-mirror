

%include class_track.h
%rename(Get) operator   TRACK*;
%template(TRACK_List)   DLIST<TRACK>;
%{
#include <class_track.h>
%}




%include class_track.h
%rename(Get) operator   TRACK*;
%{
#include <class_track.h>
%}


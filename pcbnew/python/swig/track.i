

%include pcb_track.h
%rename(Get) operator   PCB_TRACK*;
%{
#include <pcb_track.h>
%}


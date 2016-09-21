
%include pad_shapes.h
%include class_pad.h

%rename(Get) operator   D_PAD*;
%template(PAD_List)     DLIST<D_PAD>;
%{
#include <class_pad.h>
%}


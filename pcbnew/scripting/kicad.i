%module kicad

%nodefaultctor EDA_ITEM;
%ignore InitKiCadAbout;
%ignore GetCommandOptions;

%{
	#include <dlist.h>
	#include <base_struct.h>
	#include <common.h>

%}

%include <dlist.h>
%include <base_struct.h>
%include <common.h>




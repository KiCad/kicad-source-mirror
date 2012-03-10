%module kicad

%nodefaultctor EDA_ITEM;


/* swig tries to wrap SetBack/SetNext on derived classes, but this method is
   private for most childs, so if we don't ignore it it won't compile */

%ignore EDA_ITEM::SetBack;
%ignore EDA_ITEM::SetNext;


%ignore InitKiCadAbout;
%ignore GetCommandOptions;

%{
	#include <dlist.h>
	#include <base_struct.h>
	#include <common.h>
	#include <wx_helpers.h>

%}

%include <dlist.h>
%include <base_struct.h>
%include <common.h>


%include <wx.i>




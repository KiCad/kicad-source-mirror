//%module kicad

/* OFF NOW, it triggers an error with GCC 4.6 and swig-2.0.4 or trunk.. 
   http://sourceforge.net/tracker/index.php?func=detail&aid=3391906&group_id=1645&atid=101645

   %include <std_vector.i>
   %include <std_string.i>
*/
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
	#include <wx_python_helpers.h>
	#include <cstddef>
        #include <vector>
	using namespace std;

%}

%include <dlist.h>
%include <base_struct.h>
%include <common.h>

/* all the wx wrappers for wxString, wxPoint, wxRect, wxChar .. */
%include <wx.i>

/*
namespace std 
{
	%template(intVector) vector<int>;
}
*/

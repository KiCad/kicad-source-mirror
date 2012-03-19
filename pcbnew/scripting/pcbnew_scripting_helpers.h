#ifndef __PCBNEW_SCRIPTING_HELPERS_H
#define __PCBNEW_SCRIPTING_HELPERS_H

#include <wxPcbStruct.h>

/* we could be including all these methods as static in a class, but
 * we want plain pcbnew.<method_name> access from python */

#ifndef SWIG
void ScriptingSetPcbEditFrame(PCB_EDIT_FRAME *aPCBEdaFrame);
BOARD *GetBoard();
#endif 

BOARD* LoadBoard(wxString aFileName);
bool SaveBoard(wxString aFileName, BOARD* aBoard);

#endif
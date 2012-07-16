#ifndef __PCBNEW_SCRIPTING_HELPERS_H
#define __PCBNEW_SCRIPTING_HELPERS_H

#include <wxPcbStruct.h>
#include <io_mgr.h>
/* we could be including all these methods as static in a class, but
 * we want plain pcbnew.<method_name> access from python */

#ifndef SWIG
void ScriptingSetPcbEditFrame(PCB_EDIT_FRAME *aPCBEdaFrame);
#endif 

BOARD *GetBoard();

BOARD* LoadBoard(wxString& aFileName, IO_MGR::PCB_FILE_T aFormat);
BOARD* LoadBoard(wxString& aFileName);
bool SaveBoard(wxString& aFileName, BOARD* aBoard, IO_MGR::PCB_FILE_T aFormat);
bool SaveBoard(wxString& aFileName, BOARD* aBoard);


#endif
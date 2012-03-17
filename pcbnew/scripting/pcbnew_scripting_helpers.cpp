#include <pcbnew_scripting_helpers.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <build_version.h>
#include <class_board.h>
#include <kicad_string.h>
#include <io_mgr.h>

static PCB_EDIT_FRAME *PcbEditFrame=NULL;

BOARD *GetBoard()
{
	if (PcbEditFrame) return PcbEditFrame->GetBoard();
	else return NULL;
}

void ScriptingSetPcbEditFrame(PCB_EDIT_FRAME *aPCBEdaFrame)
{
	PcbEditFrame = aPCBEdaFrame;
}


BOARD* LoadBoard(wxString aFileName)
{
#ifdef USE_NEW_PCBNEW_LOAD
	try{
	   return IO_MGR::Load(IO_MGR::KICAD,aFileName);	
	} catch (IO_ERROR)
	{
		return NULL;
	}
#else
  fprintf(stderr,"Warning, LoadBoard not implemented without USE_NEW_PCBNEW_LOAD\n");
	return NULL;
#endif
}

bool SaveBoard(wxString aFileName, BOARD* aBoard)
{

#ifdef USE_NEW_PCBNEW_LOAD
	aBoard->m_Status_Pcb &= ~CONNEXION_OK;
  aBoard->SynchronizeNetsAndNetClasses();
  aBoard->SetCurrentNetClass( aBoard->m_NetClasses.GetDefault()->GetName() );

  wxString header = wxString::Format(
                            wxT( "PCBNEW-BOARD Version %d date %s\n\n# Created by Pcbnew%s\n\n" ),
                            BOARD_FILE_VERSION, DateAndTime().GetData(),
                            GetBuildVersion().GetData() );

  PROPERTIES   props;

  props["header"] = header;

	try 
	{
     IO_MGR::Save( IO_MGR::KICAD, aFileName, aBoard, &props );
     return true;
  } 
  catch (IO_ERROR)
  {
  	 return false;
  }

#else
	fprintf(stderr,"Warning, SaveBoard not implemented without USE_NEW_PCBNEW_LOAD\n");
	return false;
#endif
	
}



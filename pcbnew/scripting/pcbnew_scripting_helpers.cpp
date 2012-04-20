/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 NBEE Embedded Systems, Miguel Angel Ajo <miguelangel@nbee.es>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcbnew_scripting_helpers.cpp
 * @brief Scripting helper functions for pcbnew functionality
 */

#include <Python.h>

#include <pcbnew_scripting_helpers.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <build_version.h>
#include <class_board.h>
#include <kicad_string.h>
#include <io_mgr.h>
#include <macros.h>
#include <stdlib.h>

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

BOARD* LoadBoard(wxString& aFileName)
{
    return LoadBoard(aFileName,IO_MGR::KICAD);
}

BOARD* LoadBoard(wxString& aFileName,IO_MGR::PCB_FILE_T aFormat)
{
	static char ExceptionError[256];
#ifdef USE_NEW_PCBNEW_LOAD
	try{
	   return IO_MGR::Load(aFormat,aFileName);	
	} catch (IO_ERROR e)
	{
		sprintf(ExceptionError, "%s\n", TO_UTF8(e.errorText) );
		PyErr_SetString(PyExc_IOError,ExceptionError);
		return NULL;
	}
#else
  fprintf(stderr,"Warning, LoadBoard not implemented without USE_NEW_PCBNEW_LOAD\n");
   return NULL;
#endif
}

bool SaveBoard(wxString& aFilename, BOARD* aBoard)
{
    return SaveBoard(aFilename,aBoard,IO_MGR::KICAD);
}

bool SaveBoard(wxString& aFileName, BOARD* aBoard,
                IO_MGR::PCB_FILE_T aFormat)
{
  static char ExceptionError[256];
#ifdef USE_NEW_PCBNEW_LOAD
  aBoard->m_Status_Pcb &= ~CONNEXION_OK;
  aBoard->SynchronizeNetsAndNetClasses();
  aBoard->SetCurrentNetClass( aBoard->m_NetClasses.GetDefault()->GetName() );

  wxString header;
  PROPERTIES   props;
  
  if (aFormat==IO_MGR::LEGACY)
  {
      header = wxString::Format(
                            wxT( "PCBNEW-BOARD Version %d date %s\n\n# Created by Pcbnew%s scripting\n\n" ),
                            LEGACY_BOARD_FILE_VERSION, DateAndTime().GetData(),
                            GetBuildVersion().GetData() );
      props["header"] = header;
  }
  

  try 
  {
     IO_MGR::Save( aFormat, aFileName, aBoard, &props );
     return true;
  } 
  catch (IO_ERROR e)
  {
	sprintf(ExceptionError, "%s\n", TO_UTF8(e.errorText) );
	PyErr_SetString(PyExc_IOError,ExceptionError);

        return false;
  }

#else
	fprintf(stderr,"Warning, SaveBoard not implemented without USE_NEW_PCBNEW_LOAD\n");
	return false;
#endif
	
}



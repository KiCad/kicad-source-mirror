		/**************************************/
		/**	protos.h     liste des fonctions **/
		/**************************************/

#ifndef PROTOS_H
#define PROTOS_H

int GenNetlistPcbnew() ;
int loadcmp() ;
int listlib() ;
STOREMOD * GetModuleDescrByName(const wxString & FootprintName);


	/***********/
	/* CFG.CPP */
	/***********/
void Save_Config(wxWindow * parent);
void Read_Config( const wxString & FullFileName );  /* lit la configuration */


	/***************/
	/* MEMOIRE.CPP */
	/***************/
void FreeMemoryComponents();
	/* Routine de liberation memoire de la liste des composants
		 - remet a NULL BaseListeMod
		 - remet a 0 NbComp */

void FreeMemoryModules();
	/* Routine de liberation memoire de la liste des modules
		 - remet a NULL g_BaseListePkg
		 - rement a 0 NbLib; */

#endif	// PROTOS_H


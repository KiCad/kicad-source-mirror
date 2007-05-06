	/****************************/
	/*	EESchema - libalias.cpp	*/
	/****************************/

/* Routines de maintenanace des librairies: gestion des alias des composants
*/

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/* Variables locales */


/**************************************************************************/
bool BuildAliasData(LibraryStruct * Lib, EDA_LibComponentStruct * component)
/**************************************************************************/
/* Create the alias data for the lib component to edit
	Alias data is:
	alias name
	doc string
	keywords string
	doc file name
	
	in .m_AliastList
	Alias data (4 strings) replace each alias name.
	
*/
{
wxArrayString List;
LibCmpEntry * CmpEntry;
unsigned ii;

	if ( component == NULL ) return FALSE;
	if( Lib == NULL ) return FALSE;
	if( component->m_AliasList.GetCount() == 0 ) return FALSE;
		
	List = component->m_AliasList;
	component->m_AliasList.Clear();
	for ( ii = 0; ii < List.GetCount(); ii++ )
	{
		CmpEntry = (LibCmpEntry*) FindLibPart(List[ii].GetData(), Lib->m_Name, FIND_ALIAS);
		if(CmpEntry && CmpEntry->Type != ALIAS )
		{
			DisplayError(NULL, wxT("BuildListAlias err: alias is a ROOT entry!"));
		}
		else
		{
			if( LocateAlias( component->m_AliasList, List[ii] ) < 0 )
			{	/* Alias not found in list: create it (datas must be in this order) */
				component->m_AliasList.Add(List[ii]);
				component->m_AliasList.Add(CmpEntry->m_Doc);
				component->m_AliasList.Add(CmpEntry->m_KeyWord);
				component->m_AliasList.Add(CmpEntry->m_DocFile);
			}
		}
	}
	return TRUE;
}


/***********************************************************************/
int LocateAlias( const wxArrayString & AliasData, const wxString & Name)
/***********************************************************************/
/* Return an index in alias data list
	( -1 if not found )
*/
{
int index = -1;
unsigned ii;
	
	for ( ii = 0; ii < AliasData.GetCount(); ii += ALIAS_NEXT)
	{
		if ( Name.CmpNoCase(AliasData[ii+ALIAS_NAME].GetData()) == 0 )
		{
			index = ii;
			break;
		}
	}
	return(index);
}

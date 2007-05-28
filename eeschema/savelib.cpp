	/****************************/
	/*	EESchema - eesavlib.cpp	*/
	/****************************/

/* Write Routines to save schematic libraries and library components (::WriteDescr() members)
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/* Routines locales */

/* Variables locales */


static bool WriteLibEntryDateAndTime(FILE * ExportFile,
			EDA_LibComponentStruct * LibEntry);


static int  fill_tab[3] = { 'N', 'F', 'f' };

/***********************************************/
bool LibDrawArc::WriteDescr( FILE * ExportFile )
/***********************************************/
/* format
A centre_posx centre_posy rayon start_angle end_angle unit convert fill('N', 'F' ou 'f') startx starty endx endy
*/
{	
int x1 = t1; if(x1 > 1800) x1 -= 3600;
int x2 = t2; if(x2 > 1800) x2 -= 3600;
 
	fprintf(ExportFile,"A %d %d %d %d %d %d %d %d %c %d %d %d %d\n",
			m_Pos.x, m_Pos.y,
			m_Rayon, x1, x2,
			m_Unit,m_Convert,
			m_Width, fill_tab[m_Fill],
			m_ArcStart.x, m_ArcStart.y, m_ArcEnd.x, m_ArcEnd.y);
    return FALSE;
}

/***************************************************/
bool LibDrawCircle::WriteDescr( FILE * ExportFile )
/***************************************************/
{                
	fprintf(ExportFile,"C %d %d %d %d %d %d %c\n",
                m_Pos.x, m_Pos.y,
				m_Rayon,
				m_Unit,m_Convert,
				m_Width, fill_tab[m_Fill]);
    return FALSE;
}

/************************************************/
bool LibDrawText::WriteDescr( FILE * ExportFile )
/************************************************/
{
wxString text = m_Text;
	
	text.Replace( wxT(" "), wxT("~") );	// Spaces are not allowed: changed to '~'
	
	fprintf(ExportFile,"T %d %d %d %d %d %d %d %s\n",
				m_Horiz,
				m_Pos.x, m_Pos.y,
				m_Size.x, m_Type,
				m_Unit,m_Convert,
				CONV_TO_UTF8(text) );
    return FALSE;
}

/***************************************************/
bool LibDrawSquare::WriteDescr( FILE * ExportFile )
/***************************************************/
{
	fprintf(ExportFile,"S %d %d %d %d %d %d %d %c\n",
                m_Pos.x, m_Pos.y, m_End.x, m_End.y,
				m_Unit,m_Convert,
				m_Width, fill_tab[m_Fill]);
    return FALSE;
}


/************************************************/
bool LibDrawPin::WriteDescr( FILE * ExportFile )
/************************************************/
{
int	Etype = 'I';
wxString StringPinNum;
    
	switch(m_PinType)
		{
		case PIN_INPUT: Etype = 'I'; break;
		case PIN_OUTPUT: Etype = 'O'; break;
		case PIN_BIDI: Etype = 'B'; break;
		case PIN_TRISTATE: Etype = 'T'; break;
		case PIN_PASSIVE: Etype = 'P'; break;
		case PIN_UNSPECIFIED: Etype = 'U'; break;
		case PIN_POWER_IN: Etype = 'W'; break;
		case PIN_POWER_OUT: Etype = 'w'; break;
		case PIN_OPENCOLLECTOR: Etype = 'C'; break;
		case PIN_OPENEMITTER:	Etype = 'E'; break;
		}
        
	ReturnPinStringNum(StringPinNum);
	if ( StringPinNum.IsEmpty() ) StringPinNum = wxT("~");
	
    if( ! m_PinName.IsEmpty() )
		fprintf(ExportFile,"X %s", CONV_TO_UTF8(m_PinName) );
	else fprintf(ExportFile,"X ~");

	fprintf(ExportFile," %s %d %d %d %c %d %d %d %d %c",
					CONV_TO_UTF8(StringPinNum),
					m_Pos.x, m_Pos.y,
					(int)m_PinLen, (int)m_Orient,
					m_PinNumSize, m_PinNameSize,
					m_Unit,m_Convert, Etype);

	if( (m_PinShape) || (m_Attributs & PINNOTDRAW) )
		fprintf(ExportFile," ");
	if (m_Attributs & PINNOTDRAW) fprintf(ExportFile,"N");
	if (m_PinShape & INVERT) fprintf(ExportFile,"I");
	if (m_PinShape & CLOCK) fprintf(ExportFile,"C");
	if (m_PinShape & LOWLEVEL_IN) fprintf(ExportFile,"L");
	if (m_PinShape & LOWLEVEL_OUT) fprintf(ExportFile,"V");

	fprintf(ExportFile,"\n");
    return FALSE;
}

/****************************************************/
bool LibDrawPolyline::WriteDescr( FILE * ExportFile )
/****************************************************/
{
int ii, *ptpoly;
    
	fprintf(ExportFile,"P %d %d %d %d", 
                        n,
						m_Unit,m_Convert,
						m_Width);
	ptpoly = PolyList;
	for( ii = n ; ii > 0; ii-- )
		{
		fprintf(ExportFile,"  %d %d", *ptpoly, *(ptpoly+1) );
		ptpoly += 2;
		}
	fprintf(ExportFile," %c\n", fill_tab[m_Fill]);
    return FALSE;
}


/**************************************************/
bool LibDrawField::WriteDescr( FILE * ExportFile )
/**************************************************/
{
int hjustify, vjustify;
wxString text = m_Text;
	
	hjustify = 'C';
	if ( m_HJustify == GR_TEXT_HJUSTIFY_LEFT ) hjustify = 'L';
	else if ( m_HJustify == GR_TEXT_HJUSTIFY_RIGHT ) hjustify = 'R';
	vjustify = 'C';
	if ( m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM) vjustify = 'B';
	else if ( m_VJustify == GR_TEXT_VJUSTIFY_TOP) vjustify = 'T';
	if ( text.IsEmpty() ) text = wxT("~");
	fprintf(ExportFile,"F%d \"%s\" %d %d %d %c %c %c %c",
                m_FieldId, CONV_TO_UTF8(text),
				m_Pos.x, m_Pos.y,
				m_Size.x,
				m_Orient == 0 ? 'H' : 'V',
				(m_Attributs & TEXT_NO_VISIBLE )? 'I' : 'V',
				hjustify, vjustify );
	// Save field name, if necessary
	if ( m_FieldId >= FIELD1 && ! m_Name.IsEmpty() )
		fprintf(ExportFile," \"%s\"", CONV_TO_UTF8(m_Name) );

	fprintf(ExportFile,"\n");
    return FALSE;
}

/**********************************************************/
LibEDA_BaseStruct * CopyDrawEntryStruct( wxWindow * frame,
						 LibEDA_BaseStruct * DrawItem)
/**********************************************************/
/* Routine de Duplication d'une structure DrawLibItem d'une partlib
	Parametres d'entree:
		DrawEntry = pointeur sur la structure a dupliquer
	La structure nouvelle est creee, mais n'est pas inseree dans le
	chainage
	Retourne:
		Pointeur sur la structure creee
*/
{
LibEDA_BaseStruct * NewDrawItem = NULL;
wxString msg;

	switch(DrawItem->m_StructType)
		{
		case COMPONENT_ARC_DRAW_TYPE:
			NewDrawItem = ((LibDrawArc*)DrawItem)->GenCopy();
			break;

		case COMPONENT_CIRCLE_DRAW_TYPE:
			NewDrawItem = ((LibDrawCircle*)DrawItem)->GenCopy();
			break;

		case COMPONENT_RECT_DRAW_TYPE:
			NewDrawItem = ((LibDrawSquare*)DrawItem)->GenCopy();
			break;

		case COMPONENT_PIN_DRAW_TYPE:
			NewDrawItem = ((LibDrawPin*)DrawItem)->GenCopy();
			break;

		case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
			NewDrawItem = ((LibDrawText*)DrawItem)->GenCopy();
			break;

		case COMPONENT_POLYLINE_DRAW_TYPE:
			NewDrawItem = ((LibDrawPolyline*)DrawItem)->GenCopy();
			break;

		default:
			msg.Printf( wxT("CopyDrawLibEntryStruct: unknown Draw Type %d"),
				DrawItem->m_StructType);
			DisplayError(frame, msg);
			break;
		}

	return(NewDrawItem);
}

/*************************************************************************************************/
EDA_LibComponentStruct * CopyLibEntryStruct ( wxWindow * frame, EDA_LibComponentStruct * OldEntry)
/*************************************************************************************************/
/* Routine de copie d'une partlib
	Parametres d'entree: pointeur sur la structure de depart
	Parametres de sortie: pointeur sur la structure creee
	Do not copy new items ( i.e. with m_Flag  & IS_NEW)
*/
{
EDA_LibComponentStruct * NewStruct;
LibEDA_BaseStruct * NewDrawings, * OldDrawings;
LibEDA_BaseStruct * LastItem;
LibDrawField * OldField, * NewField;

	if( OldEntry->Type != ROOT )
	{
		DisplayError(frame, wxT("CopyLibEntryStruct(): Type != ROOT"));
		return(NULL);
	}

	NewStruct = new EDA_LibComponentStruct(NULL);
	OldEntry->m_Prefix.Copy(&NewStruct->m_Prefix);
	OldEntry->m_Name.Copy(&NewStruct->m_Name);

	NewStruct->m_UnitCount = OldEntry->m_UnitCount;
	NewStruct->m_TextInside = OldEntry->m_TextInside;
	NewStruct->m_DrawPinNum = OldEntry->m_DrawPinNum;
	NewStruct->m_DrawPinName = OldEntry->m_DrawPinName;
	NewStruct->m_Options = OldEntry->m_Options;
	NewStruct->m_UnitSelectionLocked = OldEntry->m_UnitSelectionLocked;

	/* Copie des sous structures: */
	NewStruct->m_AliasList = OldEntry->m_AliasList;
	NewStruct->m_Doc  = OldEntry->m_Doc;
	NewStruct->m_KeyWord  = OldEntry->m_KeyWord;
	NewStruct->m_DocFile  = OldEntry->m_DocFile;

	/* Copie des champs */
	for(OldField = OldEntry->Fields; OldField != NULL;
						OldField = (LibDrawField*)OldField->Pnext)
	{
		NewField = OldField->GenCopy();
		NewField->Pnext = NewStruct->Fields;
		NewStruct->Fields = NewField;
	}

	/* Copie des elements type Drawing */
	LastItem = NULL;
	for(OldDrawings = OldEntry->m_Drawings; OldDrawings != NULL; OldDrawings = OldDrawings->Next())
	{
		if ( ( OldDrawings->m_Flags  & IS_NEW) != 0 ) continue;
		NewDrawings = CopyDrawEntryStruct(frame, OldDrawings);
		if ( NewDrawings )
		{
			if (LastItem == NULL ) NewStruct->m_Drawings = NewDrawings;
			else LastItem->Pnext = NewDrawings;
			LastItem = NewDrawings;
			NewDrawings->Pnext = NULL;
		}
		else	// Probleme rencontré: arret de copie
		{
			OldDrawings->m_StructType = TYPE_NOT_INIT;
			DisplayError(frame, wxT("CopyLibEntryStruct(): error: aborted"));
			break;
		}
	}

	/* Copy the footprint filter list */
	for ( unsigned ii = 0 ; ii < OldEntry->m_FootprintList.GetCount(); ii++ )
		NewStruct->m_FootprintList.Add(OldEntry->m_FootprintList[ii]);
	
	return(NewStruct);
}


/********************************************************/
int WriteOneLibEntry(wxWindow * frame, FILE * ExportFile,
					EDA_LibComponentStruct * LibEntry)
/********************************************************/

/* Routine d'ecriture du composant pointe par LibEntry
	dans le fichier ExportFile( qui doit etre deja ouvert)
	return: 0 si Ok
			-1 si err write
			1 si composant non ecrit ( type ALIAS )
*/
#define UNUSED 0
{
LibEDA_BaseStruct *DrawEntry;
LibDrawField * Field;

	if( LibEntry->Type != ROOT ) return(1);

	/* Creation du commentaire donnant le nom du composant */
	fprintf(ExportFile,"#\n# %s\n#\n", CONV_TO_UTF8(LibEntry->m_Name.m_Text));

	/* Generation des lignes utiles */
	fprintf(ExportFile,"DEF");
	if( (LibEntry->m_Name.m_Attributs & TEXT_NO_VISIBLE) == 0)
		 fprintf(ExportFile," %s", CONV_TO_UTF8(LibEntry->m_Name.m_Text));
	else fprintf(ExportFile," ~%s", CONV_TO_UTF8(LibEntry->m_Name.m_Text));

	if( ! LibEntry->m_Prefix.m_Text.IsEmpty())
		fprintf(ExportFile," %s", CONV_TO_UTF8(LibEntry->m_Prefix.m_Text));
	else fprintf(ExportFile," ~");
	fprintf(ExportFile," %d %d %c %c %d %c %c\n",
		UNUSED, LibEntry->m_TextInside,
		LibEntry->m_DrawPinNum ? 'Y' : 'N',
		LibEntry->m_DrawPinName ? 'Y' : 'N',
		LibEntry->m_UnitCount, LibEntry->m_UnitSelectionLocked ? 'L' : 'F',
		LibEntry->m_Options == ENTRY_POWER ? 'P' : 'N');

	WriteLibEntryDateAndTime(ExportFile, LibEntry);

	/* Position / orientation / visibilite des champs */
    LibEntry->m_Prefix.WriteDescr( ExportFile );
    LibEntry->m_Name.WriteDescr( ExportFile );

	for ( Field = LibEntry->Fields; Field!= NULL;
						Field = (LibDrawField*)Field->Pnext )
	{
		if( Field->m_Text.IsEmpty() && Field->m_Name.IsEmpty() ) continue;
        Field->WriteDescr( ExportFile );
	}

	/* Sauvegarde de la ligne "ALIAS" */
	if( LibEntry->m_AliasList.GetCount() != 0)
	{
		fprintf(ExportFile,"ALIAS");
		unsigned ii;
		for ( ii = 0; ii < LibEntry->m_AliasList.GetCount(); ii++ )
			fprintf(ExportFile," %s", CONV_TO_UTF8(LibEntry->m_AliasList[ii]));
		fprintf(ExportFile,"\n");
	}

	/* Write the footprint filter list */
	if( LibEntry->m_FootprintList.GetCount() != 0)
	{
		fprintf(ExportFile,"$FPLIST\n");
		unsigned ii;
		for ( ii = 0; ii < LibEntry->m_FootprintList.GetCount(); ii++ )
			fprintf(ExportFile," %s\n", CONV_TO_UTF8(LibEntry->m_FootprintList[ii]));
		fprintf(ExportFile,"$ENDFPLIST\n");
	}

	/* Sauvegarde des elements de trace */
	DrawEntry = LibEntry->m_Drawings;
	if(LibEntry->m_Drawings)
	{
		/* we sort the draw items, in order to have an edition more easy,
		when a file editing "by hand" is made */
		LibEntry->SortDrawItems();
		
		fprintf(ExportFile,"DRAW\n");
		DrawEntry = LibEntry->m_Drawings;
		while( DrawEntry )
		{
			switch( DrawEntry->m_StructType)
			{
				case COMPONENT_ARC_DRAW_TYPE:
					#define DRAWSTRUCT ((LibDrawArc *) DrawEntry)
                    DRAWSTRUCT->WriteDescr( ExportFile );
					break;

				case COMPONENT_CIRCLE_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT ((LibDrawCircle *) DrawEntry)
                    DRAWSTRUCT->WriteDescr( ExportFile );
					break;

				case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT ((LibDrawText *) DrawEntry)
                    DRAWSTRUCT->WriteDescr( ExportFile );
					break;

				case COMPONENT_RECT_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT ((LibDrawSquare *) DrawEntry)
                    DRAWSTRUCT->WriteDescr( ExportFile );
 					break;

				case COMPONENT_PIN_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT ((LibDrawPin *) DrawEntry)
                    DRAWSTRUCT->WriteDescr( ExportFile );
					break;

				case COMPONENT_POLYLINE_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT ((LibDrawPolyline *) DrawEntry)
                    DRAWSTRUCT->WriteDescr( ExportFile );
					break;

				default: DisplayError(frame, wxT("Save Lib: Unknown Draw Type"));
					break;
			}

			DrawEntry = DrawEntry->Next();
		}
		fprintf(ExportFile,"ENDDRAW\n");
	}

	fprintf(ExportFile,"ENDDEF\n");

	return(0);
}

/*************************************************************************/
int WriteOneDocLibEntry(FILE * ExportFile, EDA_LibComponentStruct * LibEntry)
/*************************************************************************/

/* Routine d'ecriture de la doc du composant pointe par LibEntry
	dans le fichier ExportFile( qui doit etre deja ouvert)
	return: 0 si Ok
			1 si err write
	Cependant, si i tous les Pointeurs sur textes sont nulls ( pas de Doc )
		rien ne sera ecrit.
*/
{

	if( (LibEntry->m_Doc.IsEmpty() ) &&
		(LibEntry->m_KeyWord.IsEmpty() ) &&
		(LibEntry->m_DocFile.IsEmpty() ) )
		return(0);

	/* Generation des lignes utiles */
	fprintf(ExportFile,"#\n$CMP %s\n", CONV_TO_UTF8(LibEntry->m_Name.m_Text));

	if( ! LibEntry->m_Doc.IsEmpty())
		fprintf(ExportFile,"D %s\n", CONV_TO_UTF8(LibEntry->m_Doc));

	if( ! LibEntry->m_KeyWord.IsEmpty())
		fprintf(ExportFile,"K %s\n", CONV_TO_UTF8(LibEntry->m_KeyWord));

	if( ! LibEntry->m_DocFile.IsEmpty())
		fprintf(ExportFile,"F %s\n", CONV_TO_UTF8(LibEntry->m_DocFile));

	fprintf(ExportFile,"$ENDCMP\n");
	return(0);
}

/*********************************************************************************/
int SaveOneLibrary(wxWindow * frame, const wxString & FullFileName, LibraryStruct * Library)
/*********************************************************************************/
/* Sauvegarde en fichier la librairie pointee par Library, sous le nom
	FullFileName.
	2 fichiers sont crees
	 - La librarie
	 - le fichier de documentation

	une sauvegarde .bak de l'ancien fichier librairie est cree
	une sauvegarde .bck de l'ancien fichier documentation est cree

	return:
		0 si OK
		1 si erreur
*/
{
FILE * SaveFile, * SaveDocFile;
EDA_LibComponentStruct *LibEntry;
char Line[1024];
int err = 1;
wxString Name,DocName,BakName, msg;

	if(Library == NULL) return(err);

	Name = FullFileName;

	/* L'ancien fichier lib est renomme en .bak */
	if( wxFileExists(Name) )
	{
		BakName = Name; ChangeFileNameExt(BakName, wxT(".bak"));
		wxRemoveFile(BakName);
		if( ! wxRenameFile(Name, BakName) )
		{
			msg = wxT("Failed to rename old lib file ") + BakName;
			DisplayError(frame, msg, 20);
		}
	}
 

	DocName = Name; ChangeFileNameExt(DocName,DOC_EXT);
	/* L'ancien fichier doc lib est renomme en .bck */
	if( wxFileExists(DocName) )
	{
		BakName = DocName; ChangeFileNameExt(BakName, wxT(".bck") );
		wxRemoveFile(BakName);
		if( ! wxRenameFile(DocName, BakName) )
		{
			msg = wxT("Failed to save old doc lib file ") + BakName;
			DisplayError(frame, msg, 20);
		}
	}
		

	SaveFile = wxFopen(Name, wxT("wt") );
	if (SaveFile == NULL)
	{
		msg = wxT("Failed to create Lib File ") + Name;
		DisplayError(frame, msg, 20);
		return(err);
	}

	SaveDocFile = wxFopen(DocName, wxT("wt") );
	if (SaveDocFile == NULL)
	{
		msg = wxT("Failed to create DocLib File ") + DocName;
		DisplayError(frame, msg, 20);
		return(err);
	}

	Library->m_Modified = 0;

	/* Creation de l'entete de la librairie */
	Library->m_TimeStamp = GetTimeStamp();
	Library->WriteHeader(SaveFile);
	fprintf(SaveDocFile,"%s  Date: %s\n", DOCFILE_IDENT,
            DateAndTime(Line) );

 
	/* Sauvegarde des composant: */
	PQCompFunc((PQCompFuncType) LibraryEntryCompare);
	LibEntry = (EDA_LibComponentStruct *) PQFirst(&Library->m_Entries, FALSE);

	while( LibEntry )
	{
		err = WriteOneLibEntry(frame, SaveFile, LibEntry);
		err = WriteOneDocLibEntry(SaveDocFile, LibEntry);

		LibEntry = (EDA_LibComponentStruct *)
					PQNext(Library->m_Entries, LibEntry, NULL);
	}

	fprintf(SaveFile,"#\n#End Library\n");
	fprintf(SaveDocFile,"#\n#End Doc Library\n");
	fclose(SaveFile);
	fclose(SaveDocFile);
	return(err);
}


/*************************************************************************************/
static bool WriteLibEntryDateAndTime(FILE * ExportFile, EDA_LibComponentStruct * LibEntry)
/*************************************************************************************/
/* lit date et time de modif composant sous le format:
	"Ti yy/mm/jj hh:mm:ss"
*/
{
int year,mon,day,hour,min,sec;

	if ( LibEntry->m_LastDate == 0 ) return TRUE;

	sec = LibEntry->m_LastDate & 63;
	min = (LibEntry->m_LastDate >> 6) & 63 ;
	hour = (LibEntry->m_LastDate >> 12) & 31;
	day = (LibEntry->m_LastDate >> 17) & 31;
	mon = (LibEntry->m_LastDate >> 22) & 15;
	year  = (LibEntry->m_LastDate >> 26) + 1990;

	fprintf(ExportFile,"Ti %d/%d/%d %d:%d:%d\n",year,mon,day,hour,min,sec);

	return TRUE;
}



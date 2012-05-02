	/************************/
	/*	savelib.cc	*/
	/************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

#include "ed.h"
#include "eelibsl.h"

#define TEXT_SIZE 60
FILE *FileEdf, *FileNet, *FileEESchema, *FileKiPro ;
extern char FileNameKiPro[], FileNameEESchema[], FileNameLib[];
extern float scale;
char *cwd;

OutPro(LibraryStruct * Libs)
{
  int i;
  cwd = (char *)get_current_dir_name();

  sprintf(FileNameKiPro,"%s.pro", schName);
  if( (FileKiPro = fopen( FileNameKiPro, "wt" )) == NULL ) {
       fprintf(stderr, " %s impossible to create\n", FileNameKiPro);
       return(-1);
  }
  fprintf(FileKiPro,"update=16/11/2007-20:11:59\n");
  fprintf(FileKiPro,"last_client=eeschema\n");
  fprintf(FileKiPro,"[eeschema]\n");
  fprintf(FileKiPro,"version=1\n");
  //fprintf(FileKiPro,"LibDir=%s\n", cwd ); //"."
  fprintf(FileKiPro,"LibDir=\n"); //"."
  fprintf(FileKiPro,"NetFmt=1\n");
  fprintf(FileKiPro,"HPGLSpd=20\n");
  fprintf(FileKiPro,"HPGLDm=15\n");
  fprintf(FileKiPro,"HPGLNum=1\n");
  fprintf(FileKiPro,"offX_A=0\n");
  fprintf(FileKiPro,"offY_A=0\n");
  fprintf(FileKiPro,"offX_B=0\n");
  fprintf(FileKiPro,"offY_B=0\n");
  fprintf(FileKiPro,"offX_C=0\n");
  fprintf(FileKiPro,"offY_C=0\n");
  fprintf(FileKiPro,"offX_D=0\n");
  fprintf(FileKiPro,"offY_D=0\n");
  fprintf(FileKiPro,"offX_E=0\n");
  fprintf(FileKiPro,"offY_E=0\n");
  fprintf(FileKiPro,"RptD_X=0\n");
  fprintf(FileKiPro,"RptD_Y=100\n");
  fprintf(FileKiPro,"RptLab=1\n");
  fprintf(FileKiPro,"SimCmd=\n");
  fprintf(FileKiPro,"UseNetN=0\n");
  fprintf(FileKiPro,"LabSize=60\n");
  fprintf(FileKiPro,"[eeschema/libraries]\n");

  for( i=1; Libs != NULL; i++, Libs = Libs->nxt ){
	if( !Libs->isSheet )
      fprintf(FileKiPro, "LibName%d=%s\n", i, Libs->Name);
  }
}

OutHead(LibraryStruct *Libs)
{
  OutEnd(); // close if previous open
	
  sprintf(FileNameEESchema,"%s.sch", schName);
  if( (FileEESchema = fopen( FileNameEESchema, "wt" )) == NULL ) {
       fprintf(stderr, " %s impossible to create\n", FileNameEESchema);
       return(-1);
  }

  if( bug>2)fprintf(stderr,"OutHead %s\n", FileNameEESchema);

  fprintf(FileEESchema,"EESchema Schematic File Version 2 ");
  fprintf(FileEESchema,"date Mon 06 Feb 2012 06:45:22 PM MST\n");
  fprintf(FileEESchema,"LIBS:");
  //for( ; Libs != NULL; Libs = Libs->nxt ){
  //if( !Libs->isSheet )
  //  fprintf(FileEESchema, "%s,", Libs->Name);
  //}
  fprintf(FileEESchema,"\nEELAYER 24 0\nEELAYER END\n");

  fprintf(FileEESchema,"$Descr D 34000 22000\n"); // TODO - get size from EDIF

  fprintf(FileEESchema,"Sheet 1 1\n");
  fprintf(FileEESchema,"Title \"\"\n");
  fprintf(FileEESchema,"Date \"15 oct 2007\"\n");
  fprintf(FileEESchema,"Rev \"\"\n");
  fprintf(FileEESchema,"Comp \"\"\n");
  fprintf(FileEESchema,"Comment1 \"\"\n");
  fprintf(FileEESchema,"Comment2 \"\"\n");
  fprintf(FileEESchema,"Comment3 \"\"\n");
  fprintf(FileEESchema,"Comment4 \"\"\n");
  fprintf(FileEESchema,"$EndDescr\n");
  fflush(FileEESchema);

  return 1;
}

OutEnd()
{
  if(FileEESchema == NULL)
     return;
  if( bug>2)fprintf(stderr,"OutEnd %s\n", FileNameEESchema);
  fprintf(FileEESchema,"$EndSCHEMATC\n");
  fclose(FileEESchema);
}

OutSheets(struct pwr *pgs)
{
  int x, y;

  if(FileEESchema == NULL)
     return;
  for( x=1000,y=800 ; pgs != NULL ; pgs=pgs->nxt ) {
    fprintf(FileEESchema, "$Sheet\n");
    fprintf(FileEESchema, "S %d  %d  900  1500\n", x,y);
    fprintf(FileEESchema, "U %d\n", x);
    fprintf(FileEESchema, "F0 \"%s\" 60\n", pgs->s);
    fprintf(FileEESchema, "F1 \"%s.sch\" 60\n", pgs->s);
    fprintf(FileEESchema, "$EndSheet\n");
	x += 4000;
	y +=  200;
    if( x>15000 ){
		y += 1500;
		x  = 1000;
	}
  }
}

#define OFF 0

OutText(g,s,x,y,size)
char *s;
int   g, x,y;
{
  int fx, fy, fs;
  char *st, *t=malloc(strlen(s)+1);

  if( s==NULL || *s==0 )
	return;
  // fprintf(stderr,"OutText %s %d %0x %0x\n", s, strlen(s), (unsigned int)s, (unsigned int)t);
  // modify bus range
  for( st=s ;  ; s++,t++ ){
	if(*s == ':'){
		*t++ = '.'; *t = '.'; 
	}else
		*t = *s;
	if(*s == '\0')
		break;
  }
  // if s[0] == '[' assume sheet_to_sheet reference change to {1,2,5}
  if( st[0] == '[' ) {
	st[0] = '{';
	for( t=st; *t ; t++){
	  if( *t == ']' )
		*t = '}'; 
    }
  }
  fx = OFF + scale * (float)x; fy = OFF + scale * (float)y;
  fs = 0.55*scale * (float)size;  // fixme - fwb
  if(FileEESchema == NULL)
     return;
  if(g)
    fprintf(FileEESchema,"Text GLabel %d %d 0 %d UnSpc\n%s\n",fx,fy,fs,st);
  else
    fprintf(FileEESchema,"Text Label %d %d 0 %d ~ 0\n%s\n",fx,fy,fs,st);
  fflush(FileEESchema);
  free(st);
}

OutWire(x1,y1,x2,y2)
int x1,y1,x2,y2;
{
  int fx1, fy1, fx2, fy2;

  fx1 = OFF + scale * (float)x1; fy1 = OFF + scale * (float)y1;
  fx2 = OFF + scale * (float)x2; fy2 = OFF + scale * (float)y2;
  if(FileEESchema == NULL)
     return;
  fprintf(FileEESchema,"Wire Wire Line\n    %d %d %d %d\n",fx1,fy1,fx2,fy2);
  fflush(FileEESchema);
}

OutConn(ox,oy)
int ox, oy;
{
int fx, fy;

  if(FileEESchema == NULL)
     return;
  fx  = OFF + scale * (float) ox; fy  = OFF + scale * (float) oy;
  fprintf(FileEESchema,"Connection ~ %d %d\n", fx, fy);
}

OutInst(reflib, refdes, value, foot, mfgname, mfgpart, ts, ox, oy, rx, ry, vx, vy, rflg, vflg, Rot)
char *reflib, *refdes, *value, *foot, *mfgname, *mfgpart;
int ts, ox, oy, rx, ry, vx, vy, Rot[2][2], rflg, vflg;
{

/* example
$Comp
L 24C16 U1
U 1 1 2F5F7E5C
P 5750 9550
F 0    "U1" H 5900 9900 60  0000 C C
F 1 "24C16" H 5950 9200 60  0000 C C
F 2 "TO220" H 5950 9200 60  0000 C C
                            ^ flags
        1    5750 9550
        1    0    0    -1
$EndComp
*/

int fts, fx, fy, frx, fry, fvx, fvy;

  scale =1.0; //fwb
  fts = 0.6 * scale * (float) ts;
  fx  = OFF + scale * (float) ox; fy  = OFF + scale * (float) oy;
  frx = OFF + scale * (float) rx; fry = OFF + scale * (float) ry;
  fvx = OFF + scale * (float) vx; fvy = OFF + scale * (float) vy;

  if(FileEESchema == NULL)
     return;
  fprintf(FileEESchema, "$Comp\n");
  fprintf(FileEESchema,"L %s %s\n", reflib, refdes );
  fprintf(FileEESchema,"U %d %d %8.8lX\n", 1, 1, 0l);
  fprintf(FileEESchema,"P %d %d\n", fx, fy);
if(refdes != NULL){
  fprintf(FileEESchema,"F 0 \"%s\" %c %d %d %d 000%d L TNN\n", refdes, Rot[0][1]?'V':'H',
			frx, (3*fy-2*fry), fts, rflg);
}
if(value != NULL){
  fprintf(FileEESchema,"F 1 \"%s\" %c %d %d %d 000%d L TNN\n", value,  Rot[0][1]?'V':'H',
			fvx, (3*fy-2*fvy), fts, vflg);
}
if(foot != NULL && foot[0] != 0) {
  fprintf(FileEESchema,"F 2 \"%s\" H %d %d %d 0001 L T\n", foot, fx+50, fy+50, fts);
}
if(mfgname != NULL && mfgname[0] != 0) {
  fprintf(FileEESchema,"F 4 \"%s\" H %d %d %d 0001 L T\n", mfgname, fx+50, fy+50, fts);
}
if(mfgpart != NULL && mfgpart[0] != 0) {
  fprintf(FileEESchema,"F 5 \"%s\" H %d %d %d 0001 L T\n", mfgpart, fx+50, fy+50, fts);
}
  fprintf(FileEESchema,"  1 %d %d\n", fx, fy);
  fprintf(FileEESchema,"    %d %d %d %d\n", Rot[0][0], Rot[0][1], Rot[1][0], Rot[1][1]);
  fprintf(FileEESchema,"$EndComp\n");
  fflush(FileEESchema);
}

/* Routines de sauvegarde et maintenance de librairies et composants
*/

#include "fctsys.h"
#include "eelibsl.h"

#define DisplayError printf
#define TEXT_NO_VISIBLE 1

int LibraryEntryCompare(LibraryEntryStruct *LE1, LibraryEntryStruct *LE2);
static int WriteOneLibEntry(FILE * ExportFile, LibraryEntryStruct * LibEntry);

/* Variables locales */

void OutLibHead(FILE *SaveFile, LibraryStruct *CurrentLib )
{

    if( bug>2)fprintf(stderr," OutLibHead %s\n", CurrentLib->Name);
	fprintf(SaveFile,"%s\n", FILE_IDENT);
	fprintf(SaveFile,"### Library: %s ###\n", CurrentLib->Name);
}

void OutLibEnd(FILE *SaveFile)
{
    if( bug>2)fprintf(stderr," OutLibEnd %s\n", CurrentLib->Name);
	fprintf(SaveFile,"#End Library\n");
	fclose(SaveFile);
}
/**************************************************************************/
/* void SaveActiveLibrary(FILE * SaveFile, LibraryEntryStruct *LibEntry ) */
/**************************************************************************/

void SaveActiveLibrary(FILE *SaveFile, LibraryStruct *CurrentLib )
{
LibraryEntryStruct *LibEntry;
int ii;

	LibEntry = (LibraryEntryStruct *) CurrentLib->Entries;
	ii = CurrentLib->NumOfParts ; 
	fprintf(stderr, "%03d #parts %s\n", ii, CurrentLib->Name);
	for(  ; ii > 0; ii-- ) {
		if(LibEntry != NULL) {
			WriteOneLibEntry(SaveFile, LibEntry);
			fprintf(SaveFile,"#\n");
		} else {
			break;
		}
		LibEntry = (LibraryEntryStruct *) LibEntry->nxt;
	}
}


/**************************************************************************/
/* int WriteOneLibEntry(FILE * ExportFile, LibraryEntryStruct * LibEntry) */
/**************************************************************************/

/* Routine d'ecriture du composant pointe par LibEntry
	dans le fichier ExportFile( qui doit etre deja ouvert)
	return: 0 si Ok
			-1 si err write
 			 1 si composant non ecrit ( type ALIAS )
*/
#define UNUSED 0
int WriteOneLibEntry(FILE * ExportFile, LibraryEntryStruct * LibEntry)
{
LibraryDrawEntryStruct *DrawEntry;
LibraryFieldEntry * Field;
void * DrawItem;
int * ptpoly;
int ii, t1, t2, Etype;
char PinNum[5];
char FlagXpin = 0;
int x1,y1,x2,y2,r;

	if( LibEntry->Type != ROOT ) return(1);

	/* Creation du commentaire donnant le nom du composant */
	fprintf(stderr,"   %s\n", LibEntry->Name);
	fprintf(ExportFile,"# %s\n#\n", LibEntry->Name);

	/* Generation des lignes utiles */
	fprintf(ExportFile,"DEF");
	if(LibEntry->DrawName) fprintf(ExportFile," %s",LibEntry->Name);
	else 			       fprintf(ExportFile," ~%s",LibEntry->Name);

	if(LibEntry->Prefix[0] > ' ') fprintf(ExportFile," %s",LibEntry->Prefix);
	else fprintf(ExportFile," ~");
	fprintf(ExportFile," %d %d %c %c %d %d %c\n",
		UNUSED, LibEntry->TextInside,
		LibEntry->DrawPinNum ? 'Y' : 'N',
		LibEntry->DrawPinName ? 'Y' : 'N',
		LibEntry->NumOfUnits, UNUSED, 'N');

	/* Position / orientation / visibilite des champs */
        x1 = scale*(float)LibEntry->PrefixPosX; 
        y1 = scale*(float)LibEntry->PrefixPosY; 
	fprintf(ExportFile,"F0 \"%s\" %d %d %d %c %c\n",
				LibEntry->Prefix,
				x1, y1,
				(int)scale*LibEntry->PrefixSize,
				LibEntry->PrefixOrient == 0 ? 'H' : 'V',
				LibEntry->DrawPrefix ? 'V' : 'I' );

    x1 = scale*(float)LibEntry->NamePosX; 
    y1 = scale*(float)LibEntry->NamePosY; 
	fprintf(ExportFile,"F1 \"%s\" %d %d %d %c %c\n",
				LibEntry->Name,
				x1, y1,
				(int)scale*LibEntry->NameSize,
				LibEntry->NameOrient == 0 ? 'H' : 'V',
				LibEntry->DrawName ? 'V' : 'I' );

	for ( Field = LibEntry->Fields; Field!= NULL; Field = Field->nxt ) {
		if( Field->Text == NULL ) continue;
		if( strlen(Field->Text) == 0 ) continue;
        	x1 = scale*(float)Field->PosX;
        	y1 = scale*(float)Field->PosY;
		fprintf(ExportFile,"F%d \"%s\" %d %d %d %c %c\n",
				Field->FieldId,
				Field->Text,
				x1, y1,
				(int)scale*Field->Size,
				Field->Orient == 0 ? 'H' : 'V',
				(Field->Flags & TEXT_NO_VISIBLE) ? 'I' : 'V' );
	}

	/* Sauvegarde de la ligne "ALIAS" */
	if( LibEntry->AliasList )
		if( strlen(LibEntry->AliasList) )
			fprintf(ExportFile,"ALIAS %s\n", LibEntry->AliasList);

	/* Sauvegarde des elements de trace */
	DrawEntry = LibEntry->Drawings;
	if(DrawEntry) {
		fprintf(ExportFile,"DRAW\n");
		while( DrawEntry ) {
			switch( DrawEntry->DrawType) {
				case ARC_DRAW_TYPE:
					#define DRAWSTRUCT     (&(DrawEntry->U.Arc))
					t1 = DRAWSTRUCT->t1 - 1; if(t1 > 1800) t1 -= 3600;
					t2 = DRAWSTRUCT->t2 + 1; if(t2 > 1800) t2 -= 3600;
        				x1 = scale*(float)DRAWSTRUCT->x;
        				y1 = scale*(float)DRAWSTRUCT->y;
        				r  = scale*(float)DRAWSTRUCT->r;
					fprintf(ExportFile,"A %d %d %d %d %d %d %d %d\n",
						x1, y1, r, t1, t2,
						DrawEntry->Unit,DrawEntry->Convert, DRAWSTRUCT->width);
					break;

				case CIRCLE_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT (&(DrawEntry->U.Circ))
        				x1 = scale*(float)DRAWSTRUCT->x;
        				y1 = scale*(float)DRAWSTRUCT->y;
        				r  = scale*(float)DRAWSTRUCT->r;
					fprintf(ExportFile,"C %d %d %d %d %d %d\n",
						x1, y1, r,
						DrawEntry->Unit,DrawEntry->Convert, DRAWSTRUCT->width);
					break;

				case TEXT_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT (&(DrawEntry->U.Text))
					if( DRAWSTRUCT->Text != NULL &
					    strcmp(DRAWSTRUCT->Text, " ") )
        				x1 = scale*(float)DRAWSTRUCT->x;
        				y1 = scale*(float)DRAWSTRUCT->y;
					fprintf(ExportFile,"T %d %d %d %d %d %d %d %s\n",
						DRAWSTRUCT->Horiz,
						x1, y1,
						(int)scale*DRAWSTRUCT->size, DRAWSTRUCT->type,
						DrawEntry->Unit,DrawEntry->Convert,
						DRAWSTRUCT->Text );
					break;

				case SQUARE_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT (&(DrawEntry->U.Sqr))
        				x1 = scale*(float)DRAWSTRUCT->x1;
        				y1 = scale*(float)DRAWSTRUCT->y1;
        				x2 = scale*(float)DRAWSTRUCT->x2;
        				y2 = scale*(float)DRAWSTRUCT->y2;
					fprintf(ExportFile,"S %d %d %d %d %d %d %d N\n",
					x1, y1, x2, y2,
					DrawEntry->Unit, DrawEntry->Convert, DRAWSTRUCT->width);
					break;

				case PIN_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT (&(DrawEntry->U.Pin))
					FlagXpin = 1;
					Etype = 'I';
					switch(DRAWSTRUCT->PinType) {
						case PIN_INPUT: 	Etype = 'I'; break;
						case PIN_OUTPUT: 	Etype = 'O'; break;
						case PIN_BIDI: 		Etype = 'B'; break;
						case PIN_TRISTATE: 	Etype = 'T'; break;
						case PIN_PASSIVE: 	Etype = 'P'; break;
						case PIN_UNSPECIFIED: 	Etype = 'U'; break;
						case PIN_POWER: 	Etype = 'W'; break;
						case PIN_OPENCOLLECTOR: Etype = 'C'; break;
						case PIN_OPENEMITTER:	Etype = 'E'; break;
					}
					// memset(PinNum,0, sizeof(PinNum) );
					// PinNum[0] = '0';
					// strcpy(PinNum, "0");
					// if(DRAWSTRUCT->Num)
					//  strncpy(PinNum,DRAWSTRUCT->Num, 4);

					if((DRAWSTRUCT->ReName != NULL) && (DRAWSTRUCT->ReName[0] > ' '))
						  fprintf(ExportFile,"X %s", DRAWSTRUCT->ReName);
					else
					if((DRAWSTRUCT->Name != NULL) && (DRAWSTRUCT->Name[0] > ' '))
						  fprintf(ExportFile,"X %s", DRAWSTRUCT->Name);
					else fprintf(ExportFile,"X ~");

        				x1 = scale*(float)DRAWSTRUCT->posX;
        				y1 = scale*(float)DRAWSTRUCT->posY;
					x2 = 0.55*scale*(float)DRAWSTRUCT->SizeNum; // fwb
					y2 = 0.55*scale*(float)DRAWSTRUCT->SizeName;

					fprintf(ExportFile," %s %d %d %d %c %d %d %d %d %c",
						DRAWSTRUCT->Num, // PinNum,
						x1, y1,
						(int)DRAWSTRUCT->Len,
						(DRAWSTRUCT->Orient==0)?'L':DRAWSTRUCT->Orient,
						x2,y2,
						DrawEntry->Unit, DrawEntry->Convert, Etype);

					if( (DRAWSTRUCT->PinShape) || (DRAWSTRUCT->Flags & PINNOTDRAW) )
						fprintf(ExportFile," ");
					if (DRAWSTRUCT->Flags & PINNOTDRAW)
						fprintf(ExportFile,"N");
					if (DRAWSTRUCT->PinShape & INVERT)
						fprintf(ExportFile,"I");
					if (DRAWSTRUCT->PinShape & CLOCK)
						fprintf(ExportFile,"C");
					if (DRAWSTRUCT->PinShape & LOWLEVEL_IN)
						fprintf(ExportFile,"L");
					if (DRAWSTRUCT->PinShape & LOWLEVEL_OUT)
						fprintf(ExportFile,"V");

					fprintf(ExportFile,"\n");
					break;

				case POLYLINE_DRAW_TYPE:
					#undef DRAWSTRUCT
					#define DRAWSTRUCT (&(DrawEntry->U.Poly))
					fprintf(ExportFile,"P %d %d %d %d", DRAWSTRUCT->n,
						DrawEntry->Unit,DrawEntry->Convert, DRAWSTRUCT->width);
					ptpoly = DRAWSTRUCT->PolyList;
					for( ii = DRAWSTRUCT->n ; ii > 0; ii-- )
						{
						x1 = scale*(float)*ptpoly;
						y1 = scale*(float)*(ptpoly+1);
						fprintf(ExportFile,"  %d %d", x1, y1);
						ptpoly += 2;
						}
					if (DRAWSTRUCT->Fill) fprintf(ExportFile," F");
					fprintf(ExportFile,"\n");
					break;

				default: DisplayError( "Save Lib: Unknown Draw Type");
					break;
			}
			DrawEntry = DrawEntry->nxt;
		}
		fprintf(ExportFile,"ENDDRAW\n");
	}
	fprintf(ExportFile,"ENDDEF\n");

	return(0);
}


/*****************************************************************************
* Routine to compare two LibraryEntryStruct for the PriorQue module.		 *
* Comparison is based on Part name.											 *
*****************************************************************************/
int LibraryEntryCompare(LibraryEntryStruct *LE1, LibraryEntryStruct *LE2)
{
	return strcmp(LE1->Name, LE2->Name);
}



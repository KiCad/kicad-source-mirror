/**********************************/
/* Netlist generator for pads-pcb */
/**********************************/

/* read the generic netlist created by eeschema and convert it to a pads-pcb form
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef __UNIX__
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

/* Pads-pcb sample:
*PADS-PCB*
*PART*
C1 CHIP_D
C2 C1206
C3 CHIP_B
C4 CHIP_B
D1 CHIP_B
JP1 unknown

*NET*
*SIGNAL* N03791
C3.1 R1.1 JP2.7 U1.3
*SIGNAL* VCC
 U2.14 Y1.8 JP5.2 U3.2 C1.1 U1.20 JP1.8 JP1.3
 C5.2 C6.1 C7.2 U1.7 Y1.7
*SIGNAL* N01384
 JP5.1 U1.1
*SIGNAL* N02594
*END*
*/


/* Generic netlist sample:
$BeginNetlist
$BeginComponentList

$BeginComponent
TimeStamp=32568D1E
Footprint=
Reference=JP1
Value=CONN_8X2
Libname=CONN_8X2
$BeginPinList
1=GND
2=REF10_1
3=GND
4=REF11_1
5=GND
6=REF7_1
7=GND
8=REF9_1
9=GND
10=REF6_1
11=GND
12=REF8_1
13=GND
14=REF4_1
15=GND
16=REF5_1
$EndPinList
$EndComponent

$BeginComponent
TimeStamp=325679C1
Footprint=
Reference=RR1
Value=9x1K
Libref=RR9
$BeginPinList
1=VCC
2=REF5_1
3=REF4_1
4=REF8_1
5=REF6_1
6=REF9_1
7=REF7_1
8=REF11_1
9=REF10_1
10=?
$EndPinList
$EndComponent
$EndComponentList

$BeginNets
Net 0 ""
Net 1 "GND"
 BUS1 31
 U3 19
 U3 10
 U3 1
Net 172 ""
 BUS1 32
Net 173 ""
 BUS1 30
$EndNets

$EndNetlist
*/

char * GetLine(FILE *File, char *Line, int *LineNum, int SizeLine);
int ReadAndWriteComponentDataSection(FILE * InFile, FILE * OutFile, int *LineNumber);
int ReadAndWriteNetsDataSection(FILE * InFile, FILE * OutFile, int * LineNumber);



class ComponentDataClass
{
public:
	char m_Reference[256];
	char m_Value[256];
	char m_Footprint[256];
	char m_LibRef[256];
	long m_TimeStamp;

public:
	ComponentDataClass(void)
	{
		InitData();
	}
	void InitData(void)
	{
		m_TimeStamp = 0;
		m_Reference[0] = 0;
		m_Value[0] = 0;
		m_Footprint[0] = 0;
		m_LibRef[0] = 0;
	}
};


/********************************/
int main(int argc, char ** argv )
/********************************/
{
char * InputfileName, * OutputFilename;
FILE * InFile, * OutFile;
int LineNumber;
char Line[1024];

	if ( argc < 3 )
	{
		printf("\nUsage; netlist_form_pads-pcb infile outfile\n");
		return -1;
	}

	InputfileName = argv[1];
	OutputFilename = argv[2];

	if ( (InFile = fopen(InputfileName, "rt")) == NULL)
	{
		printf ( "Failed to open file %s", InputfileName);
		return -2;
	}
	if ( (OutFile = fopen(OutputFilename, "wt")) == NULL)
	{
		printf ( "Failed to create file %s", OutputFilename);
		return -3;
	}


	/* Write header: */
	fprintf( OutFile, "*PADS-PCB*\n*PART*\n" );

	/* Read and write data lines */
	while( GetLine(InFile, Line, &LineNumber, sizeof(Line) ) )
	{
		if ( stricmp(Line, "$BeginComponent") == 0 )
		{
			ReadAndWriteComponentDataSection(InFile, OutFile, &LineNumber);
			continue;
		}
		if ( stricmp(Line, "$BeginNets") == 0 )
		{
			fprintf( OutFile, "\n*NET*\n" );
			ReadAndWriteNetsDataSection(InFile, OutFile, &LineNumber);
			continue;
		}
	}


	fprintf( OutFile, "*END*\n" );

	fclose(InFile);
	fclose(OutFile);

	return 0;
}


/****************************************************************/
int ReadAndWriteComponentDataSection(FILE * InFile, FILE * OutFile, int *LineNumber)
/****************************************************************/
/* Read the Components Section from the Generic Netlist and create Components section in Pads-Pcb format
	For the component section only reference and footprint are used.
	Create lines like:
	C1 CHIP_D
	C2 unknown
*/
{
char Line[1024];
class ComponentDataClass ComponentData;
char * ident, *data;

	while( GetLine(InFile, Line, LineNumber, sizeof(Line) ) )
	{
		if ( stricmp(Line, "$BeginPinList") == 0 )
		{
			while( GetLine(InFile, Line, LineNumber, sizeof(Line) ) )
				if ( stricmp(Line, "$EndPinList") == 0 ) break;
			continue;
		}

		if ( stricmp(Line, "$EndComponent") == 0 ) // Create the output for the component:
		{
			/* Create the line like: C2 unknown */

			fprintf(OutFile, "%s ", ComponentData.m_Reference );
			fprintf(OutFile, "%s\n",
				strlen(ComponentData.m_Footprint) ? ComponentData.m_Footprint : "unknown");
			return 0;
		}
		ident = strtok(Line,"=\n\r");
		data = strtok(NULL,"=\n\r");
		if ( data == NULL ) continue;
		if ( stricmp(Line, "TimeStamp") == 0 )
		{
			ComponentData.m_TimeStamp = atol(data);
			continue;
		}
		if ( stricmp(Line, "Footprint") == 0 )
		{
			strncpy(ComponentData.m_Footprint, data, 255);
			continue;
		}
		if ( stricmp(Line, "Reference") == 0 )
		{
			strncpy(ComponentData.m_Reference, data, 255);
			continue;
		}
		if ( stricmp(Line, "Value") == 0 )
		{
			strncpy(ComponentData.m_Value, data, 255);
			continue;
		}
		if ( stricmp(Line, "Libref") == 0 )
		{
			strncpy(ComponentData.m_LibRef, data, 255);
			continue;
		}
	}

	return 1;
}

/****************************************************************/
int ReadAndWriteNetsDataSection(FILE * InFile, FILE * OutFile, int *LineNumber)
/****************************************************************/
/* Read the Nets Section from the Generic Netlist and create Nets section in Pads-Pcb format
	create info type:
	*SIGNAL* N03791
	C3.1 R1.1 JP2.7 U1.3
*/
{
char Line[1024];
char * ident, *netnum, *netname, * pin;

	while( GetLine(InFile, Line, LineNumber, sizeof(Line) ) )
	{
		if ( stricmp(Line, "$EndNets") == 0 ) return 0;
		ident = strtok(Line," \n\r");
		if ( stricmp(ident, "Net") == 0 )
		{
			netnum = strtok(NULL," \n\r");
			netname = strtok(NULL," \"\n\r");
			if ( (netname == NULL) || strlen(netname) == 0 )
			{	// Create the line like: *SIGNAL* N03791
				int num = atoi(netnum);
				sprintf(Line, "N-%6.6d", num);
				netname = Line;
			}
			fprintf(OutFile, "*SIGNAL* %s\n", netname);
		}
		else
		{	// Create the line like:   C3.1 R1.1 JP2.7 U1.3
			pin = strtok(NULL," \n\r");
			fprintf(OutFile, " %s.%s\n", ident, pin);
		}
	}

	return 1;
}


/*****************************************************************/
char * GetLine(FILE *File, char *Line, int *LineNum, int SizeLine)
/*****************************************************************/
/* Return a non empty line
	increment *LineNum for each read line
	Comment lines (starting by '#') are skipped
*/
{
	do  {
		if (fgets(Line, SizeLine, File) == NULL) return NULL;
		if( LineNum ) *LineNum += 1;
		} while (Line[0] == '#' || Line[0] == '\n' ||  Line[0] == '\r' ||
				Line[0] == 0);

	strtok(Line,"\n\r");
	return Line;
}



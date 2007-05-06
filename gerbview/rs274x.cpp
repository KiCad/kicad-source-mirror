		/********************************************************/
		/* Routine de lecture d'un fichier GERBER format RS274X */
		/********************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "protos.h"

#define IsNumber(x) ( ( ((x) >= '0') && ((x) <='9') ) ||\
					((x) == '-') || ((x) == '+') || ((x) == '.') || ((x) == ','))

#define CODE(x,y) ((x<<8) + (y))
enum rs274x_parameters
{
	FORMAT_STATEMENT_COMMAND = CODE('F','S'),
	AXIS_SELECT = CODE('A','S'),
	MIRROR_IMAGE = CODE('M','I'),
	MODE_OF_UNITS = CODE('M','O'),
	INCH = CODE('I','N'),
	MILLIMETER = CODE('M','M'),
	OFFSET = CODE('O','F'),
	SCALE_FACTOR = CODE('S','F'),

	IMAGE_NAME = CODE('I','N'),
	IMAGE_JUSTIFY = CODE('I','J'),
	IMAGE_OFFSET = CODE('I','O'),
	IMAGE_POLARITY = CODE('I','P'),
	IMAGE_ROTATION = CODE('I','R'),
	PLOTTER_FILM = CODE('P','M'),
	INCLUDE_FILE = CODE('I','F'),

	APERTURE_DESCR = CODE('A','D'),
	APERTURE_MACRO = CODE('A','M'),
	LAYER_NAME = CODE('L','N'),
	LAYER_POLARITY = CODE('L','P'),
	KNOCKOUT = CODE('K','O'),
	STEP_AND_REPEAT = CODE('S','P'),
	ROTATE = CODE('R','O')

};

/* Variables locales : */

/* Routines Locales */
static bool ReadApertureMacro( char * buff, char * &text, FILE *gerber_file);

/* Lit 2 codes ascii du texte pointé par text
retourne le code correspondant ou -1 si erreur
*/
static int ReadXCommand(char * &text)
{
int result;

		if ( text && *text )
			{
			result = (*text) << 8; text ++;
			}
		else return -1;
		if ( text && *text )
			{
			result += (*text) & 255; text ++;
			}
		else return -1;
	return result;
}


/********************************/
static int ReadInt(char * &text)
/********************************/
{
int nb = 0;
	while ( text && *text)
		{
		if ( (*text >= '0') && (*text <='9') )
			{
			nb *= 10; nb += *text & 0x0F;
			text++;
			}
		else break;
		}
	return nb;
}


/************************************/
static double ReadDouble(char * &text)
/************************************/
{
double nb = 0.0;
char buf[256], * ptchar;

	ptchar = buf;
	while ( text && *text)
		{
		if ( IsNumber(*text) )
			{
			* ptchar = * text;

			if ( * ptchar =='.' || *ptchar == ',' ) * ptchar = g_FloatSeparator;

			text++; ptchar ++;
			}
		else break;
		}
	*ptchar = 0;

	nb = atof(buf);
	return nb;
}

/****************************************************************************/
bool GERBER_Descr::ReadRS274XCommand(WinEDA_GerberFrame * frame, wxDC * DC,
						char * buff, char * &text)
/****************************************************************************/
/* Lit toutes les commandes RS274X jusqu'a trouver de code de fin %
	appelle ExecuteRS274XCommand() pour chaque commande trouvée
*/
{
int code_command;
bool eof = FALSE;

	text++;

	do
	{
		while ( *text )
		{
			switch( *text )
			{
				case '%':	// End commande
					text ++;
					m_CommandState = CMD_IDLE;
					return TRUE;

				case ' ':
				case '\r':
				case '\n':
					text++;
					break;

				case '*':
					text++;
					break;

				default:
					code_command = ReadXCommand ( text );
					ExecuteRS274XCommand(code_command,
								buff, text);
					break;
			}
		}
		// End of current line
		if ( fgets(buff,255,m_Current_File) == NULL ) eof = TRUE;
		text = buff;
	} while( !eof );

	return FALSE;
}


/*******************************************************************************/
bool GERBER_Descr::ExecuteRS274XCommand(int command, char * buff, char * &text)
/*******************************************************************************/
/* Execute 1 commande RS274X
*/
{
int code;
int xy_seq_len, xy_seq_char;
char ctmp;
bool ok = TRUE;
D_CODE * dcode;
char Line[1024];
wxString msg;
double fcoord;
double conv_scale = m_GerbMetric ? PCB_INTERNAL_UNIT/25.4 : PCB_INTERNAL_UNIT;


	switch( command )
	{
		case FORMAT_STATEMENT_COMMAND:
			xy_seq_len = 2;
			while ( *text != '*' )
			{
				switch ( *text )
				{
					case'L':	// No Leading 0
						m_NoTrailingZeros = FALSE;
						text ++;
						break;
					case'T':	// No trailing 0
						m_NoTrailingZeros = TRUE;
						text ++;
						break;
					case 'A':	// Absolute coord
						m_Relative = FALSE; text++;
						break;
					case 'I':	// Absolute coord
						m_Relative = TRUE; text++;
						break;
					case 'N':	// Sequence code (followed by the number of digits for the X,Y command
						text++;
						xy_seq_char = * text; text++;
						if ( (xy_seq_char >= '0') && (xy_seq_char <= '9') )
							xy_seq_len =  - '0';
						break;
					
					case 'X':
					case 'Y':	// Valeurs transmises :2 (really xy_seq_len : FIX ME) digits
						code = *(text ++); ctmp = *(text++) - '0';
						if ( code == 'X' )
						{
							m_FmtScale.x = * text - '0'; // = nb chiffres apres la virgule
							m_FmtLen.x = ctmp + m_FmtScale.x;  // = nb total de chiffres
						}
						else {
							m_FmtScale.y = * text - '0';
							m_FmtLen.y = ctmp + m_FmtScale.y;
						}
						text++;
						break;

					case'*':
						break;

					default:
						GetEndOfBlock( buff, text, m_Current_File );
						ok = FALSE;
						break;
				}
			}
			break;

		case AXIS_SELECT:
		case MIRROR_IMAGE:
			ok = FALSE;
			break;

		case MODE_OF_UNITS:
			code = ReadXCommand ( text );
			if ( code == INCH ) m_GerbMetric = FALSE;
			else if ( code == MILLIMETER ) m_GerbMetric = TRUE;
			conv_scale = m_GerbMetric ? PCB_INTERNAL_UNIT/25.4 : PCB_INTERNAL_UNIT;
			break;

		case OFFSET:	// command: OFAnnBnn (nn = float number)
			m_Offset.x = m_Offset.y = 0;
			while ( *text != '*' )
			{
				switch ( *text )
				{
					case'A':	// A axis offset in current unit (inch ou mm)
						text ++;
						fcoord = ReadDouble(text);
						m_Offset.x = (int) round(fcoord * conv_scale);
						break;
					
					case'B':	// B axis offset in current unit (inch ou mm)
						text ++;
						fcoord = ReadDouble(text);
						m_Offset.y = (int) round(fcoord * conv_scale);
						break;
				}
			}
			break;
			
		case SCALE_FACTOR:
		case IMAGE_JUSTIFY:
		case IMAGE_ROTATION :
		case IMAGE_OFFSET:
		case PLOTTER_FILM:
		case LAYER_NAME:
		case KNOCKOUT:
		case STEP_AND_REPEAT:
		case ROTATE:
			msg.Printf(_("Command <%c%c> ignored by Gerbview"),
				(command>>8) & 0xFF, command & 0xFF);
			wxMessageBox(msg);
			break;

		case IMAGE_NAME:
			m_Name.Empty();
			while ( *text != '*' )
			{
				m_Name.Append(*text); text++;
			}
			break;

		case IMAGE_POLARITY:
			if ( strnicmp(text, "NEG", 3 ) == 0 ) m_ImageNegative = TRUE;
			else m_ImageNegative = FALSE;
			break;
		
		case LAYER_POLARITY:
			if ( * text == 'C' ) m_LayerNegative = TRUE;
			else m_LayerNegative = FALSE;
			break;

		case APERTURE_MACRO:
			ReadApertureMacro( buff, text, m_Current_File );
			break;

		case INCLUDE_FILE:
			if ( m_FilesPtr >= 10 )
			{
				ok = FALSE;
				DisplayError(NULL, _("Too many include files!!") );
				break;
			}
			strcpy(Line, text);
			strtok(Line,"*%%\n\r");
			m_FilesList[m_FilesPtr] = m_Current_File;
			m_Current_File = fopen(Line,"rt");
			if (m_Current_File == 0)
			{
				wxString msg;
				msg.Printf( wxT("fichier <%s> non trouve"),Line);
				DisplayError(NULL, msg, 10);
				ok = FALSE;
				m_Current_File = m_FilesList[m_FilesPtr];
				break;
			}
			m_FilesPtr ++;
			break;

		case APERTURE_DESCR:
			if ( *text != 'D' )
			{
				ok = FALSE; break;
			}
			m_As_DCode = TRUE;
			text++;
			code = ReadInt(text);
			ctmp = *text;
			dcode = ReturnToolDescr(m_Layer, code);
			if ( dcode == NULL ) break;
			if ( text[1] == ',' )	// Tool usuel (C,R,O,P)
			{
				text += 2;			// text pointe size ( 1er modifier)
				dcode->m_Size.x = dcode->m_Size.y =
					(int) round(ReadDouble(text) * conv_scale);
				switch (ctmp )
				{
					case 'C':		// Circle
						dcode->m_Shape = GERB_CIRCLE;
						if ( * text == 'X' )
						{
							text++;
							dcode->m_Drill.x = dcode->m_Drill.y =
								(int) round(ReadDouble(text) * conv_scale);
							dcode->m_DrillShape = 1;
						}
						if ( * text == 'X' )
						{
							text++;
							dcode->m_Drill.y =
								(int) round(ReadDouble(text) * conv_scale);
							dcode->m_DrillShape = 2;
						}
						dcode->m_Defined = TRUE;
						break;

					case 'O':		// ovale
					case 'R':		// rect
						dcode->m_Shape = (ctmp == 'O') ? GERB_OVALE : GERB_RECT;
						if ( * text == 'X' )
						{
							text++;
							dcode->m_Size.y =
								(int) round(ReadDouble(text) * conv_scale);
						}
						if ( * text == 'X' )
						{
							text++;
							dcode->m_Drill.x = dcode->m_Drill.y =
								(int) round(ReadDouble(text) * conv_scale);
							dcode->m_DrillShape = 1;
						}
						if ( * text == 'Y' )
						{
							text++;
							dcode->m_Drill.y =
								(int) round(ReadDouble(text) * conv_scale);
							dcode->m_DrillShape = 2;
						}
						dcode->m_Defined = TRUE;
						break;

					case 'P':		// Polygone
// A modifier: temporairement la forme ronde est utilisée
						dcode->m_Shape = GERB_CIRCLE;
						dcode->m_Defined = TRUE;
						break;
				}
			}
			break;

		default:
			ok = FALSE;
			break;
	}

	GetEndOfBlock( buff, text, m_Current_File );
	return ok;
}

/*****************************************************************/
bool GetEndOfBlock( char * buff, char * &text, FILE *gerber_file)
/*****************************************************************/
{
bool eof = FALSE;

	do	{
		while ( (text < buff+255) && *text )
			{
			if ( *text == '*' ) return TRUE;
			if ( *text == '%' ) return TRUE;
			text ++;
			}
		if ( fgets(buff,255,gerber_file) == NULL ) eof = TRUE;
		text = buff;
		} while( ! eof);

	return FALSE;
}


/*******************************************************************/
bool ReadApertureMacro( char * buff, char * &text, FILE *gerber_file)
/*******************************************************************/
{
wxString macro_name;
int macro_type = 0;

	// Read macro name
	while ( (text < buff+255) && *text )
	{
		if ( *text == '*' ) break;
		macro_name.Append(*text);
		text ++;
	}

wxMessageBox(macro_name, wxT("macro name"));
	text = buff;
	fgets(buff,255,gerber_file);

	// Read parameters
	macro_type = ReadInt(text);

	while ( (text < buff+255) && *text )
		{
		if ( *text == '*' ) return TRUE;
		text ++;
		}
	return FALSE;
}


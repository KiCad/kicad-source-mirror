/************************************************************************/
/*				MODULE edamenu.cpp										*/
/************************************************************************/

/****************************/
/*  INCLUDES SYSTEMES		*/
/****************************/

#include "fctsys.h"

#include "wxstruct.h"
#include "gr_basic.h"
#include "macros.h"
#include "common.h"

#include "worksheet.h"

/* Pour imprimer / lire en unites utilisateur ( INCHES / MM ) */

/* Retourne la valeur en inch ou mm de la valeur val en unités internes
*/
double To_User_Unit(bool is_metric, int val,int internal_unit_value)
{
double value;

	if (is_metric)
		value = (double) (val) * 25.4 / internal_unit_value;
	else value = (double) (val) / internal_unit_value;

	return value;
}

/* Retourne la valeur en unités internes de la valeur val exprimée en inch ou mm
*/
int From_User_Unit(bool is_metric, double val,int internal_unit_value)
{
double value;

	if (is_metric) value = val * internal_unit_value / 25.4 ;
	else value = val * internal_unit_value;

	return (int) round(value);
}

/**********************/
wxString GenDate(void)
/**********************/
/* Retourne la chaine de caractere donnant la date */
{
static char * mois[12] =
	{
	"jan", "feb","mar","apr","may","jun","jul","aug","sep","oct","nov","dec"
	};
time_t buftime;
struct tm * Date;
wxString string_date;
char Line[1024];

	time(&buftime);
	Date = gmtime(&buftime);
	sprintf(Line,"%d %s %d", Date->tm_mday,
							mois[Date->tm_mon],
							Date->tm_year + 1900);
	string_date = Line;
	return(string_date);
}

/***********************************/
void * MyMalloc (size_t nb_octets)
/***********************************/
/* Routine d'allocation memoire */
{
void * pt_mem;
char txt[60];

	if (nb_octets == 0)
		{
		DisplayError(NULL, "Allocate 0 bytes !!");
		return(NULL);
		}
	pt_mem = malloc(nb_octets);
	if (pt_mem == NULL)
		{
		sprintf(txt,"Out of memory: allocation %d bytes", nb_octets);
		DisplayError(NULL, txt);
		}
	return(pt_mem);
}

/************************************/
void * MyZMalloc (size_t nb_octets)
/************************************/
/* Routine d'allocation memoire, avec remise a zero de la zone allouee */
{
	void * pt_mem = MyMalloc (nb_octets);
	if ( pt_mem) memset(pt_mem, 0, nb_octets);
	return(pt_mem);
}

/*******************************/
void MyFree (void * pt_mem)
/*******************************/
{
	if( pt_mem ) free(pt_mem);
}


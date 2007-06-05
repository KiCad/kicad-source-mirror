		/************/
		/* pcbplot.h*/
		/************/

#ifndef PLOTGERB_H
#define PLOTGERB_H


/* Format Gerber : NOTES :
Fonctions preparatoires:
	Gn =
	G01			interpolation lineaire ( trace de droites )
	G02,G20,G21	Interpolation circulaire , sens trigo < 0
	G03,G30,G31	Interpolation circulaire , sens trigo > 0
	G04			commentaire
	G06			Interpolation parabolique
	G07			Interpolation cubique
	G10			interpolation lineaire ( echelle 10x )
	G11			interpolation lineaire ( echelle 0.1x )
	G12			interpolation lineaire ( echelle 0.01x )
	G36			Start polygon description
	G37			End polygon description
	G52			plot symbole reference par Dnn code
	G53			plot symbole reference par Dnn ; symbole tourne de -90 degres
	G54			Selection d'outil
	G55			Mode exposition photo
	G56			plot symbole reference par Dnn A code
	G57			affiche le symbole reference sur la console
	G58			plot et affiche le symbole reference sur la console
	G60			interpolation lineaire ( echelle 100x )
	G70			Unites = Inches
	G71			Unites = Millimetres
	G74			supprime interpolation circulaire sur 360 degre, revient a G01
	G75			Active interpolation circulaire sur 360 degre
	G90			Mode Coordonnees absolues
	G91			Mode Coordonnees Relatives

Coordonnees X,Y
	X,Y sont suivies de + ou - et de m+n chiffres (non separes)
			m = partie entiere
			n = partie apres la virgule
			 formats classiques : 	m = 2, n = 3 (format 2.3)
			 						m = 3, n = 4 (format 3.4)
 	ex:
	G__ X00345Y-06123 D__*

Outils et D_CODES
	numero d'outil ( identification des formes )
	1 a 99 	(classique)
	1 a 999
	D_CODES:

	D01 ... D9 = codes d'action:
	D01			= activation de lumiere ( baisser de plume)
	D02			= extinction de lumiere ( lever de plume)
	D03			= Flash
	D09			= VAPE Flash
	D51			= precede par G54 -> Select VAPE

	D10 ... D255 = Indentification d'outils ( d'ouvertures )
				Ne sont pas tj dans l'ordre ( voir tableau ci dessous)
*/


	/*************************************/
	/* Constantes utiles en trace GERBER */
	/*************************************/

/* codes de type de forme d'outils */
#define GERB_CIRCLE 1
#define GERB_RECT 2
#define GERB_LINE 3
#define GERB_OVALE 4
#define GERB_DONUT 5

/* liste des D_CODES en fonction de leur numero d'ordre (numero d'outil)
	(l'ordre 0 n'est pas utilise) ;
	Tools have D_CODES >= 10
	D_CODES <= 9 are used for commands only:
		D01 ... D9 = command codes for photo plotting:
		D01			= Light on
		D02			= Light off
		D03			= Flash
		D04 .. D08	= ?
		D09			= VAPE Flash
*/

#define FIRST_DCODE_VALUE 10

/* Structure de Description d'un D_CODE GERBER : */
class D_CODE
{
public:
	D_CODE * m_Pnext, * m_Pback;	/* for  a linked list */
	wxSize m_Size;					/* horiz and Vert size*/
	int m_Type;						/* Type ( Line, rect , circulaire , ovale .. ); -1 = not used (free) descr */
	int m_NumDcode;					/* code number ( >= 10 ); 0 = not in use */

	D_CODE()
	{
		m_Pnext = m_Pback = NULL;
		m_Type = -1;
		m_NumDcode = 0;
	}
} ;


#endif	/* #define PLOTGERB_H */

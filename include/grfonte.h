	/* grfonte.h : codage de la fonte de caracteres utilisee en trace de
		textes graphiques */
/*
formes des caracteres : definies sur matrice 10 x 13 sour forme de vecteurs
d'origine le debut du caractere (poSH_CODE matrice 0,0) sous forme d'une sequence
ascii .les lignes au dessous de 0,0 sont notees a,b,c.
conventions :
	coord 0...9 : ascii '0'...'9' (2 octets successifs = 1 coord)
	'U'		montee de plume
	'D'		descente de plume
	'X'		fin de trace

la matrice totale du caractere est ici 12x9. et l'espacement est de 13 points
	-2..0 = jambage au dessous de la ligne
	0..9 = cadre matrice
Version actuelle:
	en X : dim 0..6 a ramener en 0..9 ( espacement = 13 )
Matrice:

	9  ----------
	8  ----------
	7  ----------
	6  ----------
	5  ----------
	4  ----------
	3  ----------
	2  ----------
	1  ----------
___ 0  ----------
	-1 ----------
	-2 ----------
	-3 ----------
	   0123456789---

	dans les descr ci dessous:
		X = fin de descr
		U = plume Up
		D = plume Down
		n, n = 2 nombres = coord X et Y ( ici = -3 ... +9 )
*/

#ifndef _GRFONTE_H_
#define _GRFONTE_H_

#define SH_CODE signed char
extern const SH_CODE *graphic_fonte_shape[256];

#ifdef EDA_DRAWBASE

#define U (SH_CODE)'U'
#define X (SH_CODE)'X'
#define D (SH_CODE)'D'

const SH_CODE noshape[] = {X};		//code non inscriptible
const SH_CODE char_shape_space[] = {U,X};			/* space */
const SH_CODE char_shape_ipoint[] = {U,0,4,D,1,4,U,4,4,D,9,4,U,X};			/*!*/
const SH_CODE char_shape_openacc[] = {U,8,3,D,9,4,U,8,5,D,9,6,U,X};			/*{*/
const SH_CODE char_shape_dieze[] = {U,3,1,D,3,7,U,5,2,D,5,8,U,1,3,D,7,4,U,1,5,D,7,6,U,X};		/*#*/
const SH_CODE char_shape_dollar[] = {U,0,4,D,9,4,U,0,5,D,9,5,U,0,2,D,0,7,1,8,3,8,4,7,4,2,5,1,7,1,8,2,8,7,U,X};	/*$*/
const SH_CODE char_shape_percent[] = {U,0,1,D,9,8,U,9,1,D,7,1,7,3,9,3,9,1,U,0,6,D,0,8,2,8,2,6,0,6,U,X};	/*%*/
const SH_CODE char_shape_and[] = {U,0,7,D,7,2,8,2,9,4,8,6,7,6,2,1,1,1,0,3,0,4,1,6,2,7,U,X};			/*&*/
const SH_CODE char_shape_el[] = {U,9,4,D,7,3,U,X};				/*'*/
const SH_CODE char_shape_openpar[] = {U,0,4,D,2,3,7,3,9,4,U,X};				/*(*/
const SH_CODE char_shape_closepar[] = {U,0,4,D,2,5,7,5,9,4,U,X};				/*)*/
const SH_CODE char_shape_star[] = {U,1,4,D,7,4,U,4,1,D,4,7,U,2,2,D,6,6,U,6,2,D,2,6,U,X};		/***/
const SH_CODE char_shape_plus[] = {U,4,1,D,4,7,U,1,4,D,7,4,U,X};					/*+*/
const SH_CODE char_shape_comma[] = {U,1,4,D,0,4,-3,3,U,X};							/*,*/
const SH_CODE char_shape_minus[] = {U,4,2,D,4,7,U,X};							/*-*/
const SH_CODE char_shape_point[] = {U,0,4,D,1,4,U,X};							/*.*/
const SH_CODE char_shape_slash[] = {U,0,2,D,9,6,U,X};							/* / */

const SH_CODE char_shape_0[] = {U,0,1,D,9,8,U,1,1,D,8,1,9,2,9,7,8,8,1,8,0,7,0,2,1,1,U,X};		/*0*/
const SH_CODE char_shape_1[] = {U,0,3,D,0,7,U,0,5,D,9,5,U,8,5,D,6,3,U,X};				/*1*/
const SH_CODE char_shape_2[] = {U,0,8,D,0,1,3,1,4,2,4,7,5,8,8,8,9,7,9,2,8,1,U,X};			/*2*/
const SH_CODE char_shape_3[] = {U,9,1,D,9,8,8,8,5,4,5,7,4,8,1,8,0,7,0,2,1,1,U,X};			/*3*/
const SH_CODE char_shape_4[] = {U,0,6,D,9,6,2,1,2,8,U,X};						/*4*/
const SH_CODE char_shape_5[] = {U,2,1,D,1,1,0,2,0,7,1,8,4,8,6,6,6,1,9,1,9,8,U,X};			/*5*/
const SH_CODE char_shape_6[] = {U,8,8,D,9,7,9,2,8,1,1,1,0,2,0,7,1,8,4,8,5,7,5,2,4,1,U,X};		/*6*/
const SH_CODE char_shape_7[] = {U,9,1,D,9,8,0,1,U,X};						/*7*/
const SH_CODE char_shape_8[] = {U,0,2,D,1,1,4,1,5,2,5,7,6,8,8,8,9,7,9,2,8,1,6,1,5,2,U,5,7,D,4,8,1,8,0,7,0,2,U,X}; /*8*/
const SH_CODE char_shape_9[] = {U,0,2,D,0,7,1,8,8,8,9,7,9,2,8,1,5,1,4,2,4,7,5,8,U,X};		/*9*/

const SH_CODE char_shape_dbpoint[] = {U,6,4,D,5,4,U,3,4,D,2,4,U,X};			/*:*/
const SH_CODE char_shape_vpoint[] = {U,5,4,D,4,4,U,2,4,D,1,4,-2,3,U,X};			/*;*/
const SH_CODE char_shape_less[] = {U,0,7,D,4,2,9,7,U,X};				/*<*/
const SH_CODE char_shape_egal[] = {U,2,2,D,2,7,U,5,2,D,5,7,U,X};			/*=*/
const SH_CODE char_shape_more[] = {U,0,2,D,4,7,9,2,U,X};				/*>*/
const SH_CODE char_shape_intpoint[] = {U,0,4,D,1,4,U,3,4,D,4,4,5,6,6,7,8,7,9,6,9,2,8,1,U,X};		/*?*/
const SH_CODE char_shape_arond[] = {U,0,8,D,0,3,2,1,6,1,8,3,8,6,6,8,3,8,3,5,4,4,5,5,5,8,U,X};		/*@*/

const SH_CODE char_shape_A[] = {U,0,1,D,7,1,9,3,9,6,7,8,0,8,U,5,1,D,5,8,U,X};		/*A*/
const SH_CODE char_shape_B[] = {U,0,1,D,9,1,9,7,8,8,6,8,5,7,5,1,U,5,7,D,3,8,1,8,0,7,0,1,U,X};	/*B*/
const SH_CODE char_shape_C[] = {U,1,8,D,0,7,0,2,1,1,8,1,9,2,9,7,8,8,U,X};			/*C*/
const SH_CODE char_shape_D[] = {U,0,2,D,9,2,U,9,1,D,9,7,8,8,1,8,0,7,0,1,U,X};		/*D*/
const SH_CODE char_shape_E[] = {U,0,8,D,0,1,9,1,9,8,U,5,1,D,5,5,U,X};			/*E*/
const SH_CODE char_shape_F[] = {U,0,1,D,9,1,9,8,U,5,1,D,5,5,U,X};				/*F*/
const SH_CODE char_shape_G[] = {U,8,7,D,9,7,9,2,8,1,1,1,0,2,0,7,1,8,4,8,4,5,U,X};		/*G*/
const SH_CODE char_shape_H[] = {U,0,1,D,9,1,U,0,8,D,9,8,U,4,1,D,4,8,U,X};			/*H*/
const SH_CODE char_shape_I[] = {U,0,2,D,0,6,U,0,4,D,9,4,U,9,2,D,9,6,U,X};			/*I*/
const SH_CODE char_shape_J[] = {U,2,1,D,0,2,0,4,1,5,9,5,U,9,3,D,9,7,U,X};			/*J*/
const SH_CODE char_shape_K[] = {U,0,1,D,9,1,U,9,7,D,5,1,0,8,U,X};			/*K*/
const SH_CODE char_shape_L[] = {U,9,1,D,0,1,0,9,U,X};				/*L*/
const SH_CODE char_shape_M[] = {U,0,1,D,9,1,5,5,9,9,0,9,U,X};			/*M*/
const SH_CODE char_shape_N[] = {U,0,1,D,9,1,0,8,9,8,U,X};				/*N*/
const SH_CODE char_shape_O[] = {U,0,2,D,1,1,8,1,9,2,9,7,8,8,1,8,0,7,0,2,U,X};		/*O*/
const SH_CODE char_shape_P[] = {U,0,1,D,9,1,9,7,8,8,5,8,4,7,4,1,U,X};			/*P*/
const SH_CODE char_shape_Q[] = {U,1,1,D,8,1,9,2,9,7,8,8,1,8,0,7,0,2,1,1,U,4,6,D,0,8,U,X};		/*Q*/
const SH_CODE char_shape_R[] = {U,0,1,D,9,1,9,7,8,8,5,8,4,7,4,1,U,4,4,D,0,8,U,X};		/*R*/
const SH_CODE char_shape_S[] = {U,1,1,D,0,2,0,7,1,8,4,8,5,7,5,2,6,1,8,1,9,2,9,7,8,8,U,X};		/*S*/
const SH_CODE char_shape_T[] = {U,9,1,D,9,9,U,9,5,D,0,5,U,X};			/*T*/
const SH_CODE char_shape_U[] = {U,9,1,D,1,1,0,2,0,7,1,8,9,8,U,X};			/*U,*/
const SH_CODE char_shape_V[] = {U,9,1,D,0,5,9,9,U,X};				/*V*/
const SH_CODE char_shape_W[] = {U,9,1,D,0,3,3,5,0,7,9,9,U,X};			/*W*/
const SH_CODE char_shape_X[] = {U,0,1,D,9,8,U,9,1,D,0,8,U,X};			/*X*/
const SH_CODE char_shape_Y[] = {U,9,1,D,4,5,9,9,U,4,5,D,0,5,U,X};			/*Y*/
const SH_CODE char_shape_Z[] = {U,9,1,D,9,8,0,1,0,8,U,X};				/*Z*/

const SH_CODE char_shape_opencr[] = {U,0,5,D,0,3,9,3,9,5,U,X};				/*[*/
const SH_CODE char_shape_backslash[] = {U,9,1,D,0,7,U,X};				/*\*/
const SH_CODE char_shape_closecr[] = {U,0,3,D,0,5,9,5,9,3,U,X};				/*]*/
const SH_CODE char_shape_xor[] = {U,7,1,D,9,5,7,8,U,X};				/*^*/
const SH_CODE char_shape_underscore[] = {U,0,1,D,0,8,U,X};				/*_*/
const SH_CODE char_shape_altel[] = {U,9,4,D,7,5,U,X};				/*`*/

const SH_CODE char_shape_a[] = {U,6,1,D,6,7,5,8,0,8,0,2,1,1,2,1,3,2,3,8,U,X};		/*a*/
const SH_CODE char_shape_b[] = {U,9,1,D,0,1,0,8,4,8,5,7,5,1,U,X};		/*b*/
const SH_CODE char_shape_c[] = {U,1,8,D,0,7,0,2,1,1,5,1,6,2,6,7,5,8,U,X};			/*c*/
const SH_CODE char_shape_d[] = {U,9,8,D,0,8,0,2,1,1,5,1,6,2,6,8,U,X};		/*d*/
const SH_CODE char_shape_e[] = {U,3,1,D,3,8,5,8,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,X};		/*e*/
const SH_CODE char_shape_f[] = {U,0,2,D,8,2,9,3,9,7,8,8,U,5,1,D,5,4,U,X};			/*f*/
const SH_CODE char_shape_g[] = {U,-2,1,D,-3,2,-3,7,-2,8,5,8,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,X};		/*g*/
const SH_CODE char_shape_h[] = {U,0,1,D,9,1,U,5,1,D,6,2,6,7,5,8,0,8,U,X};			/*h*/
const SH_CODE char_shape_i[] = {U,9,4,D,8,4,U,5,4,D,1,4,0,5,0,6,U,X};			/*i*/
const SH_CODE char_shape_j[] = {U,9,4,D,8,4,U,5,4,D,-2,4,-3,3,-2,2,U,X};			/*j*/
const SH_CODE char_shape_k[] = {U,0,2,D,9,2,U,3,2,D,3,3,6,8,U,3,3,D,0,8,U,X};		/*k*/
const SH_CODE char_shape_l[] = {U,9,3,D,9,4,0,4,U,0,3,D,0,5,U,X};			/*l*/
const SH_CODE char_shape_m[] = {U,0,1,D,6,1,U,5,1,D,6,2,6,4,5,5,0,5,U,5,5,D,6,6,6,8,5,9,0,9,U,X};	/*m*/
const SH_CODE char_shape_n[] = {U,0,1,D,6,1,U,5,1,D,6,2,6,6,5,7,0,7,U,X};			/*n*/
const SH_CODE char_shape_o[] = {U,0,2,D,1,1,5,1,6,2,6,7,5,8,1,8,0,7,0,2,U,X};		/*o*/
const SH_CODE char_shape_p[] = {U,-3,1,D,6,1,U,5,1,D,6,2,6,7,5,8,1,8,0,7,0,2,1,1,U,X};		/*p*/
const SH_CODE char_shape_q[] = {U,-3,8,D,6,8,U,5,8,D,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,X};		/*q*/
const SH_CODE char_shape_r[] = {U,0,1,D,6,1,U,4,1,D,6,3,6,6,4,8,U,X};			/*r*/
const SH_CODE char_shape_s[] = {U,1,1,D,0,2,0,7,1,8,2,8,3,7,3,2,4,1,5,1,6,2,6,7,5,8,U,X};		/*s*/
const SH_CODE char_shape_t[] = {U,1,7,D,0,6,0,3,1,2,9,2,U,6,2,D,6,5,U,X};			/*t*/
const SH_CODE char_shape_u[] = {U,6,1,D,2,1,0,3,0,6,2,8,6,8,U,X};			/*u*/
const SH_CODE char_shape_v[] = {U,6,1,D,3,1,0,4,3,7,6,7,U,X};			/*v*/
const SH_CODE char_shape_w[] = {U,6,1,D,2,1,0,3,2,5,0,7,2,9,6,9,U,X};			/*w*/
const SH_CODE char_shape_x[] = {U,0,1,D,6,8,U,6,1,D,0,8,U,X};			/*x*/
const SH_CODE char_shape_y[] = {U,6,1,D,2,1,0,3,0,6,2,8,U,6,8,D,-2,8,-3,7,-3,2,-2,1,U,X}; /*y*/
const SH_CODE char_shape_z[] = {U,6,1,D,6,8,0,1,0,8,U,X}; /*z*/

const SH_CODE char_shape_opbrack[] = {U,9,5,D,8,4,6,4,5,3,4,3,3,4,1,4,0,5,U,X}; /*{*/
const SH_CODE char_shape_or[] = {U,9,4,D,0,4,U,X};				/*|*/
const SH_CODE char_shape_closebr[] = {U,9,3,D,8,4,6,4,5,5,4,5,3,4,1,4,0,3,U,X}; /*}*/
const SH_CODE char_shape_tilda[] = {U,8,1,D,9,2,9,3,7,5,7,6,8,7,U,X}; /*~*/
const SH_CODE char_shape_del[] = {X}; /*<del>*/

/* codes utiles >= 128 */
const SH_CODE char_shape_C_Cedille[] = {U,1,8,D,0,7,0,2,1,1,8,1,9,2,9,7,8,8,U,X}; /* C cedille*/
const SH_CODE char_shape_c_cedille[] = {U,1,8,D,0,7,0,2,1,1,5,1,6,2,6,7,5,8,U,0,4,D,-3,2,U,X}; /* ç */
const SH_CODE char_shape_a_grave[] = {U,6,1,D,6,7,5,8,0,8,0,2,1,1,2,1,3,2,3,8,U,7,4,D,9,2,U,X};
const SH_CODE char_shape_a_aigu[] = {U,6,1,D,6,7,5,8,0,8,0,2,1,1,2,1,3,2,3,8,U,7,2,D,9,4,U,X};
const SH_CODE char_shape_a_circ[] = {U,6,1,D,6,7,5,8,0,8,0,2,1,1,2,1,3,2,3,8,U,7,1,D,9,4,7,7,U,X};
const SH_CODE char_shape_a_trema[] = {U,6,1,D,6,7,5,8,0,8,0,2,1,1,2,1,3,2,3,8,U,9,2,D,9,3,U,9,5,D,9,6,U,X};
const SH_CODE char_shape_e_grave[] = {U,3,1,D,3,8,5,8,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,7,4,D,9,2,U,X};
const SH_CODE char_shape_e_aigu[] = {U,3,1,D,3,8,5,8,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,7,3,D,9,5,U,X};
const SH_CODE char_shape_e_circ[] = {U,3,1,D,3,8,5,8,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,7,1,D,9,4,7,7,U,X};
const SH_CODE char_shape_e_trema[] = {U,3,1,D,3,8,5,8,6,7,6,2,5,1,1,1,0,2,0,7,1,8,U,9,2,D,9,3,U,9,5,D,9,6,U,X};
const SH_CODE char_shape_i_trema[] = {U,5,4,D,1,4,0,5,0,6,U,9,2,D,9,3,U,9,5,D,9,6,U,X};
const SH_CODE char_shape_i_circ[] = {U,5,4,D,1,4,0,5,0,6,U,7,1,D,9,4,7,7,U,X};
const SH_CODE char_shape_u_grave[] = {U,6,1,D,2,1,0,3,0,6,2,8,6,8,U,7,4,D,9,2,U,X};
const SH_CODE char_shape_o_trema[] = {U,0,2,D,1,1,5,1,6,2,6,7,5,8,1,8,0,7,0,2,U,9,2,D,9,3,U,9,5,D,9,6,U,X};
const SH_CODE char_shape_o_circ[] = {U,0,2,D,1,1,5,1,6,2,6,7,5,8,1,8,0,7,0,2,U,7,2,D,9,4,7,7,U,X};
const SH_CODE char_shape_u_circ[] = {U,6,1,D,2,1,0,3,0,6,2,8,6,8,U,7,1,D,9,4,7,7,U,X};
const SH_CODE char_shape_u_trema[] = {U,6,1,D,2,1,0,3,0,6,2,8,6,8,U,9,2,D,9,3,U,9,5,D,9,6,U,X};

const SH_CODE *graphic_fonte_shape[256] =
	{
	// codes 0..31:
	noshape, noshape, noshape, noshape, noshape, noshape, noshape, noshape,
	noshape, noshape, noshape, noshape, noshape, noshape, noshape, noshape,
	noshape, noshape, noshape, noshape, noshape, noshape, noshape, noshape,
	noshape, noshape, noshape, noshape, noshape, noshape, noshape, noshape,
	// codes 32 .. 127:
	char_shape_space,					/* space */
	char_shape_ipoint,				/*!*/
	char_shape_openacc,
	char_shape_dieze,
	char_shape_dollar,
	char_shape_percent,
	char_shape_and,
	char_shape_el,
	char_shape_openpar,
	char_shape_closepar,
	char_shape_star,
	char_shape_plus,
	char_shape_comma,
	char_shape_minus,
	char_shape_point,
	char_shape_slash,
	char_shape_0, char_shape_1, char_shape_2, char_shape_3, char_shape_4, char_shape_5, char_shape_6, char_shape_7, char_shape_8, char_shape_9,
	char_shape_dbpoint,
	char_shape_vpoint,
	char_shape_less,
	char_shape_egal,
	char_shape_more,
	char_shape_intpoint,
	char_shape_arond,

	char_shape_A, char_shape_B, char_shape_C, char_shape_D, char_shape_E, char_shape_F, char_shape_G, char_shape_H, char_shape_I,
	char_shape_J, char_shape_K, char_shape_L, char_shape_M, char_shape_N, char_shape_O, char_shape_P, char_shape_Q, char_shape_R,
	char_shape_S, char_shape_T, char_shape_U, char_shape_V, char_shape_W, char_shape_X, char_shape_Y, char_shape_Z,

	char_shape_opencr,
	char_shape_backslash,
	char_shape_closecr,
	char_shape_xor,
	char_shape_underscore,
	char_shape_altel,

	char_shape_a, char_shape_b, char_shape_c, char_shape_d, char_shape_e, char_shape_f, char_shape_g, char_shape_h, char_shape_i,
	char_shape_j, char_shape_k, char_shape_l, char_shape_m, char_shape_n, char_shape_o, char_shape_p, char_shape_q, char_shape_r,
	char_shape_s, char_shape_t, char_shape_u, char_shape_v, char_shape_w, char_shape_x, char_shape_y, char_shape_z,

	char_shape_opbrack,
	char_shape_or,
	char_shape_closebr,
	char_shape_tilda,
	char_shape_del,
	// codes >= 128:
	noshape, noshape, noshape, noshape,		// 128..131
	noshape, noshape, noshape, noshape,	// 132..135
	noshape, noshape, noshape, noshape,	//136..139
	noshape, noshape, noshape, noshape,	//140..143
	noshape, noshape, noshape, noshape,	//144..147
	noshape, noshape, noshape, noshape,	//148..151
	noshape, noshape, noshape, noshape,	//152..155
	noshape, noshape, noshape, noshape,	//156..159

	noshape, noshape, noshape, noshape,	//160..163
	noshape, noshape, noshape, noshape,	//164..167
	noshape, noshape, noshape, noshape,	// 168..171
	noshape, noshape, noshape, noshape,	//172..175
	noshape, noshape, noshape, noshape,	//176..179
	noshape, noshape, noshape, noshape,	//180..183
	noshape, noshape, noshape, noshape,	//184..187
	noshape, noshape, noshape, noshape,	//188..191

	noshape, noshape, noshape, noshape,	//192..195
	noshape, noshape, noshape, char_shape_C_Cedille,	//196..199
	noshape, noshape, noshape, noshape,	// 200..203
	noshape, noshape, noshape, noshape,	//204..207
	noshape, noshape, noshape, noshape,	//208..211
	noshape, noshape, noshape, noshape,	//212..215
	noshape, noshape, noshape, noshape,	//216..219
	noshape, noshape, noshape, noshape,	//220..223

	char_shape_a_grave, char_shape_a_aigu, char_shape_a_circ, noshape,	//224..227
	char_shape_a_trema, noshape, noshape, char_shape_c_cedille,	//228..231
	char_shape_e_grave, char_shape_e_aigu, char_shape_e_circ, char_shape_e_trema,	//232..235
	noshape, noshape, char_shape_i_circ, char_shape_i_trema,	//236..239
	noshape, noshape, noshape, noshape,	//240..243
	char_shape_o_circ, noshape, char_shape_o_trema, noshape,	//244..247
	noshape, char_shape_u_grave, noshape, char_shape_u_circ,	//248..251
	char_shape_u_trema, noshape, noshape, noshape,	//252..255

	} ;
#endif
#endif	// ifndef _GRFONTE_H_


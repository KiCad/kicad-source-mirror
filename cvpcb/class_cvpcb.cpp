/*******************/
/* class_cvpcb.cpp */
/*******************/

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "fctsys.h"
#include "common.h"

#include "cvpcb.h"

STORECMP::STORECMP(void)
{
	Pnext = Pback = NULL;
	m_Type = STRUCT_COMPONENT;
	m_Pins = NULL;
	m_Num = 0;
	m_Multi = 0;
}

STORECMP::~STORECMP(void)
{
STOREPIN * Pin, * NextPin;
	
	for( Pin = m_Pins; Pin != NULL; Pin = NextPin )
	{
		NextPin = Pin->Pnext; delete Pin;
	}
}



STOREMOD::STOREMOD(void)
{
	Pnext = Pback = NULL;
	m_Type = STRUCT_MODULE;
	m_Num = 0;
}


STOREPIN::STOREPIN(void)
{
	m_Type = STRUCT_PIN;	/* Type de la structure */
	Pnext = NULL;			/* Chainage avant */
	m_Index = 0;				/* variable utilisee selon types de netlistes */
	m_PinType = 0;			/* code type electrique ( Entree Sortie Passive..) */
}


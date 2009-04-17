/**
 * This file is part of the common libary
 * @file  confirm.h
 * @see   common.h
 */


#ifndef __INCLUDE__CONFIRM_H__
#define __INCLUDE__CONFIRM_H__ 1


void    DisplayError( wxWindow* parent, const wxString& msg, int displaytime = 0 );
void    DisplayInfoMessage( wxWindow* parent, const wxString& msg, int displaytime = 0 );

/* Routines d'affichage messages ( disparait au bout de displaytime 0.1 secondes) */

bool    IsOK( wxWindow* parent, const wxString& msg );

/* Routine affichant la fenetre "CONFIRMATION"
 *  Retourne 1 ou 0 selon reponse Yes / No */

int     Get_Message( const wxString& title,
                     const wxString& frame_caption,
                     wxString& buffer,
                     wxWindow* frame );

/* Fonction d'installation du menu de Dialogue
 *  entree: titre = titre a afficher
 *  entree/sortie :buffer : contient la reponse
 *           si a l'appel buffer n'est pas vide, son contenu est aussi
 *           affiche, mais disparait a la 1ere correction */


#endif /* __INCLUDE__CONFIRM_H__ */


/**
 * This file is part of the common libary.
 * @file  eda_doc.h
 * @see   common.h
 */

#ifndef __INCLUDE__EDA_DOC_H__
#define __INCLUDE__EDA_DOC_H__ 1


int     KeyWordOk( const wxString& KeyList,
                   const wxString& Database );

/* Recherche si dans le texte Database on retrouve tous les mots
 *  cles donnes dans KeyList ( KeyList = suite de mots cles
 *  separes par des espaces
 *  Retourne:
 *      0 si aucun mot cle trouv�
 *      1 si mot cle trouv�
 */
bool    GetAssociatedDocument( wxFrame* frame, 
                        const wxString& LibPath,
                        const wxString& DocName );


#endif /* __INCLUDE__EDA_DOC_H__ */


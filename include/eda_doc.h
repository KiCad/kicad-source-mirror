/**
 * This file is part of the common libary.
 * @file  eda_doc.h
 * @see   common.h
 */

#ifndef __INCLUDE__EDA_DOC_H__
#define __INCLUDE__EDA_DOC_H__ 1



/* Recherche si dans le texte Database on retrouve tous les mots
 *  cles donnes dans KeyList ( KeyList = suite de mots cles
 *  separes par des espaces
 *  Retourne:
 *      0 si aucun mot cle trouve
 *      1 si mot cle trouve
 */
int     KeyWordOk( const wxString& KeyList,
                   const wxString& Database );

/** Function GetAssociatedDocument
 * open a document (file) with the suitable browser
 * @param aFrame = main frame
 * @param aDocName = filename of file to open (Full filename or short filename)
 * if DocName is starting by http: or ftp: or www. the default internet browser is launched
 * @param aPaths = a wxPathList to explore.
 *              if NULL or aDocName is a full filename, aPath is not used.
*/ 
bool    GetAssociatedDocument( wxFrame* aFrame, 
                        const wxString& aDocName,
                        const wxPathList* aPaths = NULL );


#endif /* __INCLUDE__EDA_DOC_H__ */


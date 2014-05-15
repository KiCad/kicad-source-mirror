/**
 * This file is part of the common library.
 * @file  eda_doc.h
 * @see   common.h
 */

#ifndef __INCLUDE__EDA_DOC_H__
#define __INCLUDE__EDA_DOC_H__ 1


/**
 * Function KeyWordOk
 * searches \a aKeyList for any words found in \a aDatabase.
 *
 * @return 0 if no keyword is found or 1 if keyword is found.
 */
int KeyWordOk( const wxString& aKeyList, const wxString& aDatabase );

/**
 * Function GetAssociatedDocument
 * open a document (file) with the suitable browser
 * @param aParent = main frame
 * @param aDocName = filename of file to open (Full filename or short filename)
 * if \a aDocName begins with http: or ftp: or www. the default internet browser is launched
 * @param aPaths = a wxPathList to explore.
 *              if NULL or aDocName is a full filename, aPath is not used.
*/
bool GetAssociatedDocument( wxWindow* aParent,
                            const wxString& aDocName,
                            const wxPathList* aPaths = NULL );


#endif /* __INCLUDE__EDA_DOC_H__ */

/**
 * This file is part of the common library.
 * @file  eda_doc.h
 * @see   common.h
 */

#ifndef __INCLUDE__EDA_DOC_H__
#define __INCLUDE__EDA_DOC_H__ 1


/* Search the text Database for found all the key words in the KeyList.
 *
 * Returns:
 * 0 if no keyword is found
 * 1 if keyword found.
 */
int  KeyWordOk( const wxString& KeyList,
                const wxString& Database );

/** Function GetAssociatedDocument
 * open a document (file) with the suitable browser
 * @param aFrame = main frame
 * @param aDocName = filename of file to open (Full filename or short filename)
 * if DocName is starting by http: or ftp: or www. the default internet
 * browser is launched
 * @param aPaths = a wxPathList to explore.
 *              if NULL or aDocName is a full filename, aPath is not used.
*/
bool GetAssociatedDocument( wxFrame* aFrame,
                            const wxString& aDocName,
                            const wxPathList* aPaths = NULL );


#endif /* __INCLUDE__EDA_DOC_H__ */

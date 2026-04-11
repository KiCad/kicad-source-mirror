/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jerry Jacobs
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * This file is part of the common library.
 * @file  eda_doc.h
 * @see   common.h
 */

#ifndef __INCLUDE__EDA_DOC_H__
#define __INCLUDE__EDA_DOC_H__ 1

#include <kicommon.h>

class EMBEDDED_FILES;

/**
 * Open a document (file) with the suitable browser.
 *
 * Environmental variables are substituted before the document name is resolved for
 * either browser or file.  If \a aDocName has an associated URI handler on the system,
 * the default handler will be launched.
 *
 * @param aParent main frame.
 * @param aDocName filename of file to open (Full filename or short filename).
 * @param aPaths Additional paths to search for local disk datasheet files
*/
bool KICOMMON_API GetAssociatedDocument( wxWindow* aParent, const wxString& aDocName, PROJECT* aProject,
                            SEARCH_STACK* aPaths = nullptr,
                            std::vector<EMBEDDED_FILES*> aFilesStack = {} );


#endif /* __INCLUDE__EDA_DOC_H__ */

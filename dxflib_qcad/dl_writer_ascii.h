/****************************************************************************
** Copyright (C) 2001-2013 RibbonSoft, GmbH. All rights reserved.
** Copyright (C) 2001 Robert J. Campbell Jr.
**
** This file is part of the dxflib project.
**
** This file is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** Licensees holding valid dxflib Professional Edition licenses may use
** this file in accordance with the dxflib Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.ribbonsoft.com for further details.
**
** Contact info@ribbonsoft.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef DL_WRITER_ASCII_H
#define DL_WRITER_ASCII_H

#include "dl_global.h"


#include "dl_writer.h"
#include <fstream>
#include <string>

/**
 * Implements functions defined in DL_Writer for writing low
 *   level DXF constructs to an ASCII format DXF file.
 *
 * @param fname File name of the file to be created.
 * @param version DXF version. Defaults to DL_VERSION_2002.
 *
 * @todo What if \c fname is NULL?  Or \c fname can't be opened for
 * another reason?
 */
class DXFLIB_EXPORT DL_WriterA : public DL_Writer
{
public:
    DL_WriterA( const char* afname, DL_Codes::version aversion = DL_VERSION_2000 )
        : DL_Writer( aversion ), m_ofile( afname ) {}
    virtual ~DL_WriterA() {}

    bool    openFailed() const;
    void    close() const;
    void    dxfReal( int gc, double value ) const override;
    void    dxfInt( int gc, int value ) const override;
    void    dxfHex( int gc, int value ) const override;
    void    dxfString( int gc, const char* value ) const override;
    void    dxfString( int gc, const std::string& value ) const override;

    static void strReplace( char* str, char src, char dest );

private:
    /**
     * DXF file to be created.
     */
    mutable std::ofstream m_ofile;
};

#endif

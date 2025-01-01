/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2018 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file X2_gerber_attributes.h
 */

#ifndef X2_GERBER_ATTRIBUTE_H
#define X2_GERBER_ATTRIBUTE_H

/*
 * Manage the gerber extensions (attributes) in the new X2 version
 * only few extensions are handled
 * See http://www.ucamco.com/files/downloads/file/81/the_gerber_file_format_specification.pdf
 *
 * gerber attributes in the new X2 version look like:
 * %TF.FileFunction,Copper,L1,Top*%
 *
 * Currently:
 * .FileFunction .FileFunction Identifies the file's function in the PCB.
 *  Other Standard Attributes, not yet used in Gerbview:
 * .Part Identifies the part the file represents, e.g. a single PCB
 * .MD5 Sets the MD5 file signature or checksum.
 */

#include <wx/arrstr.h>

/**
 * The attribute value consists of a number of substrings separated by a comma
*/

class X2_ATTRIBUTE
{
public:
    X2_ATTRIBUTE();
    ~X2_ATTRIBUTE();

    /**
     * @return the parameters list read in TF command.
     */
    wxArrayString& GetPrms() { return m_Prms; }

    /**
     * @return a parameter read in TF command.
     * @param aIdx = the index of the parameter
     * aIdx = 0 is the parameter read after the TF function
     * (the same as GetAttribute())
     */
    const wxString& GetPrm( int aIdx );

    /**
     * @return the attribute name (for instance .FileFunction)
     * which is given by TF command (i.e. the first parameter read).
     */
    const wxString& GetAttribute();

    /**
     * @return the number of parameters read in %TF
     * (or similar like %TA %TO ...) command.
     */
    int GetPrmCount() { return int( m_Prms.GetCount() ); }

    /**
     * Parse a TF command terminated with a % and fill m_Prms by the parameters found.
     *
     * @param aFile = a FILE* ptr to the current Gerber file.
     * @param aBuffer = the buffer containing current Gerber data (can be null)
     * @param aBuffSize = the size of the buffer
     * @param aText = a pointer to the first char to read from Gerber data stored in aBuffer
     *  After parsing, text points the last char of the command line ('%') (X2 mode)
     *  or the end of line if the line does not contain '%' or aBuffer == NULL (X1 mode)
     * @param aLineNum = a point to the current line number of aFile
     * @return true if no error.
     */
    bool ParseAttribCmd( FILE* aFile, char *aBuffer, int aBuffSize, char* &aText, int& aLineNum );

    /**
     * Debug function: print using wxLogMessage le list of parameters
     */
    void DbgListPrms();

    /**
     * Return true if the attribute is .FileFunction
     */
    bool IsFileFunction()
    {
        return GetAttribute().IsSameAs( wxT(".FileFunction"), false );
    }

    /**
     * Return true if the attribute is .MD5
     */
    bool IsFileMD5()
    {
        return GetAttribute().IsSameAs( wxT(".MD5"), false );
    }

    /**
     * Return true if the attribute is .Part
     */
    bool IsFilePart()
    {
        return GetAttribute().IsSameAs( wxT(".Part"), false );
    }

protected:
    wxArrayString m_Prms;   ///< the list of parameters (after TF) in gbr file
                            ///< the first one is the attribute name,
                            ///< if starting by '.'
};

/**
 * X2_ATTRIBUTE_FILEFUNCTION ( from %TF.FileFunction in Gerber file)
 *  Example file function:
 *  %TF.FileFunction,Copper,L1,Top*%
 * - Type. Such as copper, solder mask etc.
 * - Position. Specifies where the file appears in the PCB layer structure.
 *      Corresponding position substring:
 *      Copper layer:   L1, L2, L3...to indicate the layer position followed by Top, Inr or
 *                      Bot. L1 is always the top copper layer. E.g. L2,Inr.
 *      Extra layer, e.g. solder mask: Top or Bot - defines the attachment of the layer.
 *      Drill/rout layer: E.g. 1,4 - where 1 is the start and 4 is the end copper layer. The
 *                        pair 1,4 defines the span of the drill/rout file
 * Optional index. This can be used in instances where for example there are two solder
 *                 masks on the same side. The index counts from the PCB surface outwards.
 */

class X2_ATTRIBUTE_FILEFUNCTION : public X2_ATTRIBUTE
{
public:
    X2_ATTRIBUTE_FILEFUNCTION( X2_ATTRIBUTE& aAttributeBase );

    bool IsCopper();                    ///< return true if the filefunction type is "Copper"

    /**
     * @return true if the filefunction type is "Plated" or "NotPlated"
     * therefore a drill file
     */
    bool IsDrillFile();

    const wxString& GetFileType();      ///< the type of layer (Copper, Soldermask ... )
    const wxString& GetBrdLayerId();    ///< the brd layer identifier: Ln, only for Copper type
                                        ///< or Top, Bot for other types
    /**
     * @return the brd layer pair identifier: n,m for drill files
     * (files with m_Prms.Item( 1 ) = "Plated" or "NotPlated")
     */
    const wxString GetDrillLayerPair();

    /**
     * @return the Layer Pair type for drill files
     * (PTH, NPTH, Blind or Buried)
     */
    const wxString& GetLPType();

    /**
     * @return the drill/routing type for drill files
     * (Drill, Route, Mixed)
     */
    const wxString& GetRouteType();

    const wxString& GetBrdLayerSide();  ///< the brd layer Pos: Top, Bot, Inr
                                        ///< same as GetBrdLayerId() for non copper type
    const wxString& GetLabel();         ///< the filefunction label, if any

    int GetZOrder() { return m_z_order; }           ///< the Order of the board layer,
                                                    ///< from front (Top) side to back (Bot) side
    int GetZSubOrder() { return m_z_sub_order; }    ///< the Order of the bdr copper layer,
                                                    ///< from front (Top) side to back (Bot) side

private:

    /**
     * Initialize the z order priority of the current file, from its attributes
     */
    void set_Z_Order();

    int m_z_order;              // the z order of the layer for a board
    int m_z_sub_order;          // the z sub_order of the copper layer for a board
};

#endif      // X2_GERBER_ATTRIBUTE_H

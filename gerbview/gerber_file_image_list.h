/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2016 Jean-Pierre Charras  jp.charras at wanadoo.fr
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

#ifndef GERBER_FILE_IMAGE_LIST_H
#define GERBER_FILE_IMAGE_LIST_H

#include <vector>
#include <set>
#include <unordered_map>

#include <gerber_draw_item.h>
#include <am_primitive.h>


enum class GERBER_ORDER_ENUM : int
{
    GERBER_DRILL = 0,
    GERBER_BOARD_OUTLINE,
    GERBER_KEEP_OUT,
    GERBER_MECHANICAL,
    GERBER_TOP_PASTE,
    GERBER_TOP_SILK_SCREEN,
    GERBER_TOP_SOLDER_MASK,
    GERBER_TOP_COPPER,
    GERBER_INNER,
    GERBER_BOTTOM_COPPER,
    GERBER_BOTTOM_SOLDER_MASK,
    GERBER_BOTTOM_SILK_SCREEN,
    GERBER_BOTTOM_PASTE,
    GERBER_LAYER_UNKNOWN,
};


/* gerber files have different parameters to define units and how items must be plotted.
 *  some are for the entire file, and other can change along a file.
 *  In Gerber world:
 *  an image is the entire gerber file and its "global" parameters
 *  a layer (that is very different from a board layer) is just a sub set of a file that
 *  have specific parameters
 *  if a Image parameter is set more than once, only the last value is used
 *  Some parameters can change along a file and are not layer specific: they are stored
 *  in GERBER_ITEM items, when instanced.
 *
 *  In GerbView, to handle these parameters, there are 2 classes:
 *  GERBER_FILE_IMAGE : the main class containing most of parameters and data to plot a graphic layer
 *  Some of them can change along the file
 *  There is one GERBER_FILE_IMAGE per file and one graphic layer per file or GERBER_FILE_IMAGE
 *  GerbView does not read and merge 2 gerber file in one graphic layer:
 *  I believe this is not possible due to the constraints in Image parameters.
 *  GERBER_LAYER : containing the subset of parameters that is layer specific
 *  A GERBER_FILE_IMAGE must include one GERBER_LAYER to define all parameters to plot a file.
 *  But a GERBER_FILE_IMAGE can use more than one GERBER_LAYER.
 */

typedef bool( LayerSortFunction )( const GERBER_FILE_IMAGE* const& ref,
                                   const GERBER_FILE_IMAGE* const& test );

class GERBER_FILE_IMAGE;

/**
 * @brief GERBER_FILE_IMAGE_LIST is a helper class to handle a list of GERBER_FILE_IMAGE files
 * which are loaded and can be displayed
 * there are 32 images max which can be loaded
 */
class GERBER_FILE_IMAGE_LIST
{
public:
    GERBER_FILE_IMAGE_LIST();
    ~GERBER_FILE_IMAGE_LIST();

    /**
     * @brief Utility function to guess which PCB layer of a gerber/drill file corresponds to based
     * on its file extension.
     *
     * Supports many CAD program file extension patterns. Detects gerbers, drills, edge layers, etc.
     *
     * @param filename Filename to check against the extension matching table.
     * @param order Returns the order that this layer should be placed in a set of layers.
     * @param matchedExtension The actual extension pattern that this filename matched.
     */
    static void GetGerberLayerFromFilename( const wxString& filename, enum GERBER_ORDER_ENUM& order,
                                            wxString& matchedExtension );

    static GERBER_FILE_IMAGE_LIST& GetImagesList();
    GERBER_FILE_IMAGE* GetGbrImage( int aIdx );

    unsigned ImagesMaxCount() { return m_GERBER_List.size(); }

    /**
     * Add a GERBER_FILE_IMAGE* at index aIdx or at the first free location if aIdx < 0
     *
     * @param aGbrImage is the image to add.
     * @param aIdx is the location to use ( 0 ... GERBER_DRAWLAYERS_COUNT-1 ).
     * @return true if the index used, or -1 if no room to add image
     */
    int AddGbrImage( GERBER_FILE_IMAGE* aGbrImage, int aIdx );


    /**
     * Remove all loaded data in list, and delete all images, freeing the memory.
     */
    void DeleteAllImages();

    /**
     * Delete the loaded data of image \a aIdx, freeing the memory.
     *
     * @param aIdx is the index ( 0 ... GERBER_DRAWLAYERS_COUNT-1 ).
     */
    void DeleteImage( unsigned int aIdx );

    /**
     * Get the display name for the layer at \a aIdx.
     *
     * if a file is loaded, the name is:
     * "<aIdx+1> <short filename> <X2 FileFunction info> if a X2 FileFunction info is found"
     * or (if no FileFunction info)
     * "<aIdx+1> <short filename> *"
     * if no file loaded, the name is:
     *  "Layer n"  with n = aIdx+1
     *
     * @param aIdx is the index ( 0 ... GERBER_DRAWLAYERS_COUNT-1 ).
     * @param aNameOnly set to false (default) to add the layer number (for layers manager)
     *                  or true to return only the name without layer name (status bar)/
     * @param aFullName set to false (default) to ellipsize the name, true to return the full name.
     *
     * @return a name for image aIdx which can be used in layers manager and layer selector or
     *         in the status bar
     */
    const wxString GetDisplayName( int aIdx, bool aNameOnly = false, bool aFullName = false );

    /**
     * Sort loaded images by file extension matching
     *
     * @return a mapping of old to new layer index
     */
    std::unordered_map<int, int> SortImagesByFunction( LayerSortFunction sortFunction );

    /**
     * Sort loaded images by file extension matching
     *
     * @return a mapping of old to new layer index
     */
    std::unordered_map<int, int> SortImagesByFileExtension();

    /**
     * Sort loaded images by Z order priority, if they have the X2 FileFormat info
     * (SortImagesByZOrder updates the graphic layer of these items)
     *
     * @return a mapping of old to new layer index
     */
    std::unordered_map<int, int> SortImagesByZOrder();

    /**
     * Swap two images and their orders.
     *
     * @return a mapping of old to new layer index
     */
    std::unordered_map<int, int> SwapImages( unsigned int layer1, unsigned int layer2 );

    /**
     * Removes (and deletes) an image, rotating the removed image to the end.
     *
     * @return a mapping of old to new layer index
     */
    std::unordered_map<int, int> RemoveImage( unsigned int layer );

    /**
     * Get number of loaded images
     *
     * @return number of images loaded
     */
    unsigned GetLoadedImageCount();

private:
    /**
     * When the image order has changed, call this to get a mapping
     * to pass to the frame's Remap() function to remap the widgets
     * layer order.
     *
     * @return Map describing the remap.
     */
    std::unordered_map<int, int> GetLayerRemap();

    // the list of loaded images (1 image = 1 gerber file)
    std::vector<GERBER_FILE_IMAGE*> m_GERBER_List;
};

#endif  // ifndef GERBER_FILE_IMAGE_LIST_H

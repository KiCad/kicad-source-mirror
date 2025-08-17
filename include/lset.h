/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LSET_H
#define LSET_H

#include <layer_ids.h>
#include <base_set.h>

class LSEQ;
class LAYER_RANGE;

/**
 * LSET is a set of PCB_LAYER_IDs.
 *
 * It can be converted to numerous purpose LSEQs using the various member functions, most of
 * which are based on Seq(). The advantage of converting to #LSEQ using purposeful code, is it
 * removes any dependency on order/sequence inherent in this set.
 */
class KICOMMON_API LSET : public BASE_SET
{
public:

    /**
     * Create an empty (cleared) set.
     */
    LSET() : BASE_SET( PCB_LAYER_ID_COUNT ) {} // all bits are set to zero in BASE_SET()

    LSET( const BASE_SET& aOther ) : BASE_SET( aOther ) {}

    LSET( std::initializer_list<PCB_LAYER_ID> aList );

    LSET( const std::vector<PCB_LAYER_ID>& aList );

    LSET( const LSEQ& aSeq );

    LSET( const LAYER_RANGE& aRange );

    LSET( unsigned long __val ) = delete;

    /**
     * See if the layer set contains a PCB layer.
     *
     * @param aLayer is the layer to check.
     * @return true if the layer is included.
     */
    bool Contains( PCB_LAYER_ID aLayer ) const
    {
        // At the moment, LSET cannot store negative layers, but PCB_LAYER_ID can contain them
        if( aLayer < 0 )
           return false;

        try
        {
            return test( aLayer );
        }
        catch( std::out_of_range& )
        {
            return false;
        }
    }

    /**
     * Return the fixed name association with @a aLayerId.
     *
     * @note These names must not be translated or changed.  They are used as tokens in the board
     * file format because the ordinal value of the PCB_LAYER_ID enum was not stable over time.
     * @see LayerName() for what should be used to display the default name of a layer in the GUI.
     */
    static wxString Name( PCB_LAYER_ID aLayerId );

    /**
     * Return the layer number from a layer name.
     */
    static int NameToLayer( wxString& aName );

    /**
     * Return true if aLayer is between aStart and aEnd, inclusive.
     *
     * This takes into account the direction of the layers and the fact that #B_Cu comes before
     * In*_Cu.
     */
    static bool IsBetween( PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd, PCB_LAYER_ID aLayer );

    /**
     * Return a complete set of internal copper layers which is all Cu layers
     * except #F_Cu and #B_Cu.
     */
    static const LSET& InternalCuMask();

    /**
     * Return a complete set of all top assembly layers which is all #F_SilkS and #F_Mask.
     */
    static const LSET& FrontAssembly();

    /**
     * Return a complete set of all bottom assembly layers which is all #B_SilkS and #B_Mask.
     */
    static const LSET& BackAssembly();

    /**
     * Return a mask holding the requested number of Cu PCB_LAYER_IDs.
     */
    static LSET AllCuMask( int aCuLayerCount );

    /**
     * return AllCuMask( MAX_CU_LAYERS );
     */
    static LSET AllCuMask();

    /**
     * Return a mask holding the Front and Bottom layers.
     */
    static const LSET& ExternalCuMask();

    /**
     * Return a mask holding all layer minus CU layers.
     */
    static LSET AllNonCuMask();

    static const LSET& AllLayersMask();

    /**
     * Return a mask holding all technical layers (no CU layer) on front side.
     */
    static const LSET& FrontTechMask();

    /**
     * Return a mask holding technical layers used in a board fabrication
     * (no CU layer) on front side.
     */
    static const LSET& FrontBoardTechMask();

    /**
     * Return a mask holding all technical layers (no CU layer) on back side.
     */
    static const LSET& BackTechMask();

    /**
     * Return a mask holding technical layers used in a board fabrication
     * (no CU layer) on Back side.
     */
    static const LSET& BackBoardTechMask();

    /**
     * Return a mask holding all technical layers (no CU layer) on both side.
     */
    static const LSET& AllTechMask();

    /**
     * Return a mask holding board technical layers (no CU layer) on both side.
     */
    static const LSET& AllBoardTechMask();

    /**
     * Return a mask holding all technical layers and the external CU layer on front side.
     */
    static const LSET& FrontMask();

    /**
     * Return a mask holding all technical layers and the external CU layer on back side.
     */
    static const LSET& BackMask();

    static const LSET& SideSpecificMask();

    static const LSET& UserMask();

    /**
     * Return a mask holding all layers which are physically realized.
     *
     * Equivalent to the copper layers + the board tech mask.
     */
    static const LSET& PhysicalLayersMask();

    /**
     * Return a mask with the requested number of user defined layers.
     *
     * @param aUserDefinedLayerCount The number of user defined layers
     */
    static LSET UserDefinedLayersMask( int aUserDefinedLayerCount = MAX_USER_DEFINED_LAYERS );

    /**
     * Return a sequence of copper layers in starting from the front/top
     * and extending to the back/bottom.
     */
    LSEQ CuStack() const;

    /**
     * Return the technical and user layers in the order shown in layer widget.
     */
    LSEQ TechAndUserUIOrder() const;

    /**
     * Return the copper, technical and user layers in the order shown in layer widget.
     */
    LSEQ UIOrder() const;

    /**
     * Return an #LSEQ from the union of this #LSET and a desired sequence.
     *
     * The #LSEQ element will be in the same sequence as aWishListSequence if they are present.
     *
     * @param aWishListSequence establishes the order of the returned LSEQ, and the LSEQ will only
     *        contain PCB_LAYER_IDs which are present in this set.
     */
    LSEQ Seq( const LSEQ& aSequence ) const;

    /**
     * Return a #LSEQ from this #LSET in ascending PCB_LAYER_ID order.
     *
     * Each #LSEQ element will be in the same sequence as in PCB_LAYER_ID and only present
     * in the resultant #LSEQ if present in this set.  Therefore the sequence is subject to
     * change, use it only when enumeration and not order is important.
     */
    LSEQ Seq() const;

    /**
     * Generate a sequence of layers that represent a top to bottom stack of this set of layers.
     *
     * @param aSelectedLayer is the layer to put at the top of stack when defined.
     * @return the top to bottom layer sequence.
     */
    LSEQ SeqStackupTop2Bottom( PCB_LAYER_ID aSelectedLayer = UNDEFINED_LAYER ) const;

    /**
     * Return the sequence that is typical for a bottom-to-top stack-up.
     *
     * For instance, to plot multiple layers in a single image, the top layers output last.
     */
    LSEQ SeqStackupForPlotting() const;

    /**
     * Execute a function on each layer of the #LSET.
     */
    void RunOnLayers( const std::function<void( PCB_LAYER_ID )>& aFunction ) const
    {
        for( size_t ii = 0; ii < size(); ++ii )
        {
            if( test( ii ) )
                aFunction( PCB_LAYER_ID( ii ) );
        }
    }

    /**
     * Find the first set #PCB_LAYER_ID.
     *
     * @return #UNDEFINED_LAYER if more than one is set or #UNSELECTED_LAYER if none is set.
     */
    PCB_LAYER_ID ExtractLayer() const;


    /**
     * Flip the layers in this set.
     *
     * BACK and FRONT copper layers, mask, paste, solder layers are swapped
     * internal layers are flipped only if the copper layers count is known.
     *
     * @param aMask = the LSET to flip
     * @param aCopperLayersCount = the number of copper layers. if 0 (in fact if < 4 )
     *  internal layers will be not flipped because the layer count is not known
     */
    LSET& FlipStandardLayers( int aCopperLayersCount = 0 );

    /**
     * Return the number of layers between aStart and aEnd, inclusive.
     */
    static int LayerCount( PCB_LAYER_ID aStart, PCB_LAYER_ID aEnd, int aCopperLayerCount );

    /**
     * Clear the copper layers in this set.
     */
    LSET& ClearCopperLayers();

    /**
     * Clear the non-copper layers in this set.
     */
    LSET& ClearNonCopperLayers();

    /**
     * Clear the user defined layers in this set.
     */
    LSET& ClearUserDefinedLayers();

#ifndef SWIG
    // Custom iterator to iterate over all set bits
    class KICOMMON_API all_set_layers_iterator : public BASE_SET::set_bits_iterator
    {
    public:
        all_set_layers_iterator( const BASE_SET& set, size_t index ) :
                BASE_SET::set_bits_iterator( set, index )
        {
        }

        PCB_LAYER_ID operator*() const
        {
            return PCB_LAYER_ID( BASE_SET::set_bits_iterator::operator*() );
        }

        all_set_layers_iterator& operator++()
        {
            BASE_SET::set_bits_iterator::operator++();
            return *this;
        }
    };

    all_set_layers_iterator begin() const { return all_set_layers_iterator( *this, 0 ); }
    all_set_layers_iterator end() const { return all_set_layers_iterator( *this, size() ); }

    // Custom iterators for Copper and Non-Copper layers
    class KICOMMON_API copper_layers_iterator : public BASE_SET::set_bits_iterator
    {
    public:
        copper_layers_iterator( const BASE_SET& set, size_t index );
        PCB_LAYER_ID            operator*() const;
        copper_layers_iterator& operator++();

    private:
        void advance_to_next_set_copper_bit();
        void next_copper_layer();
    };

    class KICOMMON_API non_copper_layers_iterator : public BASE_SET::set_bits_iterator
    {
    public:
        non_copper_layers_iterator( const BASE_SET& set, size_t index );
        PCB_LAYER_ID                operator*() const;
        non_copper_layers_iterator& operator++();

    private:
        void advance_to_next_set_non_copper_bit();
    };

    copper_layers_iterator copper_layers_begin() const;
    copper_layers_iterator copper_layers_end() const;
    non_copper_layers_iterator non_copper_layers_begin() const;
    non_copper_layers_iterator non_copper_layers_end() const;

#endif

};
#endif // LSET_H

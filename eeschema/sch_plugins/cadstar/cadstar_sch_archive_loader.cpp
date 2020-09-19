/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file cadstar_sch_archive_loader.cpp
 * @brief Loads a csa file into a KiCad SCHEMATIC object
 */

#include <sch_plugins/cadstar/cadstar_sch_archive_loader.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <trigo.h>


void CADSTAR_SCH_ARCHIVE_LOADER::Load( ::SCHEMATIC* aSchematic, ::SCH_SHEET* aRootSheet )
{
    Parse();

    LONGPOINT designLimit = Assignments.Settings.DesignLimit;

    //Note: can't use getKiCadPoint() due wxPoint being int - need long long to make the check
    long long designSizeXkicad = (long long) designLimit.x * KiCadUnitMultiplier;
    long long designSizeYkicad = (long long) designLimit.y * KiCadUnitMultiplier;

    // Max size limited by the positive dimension of wxPoint (which is an int)
    long long maxDesignSizekicad = std::numeric_limits<int>::max();

    if( designSizeXkicad > maxDesignSizekicad || designSizeYkicad > maxDesignSizekicad )
        THROW_IO_ERROR( wxString::Format(
                _( "The design is too large and cannot be imported into KiCad. \n"
                   "Please reduce the maximum design size in CADSTAR by navigating to: \n"
                   "Design Tab -> Properties -> Design Options -> Maximum Design Size. \n"
                   "Current Design size: %.2f, %.2f millimeters. \n"
                   "Maximum permitted design size: %.2f, %.2f millimeters.\n" ),
                (double) designSizeXkicad / SCH_IU_PER_MM,
                (double) designSizeYkicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM,
                (double) maxDesignSizekicad / SCH_IU_PER_MM ) );

    mDesignCenter =
            ( Assignments.Settings.DesignArea.first + Assignments.Settings.DesignArea.second ) / 2;

    mSchematic = aSchematic;
    mRootSheet = aRootSheet;

    loadSheets();
    // Load everything!
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheets()
{
    const std::vector<LAYER_ID>& orphanSheets = findOrphanSheets();

    if( orphanSheets.size() > 1 )
    {
        int x = 1;
        int y = 1;

        for( LAYER_ID sheetID : orphanSheets )
        {
            wxPoint pos( x * Mils2iu( 1000 ), y * Mils2iu( 1000 ) );
            wxSize  siz( Mils2iu( 1000 ), Mils2iu( 1000 ) );

            loadSheetAndChildSheets( sheetID, pos, siz, mRootSheet );

            x += 2;

            if( x > 10 ) // start next row
            {
                x = 1;
                y += 2;
            }
        }
    }
    else if( orphanSheets.size() > 0 )
    {
        LAYER_ID rootSheetID = orphanSheets.at( 0 );
        mSheetMap.insert( { rootSheetID, mRootSheet } );
        loadChildSheets( rootSheetID );
    }
    else
    {
        THROW_IO_ERROR( _( "The CADSTAR schematic might be corrupt: there is no root sheet." ) );
    }
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadSheetAndChildSheets(
        LAYER_ID aCadstarSheetID, wxPoint aPosition, wxSize aSheetSize, SCH_SHEET* aParentSheet )
{
    wxCHECK_MSG( mSheetMap.find( aCadstarSheetID ) == mSheetMap.end(), , "Sheet already loaded!" );

    SCH_SHEET*  sheet  = new SCH_SHEET( aParentSheet, aPosition );
    SCH_SCREEN* screen = new SCH_SCREEN( mSchematic );

    sheet->SetSize( aSheetSize );
    sheet->SetScreen( screen );

    //wxString name = wxString::Format( "%d", i );
    wxString name =
            wxString::Format( "%s - %s", aCadstarSheetID, Sheets.SheetNames.at( aCadstarSheetID ) );

    SCH_FIELD& sheetNameField = sheet->GetFields()[SHEETNAME];
    SCH_FIELD& filenameField  = sheet->GetFields()[SHEETFILENAME];

    sheetNameField.SetText( name );

    wxFileName  loadedFilePath = wxFileName( Filename );
    std::string filename =
            wxString::Format( "%s_%d", loadedFilePath.GetName(), getSheetNumber( aCadstarSheetID ) )
                    .ToStdString();

    ReplaceIllegalFileNameChars( &filename );
    filename += wxString( ".kicad_sch" );

    filenameField.SetText( filename );
    wxFileName fn( filename );
    sheet->GetScreen()->SetFileName( fn.GetFullPath() );
    aParentSheet->GetScreen()->Append( sheet );

    mSheetMap.insert( { aCadstarSheetID, sheet } );

    loadChildSheets( aCadstarSheetID );
}


void CADSTAR_SCH_ARCHIVE_LOADER::loadChildSheets( LAYER_ID aCadstarSheetID )
{
    wxCHECK_MSG( mSheetMap.find( aCadstarSheetID ) != mSheetMap.end(), ,
            "FIXME! Parent sheet should be loaded before attempting to load subsheets" );

    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK& block = blockPair.second;

        if( block.LayerID == aCadstarSheetID && block.Type == BLOCK::TYPE::CHILD )
        {
            // In KiCad you can only draw rectangular shapes whereas in Cadstar arbitrary shapes
            // are allowed. We will calculate the extents of the Cadstar shape and draw a rectangle

            std::pair<wxPoint, wxSize> blockExtents;

            if( block.Figures.size() > 0 )
            {
                blockExtents = getFigureExtentsKiCad( block.Figures.begin()->second );
            }
            else
            {
                THROW_IO_ERROR( wxString::Format(
                        _( "The CADSTAR schematic might be corrupt: Block %s references a "
                           "child sheet but has no Figure defined." ),
                        block.ID ) );
            }

            loadSheetAndChildSheets( block.AssocLayerID, blockExtents.first, blockExtents.second,
                    mSheetMap.at( aCadstarSheetID ) );

            if( block.HasBlockLabel )
            {
                // Add the block label as a separate field
                SCH_SHEET* loadedSheet = mSheetMap.at( block.AssocLayerID );
                SCH_FIELDS fields      = loadedSheet->GetFields();

                for( SCH_FIELD& field : fields )
                {
                    field.SetVisible( false );
                }

                SCH_FIELD blockNameField( getKiCadPoint( block.BlockLabel.Position ), 2,
                        loadedSheet, wxString( "Block name" ) );
                applyTextSettings( block.BlockLabel.TextCodeID, block.BlockLabel.Alignment,
                        block.BlockLabel.Justification, &blockNameField );
                blockNameField.SetTextAngle( getAngleTenthDegree( block.BlockLabel.OrientAngle ) );
                blockNameField.SetText( block.Name );
                blockNameField.SetVisible( true );
                fields.push_back( blockNameField );
                loadedSheet->SetFields( fields );
            }
        }
    }
}


std::vector<CADSTAR_SCH_ARCHIVE_LOADER::LAYER_ID> CADSTAR_SCH_ARCHIVE_LOADER::findOrphanSheets()
{
    std::vector<LAYER_ID> childSheets, orphanSheets;

    //Find all sheets that are child of another
    for( std::pair<BLOCK_ID, BLOCK> blockPair : Schematic.Blocks )
    {
        BLOCK&    block        = blockPair.second;
        LAYER_ID& assocSheetID = block.AssocLayerID;

        if( block.Type == BLOCK::TYPE::CHILD )
            childSheets.push_back( assocSheetID );
    }

    //Add sheets that do not have a parent
    for( LAYER_ID sheetID : Sheets.SheetOrder )
    {
        if( std::find( childSheets.begin(), childSheets.end(), sheetID ) == childSheets.end() )
            orphanSheets.push_back( sheetID );
    }

    return orphanSheets;
}


int CADSTAR_SCH_ARCHIVE_LOADER::getSheetNumber( LAYER_ID aCadstarSheetID )
{
    int i = 1;

    for( LAYER_ID sheetID : Sheets.SheetOrder )
    {
        if( sheetID == aCadstarSheetID )
            return i;

        ++i;
    }

    return -1;
}


void CADSTAR_SCH_ARCHIVE_LOADER::checkAndLogHatchCode( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    if( mHatchcodesTested.find( aCadstarHatchcodeID ) != mHatchcodesTested.end() )
    {
        return; //already checked
    }
    else
    {
        HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

        if( hcode.Hatches.size() != 2 )
        {
            wxLogWarning( wxString::Format(
                    _( "The CADSTAR Hatching code '%s' has %d hatches defined. "
                       "KiCad only supports 2 hatches (crosshatching) 90 degrees apart. "
                       "The imported hatching is crosshatched." ),
                    hcode.Name, (int) hcode.Hatches.size() ) );
        }
        else
        {
            if( hcode.Hatches.at( 0 ).LineWidth != hcode.Hatches.at( 1 ).LineWidth )
            {
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR Hatching code '%s' has different line widths for each "
                           "hatch. KiCad only supports one width for the haching. The imported "
                           "hatching uses the width defined in the first hatch definition, i.e. "
                           "%.2f mm." ),
                        hcode.Name,
                        (double) ( (double) getKiCadLength( hcode.Hatches.at( 0 ).LineWidth ) )
                                / 1E6 ) );
            }

            if( hcode.Hatches.at( 0 ).Step != hcode.Hatches.at( 1 ).Step )
            {
                wxLogWarning( wxString::Format(
                        _( "The CADSTAR Hatching code '%s' has different step sizes for each "
                           "hatch. KiCad only supports one step size for the haching. The imported "
                           "hatching uses the step size defined in the first hatching definition, "
                           "i.e. %.2f mm." ),
                        hcode.Name,
                        (double) ( (double) getKiCadLength( hcode.Hatches.at( 0 ).Step ) )
                                / 1E6 ) );
            }

            if( abs( hcode.Hatches.at( 0 ).OrientAngle - hcode.Hatches.at( 1 ).OrientAngle )
                    != 90000 )
            {
                wxLogWarning( wxString::Format(
                        _( "The hatches in CADSTAR Hatching code '%s' have an angle  "
                           "difference of %.1f degrees. KiCad only supports hatching 90 "
                           "degrees apart.  The imported hatching has two hatches 90 "
                           "degrees apart, oriented %.1f degrees from horizontal." ),
                        hcode.Name,
                        getAngleDegrees( abs( hcode.Hatches.at( 0 ).OrientAngle
                                              - hcode.Hatches.at( 1 ).OrientAngle ) ),
                        getAngleDegrees( hcode.Hatches.at( 0 ).OrientAngle ) ) );
            }
        }

        mHatchcodesTested.insert( aCadstarHatchcodeID );
    }
}


int CADSTAR_SCH_ARCHIVE_LOADER::getLineThickness( const LINECODE_ID& aCadstarLineCodeID )
{
    wxCHECK( Assignments.Codedefs.LineCodes.find( aCadstarLineCodeID )
                     != Assignments.Codedefs.LineCodes.end(),
            mSchematic->Settings().m_DefaultWireThickness );

    return getKiCadLength( Assignments.Codedefs.LineCodes.at( aCadstarLineCodeID ).Width );
}


CADSTAR_SCH_ARCHIVE_LOADER::HATCHCODE CADSTAR_SCH_ARCHIVE_LOADER::getHatchCode(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    wxCHECK( Assignments.Codedefs.HatchCodes.find( aCadstarHatchcodeID )
                     != Assignments.Codedefs.HatchCodes.end(),
            HATCHCODE() );

    return Assignments.Codedefs.HatchCodes.at( aCadstarHatchcodeID );
}


CADSTAR_SCH_ARCHIVE_LOADER::TEXTCODE CADSTAR_SCH_ARCHIVE_LOADER::getTextCode(
        const TEXTCODE_ID& aCadstarTextCodeID )
{
    wxCHECK( Assignments.Codedefs.TextCodes.find( aCadstarTextCodeID )
                     != Assignments.Codedefs.TextCodes.end(),
            TEXTCODE() );

    return Assignments.Codedefs.TextCodes.at( aCadstarTextCodeID );
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getAttributeName( const ATTRIBUTE_ID& aCadstarAttributeID )
{
    wxCHECK( Assignments.Codedefs.AttributeNames.find( aCadstarAttributeID )
                     != Assignments.Codedefs.AttributeNames.end(),
            wxEmptyString );

    return Assignments.Codedefs.AttributeNames.at( aCadstarAttributeID ).Name;
}


CADSTAR_SCH_ARCHIVE_LOADER::PART CADSTAR_SCH_ARCHIVE_LOADER::getPart(
        const PART_ID& aCadstarPartID )
{
    wxCHECK( Parts.PartDefinitions.find( aCadstarPartID ) != Parts.PartDefinitions.end(), PART() );

    return Parts.PartDefinitions.at( aCadstarPartID );
}


CADSTAR_SCH_ARCHIVE_LOADER::ROUTECODE CADSTAR_SCH_ARCHIVE_LOADER::getRouteCode(
        const ROUTECODE_ID& aCadstarRouteCodeID )
{
    wxCHECK( Assignments.Codedefs.RouteCodes.find( aCadstarRouteCodeID )
                     != Assignments.Codedefs.RouteCodes.end(),
            ROUTECODE() );

    return Assignments.Codedefs.RouteCodes.at( aCadstarRouteCodeID );
}


wxString CADSTAR_SCH_ARCHIVE_LOADER::getAttributeValue( const ATTRIBUTE_ID& aCadstarAttributeID,
        const std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>&                      aCadstarAttributeMap )
{
    wxCHECK( aCadstarAttributeMap.find( aCadstarAttributeID ) != aCadstarAttributeMap.end(),
            wxEmptyString );

    return aCadstarAttributeMap.at( aCadstarAttributeID ).Value;
}


double CADSTAR_SCH_ARCHIVE_LOADER::getHatchCodeAngleDegrees(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return mSchematic->Settings().m_DefaultWireThickness;
    else
        return getAngleDegrees( hcode.Hatches.at( 0 ).OrientAngle );
}


int CADSTAR_SCH_ARCHIVE_LOADER::getKiCadHatchCodeThickness(
        const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return mSchematic->Settings().m_DefaultWireThickness;
    else
        return getKiCadLength( hcode.Hatches.at( 0 ).LineWidth );
}


int CADSTAR_SCH_ARCHIVE_LOADER::getKiCadHatchCodeGap( const HATCHCODE_ID& aCadstarHatchcodeID )
{
    checkAndLogHatchCode( aCadstarHatchcodeID );
    HATCHCODE hcode = getHatchCode( aCadstarHatchcodeID );

    if( hcode.Hatches.size() < 1 )
        return mSchematic->Settings().m_DefaultWireThickness;
    else
        return getKiCadLength( hcode.Hatches.at( 0 ).Step );
}

int CADSTAR_SCH_ARCHIVE_LOADER::getKiCadUnitNumberFromGate( const GATE_ID& aCadstarGateID )
{
    if( aCadstarGateID.IsEmpty() )
        return 1;

    return (int) aCadstarGateID.Upper().GetChar( 0 ) - (int) wxUniChar( 'A' ) + 1;
}


void CADSTAR_SCH_ARCHIVE_LOADER::applyTextSettings( const TEXTCODE_ID& aCadstarTextCodeID,
        const ALIGNMENT& aCadstarAlignment, const JUSTIFICATION& aCadstarJustification,
        EDA_TEXT* aKiCadTextItem )
{
    TEXTCODE textCode = getTextCode( aCadstarTextCodeID );

    aKiCadTextItem->SetTextWidth( getKiCadLength( textCode.Width ) );
    aKiCadTextItem->SetTextHeight( getKiCadLength( textCode.Height ) );
    aKiCadTextItem->SetTextThickness( getKiCadLength( textCode.LineWidth ) );

    switch( aCadstarAlignment )
    {
    case ALIGNMENT::NO_ALIGNMENT: // Default for Single line text is Bottom Left
    case ALIGNMENT::BOTTOMLEFT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::BOTTOMCENTER:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::BOTTOMRIGHT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_BOTTOM );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::CENTERLEFT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::CENTERCENTER:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::CENTERRIGHT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_CENTER );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;

    case ALIGNMENT::TOPLEFT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        break;

    case ALIGNMENT::TOPCENTER:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_CENTER );
        break;

    case ALIGNMENT::TOPRIGHT:
        aKiCadTextItem->SetVertJustify( GR_TEXT_VJUSTIFY_TOP );
        aKiCadTextItem->SetHorizJustify( GR_TEXT_HJUSTIFY_RIGHT );
        break;
    }
}


std::pair<wxPoint, wxSize> CADSTAR_SCH_ARCHIVE_LOADER::getFigureExtentsKiCad(
        const FIGURE& aCadstarFigure )
{
    wxPoint upperLeft( Assignments.Settings.DesignLimit.x, 0 );
    wxPoint lowerRight( 0, Assignments.Settings.DesignLimit.y );

    for( const VERTEX& v : aCadstarFigure.Shape.Vertices )
    {
        if( upperLeft.x > v.End.x )
            upperLeft.x = v.End.x;

        if( upperLeft.y < v.End.y )
            upperLeft.y = v.End.y;

        if( lowerRight.x < v.End.x )
            lowerRight.x = v.End.x;

        if( lowerRight.y > v.End.y )
            lowerRight.y = v.End.y;
    }

    for( CUTOUT cutout : aCadstarFigure.Shape.Cutouts )
    {
        for( const VERTEX& v : aCadstarFigure.Shape.Vertices )
        {
            if( upperLeft.x > v.End.x )
                upperLeft.x = v.End.x;

            if( upperLeft.y < v.End.y )
                upperLeft.y = v.End.y;

            if( lowerRight.x < v.End.x )
                lowerRight.x = v.End.x;

            if( lowerRight.y > v.End.y )
                lowerRight.y = v.End.y;
        }
    }

    wxPoint upperLeftKiCad  = getKiCadPoint( upperLeft );
    wxPoint lowerRightKiCad = getKiCadPoint( lowerRight );

    wxPoint size = lowerRightKiCad - upperLeftKiCad;

    return { upperLeftKiCad, wxSize( abs( size.x ), abs( size.y ) ) };
}


wxPoint CADSTAR_SCH_ARCHIVE_LOADER::getKiCadPoint( wxPoint aCadstarPoint )
{
    wxPoint retval;

    retval.x = ( aCadstarPoint.x - mDesignCenter.x ) * KiCadUnitMultiplier;
    retval.y = -( aCadstarPoint.y - mDesignCenter.y ) * KiCadUnitMultiplier;

    return retval;
}


double CADSTAR_SCH_ARCHIVE_LOADER::getPolarAngle( wxPoint aPoint )
{

    return NormalizeAnglePos( ArcTangente( aPoint.y, aPoint.x ) );
}

/******************************************************************************
 **	Filename:    normfeat.c
 **	Purpose:     Definition of char normalization features.
 **	Author:      Dan Johnson
 **	History:     12/14/90, DSJ, Created.
 **
 **	(c) Copyright Hewlett-Packard Company, 1988.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 ******************************************************************************/
/**----------------------------------------------------------------------------
          Include Files and Type Defines
----------------------------------------------------------------------------**/
#include "normfeat.h"
#include "mfoutline.h"

#include "ocrfeatures.h"         //Debug
#include <stdio.h>               //Debug
#include "efio.h"                //Debug
//#include "christydbg.h"

/**----------------------------------------------------------------------------
        Global Data Definitions and Declarations
----------------------------------------------------------------------------**/
/* define all of the parameters for this feature type*/
StartParamDesc (CharNormParams)
DefineParam (0, 0, -0.25, 0.75)
DefineParam (0, 0, 0.0, 1.0)
DefineParam (0, 0, 0.0, 1.0) DefineParam (0, 0, 0.0, 1.0) EndParamDesc
/* now define the feature type itself (see features.h for info about each
  parameter).*/
DefineFeature (CharNormDesc, 4, 0, 1, 1, "CharNorm", "cn", CharNormParams, ExtractCharNormFeatures
/*, NULL,
                  NULL, NULL */
, DefaultInitFXVars /*, NULL */ )
/**----------------------------------------------------------------------------
              Public Code
----------------------------------------------------------------------------**/
/*---------------------------------------------------------------------------*/
FLOAT32 ActualOutlineLength(FEATURE Feature) { 
/*
 **	Parameters:
 **		Feature		normalization feature
 **	Globals: none
 **	Operation: This routine returns the length that the outline
 **		would have been if it were baseline normalized instead
 **		of character normalized.
 **	Return: Baseline normalized length of outline.
 **	Exceptions: none
 **	History: Thu Dec 20 14:50:57 1990, DSJ, Created.
 */
  return (ParamOf (Feature, CharNormLength) * LENGTH_COMPRESSION);

}                                /* ActualOutlineLength */


/*---------------------------------------------------------------------------*/
FEATURE_SET ExtractCharNormFeatures(TBLOB *Blob, LINE_STATS *LineStats) { 
/*
 **	Parameters:
 **		Blob		blob to extract char norm feature from
 **		LineStats	statistics on text row blob is in
 **	Globals: none
 **	Operation: Compute a feature whose parameters describe how a
 **		character will be affected by the character normalization
 **		algorithm.  The feature parameters are:
 **			y position of center of mass in baseline coordinates
 **			total length of outlines in baseline coordinates
 **				divided by a scale factor
 **			radii of gyration about the center of mass in
 **				baseline coordinates
 **	Return: Character normalization feature for Blob.
 **	Exceptions: none
 **	History: Wed May 23 18:06:38 1990, DSJ, Created.
 */
  FEATURE_SET FeatureSet;
  FEATURE Feature;
  FLOAT32 ScaleFactor;
  FLOAT32 Baseline;
  LIST Outlines;
  OUTLINE_STATS OutlineStats;

  /* allocate the feature and feature set - note that there is always one
     and only one char normalization feature for any blob */
  FeatureSet = NewFeatureSet (1);
  Feature = NewFeature (&CharNormDesc);
  AddFeature(FeatureSet, Feature); 

  /* compute the normalization statistics for this blob */
  Outlines = ConvertBlob (Blob);
  /*---------Debug--------------------------------------------------*
  OFile = fopen ("f:/ims/debug/nfOutline.logCPP", "r");
  if (OFile == NULL)
  {
    OFile = Efopen ("f:/ims/debug/nfOutline.logCPP", "w");
    WriteOutlines(OFile, Outlines);
  }
  else
  {
    fclose (OFile);
    OFile = Efopen ("f:/ims/debug/nfOutline.logCPP", "a");
  }
  WriteOutlines(OFile, Outlines);
  fclose (OFile);
  *--------------------------------------------------------------------*/
  ComputeOutlineStats(Outlines, &OutlineStats); 

  /* convert outline statistics to normalization features */
  ScaleFactor = ComputeScaleFactor (LineStats);
  Baseline = BaselineAt (LineStats, OutlineStats.x);
  ParamOf (Feature, CharNormY) = (OutlineStats.y - Baseline) * ScaleFactor;
  ParamOf (Feature, CharNormLength) =
    OutlineStats.L * ScaleFactor / LENGTH_COMPRESSION;
  ParamOf (Feature, CharNormRx) = OutlineStats.Rx * ScaleFactor;
  ParamOf (Feature, CharNormRy) = OutlineStats.Ry * ScaleFactor;

  /*---------Debug--------------------------------------------------*
  File = fopen ("f:/ims/debug/nfFeatSet.logCPP", "r");
  if (File == NULL)
  {
    File = Efopen ("f:/ims/debug/nfFeatSet.logCPP", "w");
    WriteFeatureSet(File, FeatureSet);
  }
  else
  {
    fclose (File);
    File = Efopen ("f:/ims/debug/nfFeatSet.logCPP", "a");
  }
  WriteFeatureSet(File, FeatureSet);
  fclose (File);
  *--------------------------------------------------------------------*/
  FreeOutlines(Outlines); 
  return (FeatureSet);
}                                /* ExtractCharNormFeatures */

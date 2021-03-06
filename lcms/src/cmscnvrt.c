//
//  Little cms
//  Copyright (C) 1998-2001 Marti Maria
//
// THIS SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
// EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
// WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
//
// IN NO EVENT SHALL MARTI MARIA BE LIABLE FOR ANY SPECIAL, INCIDENTAL,
// INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
// OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
// WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
// LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
// OF THIS SOFTWARE.
//
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

#include "lcms.h"




/*
       This module provides conversion stages for handling intents.

The chain of evaluation in a transform is:

                PCS1            PCS2                    PCS3          PCS4

|From |  |From  |  |Conversion |  |Preview |  |Gamut   |  |Conversion |  |To    |  |To     |
|Input|->|Device|->|Stage 1    |->|handling|->|Checking|->|Stage 2    |->|Device|->|output |

--------  -------  -------------   ---------  ----------  -------------   -------  ---------

          AToB0                     prew0       gamut                     BToA0
Formatting LUT      Adjusting        LUT         LUT       Adjusting       LUT      Formatting
          Intent     Intent 1       intent      intent      Intent 2      Intent


Some of these LUT may be missing

There are two intents involved here, the intent of the transform itself, and the
intent the proof is being done, if is the case. Since the first intent is to be
applied to preview, is the proofing intent. The second intent  identifies the
transform intent. Input data of any stage is taked as relative colorimetric
always.

*/




int cdecl cmsChooseCnvrt(int Absolute,
                 int Phase1, LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                int DoBlackPointCompensation,
                 _cmsADJFN *fn1,
                 LPWMAT3 wm, LPWVEC3 wof);


// -------------------------------------------------------------------------

// D50 - Widely used

LCMSAPI LPcmsCIEXYZ LCMSEXPORT cmsD50_XYZ(void)
{
    static cmsCIEXYZ D50XYZ = {D50X, D50Y, D50Z};

    return &D50XYZ;
}

LCMSAPI LPcmsCIExyY LCMSEXPORT cmsD50_xyY(void)
{
    static cmsCIExyY D50xyY;
    cmsXYZ2xyY(&D50xyY, cmsD50_XYZ());

    return &D50xyY;
}


// ---------------- From LUT to LUT --------------------------


// Calculate m, offset Relativ -> Absolute undoing any chromatic 
// adaptation done by the profile. 

#ifdef _MSC_VER
#pragma warning(disable : 4100 4505)
#endif

static
void Rel2AbsCoefs(LPcmsCIEXYZ BlackPoint,
                  LPcmsCIEXYZ WhitePoint,
                  LPcmsCIEXYZ Illuminant,
                  LPMAT3 ChromaticAdaptationMatrix,
                  LPMAT3 m, LPVEC3 of)
{
     MAT3 Chrm;
            
       Chrm = *ChromaticAdaptationMatrix;
       MAT3inverse(&Chrm, m);
       VEC3init(of, 0, 0, 0);     
      
}

// Calculate a, b for Absolute -> Relativ undoing any chromatic adaptation
// done by the profile

static
void Abs2RelCoefs(LPcmsCIEXYZ BlackPoint,
                  LPcmsCIEXYZ WhitePoint,
                  LPcmsCIEXYZ Illuminant,
                  LPMAT3 ChromaticAdaptationMatrix,
                  LPMAT3 m, LPVEC3 of)
{
      
       *m = *ChromaticAdaptationMatrix;
        VEC3init(of, 0, 0, 0);       
}

// join scalings to obtain:
//     relative input to absolute and then to relative output

static
void Rel2RelStepAbsCoefs(LPcmsCIEXYZ BlackPointIn,
                      LPcmsCIEXYZ WhitePointIn,
                      LPcmsCIEXYZ IlluminantIn,
                      LPMAT3 ChromaticAdaptationMatrixIn,

                      LPcmsCIEXYZ BlackPointOut,
                      LPcmsCIEXYZ WhitePointOut,
                      LPcmsCIEXYZ IlluminantOut,
                      LPMAT3 ChromaticAdaptationMatrixOut,

                      LPMAT3 m, LPVEC3 of)
{
       MAT3 min, mout;
       VEC3 ofin, ofout;

       
       Rel2AbsCoefs(BlackPointIn,
                    WhitePointIn,
                    IlluminantIn, 
                    ChromaticAdaptationMatrixIn,
                    &min, &ofin);

       Abs2RelCoefs(BlackPointOut,
                    WhitePointOut,
                    IlluminantOut, 
                    ChromaticAdaptationMatrixOut,
                    &mout, &ofout);


       // Chromatic adaptation matrix should be conmutative
       // min * mout = mout * min
           
       MAT3per(m,  &min, &mout);
       VEC3init(of, 0.0, 0.0, 0.0);
              
}


// The (in)famous black point compensation. Right now implemented as
// a linear scaling in XYZ

static
void ComputeBlackPointCompensationFactors(LPcmsCIEXYZ BlackPointIn,
                      LPcmsCIEXYZ WhitePointIn,
                      LPcmsCIEXYZ IlluminantIn,
                      LPcmsCIEXYZ BlackPointOut,
                      LPcmsCIEXYZ WhitePointOut,
                      LPcmsCIEXYZ IlluminantOut,
                      LPMAT3 m, LPVEC3 of)
{
    

   cmsCIEXYZ RelativeBlackPointIn, RelativeBlackPointOut;
   double ax, ay, az, bx, by, bz, tx, ty, tz;
   
   // At first, convert both black points to relative.

   cmsAdaptToIlluminant(&RelativeBlackPointIn,  WhitePointIn, IlluminantIn, BlackPointIn);
   cmsAdaptToIlluminant(&RelativeBlackPointOut, WhitePointOut, IlluminantOut, BlackPointOut);
   
   // Now we need to compute a matrix plus an offset m and of such of
   // [m]*bpin + off = bpout
   // [m]*D50  + off = D50
   //
   // This is a linear scaling in the form ax+b, where
   // a = (bpout - D50) / (bpin - D50)
   // b = - D50* (bpout - bpin) / (bpin - D50)


   tx = RelativeBlackPointIn.X - IlluminantIn ->X;
   ty = RelativeBlackPointIn.Y - IlluminantIn ->Y;
   tz = RelativeBlackPointIn.Z - IlluminantIn ->Z;

   ax = (RelativeBlackPointOut.X - IlluminantOut ->X) / tx;
   ay = (RelativeBlackPointOut.Y - IlluminantOut ->Y) / ty;
   az = (RelativeBlackPointOut.Z - IlluminantOut ->Z) / tz;

   bx = - IlluminantOut -> X * (RelativeBlackPointOut.X - RelativeBlackPointIn.X) / tx;
   by = - IlluminantOut -> Y * (RelativeBlackPointOut.Y - RelativeBlackPointIn.Y) / ty;
   bz = - IlluminantOut -> Z * (RelativeBlackPointOut.Z - RelativeBlackPointIn.Z) / tz;


   MAT3identity(m);

   m->v[VX].n[0] = ax;
   m->v[VY].n[1] = ay;
   m->v[VZ].n[2] = az;

   VEC3init(of, bx, by, bz);

}

// ----------------------------------------- Inter PCS conversions

// XYZ to XYZ linear scaling. Aso used on Black point compensation

static
void XYZ2XYZ(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{

    WVEC3 a, r;

    a.n[0] = In[0] << 1;
    a.n[1] = In[1] << 1;
    a.n[2] = In[2] << 1;

    MAT3evalW(&r, m, &a);

    Out[0] = Clamp_XYZ((r.n[VX] + of->n[VX]) >> 1);
    Out[1] = Clamp_XYZ((r.n[VY] + of->n[VY]) >> 1);
    Out[2] = Clamp_XYZ((r.n[VZ] + of->n[VZ]) >> 1);
}


// XYZ to Lab, scaling first

static
void XYZ2Lab(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{
  WORD XYZ[3];

  XYZ2XYZ(In, XYZ, m, of);
  cmsXYZ2LabEncoded(XYZ, Out);
}

// Lab to XYZ, then scalling

static
void Lab2XYZ(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{
       WORD XYZ[3];

       cmsLab2XYZEncoded(In, XYZ);
       XYZ2XYZ(XYZ, Out, m, of);
}

// Lab to XYZ, scalling and then, back to Lab

static
void Lab2XYZ2Lab(WORD In[], WORD Out[], LPWMAT3 m, LPWVEC3 of)
{
       WORD XYZ[3], XYZ2[3];

       cmsLab2XYZEncoded(In, XYZ);
       XYZ2XYZ(XYZ, XYZ2, m, of);
       cmsXYZ2LabEncoded(XYZ2, Out);
}

// ------------------------------------------------------------------

// Dispatcher for XYZ Relative LUT

static
int FromXYZRelLUT(int Absolute,
                             LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                 int DoBlackPointCompensation,
                 _cmsADJFN *fn1,
                 LPMAT3 m, LPVEC3 of)

{
              switch (Phase2) {

                     // From relative XYZ to Relative XYZ.

                     case XYZRel:

                            if (Absolute)
                            {
                                   // From input relative to absolute, and then
                                   // back to output relative

                                   Rel2RelStepAbsCoefs(BlackPointIn,
                                                  WhitePointIn,
                                                  IlluminantIn,
                                                  ChromaticAdaptationMatrixIn,
                                                  BlackPointOut,
                                                  WhitePointOut,
                                                  IlluminantOut,
                                                  ChromaticAdaptationMatrixOut,
                                                  m, of);
                                   *fn1 = XYZ2XYZ;

                            }
                            else
                            {
                                   // XYZ Relative to XYZ relative, no op required                                   
                                   *fn1 = NULL;
                                   if (DoBlackPointCompensation) {

                                      *fn1 = XYZ2XYZ;
                                      ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                      WhitePointIn,
                                                                      IlluminantIn,
                                                                      BlackPointOut,
                                                                      WhitePointOut,
                                                                      IlluminantOut,
                                                                      m, of);

                                   }
                            }
                            break;

                     // From relative XYZ to absolute XYZ. 

                     case XYZAbs:

                            Rel2AbsCoefs(BlackPointIn, 
                                         WhitePointIn, 
                                         IlluminantIn, 
                                         ChromaticAdaptationMatrixIn,
                                         m, of);
                            *fn1 = XYZ2XYZ;
                            break;

                     // From relative XYZ to Relative Lab

                     case LabRel:

                            // First pass XYZ to absolute, then to relative and
                            // finally to Lab. I use here D50 for output in order
                            // to prepare the "to Lab" conversion.

                            if (Absolute)
                            {   

                                Rel2RelStepAbsCoefs(BlackPointIn,
                                                    WhitePointIn,
                                                    IlluminantIn,
                                                    ChromaticAdaptationMatrixIn,
                                                    BlackPointOut,
                                                    WhitePointOut,
                                                    IlluminantOut,
                                                    ChromaticAdaptationMatrixOut,
                                                    m, of);
                                
                                *fn1 = XYZ2Lab;

                            }
                            else
                            {
                                   // Just Convert to Lab

                                   MAT3identity(m);
                                   VEC3init(of, 0, 0, 0);
                                   *fn1 = XYZ2Lab;

                                   if (DoBlackPointCompensation) {

                                    ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                          WhitePointIn,
                                                                          IlluminantIn,
                                                                          BlackPointOut,
                                                                          WhitePointOut,
                                                                          IlluminantOut,
                                                                          m, of);
                                   }
                            }
                            break;

                     // From relative XYZ To Absolute Lab, adjusting to D50

                     case LabAbs:
                                {
                                Rel2RelStepAbsCoefs(BlackPointIn,
                                                    WhitePointIn,
                                                    IlluminantIn,
                                                    ChromaticAdaptationMatrixIn,
                                                    BlackPointOut,
                                                    WhitePointOut,
                                                    IlluminantOut,
                                                    ChromaticAdaptationMatrixOut,
                                                    m, of);
                                
                                *fn1 = XYZ2Lab;
                                }
                            break;


                     default: return FALSE;
                     }

              return TRUE;
}


// Since XYZ comes in absolute colorimetry, no endpoints on input
// are needed.

static
int FromXYZAbsLUT(
                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                 _cmsADJFN *fn1,
                 LPMAT3 m, LPVEC3 of)

{

          switch (Phase2) {

              case XYZRel:
                     Abs2RelCoefs(BlackPointOut, 
                                  WhitePointOut, IlluminantOut, 
                                  ChromaticAdaptationMatrixOut, m, of);
                     *fn1 = XYZ2XYZ;
                     break;

              case XYZAbs:         // Identity
                     *fn1 = NULL;
                     break;

              case LabRel:
                     Abs2RelCoefs(BlackPointOut, 
                                  WhitePointOut, IlluminantOut, 
                                  ChromaticAdaptationMatrixOut, m, of);
                     *fn1 = XYZ2Lab;
                     break;

              case LabAbs:
                     *fn1 = XYZ2Lab;
                     break;

              default: return FALSE;
              }
       return TRUE;
}


// From Lab Relative type LUT

static
int FromLabRelLUT(int Absolute,
                             LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                int DoBlackPointCompensation,
                 _cmsADJFN *fn1,
                 LPMAT3 m, LPVEC3 of)
{

          switch (Phase2) {

              // From Lab Relative to XYZ Relative, very usual case

              case XYZRel:

                  if (Absolute) {  // Absolute intent

                            // From lab relative, to XYZ absolute, and then,
                            // back to XYZ relative

                            Rel2RelStepAbsCoefs(BlackPointIn,
                                           WhitePointIn,
                                           cmsD50_XYZ(),
                                           ChromaticAdaptationMatrixIn,
                                           BlackPointOut,
                                           WhitePointOut,
                                           IlluminantOut,
                                           ChromaticAdaptationMatrixOut,
                                           m, of);

                            *fn1 = Lab2XYZ;

                     }
                     else
                     {
                            // From Lab relative, to XYZ relative.
                            
                            *fn1 = Lab2XYZ;
                            if (DoBlackPointCompensation) {

                                 ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                      WhitePointIn,
                                                                      IlluminantIn,
                                                                      BlackPointOut,
                                                                      WhitePointOut,
                                                                      IlluminantOut,
                                                                      m, of);

                            }
                     }
                     break;


              // From Relative Lab to XYZ absolute. First covert to relative XYZ,
              // then to absolute XYZ

              case XYZAbs: {

                     Rel2AbsCoefs(BlackPointIn, 
                                  WhitePointIn, 
                                  cmsD50_XYZ(), 
                                  ChromaticAdaptationMatrixIn,
                                  m, of);

                     *fn1 = Lab2XYZ;
                     }
                     break;

              case LabRel:

                     if (Absolute) {

                     // First pass to XYZ using the input illuminant
                     // * InIlluminant / D50, then to absolute. Then
                     // to relative, but for input

                     Rel2RelStepAbsCoefs(BlackPointIn, 
                                         WhitePointIn, IlluminantIn,
                                         ChromaticAdaptationMatrixIn,
                                         BlackPointOut, 
                                         WhitePointOut, cmsD50_XYZ(),
                                         ChromaticAdaptationMatrixOut,
                                         m, of);
                     *fn1 = Lab2XYZ2Lab;
                     }
                     else
                     {      // Lab -> Lab relative don't need any adjust unless
                            // black point compensation

                            *fn1 = NULL;
                             if (DoBlackPointCompensation) {

                                 *fn1 = Lab2XYZ2Lab;
                                 ComputeBlackPointCompensationFactors(BlackPointIn,
                                                                      WhitePointIn,
                                                                      IlluminantIn,
                                                                      BlackPointOut,
                                                                      WhitePointOut,
                                                                      IlluminantOut,
                                                                      m, of);

                                
                            }
                     }
                     break;

              case LabAbs:
                     Rel2AbsCoefs(BlackPointIn, WhitePointIn, cmsD50_XYZ(), 
                         ChromaticAdaptationMatrixIn, m, of);
                     *fn1 = Lab2XYZ2Lab;
                     break;

              default: return FALSE;
              }

   return TRUE;
}


// From Lab Absolute LUT, always absolute

static
int FromLabAbsLUT(           LPcmsCIEXYZ BlackPointIn,
                             LPcmsCIEXYZ WhitePointIn,
                             LPcmsCIEXYZ IlluminantIn,
                             LPMAT3 ChromaticAdaptationMatrixIn,

                 int Phase2, LPcmsCIEXYZ BlackPointOut,
                             LPcmsCIEXYZ WhitePointOut,
                             LPcmsCIEXYZ IlluminantOut,
                             LPMAT3 ChromaticAdaptationMatrixOut,

                 _cmsADJFN *fn1,
                 LPMAT3 m, LPVEC3 of)
{


          switch (Phase2) {

              
              case XYZRel:                     
                     *fn1 = Lab2XYZ;
                     break;

              case XYZAbs:
                     Abs2RelCoefs(BlackPointOut, 
                                  WhitePointOut, IlluminantOut, 
                                  ChromaticAdaptationMatrixOut, m, of);
                     *fn1 = Lab2XYZ;
                     break;

              case LabRel:
                     Abs2RelCoefs(BlackPointOut, 
                                  WhitePointOut, IlluminantOut, 
                                  ChromaticAdaptationMatrixOut, m, of);
                     *fn1 = Lab2XYZ2Lab;
                     break;

              case LabAbs:
                     *fn1 = NULL;
                     break;

              default: return FALSE;
              }
        
        return TRUE;
}


// This function does calculate the necessary conversion operations
// needed from transpassing data from a LUT to a LUT. The conversion
// is modeled as a pointer of function and two coefficients, a and b
// The function is actually called only if not null pointer is provided,
// and the two paramaters are passed in. There are several types of
// conversions, but basically they do a linear scalling and a interchange



// Main dispatcher

int cmsChooseCnvrt(int Absolute,
                  int Phase1, LPcmsCIEXYZ BlackPointIn,
                              LPcmsCIEXYZ WhitePointIn,
                              LPcmsCIEXYZ IlluminantIn,
                              LPMAT3 ChromaticAdaptationMatrixIn,

                  int Phase2, LPcmsCIEXYZ BlackPointOut,
                              LPcmsCIEXYZ WhitePointOut,
                              LPcmsCIEXYZ IlluminantOut,
                              LPMAT3 ChromaticAdaptationMatrixOut,

                  int DoBlackPointCompensation,
                  _cmsADJFN *fn1,
                  LPWMAT3 wm, LPWVEC3 wof)
{

       int rc;
       MAT3 m;
       VEC3 of;


       MAT3identity(&m);
       VEC3init(&of, 0, 0, 0);

       switch (Phase1) {

       // Input LUT is giving XYZ relative values.

       case XYZRel:  rc = FromXYZRelLUT(Absolute,
                                          BlackPointIn,
                                          WhitePointIn,
                                          IlluminantIn,
                                          ChromaticAdaptationMatrixIn,
                                          Phase2,
                                          BlackPointOut,
                                          WhitePointOut,
                                          IlluminantOut,
                                          ChromaticAdaptationMatrixOut,
                                          DoBlackPointCompensation,
                                          fn1, &m, &of);
                     break;

       
       // Input LUT is giving XYZ Absolute values. 

       case XYZAbs:  rc = FromXYZAbsLUT(Phase2,
                                          BlackPointOut,
                                          WhitePointOut,
                                          IlluminantOut,
                                          ChromaticAdaptationMatrixOut,
                                          fn1, &m, &of);
                     break;
       

       // Input LUT is giving Lab relative values

       case LabRel:  rc =  FromLabRelLUT(Absolute,
                                          BlackPointIn,
                                          WhitePointIn,
                                          IlluminantIn,
                                          ChromaticAdaptationMatrixIn,
                                          Phase2,
                                          BlackPointOut,
                                          WhitePointOut,
                                          IlluminantOut,
                                          ChromaticAdaptationMatrixOut,
                                          DoBlackPointCompensation,
                                          fn1, &m, &of);
                     break;

       
       // Input LUT is giving absolute Lab values.
        
       case LabAbs:  rc = FromLabAbsLUT(BlackPointIn,
                                          WhitePointIn,
                                          IlluminantIn,
                                          ChromaticAdaptationMatrixIn,
                                          Phase2,
                                          BlackPointOut,
                                          WhitePointOut,
                                          IlluminantOut,
                                          ChromaticAdaptationMatrixOut,
                                          fn1, &m, &of);
                     break;
        

       // Unrecognized combination

       default:    cmsSignalError(LCMS_ERRC_ABORTED, "(internal) Phase error");
                   return FALSE;

       }

       MAT3toFix(wm, &m);
       VEC3toFix(wof, &of);

       return rc;
}

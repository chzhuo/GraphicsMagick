//
//  Little cms
//  Copyright (C) 1998-2000 Marti Maria
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


// ---------------------------------------------------------------------------------

// Quantize a value 0 <= i < MaxSamples

WORD _cmsQuantizeVal(double i, int MaxSamples)
{
       double x;

       x = ((double) i * 65535.) / (double) (MaxSamples - 1);

       return (WORD) floor(x + .5);
}


// Is a table linear?

int cmsIsLinear(WORD Table[], int nEntries)
{
       register int i;
       int diff;

       for (i=0; i < nEntries; i++) {

           diff = abs((int) Table[i] - (int) _cmsQuantizeVal(i, nEntries));              
           if (diff > 3)
                     return 0;
       }

       return 1;
}



// pow() restricted to integer

static
int ipow(int base, int exp)
{
        int res = base;

        while (--exp)
               res *= base;

        return res;
}


// Given n, 0<=n<=clut^dim, returns the colorant.

static
int ComponentOf(int n, int clut, int nColorant)
{
        if (nColorant <= 0)
                return (n % clut);

        n /= ipow(clut, nColorant);

        return (n % clut);
}



// This routine does a sweep on whole input space, and calls its callback
// function on knots. returns TRUE if all ok, FALSE otherwise.

BOOL LCMSEXPORT cmsSample3DGrid(LPLUT Lut, _cmsSAMPLER Sampler, LPVOID Cargo, DWORD dwFlags)
{
   int i, t, nTotalPoints, Colorant, index;
   WORD In[MAXCHANNELS], Out[MAXCHANNELS];

   nTotalPoints = ipow(Lut->cLutPoints, Lut -> InputChan);

   index = 0;
   for (i = 0; i < nTotalPoints; i++) {

        for (t=0; t < (int) Lut -> InputChan; t++) {

                Colorant =  ComponentOf(i, Lut -> cLutPoints, (Lut -> InputChan - t  - 1 ));
                In[t]    = _cmsQuantizeVal(Colorant, Lut -> cLutPoints);
        }


        if (dwFlags & SAMPLER_HASTL1) {

                 for (t=0; t < (int) Lut -> InputChan; t++)
                     In[t] = cmsReverseLinearInterpLUT16(In[t],
                                                Lut -> L1[t],
                                                &Lut -> In16params);
        }


        if (dwFlags & SAMPLER_INSPECT) {

             for (t=0; t < (int) Lut -> OutputChan; t++)
                        Out[t] = Lut->T[index + t];
        }


        if (!Sampler(In, Out, Cargo))
                return FALSE;

        if (!(dwFlags & SAMPLER_INSPECT)) {
            
            if (dwFlags & SAMPLER_HASTL2) {

                for (t=0; t < (int) Lut -> OutputChan; t++)
                     Out[t] = cmsReverseLinearInterpLUT16(Out[t],
                                                   Lut -> L2[t],
                                                   &Lut -> Out16params);
                }

        
            for (t=0; t < (int) Lut -> OutputChan; t++)
                        Lut->T[index + t] = Out[t];

        }

        index += Lut -> OutputChan;

    }

    return TRUE;
}



// Sampler implemented by another transform. This is a clean way to
// precalculate the devicelink 3D CLUT for almost any transform

static
int XFormSampler(register WORD In[], register WORD Out[], register LPVOID Cargo)
{
        cmsDoTransform((cmsHTRANSFORM) Cargo, In, Out, 1);
        return TRUE;
}



// choose reasonable resolution
int _cmsReasonableGridpointsByColorspace(icColorSpaceSignature Colorspace, DWORD dwFlags)
{

    int nChannels = _cmsChannelsOf(Colorspace);

    // HighResPrecalc is maximum resolution

    if (dwFlags & cmsFLAGS_HIGHRESPRECALC) {

        if (nChannels > 4) 
                return 7;       // 7 for Hifi

        if (nChannels == 4)     // 23 for CMYK
                return 23;
    
        return 48;      // 48 for RGB and others        
    }


    // LowResPrecal is stripped resolution

    if (dwFlags & cmsFLAGS_LOWRESPRECALC) {
        
        if (nChannels > 4) 
                return 6;       // 7 for Hifi

        if (nChannels == 1) 
                return 33;      // For monochrome

        return 17;              // 17 for remaining
    }

    // Default values

    if (nChannels > 4) 
                return 7;       // 7 for Hifi

    if (nChannels == 4)
                return 17;      // 17 for CMYK

    return 33;                  // 33 for RGB
    
}


// This routine does compute the devicelink CLUT containing whole
// transform. Handles any channel number.

LPLUT _cmsPrecalculateDeviceLink(cmsHTRANSFORM h, DWORD dwFlags)
{
        _LPcmsTRANSFORM p = (_LPcmsTRANSFORM) h;
       LPLUT Grid;
       int nGridPoints;
       DWORD dwFormatIn, dwFormatOut;
       int ChannelsIn, ChannelsOut;
    
       
       ChannelsIn   = _cmsChannelsOf(p -> EntryColorSpace);
       ChannelsOut  = _cmsChannelsOf(p -> ExitColorSpace);
          

       nGridPoints = _cmsReasonableGridpointsByColorspace(p -> EntryColorSpace, dwFlags);
       Grid =  cmsAllocLUT();
       if (!Grid) return NULL;

       Grid = cmsAlloc3DGrid(Grid, nGridPoints, ChannelsIn, ChannelsOut);

       // Compute device link on 16-bit basis
       dwFormatIn   = (CHANNELS_SH(ChannelsIn)|BYTES_SH(2));
       dwFormatOut  = (CHANNELS_SH(ChannelsOut)|BYTES_SH(2));

       p -> FromInput = _cmsIdentifyInputFormat(p, dwFormatIn);
       p -> ToOutput  = _cmsIdentifyOutputFormat(p, dwFormatOut);

       // Fix gamut & gamma possible mismatches. 
       // Only operates on RGB to RGB!!
           
       if (p ->EntryColorSpace == icSigRgbData && 
           p ->ExitColorSpace  == icSigRgbData &&
           !(dwFlags & cmsFLAGS_NOPRELINEARIZATION))
                _cmsComputePrelinearizationTablesFromXFORM((cmsHTRANSFORM) &p, 1, Grid);
        
            
       // Attention to this typecast! we can take the luxury to
       // do this since cmsHTRANSFORM is only an alias to a pointer
       // to the transform struct.

       if (!cmsSample3DGrid(Grid, XFormSampler, (LPVOID) p, Grid -> wFlags)) {

                cmsFreeLUT(Grid);
                return NULL;
       }
      
      
       return Grid;
}


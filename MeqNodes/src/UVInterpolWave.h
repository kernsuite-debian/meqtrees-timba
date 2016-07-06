//# UVInterpolWAVE.h
//#
//# Copyright (C) 2002-2007
//# ASTRON (Netherlands Foundation for Research in Astronomy)
//# and The MeqTree Foundation
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//#
//# This program is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#
//# $Id$

#ifndef MEQNODES_UVINTERPOLWAVE_H
#define MEQNODES_UVINTERPOLWAVE_H

//# Includes
#include <MEQ/Node.h>

#include <MeqNodes/TID-MeqNodes.h>
#include <MeqNodes/AID-MeqNodes.h>

#pragma aidgroup MeqNodes
#pragma types #Meq::UVInterpolWave

#pragma aids UVInterpolWave_method Coeff Brick UVW

namespace Meq {
  
class UVInterpolWave: public Node  {
  public:
    // The default constructor.
    // The object should be filled by the init method.
    UVInterpolWave();

    virtual ~UVInterpolWave();

    virtual TypeId objectType() const
    { return TpMeqUVInterpolWave; }
    
    /*
    // Evaluate the value for the given request.
    virtual Vells evaluate (const Request&,const LoShape &,
    const vector<const Vells*>& values);
    */

    // Get the requested result of the Node.
    virtual int getResult (Result::Ref &resref, 
			   const std::vector<Result::Ref> &childres,
			   const Request &req,bool newreq);
  
  protected:

    virtual void setStateImpl (DMI::Record::Ref &rec,bool initializing);

  private:
    // the resampling function lookup table (LUT) is static, since it's shared
    // across all nodes
    static LoVec_double weights_arr;
    // these parameters determine the shape of the LUT
    static int griddivisions;
    static int weightsparam;
    static int cutoffparam;
    // this function fills the LUT (if not already filled)
    static void fillWeightsArray ();
    static Thread::Mutex weights_arr_mutex;
    static bool weights_arr_filled;
    
    // flag: already reported frequency extrapolation warning for this result
    static const Result * reported_freq_warning_;
    
   // How many values in the u and v and t directions 
   // INI: Added 'nf' to hold the no. of freq. planes in the brick
   // lfreq, ufreq to hold the indices of the required freq. planes in the brick
    int nu, nv, nt, nn, nf, lfreq, ufreq;
    double tmin, tmax, fmin, fmax;

    //INI: Flags used to determine whether extrapolation or interpolation is to be done 
    //and to select an appropriate freq. plane for extrapolation
    bool extrapolate, ffbrick_asc, freq_match;
 
    // which axes are treated at input?
    std::vector<HIID> _in1_axis_id;
    std::vector<HIID> _in2_axis_id;
    // which axes are the output?
    std::vector<HIID> _out_axis_id;

    // axis numbers -- filled in by getResult()
    uint _inaxis0; 
    uint _inaxis1; 
    uint _inaxis2; 
    uint _inaxis3; 
    uint _outaxis0;
    uint _outaxis1;

    // OMS: removed the three coeff parameters
    void doInterpol(Vells & output_vells,
		    const Vells &input_vells_u,
		    const Vells &input_vells_v,
		    const Vells &input_vells_X, 
		    const Cells &rcells, 
		    const Cells &brickcells);
    static dcomplex bilinear(double s, double t, 
			     dcomplex fiaja, dcomplex fiajb, 
			     dcomplex fibjb, dcomplex fibja );
    static dcomplex BiCubic(double s, double t, 
			    dcomplex fiaja, dcomplex fiajb, 
			    dcomplex fibjb, dcomplex fibja, 
			    dcomplex fuiaja, dcomplex fuiajb, 
			    dcomplex fuibjb, dcomplex fuibja, 
			    dcomplex fviaja, dcomplex fviajb, 
			    dcomplex fvibjb, dcomplex fvibja, 
			    dcomplex fuviaja, dcomplex fuviajb, 
			    dcomplex fuvibjb, dcomplex fuvibja);
  };  
 
} // namespace Meq

#endif

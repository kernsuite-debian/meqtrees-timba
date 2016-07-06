//# InterpolCoeff.cc: Stored parameter with polynomial coefficients
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
//# $Id: InterpolCoeff.cc fba $

#include <MeqNodes/InterpolCoeff.h>
#include <MEQ/Request.h>
#include <MEQ/VellSet.h>
#include <MEQ/Cells.h>
#include <MEQ/Vells.h>
#include <MEQ/Axis.h>
#include <MEQ/VellsSlicer.h>

#include <fftw3.h>
#include <blitz/array/stencilops.h>

namespace Meq {
  
const HIID FAxesIn = AidAxes|AidIn;
const HIID FAxesOut = AidAxes|AidOut;

InterpolCoeff::InterpolCoeff()
  : Node(1)     // exactly 1 child expected
{
  _in_axis_id.resize(2);
  _in_axis_id[0] = "U";
  _in_axis_id[1] = "V";
  _out_axis_id.resize(2);
  _out_axis_id[0] = "U";
  _out_axis_id[1] = "V";
  Axis::addAxis("U");
  Axis::addAxis("V");
};

InterpolCoeff::~InterpolCoeff(){};

int InterpolCoeff::getResult (Result::Ref &resref,
			      const std::vector<Result::Ref> &child_results,
			      const Request &request,bool newreq)
{
  Assert(child_results.size()==1);
  const Result & childres = child_results[0];
  
  // first, figure out the axes. Exception will be thrown for us if any are
  // invalid
  _inaxis0 = Axis::axis(_in_axis_id[0]);
  _inaxis1 = Axis::axis(_in_axis_id[1]);
  _outaxis0 = Axis::axis(_out_axis_id[0]);
  _outaxis1 = Axis::axis(_out_axis_id[1]);
  
  // Get the Cells of the child and ensure that input axes are present and
  // uniformly gridded
  const Cells & input_cells = childres.cells();
  FailWhen(!input_cells.isDefined(_inaxis0) 
	   || !input_cells.isDefined(_inaxis1),
      "one or both input axes are not defined in the child cells");
  // disable this check for now
   
  // Figure out the shapes 
  // NB: for historical reasons, we'll use l,m to name variables referring to 
  // the input axes, and u,v when referring to the output axes. The real axes
  // in use are of course determined above.
  nu = input_cells.ncells(_inaxis0);
  nv = input_cells.ncells(_inaxis1);

  nu1 = int(nu/2)+1;  // center point in UV plane
  nv1 = int(nv/2)+1;
  
  // Now, the Result dimensions. Since we generate 4 output interpolation
  // planes per a single input plane, we add an extra tensor dimension of 
  // size 4
  Result::Dims dims = childres.dims();
  dims.resize(dims.size()-1);
  dims.push_back(4);
  
  // make a new Result
  Result & result = resref <<= new Result(dims);
  
  // now loop over input VellSets and FFT them one by one
  int ovs = 0;  // counter of output VellSets
  for( int ivs = 0; ivs<childres.numVellSets(); ivs++ )
  {
    const VellSet &vs_input = childres.vellSet(ivs);
    // create 4 new output VellSets
    VellSet * pvs_output[4];
    for( int i=0; i<4; i++,ovs++ )
      pvs_output[i] = &( result.setNewVellSet(ovs) );
    
    // if the input VellSet is a null, make null outputs and continue
    if( vs_input.isNull() )
    {
      Vells::Ref null_vells(DMI::ANONWR);
      for( int i=0; i<4; i++,ovs++ )
        pvs_output[i]->setValue(null_vells);
      continue;
    }
      
    // ok, we have actual values (main + possibly perturbed). 
    // Setup copies of spid/perturbations.
    for( int i=0; i<4; i++ )
    {
      pvs_output[i]->setNumPertSets(vs_input.numPertSets());
      pvs_output[i]->copySpids(vs_input);
      pvs_output[i]->copyPerturbations(vs_input);
    }
    
    // Now calculate coeffs for all values
    // this holds the 4 result Vells for each FFT
    Vells::Ref output_vells[4];
    
    // CalcCoeff the main value
    doCalcCoeff(output_vells,vs_input.getValue());
    for( int i=0; i<4; i++ )
      pvs_output[i]->setValue(output_vells[i]);
    
    // CalcCoeff each perturbed value
    for( int ipset=0; ipset<vs_input.numPertSets(); ipset++ )
      for( int ipert=0; ipset<vs_input.numSpids(); ipert++ )
      {
        doCalcCoeff(output_vells,vs_input.getPerturbedValue(ipert,ipset));
        for( int i=0; i<4; i++ )
          pvs_output[i]->setPerturbedValue(ipert,output_vells[i],ipset);
      }
      
    // finalize shapes of output vellsets
    for( int i=0; i<4; i++ )
      pvs_output[i]->verifyShape();
  }
  
  // Now figure out the output cells
  
  // NB: cell size is uniform thanks to check at start of getResult()
  const double du = input_cells.cellSize(_inaxis0)(0); 
  const double dv = input_cells.cellSize(_inaxis1)(0);

  const double umax = (nu-nu1+0.5)*du;
  const double umin = -(nu1-0.5)*du;

  const double vmax = (nv-nv1+0.5)*dv;
  const double vmin = -(nv1-0.5)*dv;

  // Construct the result domain 
  Domain::Ref domain(DMI::ANONWR);
  const Domain &input_domain = input_cells.domain();
  // copy over all axes not involved in the FFT
  for( uint i=0; i<uint(Axis::MaxAxis); i++ )
    if( i != _inaxis0 && i != _inaxis1 && input_domain.isDefined(i) )
      domain().defineAxis(i,input_domain.start(i),input_domain.end(i));
  // define output axes
  domain().defineAxis(_outaxis0,umin,umax);
  domain().defineAxis(_outaxis1,vmin,vmax);
  // construct the result cells
  Cells::Ref cells;
  cells <<= new Cells(*domain);
  // copy over all axes not involved in the FFT
  for( uint i=0; i<uint(Axis::MaxAxis); i++ )
    if( i != _inaxis0 && i != _inaxis1 && input_domain.isDefined(i) ){
      cells().setCells(i,input_cells.center(i),input_cells.cellSize(i));
      cells().recomputeSegments(i);
    }
  // define output axes
  cells().setCells(_outaxis0,umin,umax,nu);
  cells().setCells(_outaxis1,vmin,vmax,nv);
  // Set the Cells in the Result
  result.setCells(*cells);
  return 0;
};

using namespace blitz;

BZ_DECLARE_STENCIL4(make_interpolation_planes,U,V,UV,X)
  U = (X(-1,0) + X(+1,0))/2;
  V = (X(0,-1) + X(0,+1))/2;
  UV = (X(-1,-1) + X(-1,+1) + X(+1,-1) + X(+1,+1))/4;
BZ_END_STENCIL_WITH_SHAPE(shape(-1,-1),shape(+1,+1))

void InterpolCoeff::doCalcCoeff (Vells::Ref output_vells[4],
				 const Vells &input_vells)
{
  Vells::Shape input_shape = input_vells.shape();
  if ( _inaxis0>=input_shape.size() && _inaxis1>=input_shape.size() ) {
    //FailWhen(_inaxis0>=input_shape.size() && _inaxis1>=input_shape.size(),
    //    "one or both input axes are not present in input Vells");
    // No input shape, just a constant input value.
    output_vells[0] <<= new Vells(make_dcomplex(0.0));
    // create additional Vells for higher order interpolation coefficients
    output_vells[1] <<= new Vells(make_dcomplex(0.0));
    output_vells[2] <<= new Vells(make_dcomplex(0.0));
    output_vells[3] <<= new Vells(make_dcomplex(0.0));
  } else {
  
  // check that input axes are present
    //RJN 21-04-2006 
    //FailWhen(_inaxis0>=input_shape.size() && _inaxis1>=input_shape.size(),
    //    "one or both input axes are not present in input Vells");
  // check that output axes of input shape are trivial (unless we're
  // transforming in-place)
  FailWhen((_outaxis0!=_inaxis0 && _outaxis0<input_shape.size() && 
	    input_shape[_outaxis0]>1) ||
           (_outaxis1!=_inaxis1 && _outaxis1<input_shape.size() && 
	    input_shape[_outaxis1]>1),
      "one or both output axes are present in input Vells");
  FailWhen(input_shape[_inaxis0]!=nu && input_shape[_inaxis1]!=nv,
	   "one or both input axes are collapsed in input Vells, this is not yet supported");
  
  // figure out output vells shape 
  int maxrank = std::max(_outaxis0,_outaxis1);
  Vells::Shape output_shape = input_shape;
  output_shape.resize(maxrank+1,1); // resize to max rank and fill with 1s
  output_shape[_inaxis0] = 1;       // collapse input axes
  output_shape[_inaxis1] = 1;
  output_shape[_outaxis0] = nu;     // fill output axes
  output_shape[_outaxis1] = nv;

  Vells & vells0  = output_vells[0] <<= new Vells(make_dcomplex(0.0),output_shape,true);
  // create additional Vells for higher order interpolation coefficients
  Vells & vells0u = output_vells[1] <<= new Vells(make_dcomplex(0.0),output_shape,false);
  Vells & vells0v = output_vells[2] <<= new Vells(make_dcomplex(0.0),output_shape,false);
  Vells & vells0uv = output_vells[3] <<= new Vells(make_dcomplex(0.0),output_shape,false);

  // Reshuffle fft'd data around, and fill in the interpolation planes
  // This is similar to the operation above.
  // We'll use VellsSlicers to repeat this for all other dimensions
  ConstVellsSlicer<dcomplex,2> fft_slicer(input_vells,_outaxis0,_outaxis1);
  VellsSlicer<dcomplex,2> result_slicer(vells0,_outaxis0,_outaxis1);
  VellsSlicer<dcomplex,2> iu_slicer(vells0u,_outaxis0,_outaxis1);
  VellsSlicer<dcomplex,2> iv_slicer(vells0v,_outaxis0,_outaxis1);
  VellsSlicer<dcomplex,2> iuv_slicer(vells0uv,_outaxis0,_outaxis1);
  for( ; fft_slicer.valid(); 
       fft_slicer.incr(),result_slicer.incr(),iu_slicer.incr(),
       iv_slicer.incr(),iuv_slicer.incr() )
  {
    Assert(result_slicer.valid()); 
    // must be true since all other axes have the same shape
    Assert(iu_slicer.valid()); 
    // must be true since all other axes have the same shape
    Assert(iv_slicer.valid()); 
    // must be true since all other axes have the same shape
    Assert(iuv_slicer.valid()); 
    // must be true since all other axes have the same shape
    const blitz::Array<dcomplex,2> fft_arr = fft_slicer();
    blitz::Array<dcomplex,2> out_arr = result_slicer();
    blitz::Array<dcomplex,2> u_arr = iu_slicer();
    blitz::Array<dcomplex,2> v_arr = iv_slicer();
    blitz::Array<dcomplex,2> uv_arr = iuv_slicer();
    
    for (int i = 1; i < nu-1; i++)
      {
        out_arr(i,0) = fft_arr(i,0);
        u_arr(i,0) = (fft_arr(i+1,0) + fft_arr(i-1,0))/2.;
        v_arr(i,0) = (fft_arr(i,1) + fft_arr(i,nv-1))/2.;
        uv_arr(i,0) = (fft_arr(i+1,1) + fft_arr(i-1,1) +
		       fft_arr(i+1,nv-1) + fft_arr(i-1,nv-1))/4.;
	
        out_arr(i,nv-1) = fft_arr(i,nv-1);
        u_arr(i,nv-1) = (fft_arr(i+1,nv-1) + fft_arr(i-1,nv-1))/2.;
        v_arr(i,nv-1) = (fft_arr(i,0) + fft_arr(i,nv-2))/2.;
        uv_arr(i,nv-1) = (fft_arr(i+1,0) + fft_arr(i-1,0) +
			  fft_arr(i+1,nv-2) + fft_arr(i-1,nv-2))/4.;
	
      };
    
    for (int j = 1; j < nv-1; j++)
      {
	out_arr(0,j) = fft_arr(0,j);
	u_arr(0,j) = (fft_arr(1,j) + fft_arr(nu-1,j))/2.;
	v_arr(0,j) = (fft_arr(0,j+1) + fft_arr(0,j-1))/2.;
	uv_arr(0,j) = (fft_arr(1,j+1) + fft_arr(nu-1,j+1) +
		       fft_arr(1,j-1) + fft_arr(nu-1,j-1))/4.;
	
	out_arr(nu-1,j) = fft_arr(nu-1,j);
	u_arr(nu-1,j) = (fft_arr(0,j) + fft_arr(nu-2,j))/2.;
	v_arr(nu-1,j) = (fft_arr(nu-1,j+1) + fft_arr(nu-1,j-1))/2.;
	uv_arr(nu-1,j) = (fft_arr(0,j+1) + fft_arr(nu-2,j+1) +
			  fft_arr(0,j-1) + fft_arr(nu-2,j-1))/4.;
	
      };
    
    out_arr(0,0) = fft_arr(0,0);
    u_arr(0,0) = (fft_arr(1,0) + fft_arr(nu-1,0))/2.;
    v_arr(0,0) = (fft_arr(0,1) + fft_arr(0,nv-1))/2.;
    uv_arr(0,0) = (fft_arr(1,1) + fft_arr(nu-1,1) +
		   fft_arr(1,nv-1) + fft_arr(nu-1,nv-1))/4.;
    
    out_arr(0,nv-1) = fft_arr(0,nv-1);
    u_arr(0,nv-1) = (fft_arr(1,nv-1) + fft_arr(nu-1,nv-1))/2.;
    v_arr(0,nv-1) = (fft_arr(0,0) + fft_arr(0,nv-2))/2.;
    uv_arr(0,nv-1) = (fft_arr(1,0) + fft_arr(nu-1,0) +
		      fft_arr(1,nv-2) + fft_arr(nu-1,nv-2))/4.;

    out_arr(nu-1,0) = fft_arr(nu-1,0);
    u_arr(nu-1,0) = (fft_arr(0,0) + fft_arr(nu-2,0))/2.;
    v_arr(nu-1,0) = (fft_arr(nu-1,1) + fft_arr(nu-1,nv-1))/2.;
    uv_arr(nu-1,0) = (fft_arr(0,1) + fft_arr(nu-2,1) +
		      fft_arr(0,nv-1) + fft_arr(nu-2,nv-1))/4.;

    out_arr(nu-1,nv-1) = fft_arr(nu-1,nv-1);
    u_arr(nu-1,nv-1) = (fft_arr(0,nv-1) + fft_arr(nu-2,nv-1))/2.;
    v_arr(nu-1,nv-1) = (fft_arr(nu-1,0) + fft_arr(nu-1,nv-2))/2.;
    uv_arr(nu-1,nv-1) = (fft_arr(0,0) + fft_arr(nu-2,0) +
		         fft_arr(0,nv-2) + fft_arr(nu-2,nv-2))/4.;
  }
  }
}

void InterpolCoeff::setStateImpl (DMI::Record::Ref& rec, bool initializing)
{
  Node::setStateImpl(rec,initializing);

  std::vector<HIID> in = _in_axis_id;
  if( rec[FAxesIn].get_vector(in,initializing) || initializing )
  {
    FailWhen(in.size()!=2,FAxesIn.toString()+" field must have 2 elements");
    _in_axis_id = in;
    Axis::addAxis(_in_axis_id[0]);
    Axis::addAxis(_in_axis_id[1]);
  }
  std::vector<HIID> out = _out_axis_id;
  if( rec[FAxesOut].get_vector(out,initializing) || initializing )
  {
    FailWhen(out.size()!=2,FAxesOut.toString()+" field must have 2 elements");
    _out_axis_id = out;
    Axis::addAxis(_out_axis_id[0]);
    Axis::addAxis(_out_axis_id[1]);
  }
  // do not check that axes are valid -- this is done in getResult()
};
  
} // namespace Meq

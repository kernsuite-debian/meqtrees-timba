//# UVInterpol.h
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

#ifndef MEQNODES_UVINTERPOL_H
#define MEQNODES_UVINTERPOL_H

//# Includes
#include <MEQ/Node.h>

#include <MeqNodes/TID-MeqNodes.h>
#include <MeqNodes/AID-MeqNodes.h>

#pragma aidgroup MeqNodes
#pragma types #Meq::UVInterpol

#pragma aids UVInterpol_method

namespace Meq {

class UVInterpol: public Node  {
  public:
    // The default constructor.
    // The object should be filled by the init method.
    UVInterpol();

    virtual ~UVInterpol();

    virtual TypeId objectType() const
    { return TpMeqUVInterpol; }
    
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
    int _method;
    // How many values in the u and v and t directions 
    int nu, nv, nt, nn;
    // which axes are treated at input?
    std::vector<HIID> _in_axis_id;
    // which axes are the output?
    std::vector<HIID> _out_axis_id;

    // axis numbers -- filled in by getResult()
    uint _inaxis0; 
    uint _inaxis1; 
    uint _inaxis2; 
    uint _inaxis3; 
    uint _outaxis0;
    uint _outaxis1;

    void doInterpol(Vells::Ref &output_vells,
		    const Vells &input_vells_uv,
		    const Vells &input_vells_X);
    
  };  
  
} // namespace Meq

#endif

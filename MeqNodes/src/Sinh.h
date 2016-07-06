//# Sinh.h: Take hyperbolic sine of a node
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

#ifndef MEQNODES_SINH_H
#define MEQNODES_SINH_H
    
#include <MEQ/Function.h>

#include <MeqNodes/TID-MeqNodes.h>
#pragma aidgroup MeqNodes
#pragma types #Meq::Sinh

namespace Meq {    


//##ModelId=400E5303028B
class Sinh : public Function1
{
public:
    //##ModelId=400E530500C3
  Sinh();

    //##ModelId=400E530500C4
  virtual ~Sinh();

    //##ModelId=400E530500C7
    virtual TypeId objectType() const
    { return TpMeqSinh; }

  // Evaluate the value for the given request.
    //##ModelId=400E530500C9
  virtual Vells evaluate (const Request&,const LoShape &,
			  const vector<const Vells*>& values);

};


} // namespace Meq

#endif
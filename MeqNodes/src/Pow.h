//# Pow.h: Take the power of one node to another
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

#ifndef MEQNODES_POW_H
#define MEQNODES_POW_H
    
#include <MEQ/Function.h>

#include <MeqNodes/TID-MeqNodes.h>
#pragma aidgroup MeqNodes
#pragma types #Meq::Pow

namespace Meq {    


//##ModelId=400E5304003D
class Pow : public Function2
{
public:
    //##ModelId=400E53540382
  Pow();

    //##ModelId=400E53540383
  virtual ~Pow();

    //##ModelId=400E53540386
    virtual TypeId objectType() const
    { return TpMeqPow; }

  // Evaluate the value for the given request.
    //##ModelId=400E53540389
  virtual Vells evaluate (const Request&,const LoShape &,
			  const vector<const Vells*>& values);
};


} // namespace Meq

#endif

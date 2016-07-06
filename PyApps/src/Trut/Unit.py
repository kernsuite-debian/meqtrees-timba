#
#% $Id$ 
#
#
# Copyright (C) 2002-2007
# The MeqTree Foundation & 
# ASTRON (Netherlands Foundation for Research in Astronomy)
# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>,
# or write to the Free Software Foundation, Inc., 
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

import sys
from Timba import dmi

from Trut import _dprint,_dprintf


class Unit (object):
  """Trut.Unit is an abstract class representing a Trut "test unit".
  A test unit is created with a "name" and an optional parent unit.
  A test unit can be:
    * executed()'d
      This is called when all stanzas and directives pertaining to a test unit have
      been read in. The unit can call success() or fail() to report its status
    * fail()'d
      Marks a unit as failed. Normal behaviour is to report to parents.
  """;
  def __init__ (self,name,parent=None):
    _dprint(2,"new unit",name,self.__class__);
    self.name = name;
    self.parent = parent;
    self.failed = None;
    self._fail_logged = False;
    self._success_logged = False;
    # inherit parent's options, and persistence level (-1)
    if parent:
      self.options = dmi.record(**parent.options);
      self.persist = parent.persist - 1;
    else:
      self.options = dmi.record();
      self.persist = 0;
  
  def execute (self):
    """execute() is called after all stanzas related to a test unit have been read""";
    pass;
  
  def cleanup (self):
    """cleanup() is called after an execute (whethere successful or not), to clean up after testing""";
    self.success();
    
  def success (self,message=''):
    """Reports a success with given message, unless we've already been failed. Returns
    True if no fail, False if failed""";
    if self.failed:
      self.fail();
      return False;
    else:
      if not self._success_logged:
        self.log(message,"OK",level=1);
        self._success_logged = True;
      return True;
    
  def fail (self,message='',from_child=False):
    """Reports a fail with given message, fails parent. Message can be a string,
    or an exception object""";
    # if we're persistent, then do not report fails when called from a child
    # -- we want to report a single big fail later, after all children have checked in
    if not self._fail_logged:
      if not from_child or self.persist <= 0:
        if isinstance(message,Exception):
          message = message.__class__.__name__;
        self.log(message,"FAIL",level=-999999);
        self._fail_logged = True;
    if not self.failed:
      self.parent and self.parent.fail(from_child=True);
      self.failed = True;
      
  def giveup (self):
    """Returns True if we're marked as failed, and not asked to be persistent.
    Useful for compound units that can run multiple subtests -- they should check in 
    between subtests, and give up if giveup() is true""";
    return self.failed and self.persist <= 0;
  
  def log_message (self,message,status,level=1,progress=False):
    """Logs message at a given verbosity level""";
    self.parent and self.parent.log_message("  "+message,status,level+10,progress=progress);
      
  def log (self,message,status='',level=1):
    """Logs message at a given verbosity level, prepending our unit class and name""";
    self.log_message("%s %s: %s"%(self.__class__.__name__,self.name,message),status,level);

  def log_exc (self,exctype,excvalue,exctb,level=1):
    """Logs exception at a given verbosity level""";
    self.parent and self.parent.log_exc(exctype,excvalue,exctb,level=level+10);
    
  def log_progress (self,message):
    """Logs progress message. These will only appear on the console (normally)""";
    self.log_message("%s %s: %s"%(self.__class__.__name__,self.name,message),status="",progress=True);
    
  def set_option (self,option,value):
    """set_option() is called if an option line is found inside a stanza for that unit""";
    self.options[option] = value;
    
  def get_option (self,option,default=None):
    return self.options.get(option,default);
  
  def _allocate_tmp_loggers (self):
    """returns a set of temporary logfiles for use by child jobs""";
    return self.parent and self.parent._allocate_tmp_loggers();
  
  def _set_tmp_loggers (self,tmplogs):
    """switches loggers to temporary logfiles for use by child jobs. The tmplogs
    argument should be the return value of _allocate_tmp_loggers(), above.""";
    return self.parent and self.parent._set_tmp_loggers(tmplogs);
  
  def _merge_tmp_loggers (self,tmplogs):
    """merges in temporary logfiles filled in by child jobs. The tmplogs argument 
    should be the return value of _allocate_tmp_loggers(), above.""";
    return self.parent and self.parent._merge_tmp_loggers(tmplogs);

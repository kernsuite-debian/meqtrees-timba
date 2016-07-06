include 'mqsinit_test.g'
include 'table.g'
include 'measures.g'
include 'quanta.g'







# helper func
# creates fully-qualified node name, by pasting a bunch of suffixes after
# the name, separated by dots.
const fq_name := function (name,...)
{
  res := name;
  for( i in 1:num_args(...) )
  {
    q := nth_arg(i,...);
    if( !is_string(q) || q )
      res := paste(res,q,sep='.');
  }
  return res;
}







# creates common nodes:
#   'ra0' 'dec0':       phase center
const create_common_parms := function (ra0,dec0)
{
  mqs.createnode(meq.parm('ra0',ra0));
  mqs.createnode(meq.parm('dec0',dec0));
}



const create_constants := function()
{
    mqs.createnode(meq.node('MeqConstant','one',[value=1.0]));
}






# creates all source-related nodes and subtrees:
#   'stokesI':          flux
#   'ra' 'dec':         source position
#   'lmn','n':          LMN coordinates, N coordinate
# src specifies the source suffix ('' for none)
const create_source_subtrees := function (sti,ra,dec,src='', mep_table_name='')
{
    global ms_timerange, ms_freqranges;

    # 3rd order frequency-dependence, 0th order time dependence
    polc_array := array(as_double(0),1,1);
    polc_array[1,1] := sti;
    
    #fmin := ms_freqranges[1][1];
    #fmax := ms_freqranges[2][1];
    #tmin := ms_timerange[1];
    #tmax := ms_timerange[2];
    polc := meq.polc(polc_array,scale=[10000.0, 1e6], offset=[4.47204e9,1.175e9]);#,domain=meq.domain(fmin,fmax,tmin,tmax)); # domain: entire dataset
    print polc;
    # meq.parm(), meq.node() return init-records
    # mqs.createnode() actually creates a node from an init-record.
    
    stokes_I_node := meq.parm(fq_name('stokes_i',src),polc,groups="a");
    if( mep_table_name != ''){
        stokes_I_node.table_name := mep_table_name;
    }
    
    mqs.createnode(stokes_I_node);
    # note the nested-record syntax here, to create child nodes implicitly
    mqs.createnode(meq.node('MeqLMN',
                            fq_name('lmn',src),
                            children=[
                                      ra_0  ='ra0',
                                      dec_0 ='dec0',
                                      ra    =meq.parm(fq_name('ra',src),ra,
                                                      groups="a"),
                                      dec   =meq.parm(fq_name('dec',src),dec,
                                                      groups="a")]));
    mqs.createnode(meq.node('MeqSelector',fq_name('l',src),[index=1],
                            children=fq_name("lmn",src)));
    mqs.createnode(meq.node('MeqSelector',fq_name('m',src),[index=2],
                            children=fq_name("lmn",src)));
    mqs.createnode(meq.node('MeqSelector',fq_name('n',src),[index=3],
                            children=fq_name("lmn",src)));
}









# builds an init-record for a "dft" tree for one station (st)
const sta_dft_tree := function (st,src='')
{
  global ms_antpos; # station positions from MS
  global mepuvw;    # MEP table with UVWs
  pos := ms_antpos[st];
  # create a number of UVW nodes, depending on settings
  uvwlist := dmi.list();
  # parms from MEP table
  if( mepuvw )
  {
    dmi.add_list(uvwlist,
      meq.node('MeqComposer',fq_name('mep.uvw',st),children=meq.list(
        meq.node('MeqParm',fq_name('U',st),[table_name=mepuvw]),
        meq.node('MeqParm',fq_name('V',st),[table_name=mepuvw]),
        meq.node('MeqParm',fq_name('W',st),[table_name=mepuvw])
      )) );
  }
  # const values based on what was read from the MS
  if( !is_boolean(ms_antuvw) )
  {
    dmi.add_list(uvwlist,
      meq.node('MeqComposer',fq_name('ms.uvw',st),children=meq.list(
        meq.node('MeqConstant',fq_name('u',st),[value=ms_antuvw[1,st]]),
        meq.node('MeqConstant',fq_name('v',st),[value=ms_antuvw[2,st]]),
        meq.node('MeqConstant',fq_name('w',st),[value=ms_antuvw[3,st]])
      )) );
  }
  # finally, this node computes UVWs directly
  dmi.add_list(uvwlist,
            meq.node('MeqUVW',fq_name('uvw',st),children=[
                         x = meq.parm(fq_name('x',st),pos.x),
                         y = meq.parm(fq_name('y',st),pos.y),
                         z = meq.parm(fq_name('z',st),pos.z),
                         ra = 'ra0',dec = 'dec0',
                         x_0='x0',y_0='y0',z_0='z0' ]));
                        
  uvw := meq.node('MeqReqSeq',fq_name('uvw.seq',st),[result_index=1,
                  link_or_create=T],children=uvwlist);
    
  # builds an init-rec for a node called 'dft.N' with two children: 
  # lmn and uvw.N
  n_minus_one := meq.node('MeqSubtract', fq_name('nminusone', src),
                          [link_or_create=T],
                          children=meq.list(fq_name('n', src),
                                            'one'));
  
  lmn_minus_one := meq.node('MeqComposer', fq_name('lmnminusone', src),
                            [link_or_create=T],
                            children=meq.list(fq_name('l', src),
                                              fq_name('m', src),
                                              n_minus_one));
  dft := meq.node('MeqVisPhaseShift',fq_name('dft0',src,st),
                  [link_or_create=T],
                  children=[lmn = lmn_minus_one, uvw=uvw ]);
  # add antenna gains/phases
  amp_node := meq.parm(fq_name('GA',st),1.0,groups="a");
  amp_node.table_name := '3C343.mep';

  phase_node :=meq.parm(fq_name('GP',st),0.0,groups="a");
  phase_node.table_name := '3C343.mep';
  
  gain := meq.node('MeqPolar',fq_name('G',st),[link_or_create=T],
                   children=meq.list(amp_node, phase_node) );
              
  return meq.node('MeqMultiply',fq_name('dft',src,st),[link_or_create=T],
                    children=meq.list(dft,gain));
}







# builds an init-record for a "dft" tree for source 'src' and two stations (st1,st2)
const ifr_source_predict_tree := function (st1,st2,src='')
{
    dft1 := sta_dft_tree(st1, src);
    dft2 := sta_dft_tree(st2, src);
    
    conj_st2 := meq.node('MeqConj', fq_name('pointsource_conj', src, st2),
                         [link_or_create=T],
                         children=meq.list(dft2));
    
    phasedft := meq.node('MeqMultiply', fq_name('phasedft', src, st1,st2),
                         [link_or_create=T],
                         children=meq.list(dft1, conj_st2));
    
    modflux  := meq.node('MeqDivide', fq_name('modified_i',src),
                         [link_or_create=T],
                         children=meq.list(fq_name('stokes_i',src),
                                           fq_name('n', src)));
    
    predict  := meq.node('MeqMultiply', fq_name('predict',src,st1,st2),
                         [link_or_create=T],
                         children=meq.list(phasedft, modflux));
    return predict;
}










# builds an init-record for a sum of "dft" trees for all sources and st1,st2
const ifr_predict_tree := function (st1,st2,src=[''])
{
  if( len(src) == 1 )
    return ifr_source_predict_tree(st1,st2,src);
  list := dmi.list();
  for( s in src ) 
    dmi.add_list(list,ifr_source_predict_tree(st1,st2,s));
  return meq.node('MeqAdd',fq_name('predict',st1,st2),children=list);
}












# creates nodes shared among trees: source parms, array center (x0,y0,z0)
const make_shared_nodes := function (stokesi=1,ra=0,dec=0,src=[''],
                                     mep_table_name='')
{
  global ms_phasedir;
  ra0  := ms_phasedir[1];  # phase center
  dec0 := ms_phasedir[2];
  # setup source parameters and subtrees
  create_common_parms(ra0,dec0);
  
  create_constants();
  
  for( i in 1:len(src) ) {
    print src[i];
    print create_source_subtrees(stokesi[i],ra[i],dec[i],src[i], mep_table_name);
  }
  # setup zero position
  global ms_antpos;
  names := "x0 y0 z0";
  for( i in 1:3 )
    mqs.createnode(meq.node('MeqConstant',names[i],[value=ms_antpos[1][i]]));
}













# builds a predict tree for stations st1, st2
const make_predict_tree := function (st1,st2,src=[''])
{
  sinkname := fq_name('sink',st1,st2);
  if( len(src) == 1 )
    pred := ifr_predict_tree(st1,st2,src);
  else 
  {
    list := dmi.list();
    for( s in src ) 
      dmi.add_list(list,ifr_predict_tree(st1,st2,s));
    pred := meq.node('MeqAdd',fq_name('predict',st1,st2),children=list);
  }
  # create a sink
  mqs.createnode(meq.node('MeqSink',sinkname,
                         [ output_col      = '',   # init-rec for sink
                           station_1_index = st1,
                           station_2_index = st2,
                           corr_index      = [1],
                           flag_mask       = -1 ],
                           children=dmi.list(
                            ifr_predict_tree(st1,st2,src)
                           )));
  return sinkname;
}










# builds a read-predict-subtract tree for stations st1, st2
const make_subtract_tree := function (st1,st2,src=[''])
{
  sinkname := fq_name('sink',st1,st2);
  
  # create a sink & subtree attached to it
  # note how meq.node() can be passed a record in the third argument, to specify
  # other fields in the init-record
  mqs.createnode(
    meq.node('MeqSink',sinkname,
                         [ output_col      = '',
                           station_1_index = st1,
                           station_2_index = st2,
                           corr_index      = [1],
                           flag_mask        = -1 ],
                         children=meq.list(
      meq.node('MeqSubtract',fq_name('sub',st1,st2),children=meq.list(
        meq.node('MeqSelector',fq_name('xx',st1,st2),[index=1],children=meq.list(
          meq.node('MeqSpigot',fq_name('spigot',st1,st2),[ 
            station_1_index=st1,
            station_2_index=st2,
            flag_bit=4,
            input_column='DATA'])
        )),
        ifr_predict_tree(st1,st2,src)
      ))
    ))
  );
  return sinkname;
}










# builds a solve tree for stations st1, st2
const make_solve_tree := function (st1,st2,src=[''],subtract=F,flag=F)
{
  sinkname := fq_name('sink',st1,st2);
  predtree := ifr_predict_tree(st1,st2,src);
  predname := predtree.name;
  mqs.createnode(predtree);
  
  spigot_node:=meq.node('MeqSpigot',fq_name('spigot',st1,st2),
                        [station_1_index=st1,
                         station_2_index=st2,
                         flag_bit=4,
                         link_or_create=T,
                         input_column='DATA']);
  
  xx_node := meq.node('MeqSelector',fq_name('xx',st1,st2),
                      [index=1],
                      children=meq.list(spigot_node));
  
  yy_node := meq.node('MeqSelector',fq_name('yy',st1,st2),
                      [index=4],
                      children=meq.list(spigot_node));

  observed_i_node :=meq.node('MeqAdd', fq_name('observed_i', st1, st2),
                             children=meq.list(xx_node,yy_node));
                                    
  #print spigot_node;
  #print xx_node;
  #print yy_node;
  #print observed_i_node;
  #exit
  # create condeq tree (solver will plug into this)
  mqs.createnode(meq.node('MeqCondeq',fq_name('ce',st1,st2),
                          children=meq.list(predname, observed_i_node)
                          )
                 );
  # create subtract sub-tree
  if( subtract )
  {
    subname := fq_name('sub',st1,st2);
    mqs.createnode(meq.node('MeqSubtract',subname,
                      children=[fq_name('xx',st1,st2),predname]));
    if( !is_boolean(flag) )
    {
      datanodename:=fq_name('mof',st1,st2);
      mqs.createnode(
        meq.node('MeqMergeFlags',datanodename,children=meq.list(
          subname,
          meq.node('MeqZeroFlagger',fq_name('zf',st1,st2),[flag_bit=2,oper='GE',force_output=T],children=meq.list(
            meq.node('MeqSubtract',fq_name('zfsub',st1,st2),children=meq.list(
              meq.node('MeqAbs',fq_name('zfabs',st1,st2),children=subname),
              meq.node('MeqConstant',fq_name('of1threshold',st1,st2),[value=flag])
            ))
          ))
        ))
      );
    }
    else
      datanodename := subname;
  }
  else
    subname := fq_name('spigot',st1,st2);
  
  # create root tree (plugs into solver & subtract)     
  mqs.createnode(
    meq.node('MeqSink',sinkname,[ output_col      = '',
                                  station_1_index = st1,
                                  station_2_index = st2,
                                  corr_index      = [1], 
                                  flag_mask       = -1 ],children=meq.list(
      meq.node('MeqReqSeq',fq_name('seq',st1,st2),[result_index=2],
        children=['solver',datanodename])
   ))
 );

  return sinkname;
}











# reads antenna positions and phase center from MS,
# puts them into global variables
const get_ms_info := function (msname='test.ms',uvw=T)
{
  global ms_phasedir,ms_antpos, ms_timerange, ms_freqranges;
  
  ms := table(msname);
  msant := table(ms.getkeyword('ANTENNA'));
  pos := msant.getcol('POSITION');
  num_ant := msant.nrows();
  msant.done();
  
  tcol := ms.getcol('TIME');
  t_min := min(tcol) - 1;
  t_max := max(tcol) + 1;
  ms_timerange := [t_min, t_max];
  tcol := F;

  freqtab := table(ms.getkeyword('SPECTRAL_WINDOW'));
  num_chan := freqtab.getcol('NUM_CHAN');
  chan_freqs := freqtab.getcol('CHAN_FREQ');
  num_spw := freqtab.nrows();
  print num_spw;
  print 'CHAN_FREQ SHAPE: ', shape(chan_freqs);
  ms_freqranges := array(0.0,2,num_spw);
  for( i in 1:num_spw){
      print i;
      ms_freqranges[,i] := [min(chan_freqs[,i]), max(chan_freqs[,i])];
  }
  freqtab.close();

  time0 := ms.getcell('TIME',1);
  
  # convert position to x y z
  ms_antpos := [=];
  for(i in 1:len(pos[1,]))
    ms_antpos[i] := [ x=pos[1,i],y=pos[2,i],z=pos[3,i] ];
    
  msfld := table(ms.getkeyword('FIELD'));
  ms_phasedir := msfld.getcol('PHASE_DIR');
  msfld.done();
  
  if( uvw )
  {
    global ms_antuvw;
    ms_antuvw := array(0.,3,num_ant);
    mss := ms.query('DATA_DESC_ID==0');
    # get UVW coordinates from ms
    ant1 := mss.getcol('ANTENNA1');
    ant2 := mss.getcol('ANTENNA2');
    uvw  := mss.getcol('UVW');
    mask1 := ant1 == 0;
    uvw0 := uvw[,mask1];
    ant2 := ant2[mask1];
    for( a2 in 2:num_ant )
    {
      t := uvw0[,ant2==(a2-1)];
      if( len(t) != 3 )
      {
        print 'cannot use UVWs from MS if more than one time slot';
        fail 'cannot use UVWs from MS if more than one time slot';
      }
      ms_antuvw[,a2] := t;
    }
    print 'Antenna UVWs:',ms_antuvw;
  }
  
  # get some UVWs, just for shits and giggles
  a0 := ms_antpos[1];
  pos0 := dm.position('itrf',dq.unit(a0.x,'m'),dq.unit(a0.y,'m'),dq.unit(a0.z,'m'));
  a1 := ms_antpos[1];
  a2 := ms_antpos[2];
  ba1 := dm.baseline('itrf',dq.unit(a2.x-a1.x,'m'),dq.unit(a2.y-a1.y,'m'),dq.unit(a2.z-a1.z,'m'));
  ba2 := dm.baseline('itrf',dq.unit(a1.x,'m'),dq.unit(a1.y,'m'),dq.unit(a1.z,'m'));
  dm.doframe(pos0);
  dm.doframe(dm.direction('j2000',dq.unit(ms_phasedir[1],"rad"),dq.unit(ms_phasedir[2],"rad")));
  dm.doframe(dm.epoch('utc',dq.unit(time0,'s')));
  local uvw1a;
  local dot;
  uvw1b := dm.touvw(ba1,dot,uvw1a);
  uvw1c := dm.addxvalue(uvw1b);
  uvw2b := dm.touvw(ba2,dot,uvw2a);
  uvw2c := dm.addxvalue(uvw2b);
  
  ms.done();
  
  print 'Antenna position 1: ',ms_antpos[1];
  print 'Antenna position 2: ',ms_antpos[2];
  print 'Phase dir: ',ms_phasedir[1],' ',ms_phasedir[2];
  print 'UVW1a:',uvw1a;
  print 'UVW1b:',uvw1b;
  print 'UVW1c:',uvw1c;
  print 'UVW2a:',uvw2a;
  print 'UVW2b:',uvw2b;
  print 'UVW2c:',uvw2c;
  print 'Does this look sane?';
  
  return T;
}










const reexec := function (node='dft.a.4',nfreq=10,ntime=10)
{
  global rqid;
  st := mqs.getnodestate(node);
  dom := st.request.cells.domain;
  cells := meq.cells(dom,nfreq,ntime);
  req := meq.request(cells,hiid());
  mqs.execute(node,req);
  print 'you should be looking at node ',node;
  print 'cells are: ',cells;
  print 'domain was: ',cells;
}













# predict=T:  predict tree only (writes predict to output column)
# subtract=T: predict+subtract trees (writes residual to output column)
# solve=T,subtract=T: solve+subtract trees (writes residual to output column)
# solve=T,subtract=F: solve but no subtract
#
# run=F: build trees and stop, run=T: run over the measurement set
const do_test := function (predict=F,subtract=F,solve=F,run=T,
    flag=F,                         # supply threshold to flag output
    msname='test.ms',
    stset=1:4,                      # stations for which to make trees
    solve_fluxes=F,
    solve_gains=F,
    solve_phases=F,
    mep_table_name='',
    msuvw=F,                        # use UVW values from MS
    mepuvw=F,                       # use UVW from MEP table (should be filled already)
    load='',                        # load forest from file 
    save='',                        # save forest to file
    set_breakpoint=F,               # set breakpoint on solver
    publish=3)    # node publish: higher means more detail
{
  # clear output column in MS
  # (I still can't get this to work from C++, hence this bit of glish here)
  if( use_initcol )
  {
    print 'Clearing',outcol,'column, please ignore error messages'
    tbl := table(msname,readonly=F);
    desc := tbl.getcoldesc(outcol);
    if( is_fail(desc) )
      desc := [=];
    # insert column anew, if no shape
    if( has_field(desc,'shape') )
      cellshape := desc.shape;
    else
    {
      if( len(desc) )
        tbl.removecols(outcol);
      cellshape := tbl.getcell('DATA',1)::shape;
      desc := tablecreatearraycoldesc(outcol,complex(0),2,shp);
      tbl.addcols(desc);
    }
    # insert zeroes
    tbl.putcol(outcol,array(complex(0),cellshape[1],cellshape[2],tbl.nrows()));
    tbl.done();
  }

  # read antenna positions, etc.  
  get_ms_info(msname,uvw=msuvw);
  
  # initialize meqserver (see mqsinit_test.g)
  if( is_fail(mqsinit()) )
  {
    print mqs;
    fail;
  }
  mqs.track_results(F);

  mqs.meq('Clear.Forest',[=]);
  # load forest if asked
  if( load )
      mqs.meq('Load.Forest',[file_name=load]);
  else # else build trees
  {
      # create common nodes (source parms and such)
      
      make_shared_nodes(src_sti,src_ra,src_dec,src_names,mep_table_name);
      
      # make a solver node (since it's only one)
      if( solve )
      {
          # accumulate list of IFR condeqs
          condeqs := [];
          for( st1 in stset )
              for( st2 in stset )
                  if( st1 < st2 )
                      condeqs := [condeqs,fq_name('ce',st1,st2)];
          # solvable parms
          solvables := "";
          if(solve_fluxes){
              for(sourcename in src_names){
                  solvables := [solvables, fq_name('stokes_i', sourcename)];
              }
          }
          if( solve_gains ){
              for( st in stset[1:len(stset)] ){
                  solvables := [solvables,fq_name('GA',st)];
              }
          }

          if( solve_phases ){
              for( st in stset[2:len(stset)] ){
                  solvables := [solvables,fq_name('GP',st)];
              }
          }

          print solvables;

          # note that child names will be resolved later
          global solver_defaults;
          rec := meq.node('MeqSolver','solver',[
                                                parm_group = hiid("a"),
                                                default    = solver_defaults,
                                                solvable   = meq.solvable_list(solvables) ],
                          children=condeqs);
          mqs.createnode(rec);
      }
      if( publish>0 )
      {
          for( s in src_names )
          {
              mqs.meq('Node.Publish.Results',[name=fq_name("ra",s)]);
              mqs.meq('Node.Publish.Results',[name=fq_name("dec",s)]);
              mqs.meq('Node.Publish.Results',[name=fq_name("stokes_i",s)]);
          }
      }
      rootnodes := [];
      # make predict/condeq trees
      for( st1 in stset )
          for( st2 in stset )
              if( st1 < st2 )
              {
                  if( solve )
                  {
                      rootnodes := [rootnodes,
                                    make_solve_tree(st1,st2,src=src_names,
                                                    subtract=subtract,
                                                    flag=flag)];
                      if( publish>1 )
                          mqs.meq('Node.Publish.Results',
                                  [name=fq_name('ce',st1,st2)]);
                  }
                  else if( subtract )
                      rootnodes := [rootnodes,
                                    make_subtract_tree(st1,st2,src_names)];
                  else
                      rootnodes := [rootnodes,
                                    make_predict_tree(st1,st2,src_names)];
                  if( publish>2 )
                      mqs.meq('Node.Publish.Results',
                              [name=fq_name('predict',st1,st2)]);
              }
      # resolve children on all root nodes
      # print 'Root nodes are: ',rootnodes;
      print "Resolving root nodes";
      for( r in rootnodes )
          mqs.resolve(r);
  }
  # save forest if requested
  if( save )
    mqs.meq('Save.Forest',[file_name=save]);
  
  # get a list of nodes
  nodelist := mqs.getnodelist(children=T);
  # print 'Nodes: ',nodelist.name;
  
  # enable publishing of solver results
  if( solve && publish>0 ) {
    mqs.meq('Node.Publish.Results',[name='solver']);
#    mqs.meq('Node.Publish.Results',[name='GP.12']);
#    mqs.meq('Node.Publish.Results',[name='G.12']);
#    mqs.meq('Node.Publish.Results',[name='modified_i.3C343']);
#    mqs.meq('Node.Publish.Messages',[name='x.1']);
#    mqs.meq('Node.Publish.Messages',[name='dft0.3D343_1.1']);
#    mqs.meq('Node.Publish.Results',[name=fq_name('dft.b',4,8)]);
#    mqs.meq('Node.Publish.Results',[name=fq_name('U',8)]);
  }
  
  if( set_breakpoint ){
      mqs.meq('Node.Set.Breakpoint',[name='solver']);
      mqs.meq('Debug.Set.Level',[debug_level=100]);
  }
  
  if( any(argv == '-cacheall') )
    mqs.meq('Set.Forest.State',[state=[cache_policy=100]]);

  # run over MS
  if( run )
    do_run();
}

const do_run := function ()
{
  # activate input and watch the fur fly  
  global inputrec,outputrec;
  mqs.init([mandate_regular_grid=F,output_col='PREDICT'],input=inputrec,output=outputrec); 
}




#--------------------------------------------
#
# Source flux fitting on raw data
#
#--------------------------------------------
source_flux_fit_no_calibration := function()
{
    global inputrec, outputrec,solver_defaults,msname,mepuvw;
    global src_ra, src_dec, src_sti, src_names, outcol;
    global use_initcol;

    use_initcol := F;       # initialize output column with zeroes

    msname := '3C343.MS';
    mep_table_name := '3C343.mep';

    # Clear MEP table
    meptable := table(mep_table_name, readonly=F);
    nrows := meptable.nrows();
    meptable.removerows(1:nrows);
    meptable.done();

    mepuvw := F;
    filluvw := any(argv=='-filluvw');
    solve_fluxes:= T;#any(argv == '-fluxes');
    solve_gains := any(argv=='-gains');
    solve_phases := any(argv=='-phases');
    set_breakpoint := any(argv=='-bp');
    
    src_ra  := ([4.356645791155902,4.3396003966265599]);
    src_dec := ([1.092208429052697,1.0953677174056471]);
    src_sti  := [1,1];
    src_names := "3C343_1 3C343";
    
    
# fill UVW parms from MS if requested
    if( mepuvw )
    {
        include 'meq/msuvw_to_mep.g'
            mepuvw := msname ~ s/.ms/.mep/;
        if( filluvw )
            fill_uvw(msname,mepuvw);
    }
    else
        mepuvw := F;
    
    outcol := 'PREDICTED_DATA';
    solver_defaults := [ num_iter=6,save_funklets=T,last_update=T ];
    
    inputrec := [ ms_name = msname,data_column_name = 'DATA',
                 tile_size=1500,# clear_flags=T,
                 selection = [ channel_start_index=5,
                              channel_end_index=60 ,
                              selection_string=''] ];
    
    outputrec := [ write_flags=F,predict_column=outcol ];
    
    res := do_test(msname=msname,solve=T,subtract=T,run=T,flag=F,
                   stset=[1:14],
                   solve_fluxes=solve_fluxes,
                   solve_gains=solve_gains,
                   solve_phases=solve_phases,
                   set_breakpoint=set_breakpoint,
                   mep_table_name=mep_table_name,
                   publish=1,mepuvw=mepuvw,msuvw=msuvw);
    
    
    print res;
    
    print 'errors reported:',mqs.num_errors();
}





#--------------------------------------------
#
# Source flux fitting on raw data
#
#--------------------------------------------
phase_solution_with_given_fluxes := function()
{
    global inputrec, outputrec,solver_defaults,msname,mepuvw;
    global src_ra, src_dec, src_sti, src_names, outcol;
    global use_initcol

    use_initcol := F;       # initialize output column with zeroes

    msname := '3C343.MS';
    mep_table_name := '3C343.mep';
    
    # Clear MEP table
    meptable := table(mep_table_name, readonly=F);
    nrows := meptable.nrows();
    meptable.removerows(1:nrows);
    meptable.done();
    
    mepuvw := F;
    filluvw := any(argv=='-filluvw');
    solve_fluxes:= any(argv == '-fluxes');
    solve_gains := any(argv=='-gains');
    solve_phases := T;#any(argv=='-phases');
    set_breakpoint := any(argv=='-bp');
    
    src_ra  := ([4.356645791155902,4.3396003966265599]);
    src_dec := ([1.092208429052697,1.0953677174056471]);
    src_sti  := [5.35112656665,1.60887755917];
    src_names := "3C343_1 3C343";
    
    
# fill UVW parms from MS if requested
    if( mepuvw )
    {
        include 'meq/msuvw_to_mep.g'
            mepuvw := msname ~ s/.ms/.mep/;
        if( filluvw )
            fill_uvw(msname,mepuvw);
    }
    else
        mepuvw := F;
    
    outcol := 'PREDICTED_DATA';
    solver_defaults := [ num_iter=10,save_funklets=T,last_update=T ];
    
    inputrec := [ ms_name = msname,data_column_name = 'DATA',
                 tile_size=1,# clear_flags=T,
                 selection = [ channel_start_index=5,
                              channel_end_index=60, 
                              selection_string=''] ];
    
    outputrec := [ write_flags=F,predict_column=outcol ]; 
    
    res := do_test(msname=msname,solve=T,subtract=T,run=T,flag=F,
                   stset=1:14,
                   solve_fluxes=solve_fluxes,
                   solve_gains=solve_gains,
                   solve_phases=solve_phases,
                   set_breakpoint=set_breakpoint,
                   mep_table_name=mep_table_name,
                   publish=1,mepuvw=mepuvw,msuvw=msuvw);
    
    
    print res;
    
    print 'errors reported:',mqs.num_errors();
}



polar_test := function()
{
    mqsinit();
# add antenna gains/phases
    amp_node := meq.parm('GA',1.0,groups="a");
    amp_node.link_or_create:=T;
    
    phase_node :=meq.parm('GP',3.14159265358/2.0,groups="a");
    phase_node.link_or_create:=T;
    
    gain := meq.node('MeqPolar','G',[link_or_create=T],
                     children=meq.list(amp_node, phase_node) );

    mqs.createnode(gain);
    mqs.resolve('G');

    cells := meq.cells(meq.domain(0,1,0,1),1,1);
    request := meq.request(cells,rqid=meq.rqid(),eval_mode=F);
    res := mqs.meq('Node.Execute', [name='G',request=request], F);
}


visphaseshift_test := function()
{
    mqsinit();
    u1 := meq.node('MeqConstant', 'u1', [value=300, link_or_create=T]);
    v1 := meq.node('MeqConstant', 'v1', [value=-1000.0, link_or_create=T]);
    w1 := meq.node('MeqConstant', 'w1', [value=200.0, link_or_create=T]);

    uvw1 := meq.node('MeqComposer', 'uvw1', [link_or_create=T],
                     children=meq.list(u1,v1,w1));

    
    u2 := meq.node('MeqConstant', 'u2', [value=300+100.0, link_or_create=T]);
    v2 := meq.node('MeqConstant', 'v2', [value=-1000+500.0, link_or_create=T]);
    w2 := meq.node('MeqConstant', 'w2', [value=200+30.0, link_or_create=T]);

    uvw2 := meq.node('MeqComposer', 'uvw2', [link_or_create=T],
                     children=meq.list(u2,v2,w2));
    lval := -0.05;
    mval := +0.01;
    l := meq.node('MeqConstant', 'l', [value=lval, link_or_create=T]);
    m := meq.node('MeqConstant', 'm', [value=mval, link_or_create=T]);
    n :=  meq.node('MeqConstant', 'n', [value=sqrt(1.0-lval^2 -mval^2), link_or_create=T]);
    
    nminusone := meq.node('MeqConstant', 'nminusone', [value=(sqrt(1.0-lval^2 -mval^2)-1.0), link_or_create=T]);
         
    lmnminusone := meq.node('MeqComposer', 'lmnminusone', [link_or_create=T],
                        children=meq.list(l,m,nminusone));

    visphase1 := meq.node('MeqVisPhaseShift', 'visphase1', [link_or_create=T],
                 children=[lmn=lmnminusone, uvw=uvw1]);
    visphase2 := meq.node('MeqVisPhaseShift', 'visphase2', [link_or_create=T],
                 children=[lmn=lmnminusone, uvw=uvw2]);

    conj2 := meq.node('MeqConj', 'conj2', [link_or_create=T],
             children=meq.list(visphase2));

    vis := meq.node('MeqMultiply', 'vis', [link_or_create=T],
           children=meq.list(visphase1, conj2));
    mqs.createnode(vis);
    mqs.resolve('vis');
    cells := meq.cells(meq.domain(1e+9,1.2e+9,0,1),20,10);
    request := meq.request(cells,rqid=meq.rqid(),eval_mode=F);
    res := mqs.meq('Node.Execute', [name='vis',request=request], F);
}

#source_flux_fit_no_calibration();
phase_solution_with_given_fluxes();
#polar_test();
#visphaseshift_test();

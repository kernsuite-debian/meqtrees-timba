include 'imager.g';

pixels := 1440;
pixel_size := '4.0arcsec';

nchan := 16;#6;
start := 26;#30;
step  := 2;


imgr := imager('3C343.MS');
imgr.setdata(mode='channel',nchan=nchan,start=start, step=step,spwid=[1]);
imgr.setimage(nx=pixels, ny=pixels, cellx=pixel_size, celly=pixel_size,stokes='IQUV',mode='channel',nchan=nchan,start=start,spwid=[1]);

imgr.weight('radial');
imgr.setoptions(cache=100000000, tile=32, padding=1.5);
imgr.makeimage(type='corrected',image='343subtracted16.img');
imgr.done();
#imgr.clean(algorithm='clark', niter=1000,threshold='1mJy', displayprogress=F, model="343model.img",image='343clean.img');


#imgr := imager('3C343.MS');
#imgr.setdata(mode='channel',nchan=nchan,start=start, step=step);
#imgr.setimage(nx=pixels, ny=pixels, cellx=pixel_size, celly=pixel_size,stokes='I',mode='mfs',nchan=1,start=start,step=step);
#
#imgr.weight('radial');
#imgr.setoptions(padding=1.5);
#imgr.clean(algorithm='clark', niter=1000,threshold='1mJy', displayprogress=F, model="343mfsmodel.img",image='343mfsclean.img');
#
#imgr.done();
exit;

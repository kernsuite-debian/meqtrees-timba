#include <ms/MeasurementSets/MeasurementSet.h>
#include <tables/Tables/ArrayColumn.h>
#include <tables/Tables/ScalarColumn.h>
#include <tables/Tables/ArrColDesc.h>
#include <tables/Tables/ScaColDesc.h>
#include <tables/Tables/TiledStManAccessor.h>
#include <tables/Tables/TiledColumnStMan.h>
#include <tables/Tables/IncrementalStMan.h>

#include <iostream>
using namespace std;
using namespace casa;

int main (int argc,char *argv[])
{
  if( argc != 10 )
  {
    cerr<<argv[0]<<": adds a tiled Ncorr,Nfreq column (with tileshape Tcorr,Tfreq,Trow) to an MS\n";
    cerr<<"Usage: "<<argv[0]<<" MS column_name (bool|complex) Ncorr Nfreq Tcorr Tfreq Trow option_flag\n";
    return 1;
  }
  char *msname = argv[1];
  std::string colname(argv[2]);
  std::string type(argv[3]);
  int ncorr = atoi(argv[4]);
  int nfreq = atoi(argv[5]);
  int ts1 = atoi(argv[6]);
  int ts2 = atoi(argv[7]);
  int ts3 = atoi(argv[8]);
  int option = atoi(argv[9]);
 
  IPosition colshape(2,ncorr,nfreq);
  IPosition tileshape(3,ts1,ts2,ts3);
  
  try
  {
    Table ms(msname,TableLock(TableLock::AutoNoReadLocking),Table::Update);
    if( type == "complex" )
    {
      ArrayColumnDesc<Complex> coldesc(colname,
                              "addtiledmscol utility",
                              colshape,
                              option);
      TiledColumnStMan stman("Tiled_"+colname,tileshape);
      ms.addColumn(coldesc,stman);
    }
    else if( type == "bool" )
    {
      ArrayColumnDesc<Bool> coldesc(colname,
                              "addtiledmscol utility",
                              colshape,
                              option);
      TiledColumnStMan stman("Tiled_"+colname,tileshape);
      ms.addColumn(coldesc,stman);
    }
    else if( type == "int" )
    {
      ArrayColumnDesc<Int> coldesc(colname,
                              "addtiledmscol utility",
                              colshape,
                              option);
      TiledColumnStMan stman("Tiled_"+colname,tileshape);
      ms.addColumn(coldesc,stman);
    }
    else
    {
      cerr<<"Unknown column type: "<<type<<endl;
      return 1;
    }
    ms.flush();
    return 0;
  }
  catch( std::exception &exc )
  {
    cerr<<"Caught exception: "<<exc.what()<<endl;
    return 1;
  }
  catch( ... )
  {
    cerr<<"Caught unknown exception, exiting.\n";
    return 1;
  }
}
        
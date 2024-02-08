/**C++File**************************************************************

  FileName    [ac_wrapper.cpp]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Ashenhurst-Curtis decomposition.]

  Synopsis    [Interface with the FPGA mapping package.]

  Author      [Alessandro Tempia Calvino]
  
  Affiliation [EPFL]

  Date        [Ver. 1.0. Started - November 20, 2023.]

***********************************************************************/

#include "ac_wrapper.h"
#include "ac_decomposition.hpp"
#include "acd66.hpp"

ABC_NAMESPACE_IMPL_START

int acd_evaluate( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned *cost, int try_no_late_arrival )
{
  using namespace acd;

  ac_decomposition_params ps;
  ps.lut_size = lutSize;
  ps.try_no_late_arrival = static_cast<bool>( try_no_late_arrival ); /* TODO: additional tests */
  ac_decomposition_stats st;

  ac_decomposition_impl acd( nVars, ps, &st );
  int val = acd.run( pTruth, *pdelay );

  if ( val < 0 )
  {
    *pdelay = 0;
    return -1;
  }

  *pdelay = acd.get_profile();
  *cost = st.num_luts;

  return val;
}

int acd_decompose( word * pTruth, unsigned nVars, int lutSize, unsigned *pdelay, unsigned char *decomposition )
{
  using namespace acd;

  ac_decomposition_params ps;
  ps.lut_size = lutSize;
  ac_decomposition_stats st;

  ac_decomposition_impl acd( nVars, ps, &st );
  acd.run( pTruth, *pdelay );
  int val = acd.compute_decomposition();

  if ( val < 0 )
  {
    *pdelay = 0;
    return -1;
  }

  *pdelay = acd.get_profile();

  acd.get_decomposition( decomposition );
  return 0;
}

int acd66_evaluate( word * pTruth, unsigned nVars, int compute_decomposition )
{
  using namespace acd;

  acd66_params ps;
  ps.verify = false;
  acd66_impl acd( nVars, ps );

  if ( acd.run( pTruth ) == 0 )
    return 0;
  
  if ( !compute_decomposition )
    return 1;

  int val = acd.compute_decomposition();
  if ( val != 0 )
  {
    return 0;
  }

  return 1;
}

int acd66_decompose( word * pTruth, unsigned nVars, unsigned char *decomposition )
{
  using namespace acd;

  acd66_params ps;
  acd66_impl acd( nVars, ps );
  acd.run( pTruth );

  int val = acd.compute_decomposition();
  if ( val != 0 )
  {
    return -1;
  }

  acd.get_decomposition( decomposition );
  return 0;
}

ABC_NAMESPACE_IMPL_END

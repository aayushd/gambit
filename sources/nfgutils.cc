//
// FILE nfgutils.cc -- useful utilities for the normal form
//
// $Id$
//

#include "gmisc.h"
#include "nfg.h"

// prototype in nfg.h

template <class T> void Random(Nfg<T> &nfg, int seed = 0)
{
  SetSeed(seed);
  for (int i = 1; i <= nfg.NumPlayers(); i++)
    for (int j = 0; j < nfg.NumPayPerPlayer; j++)
      (nfg.payoffs)[i][j] = (T) Uniform();
}

#ifdef __GNUG__
#define TEMPLATE template
#elif defined __BORLANDC__
#define TEMPLATE
#pragma option -Jgd
#endif   // __GNUG__, __BORLANDC__

#include "rational.h"

TEMPLATE void Random(Nfg<double> &nfg, int seed = 0);
TEMPLATE void Random(Nfg<gRational> &nfg, int seed = 0);

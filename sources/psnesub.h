//
// FILE: psnesub.h -- Solve efg by subgames in pure nfg strategies
//
// $Id$
//

#ifndef PSNESUB_H
#define PSNESUB_H

#include "subsolve.h"
#include "nfgpure.h"

template <class T> class PureNashBySubgame : public SubgameSolver<T>  {
  private:
    int SolveSubgame(const Efg &, const EFSupport &,
		     gList<BehavSolution> &);
    EfgAlgType AlgorithmID() const { return EfgAlg_PURENASHSUB; }    

  public:
    PureNashBySubgame(const Efg &E, const EFSupport &, int max = 0);
    virtual ~PureNashBySubgame();
};

#endif   // PSNESUB_H


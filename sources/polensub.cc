//
// FILE: polensub.cc -- Implementation of PolEnumBySubgame
//
// $Id$ 
//

#include "polensub.h"

int PolEnumBySubgame::SolveSubgame(const Efg &E, const EFSupport &sup,
				   gList<BehavSolution> &solns)
{
  Nfg *N = MakeReducedNfg(sup);

  NFSupport *S = new NFSupport(*N);
  
  ViewNormal(*N, S);
  
  gList<MixedSolution> solutions;

  long neval;
  double time;

  PolEnum(*S, params, solutions, neval, time);

  nevals += neval;
  
  for (int i = 1; i <= solutions.Length(); i++)  {
    BehavProfile<gNumber> bp(sup);
    MixedToBehav(*N, MixedProfile<gNumber>(solutions[i]), E, bp);
    solns.Append(bp);
  }

  delete S;
  delete N;

  return 0;
}

PolEnumBySubgame::PolEnumBySubgame(const EFSupport &S,
				   const PolEnumParams &p, int max)
  : SubgameSolver(max), nevals(0), params(p)
{ }

PolEnumBySubgame::~PolEnumBySubgame()   { }

int PolEnum(const EFSupport &support, const PolEnumParams &params,
	    gList<BehavSolution> &solutions, long &nevals, double &time)
{
  PolEnumBySubgame module(support, params);
  module.Solve(support);
  nevals = module.NumEvals();
  time = module.Time();
  solutions = module.GetSolutions();
  return 1;
}


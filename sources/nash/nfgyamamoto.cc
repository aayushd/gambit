//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Yamamoto's algorithm for computing one proper equilibrium
//
// This file is part of Gambit
// Copyright (c) 2002, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#include "nfgyamamoto.h"
#include "base/gstatus.h"
#include "math/gsmatrix.h"

class gbtNfgNashYamamoto {
public:
  gbtNfgNashYamamoto(void) { }
  virtual ~gbtNfgNashYamamoto() { }

  std::string GetAlgorithm(void) const { return "Yamamoto"; }
  gbtList<gbtMixedProfile<double> > Solve(const gbtGame &, gbtStatus &);
};

static int NumMembers(const gbtMatrix<int> &p_partition, int p_index)
{
  int count = 0;
  
  for (int col = 1; col <= p_partition.NumColumns(); col++) {
    if (p_partition(p_index, col) > 0) {
      count++;
    }
  }

  return count;
}

static int FirstMember(const gbtMatrix<int> &p_partition, int p_index)
{
  for (int col = 1; col <= p_partition.NumColumns(); col++) {
    if (p_partition(p_index, col) > 0) {
      return col;
    }
  }

  // shouldn't happen!
  return 0;
}

static double Payoff(const gbtMixedProfile<double> &p_profile, int p_player,
		     const gbtMatrix<int> &p_partition, int p_index)
{
  for (int st = 1; st <= p_profile->GetGame()->GetPlayer(p_player)->NumStrategies(); st++) {
    if (p_partition(p_index, st) > 0) {
      return p_profile->GetStrategyValue(p_profile->GetGame()->GetPlayer(p_player)->GetStrategy(st));
    }
  }
  
  // shouldn't happen!
  return 0.0;
}


static gbtMatrix<int> RankStrategies(const gbtMixedProfile<double> &p_profile,
				   int p_player)
{
  gbtVector<double> payoffs(p_profile->GetGame()->GetPlayer(p_player)->NumStrategies());
  gbtArray<int> strategies(p_profile->GetGame()->GetPlayer(p_player)->NumStrategies());
  for (int st = 1; st <= payoffs.Length(); st++) {
    p_profile->GetStrategyValue(p_profile->GetGame()->GetPlayer(p_player)->GetStrategy(st));
  }

  for (int i = 1; i <= strategies.Length(); i++) {
    strategies[i] = i;
  }

  // bubble sort; crude but effective
  bool changed;

  do { 
    changed = false;
    
    for (int i = 1; i <= strategies.Length() - 1; i++) {
      if (payoffs[i] < payoffs[i+1]) {
	double tmp1 = payoffs[i];
	payoffs[i] = payoffs[i+1];
	payoffs[i+1] = tmp1;

	int tmp2 = strategies[i];
	strategies[i] = strategies[i+1];
	strategies[i+1] = tmp2;

	changed = true;
      }
    }
  } while (changed);

  gbtMatrix<int> partition(strategies.Length(), strategies.Length());
  for (int row = 1; row <= partition.NumRows(); row++) {
    for (int col = 1; col <= partition.NumColumns(); col++) {
      partition(row, col) = 0;
    }
  }

  // FIXME: This assumes that no strategies have the same payoff
  for (int row = 1; row <= strategies.Length(); row++) {
    partition(row, strategies[row]) = 1;
  } 

  return partition;
}

static void YamamotoJacobian(const gbtMixedProfile<double> &p_profile,
			     double p_lambda,
			     const gbtList<gbtMatrix<int> > &p_partition,
			     gbtMatrix<double> &p_matrix)
{
  int rowno = 0;
  p_matrix = 0.0;

  for (int pl = 1; pl <= p_profile->GetGame()->NumPlayers(); pl++) {
    int strats = 0;
    for (int part = 1; part <= p_partition[pl].NumRows(); part++) {
      if (NumMembers(p_partition[pl], part) > 0) {
	// First, the probability equation
	rowno++;

	int colno = 0;
	for (int pl2 = 1; pl2 <= p_profile->GetGame()->NumPlayers(); pl2++) {
	  for (int st2 = 1; st2 <= p_profile->GetGame()->GetPlayer(pl2)->NumStrategies(); st2++) {
	    colno++;
	    if (pl != pl2) {
	      continue;
	    }

	    if (p_partition[pl](part, st2) > 0) {
	    // strategy number st2 is in this partition
	      for (int i = 1; i <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); i++) {
		p_matrix(rowno, colno) += pow(p_lambda, (double) (i-1));
	      }
	    }
	  }
	}

	// The final column is the derivative wrt lambda
	colno++;
	double totalprob = 0.0;
	for (int st = 1; st <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); st++) {
	  if (p_partition[pl](part, st) > 0) {
	    totalprob += p_profile(pl, st);
	  }
	}

	for (int i = 1; i <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); i++) {
	  p_matrix(rowno, colno) += ((double) (i - 1)) * pow(p_lambda, (double) (i-2)) * totalprob;
	}

	for (int st = strats + 1; st <= strats + NumMembers(p_partition[pl], part); st++) {
	  p_matrix(rowno, colno) -= ((double) st - 1) * pow(p_lambda, (double) (st - 2));
	}
      }

      if (NumMembers(p_partition[pl], part) > 1) {
	// We need to have #members - 1 equations
	int st1 = FirstMember(p_partition[pl], part);

	for (int st = st1 + 1; st <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); st++) {
	  if (p_partition[pl](part, st) > 0) {
	    rowno++;

	    int colno = 0;
	    for (int pl2 = 1; pl2 <= p_profile->GetGame()->NumPlayers(); pl2++) {
	      for (int st2 = 1; st2 <= p_profile->GetGame()->GetPlayer(pl2)->NumStrategies(); st2++) {
		colno++;
		if (pl == pl2) {
		  continue;
		}

		p_matrix(rowno, colno) = p_profile->GetPayoffDeriv(p_profile->GetGame()->GetPlayer(pl), p_profile->GetGame()->GetPlayer(pl)->GetStrategy(st1), p_profile->GetGame()->GetPlayer(pl2)->GetStrategy(st2)) - p_profile->GetPayoffDeriv(p_profile->GetGame()->GetPlayer(pl), p_profile->GetGame()->GetPlayer(pl)->GetStrategy(st), p_profile->GetGame()->GetPlayer(pl2)->GetStrategy(st2));
	      }
	    }
	  }
	}
      }
      strats += NumMembers(p_partition[pl], part);
    }
  }
}
		      
static void YamamotoComputeStep(const gbtMixedProfile<double> &p_profile,
				const gbtMatrix<double> &p_matrix,
				gbtPVector<double> &p_delta, double &p_lambdainc,
				double p_initialsign, double p_stepsize)
{
  double sign = p_initialsign;
  int rowno = 0; 

  gbtSquareMatrix<double> M(p_matrix.NumRows());

  for (int row = 1; row <= M.NumRows(); row++) {
    for (int col = 1; col <= M.NumColumns(); col++) {
      M(row, col) = p_matrix(row, col + 1);
    }
  }

  for (int pl = 1; pl <= p_profile->GetGame()->NumPlayers(); pl++) {
    for (int st = 1; st <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); st++) {
      rowno++;
      p_delta(pl, st) = sign * M.Determinant();   
      sign *= -1.0;

      for (int row = 1; row <= M.NumRows(); row++) {
	M(row, rowno) = p_matrix(row, rowno);
	if (rowno < M.NumColumns()) {
	  M(row, rowno + 1) = p_matrix(row, rowno + 2);
	}
      }
    }
  }   

  p_lambdainc = sign * M.Determinant();

  double norm = 0.0;
  for (int pl = 1; pl <= p_profile->GetGame()->NumPlayers(); pl++) {
    for (int st = 1; st <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); st++) {
      norm += p_delta(pl, st) * p_delta(pl, st);
    }
  }
  norm += p_lambdainc * p_lambdainc; 
  
  for (int pl = 1; pl <= p_profile->GetGame()->NumPlayers(); pl++) {
    for (int st = 1; st <= p_profile->GetGame()->GetPlayer(pl)->NumStrategies(); st++) {
      p_delta(pl, st) /= sqrt(norm / p_stepsize);
    }
  }

  p_lambdainc /= sqrt(norm / p_stepsize);
}

static gbtList<int> SortProbs(const gbtMixedProfile<double> &p_profile,
			    int p_player,
			    const gbtMatrix<int> &p_partition,
			    int p_index)
{
  gbtList<int> strategies;
  gbtList<double> probs;

  for (int st = 1; st <= p_partition.NumColumns(); st++) {
    if (p_partition(p_index, st) > 0) {
      strategies.Append(st);
      probs.Append(p_profile(p_player, st));
    }
  }

  bool changed;

  do { 
    changed = false;
    
    for (int i = 1; i <= strategies.Length() - 1; i++) {
      if (probs[i] < probs[i+1]) {
	double tmp1 = probs[i];
	probs[i] = probs[i+1];
	probs[i+1] = tmp1;

	int tmp2 = strategies[i];
	strategies[i] = strategies[i+1];
	strategies[i+1] = tmp2;

	changed = true;
      }
    }
  } while (changed);

  return strategies;
}

static double PDenom(double p_lambda, int p_m)
{
  double total = 0.0;

  for (int i = 1; i <= p_m; i++) {
    total += pow(p_lambda, (double) (i - 1));
  }

  return total;
}

gbtList<gbtMixedProfile<double> >
gbtNfgNashYamamoto::Solve(const gbtGame &p_game, gbtStatus &p_status)
{
  // In the notation of Yamamoto's paper, profile(i,j)=x_{ij}
  // and lambda=t
  gbtMixedProfile<double> profile = p_game->NewMixedProfile(0.0);
  double lambda = 1.0;
  double initialsign = -1.0; 
  gbtList<gbtMatrix<int> > partitions;
  gbtMatrix<double> H(p_game->StrategyProfileLength(),
		      p_game->StrategyProfileLength() + 1);

  for (int pl = 1; pl <= p_game->NumPlayers(); pl++) {
    partitions.Append(RankStrategies(profile, pl));
  }

  gbtList<gbtMixedProfile<double> > solutions;

  for (int step = 1; step <= 50000 && lambda > 0.01; step++) { 
    if (step % 25 == 0) {
      p_status.Get();
      p_status.SetProgress(1.0 - lambda,
			   std::string("Lambda = ") + ToText(lambda));
    }
    YamamotoJacobian(profile, lambda, partitions, H);
      
    gbtPVector<double> delta(profile->GetGame()->NumStrategies());
    for (int i = 1; i <= delta.Length(); i++) {
      delta[i] = profile[i];
    }
    double lambdainc;

    YamamotoComputeStep(profile, H, delta, lambdainc, initialsign, .000001);

    ((gbtPVector<double> &) profile) += delta;
    lambda += lambdainc;

    //    gout << lambda << ' ' << profile << '\n';
    // Check for inequalities
    for (int pl = 1; pl <= p_game->NumPlayers(); pl++) {
      int strats = 0;
      for (int part = 1; part < p_game->GetPlayer(pl)->NumStrategies(); part++) {
	if (NumMembers(partitions[pl], part) > 0 &&
	    NumMembers(partitions[pl], part + 1) > 0) {
	  if (Payoff(profile, pl, partitions[pl], part) <=
	      Payoff(profile, pl, partitions[pl], part + 1)) {
	    // Combine partitions part and part+1
	    partitions[pl].SetRow(part, partitions[pl].Row(part) + partitions[pl].Row(part+1));
	    for (int p = part + 1; p < p_game->GetPlayer(pl)->NumStrategies(); p++) {
	      partitions[pl].SetRow(p, partitions[pl].Row(p+1));
	    }  
	    for (int col = 1; col <= partitions[pl].NumColumns(); col++) {
	      partitions[pl](partitions[pl].NumRows(), col) = 0;
	    }
	    // Redo this partition, in case multiple partitions 
	    // coalesce (non-generic, but conceivable)
	    // part--;
	    //    gout << "Relax!\n";
	    // Working hypothesis: need to flip the sign at every relaxation
	    initialsign *= -1.0;
	  }
	  else if (NumMembers(partitions[pl], part) > 1) {
	    // check for possible splitting of partition
	    gbtList<int> sortstrats = SortProbs(profile, pl, partitions[pl], 
					      part);
	    double totX = 0.0, totP = 0.0;
	    for (int i = 1; i < sortstrats.Length(); i++) {
	      totP += pow(lambda, (double) (strats + i - 1)) / PDenom(lambda, p_game->GetPlayer(pl)->NumStrategies());
	      totX += profile(pl, sortstrats[i]);
	      if (totX >= totP) {
		//		gout << pl << " " << part << " " << totP << " " << totX << "Xaler!\n";
		for (int p = part + 1; p < p_game->GetPlayer(pl)->NumStrategies(); p++) {
		  partitions[pl].SetRow(p+1, partitions[pl].Row(p));
		}  
		for (int col = 1; col <= partitions[pl].NumColumns(); col++) {
		  partitions[pl](part, col) = 0;
		  partitions[pl](part + 1, col) = 0;
		}
		for (int st = 1; st <= i; st++) {
		  partitions[pl](part, st) = 1;
		}
		for (int st = i + 1; st <= sortstrats.Length(); st++) {
		  partitions[pl](part + 1, st) = 1;
		}
		//		initialsign *= -1.0;
		break;
	      }
	    }
	  }
	}
	strats += NumMembers(partitions[pl], part);
      }
    } 
  }

  solutions.Append(profile);

  return solutions;
}

gbtList<gbtMixedProfile<double> > gbtNashYamamotoNfg(const gbtGame &p_game)
{
  gbtNfgNashYamamoto algorithm;
  gbtNullStatus status;
  return algorithm.Solve(p_game, status);
}


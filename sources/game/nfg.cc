//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Implementation of normal form game classes
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

#include "math/rational.h"
#include "nfg.h"
#include "nfstrat.h"
#include "nfplayer.h"

//----------------------------------------------------------------------
//                gbt_nfg_outcome_rep: Declaration
//----------------------------------------------------------------------

struct gbt_nfg_outcome_rep {
  int m_id;
  Nfg *m_nfg;
  bool m_deleted;
  gText m_label;
  gBlock<gNumber> m_payoffs;
  gBlock<double> m_doublePayoffs;
  int m_refCount;

  gbt_nfg_outcome_rep(Nfg *, int);
};

gbt_nfg_outcome_rep::gbt_nfg_outcome_rep(Nfg *p_nfg, int p_id)
  : m_id(p_id), m_nfg(p_nfg), m_deleted(false), 
    m_payoffs(p_nfg->NumPlayers()), m_doublePayoffs(p_nfg->NumPlayers()),
    m_refCount(1)
{
  for (int i = 1; i <= m_payoffs.Length(); i++) {
    m_payoffs[i] = 0;
    m_doublePayoffs[i] = 0.0;
  }
}

gbtNfgOutcome::gbtNfgOutcome(void)
  : rep(0)
{ }

gbtNfgOutcome::gbtNfgOutcome(gbt_nfg_outcome_rep *p_rep)
  : rep(p_rep)
{
  if (rep) {
    rep->m_refCount++;
  }
}

gbtNfgOutcome::gbtNfgOutcome(const gbtNfgOutcome &p_outcome)
  : rep(p_outcome.rep)
{
  if (rep) {
    rep->m_refCount++;
  }
}

gbtNfgOutcome::~gbtNfgOutcome()
{
  if (rep) {
    if (--rep->m_refCount == 0) {
      delete rep;
    }
  }
}

gbtNfgOutcome &gbtNfgOutcome::operator=(const gbtNfgOutcome &p_outcome)
{
  if (this == &p_outcome) {
    return *this;
  }

  if (rep && --rep->m_refCount == 0) {
    delete rep;
  }

  if ((rep = p_outcome.rep) != 0) {
    rep->m_refCount++;
  }
  return *this;
}

bool gbtNfgOutcome::operator==(const gbtNfgOutcome &p_outcome) const
{
  return (rep == p_outcome.rep);
} 

bool gbtNfgOutcome::operator!=(const gbtNfgOutcome &p_outcome) const
{
  return (rep != p_outcome.rep);
} 

int gbtNfgOutcome::GetId(void) const
{
  return (rep) ? rep->m_id : 0;
}

bool gbtNfgOutcome::IsNull(void) const
{
  return (rep == 0);
}

Nfg *gbtNfgOutcome::GetGame(void) const
{
  return (rep) ? rep->m_nfg : 0;
}

gText gbtNfgOutcome::GetLabel(void) const
{
  if (rep) {
    return rep->m_label;
  }
  else {
    return "";
  }
}

gOutput &operator<<(gOutput &p_stream, const gbtNfgOutcome &)
{ 
  return p_stream;
}

//--------------------------------------
// Strategy:  Constructors, Destructors
//--------------------------------------

Strategy::Strategy(NFPlayer *p) : m_number(0), m_player(p), m_index(0L)
{ }

Strategy::Strategy(const Strategy &s) : m_player(s.m_player), m_name(s.m_name)
{ }

Strategy &Strategy::operator=(const Strategy &s)
{
  m_player = s.m_player;
  m_name = s.m_name;
  return *this;
}

Strategy::~Strategy()
{ }


//----------------------------------------------------
// Nfg: Constructors, Destructors, Operators
//----------------------------------------------------


int Nfg::Product(const gArray<int> &dim)
{
  int accum = 1;
  for (int i = 1; i <= dim.Length(); accum *= dim[i++]);
  return accum;
}
  
Nfg::Nfg(const gArray<int> &dim)
  : m_dirty(false), m_revision(0),  m_outcome_revision(-1), 
    title("UNTITLED"), dimensions(dim), players(dim.Length()),
    results(Product(dim)), efg(0)
{
  for (int pl = 1; pl <= players.Length(); pl++)  {
    players[pl] = new NFPlayer(pl, this, dim[pl]);
	  players[pl]->name = ToText(pl);
    for (int st = 1; st <= players[pl]->NumStrats(); st++)
      players[pl]->strategies[st]->m_name = ToText(st);
  }
  IndexStrategies();

  for (int cont = 1; cont <= results.Length();
       results[cont++] = (gbt_nfg_outcome_rep *) 0);
}

Nfg::Nfg(const Nfg &b)
  : m_dirty(false), m_revision(0),  m_outcome_revision(-1), 
    title(b.title), comment(b.comment), 
    dimensions(b.dimensions),
    players(b.players.Length()), outcomes(b.outcomes.Length()),
    results(b.results.Length()), efg(0)
{
  for (int pl = 1; pl <= players.Length(); pl++)  {
    players[pl] = new NFPlayer(pl, this, dimensions[pl]);
    players[pl]->name = b.players[pl]->name;
    for (int st = 1; st <= players[pl]->NumStrats(); st++)  {
      *(players[pl]->strategies[st]) = *(b.players[pl]->strategies[st]);
      players[pl]->strategies[st]->m_player = players[pl];
    }
  }
  IndexStrategies();
  
  for (int outc = 1; outc <= outcomes.Length(); outc++)  {
    outcomes[outc] = new gbt_nfg_outcome_rep(this, outc);
    outcomes[outc]->m_label = b.outcomes[outc]->m_label;
    outcomes[outc]->m_payoffs = b.outcomes[outc]->m_payoffs;
    outcomes[outc]->m_doublePayoffs = b.outcomes[outc]->m_doublePayoffs;
  }

  for (int cont = 1; cont <= results.Length(); cont++)    
    results[cont] = (b.results[cont]) ?
                     outcomes[b.results[cont]->m_id] : (gbt_nfg_outcome_rep *) 0;
}

#include "efg.h"
#include "lexicon.h"

Nfg::~Nfg()
{
  for (int pl = 1; pl <= players.Length(); pl++)
    delete players[pl];
  for (int outc = 1; outc <= outcomes.Length(); outc++)
    delete outcomes[outc];

  if (efg)  {
    const efgGame *tmp = (efgGame *) efg;
    // note that Lexicon dtor unsets the efg member...

    delete ((efgGame *) efg)->lexicon;
    tmp->lexicon = 0;
  }
  efg = 0;
}

void Nfg::BreakLink(void)
{
  if (efg)  {
    const efgGame *tmp = (efgGame *) efg;
    // note that Lexicon dtor unsets the efg member...

    delete ((efgGame *) efg)->lexicon;
    tmp->lexicon = 0;
  }
  efg = 0;
}

//-------------------------------
// Nfg: Member Functions
//-------------------------------

#include "nfgiter.h"

void Nfg::WriteNfgFile(gOutput &p_file, int p_nDecimals) const
{ 
  int oldDecimals = p_file.GetPrec();
  p_file.SetPrec(p_nDecimals);

  try {
    p_file << "NFG 1 R";
    p_file << " \"" << EscapeQuotes(GetTitle()) << "\" { ";

    for (int i = 1; i <= NumPlayers(); i++)
      p_file << '"' << EscapeQuotes(Players()[i]->GetName()) << "\" ";

    p_file << "}\n\n{ ";
  
    for (int i = 1; i <= NumPlayers(); i++)   {
      NFPlayer *player = Players()[i];
      p_file << "{ ";
      for (int j = 1; j <= player->NumStrats(); j++)
	p_file << '"' << EscapeQuotes(player->Strategies()[j]->Name()) << "\" ";
      p_file << "}\n";
    }
  
    p_file << "}\n";

    p_file << "\"" << EscapeQuotes(comment) << "\"\n\n";

    int ncont = 1;
    for (int i = 1; i <= NumPlayers(); i++)
      ncont *= NumStrats(i);

    p_file << "{\n";
    for (int outc = 1; outc <= outcomes.Length(); outc++)   {
      p_file << "{ \"" << EscapeQuotes(outcomes[outc]->m_label) << "\" ";
      for (int pl = 1; pl <= players.Length(); pl++)  {
	p_file << outcomes[outc]->m_payoffs[pl];
	if (pl < players.Length()) {
	  p_file << ", ";
	}
	else {
	  p_file << " }\n";
	}
      }
    }
    p_file << "}\n";
  
    for (int cont = 1; cont <= ncont; cont++)  {
      if (results[cont] != 0)
	p_file << results[cont]->m_id << ' ';
      else
	p_file << "0 ";
    }

    p_file << '\n';
    p_file.SetPrec(oldDecimals);
    m_dirty = false;
  }
  catch (...) {
    p_file.SetPrec(oldDecimals);
    throw;
  }
}

gbtNfgOutcome Nfg::NewOutcome(void)
{
  m_dirty = true;
  m_revision++;
  gbt_nfg_outcome_rep *outcome = new gbt_nfg_outcome_rep(this, 
							 outcomes.Length()+1);
  outcomes.Append(outcome);
  return outcome;
}

void Nfg::DeleteOutcome(gbtNfgOutcome p_outcome)
{
  m_dirty = true;
  m_revision++;

  if (p_outcome.rep) {
    for (int i = 1; i <= results.Length(); i++) {
      if (results[i] == p_outcome.rep)
	results[i] = 0;
    }

    delete outcomes.Remove(p_outcome.rep->m_id);

    for (int outc = 1; outc <= outcomes.Length(); outc++) {
      outcomes[outc]->m_id = outc;
    }
  }
}

const gArray<Strategy *> &Nfg::Strategies(int p) const
{
  return (players[p]->Strategies());
}

void Nfg::SetTitle(const gText &s) 
{
  title = s; 
  m_dirty = true;
  m_revision++;
}

const gText &Nfg::GetTitle(void) const 
{ return title; }

void Nfg::SetComment(const gText &s)
{
  comment = s; 
  m_dirty = true;
  m_revision++;
}

const gText &Nfg::GetComment(void) const
{ return comment; }


int Nfg::NumPlayers(void) const 
{ 
  return (players.Length()); 
}

const gArray<NFPlayer *> &Nfg::Players(void) const
{
  return players;
}

int Nfg::NumStrats(int pl) const
{
  return ((pl > 0 && pl <= NumPlayers()) ? 
	  (players[pl])->strategies.Length() : 0);
}

int Nfg::ProfileLength(void) const
{
  int nprof = 0;
  for (int i = 1; i <= players.Length(); i++)
    nprof += players[i]->strategies.Length();
  return nprof;
}

gbtNfgOutcome Nfg::GetOutcomeId(int p_id) const
{
  return outcomes[p_id];
}

void Nfg::SetLabel(gbtNfgOutcome p_outcome, const gText &p_label)
{
  if (p_outcome.rep) { 
    p_outcome.rep->m_label = p_label;
  }
}
void Nfg::SetOutcome(const gArray<int> &p_profile,
		     const gbtNfgOutcome &p_outcome)
{
  int index = 1;
  for (int i = 1; i <= p_profile.Length(); i++) {
    index += players[i]->strategies[p_profile[i]]->m_index;
  }
  results[index] = p_outcome.rep;
  m_dirty = true;
  m_revision++;
  BreakLink();
}


void Nfg::SetOutcome(const StrategyProfile &p, const gbtNfgOutcome &outcome)
{
  results[p.index + 1] = outcome.rep;
  m_dirty = true;
  m_revision++;
  BreakLink();
}

void Nfg::SetOutcomeIndex(int p_index, const gbtNfgOutcome &p_outcome)
{
  results[p_index] = p_outcome.rep;
}

gbtNfgOutcome Nfg::GetOutcome(const gArray<int> &profile) const 
{
  int index = 1;
  for (int i = 1; i <= profile.Length(); i++) {
    index += players[i]->strategies[profile[i]]->m_index;
  }
  return results[index];
}

gbtNfgOutcome Nfg::GetOutcome(const StrategyProfile &p) const
{
  return results[p.index + 1];
}

void Nfg::SetPayoff(gbtNfgOutcome outcome, int pl, const gNumber &value)
{
  if (outcome.rep) {
    outcome.rep->m_payoffs[pl] = value;
    outcome.rep->m_doublePayoffs[pl] = (double) value;
    m_dirty = true;
    m_revision++;
  }
}

gNumber Nfg::Payoff(gbtNfgOutcome outcome, int pl) const
{
  if (outcome.rep) {
    return outcome.rep->m_payoffs[pl];
  }
  else {
    return 0;
  }
}

// ---------------------------------------
// Nfg: Private member functions
// ---------------------------------------

void Nfg::IndexStrategies(void)
{
  long offset = 1L;

  for (int i = 1; i <= NumPlayers(); i++)  {
    int j;
    for (j = 1; j <= NumStrats(i); j++)  {
      Strategy *s = (players[i])->strategies[j];
      s->m_number = j;
      s->m_index = (j - 1) * offset;
    }
    offset *= (j - 1);
  }
}

// --------------------------
// NFPlayer: Member functions 
// --------------------------

NFPlayer::NFPlayer(int n, Nfg *N, int num)
: number(n), N(N), strategies(num)
{ 
  for (int j = 1; j <= num; j++)
    strategies[j] = new Strategy(this);
}

NFPlayer::~NFPlayer()
{ 
  for (int j = 1; j <= strategies.Length(); j++)
    delete strategies[j];
}

Nfg &NFPlayer::Game(void) const
{
  return *N;
}

const gText &NFPlayer::GetName(void) const
{
  return name;
}

void NFPlayer::SetName(const gText &s)
{
  name = s;
}

int NFPlayer::NumStrats(void) const 
{
  return strategies.Length();
}


const gArray<Strategy *> &NFPlayer::Strategies(void) const
{
  return strategies;
}

void Nfg::InitPayoffs(void) const 
{
  if (m_outcome_revision == RevisionNumber()) {
    return;
  }

  for (int outc = 1; outc <= NumOutcomes(); outc++) {
    for (int pl = 1; pl <= NumPlayers(); pl++) {
      outcomes[outc]->m_doublePayoffs[pl] = outcomes[outc]->m_payoffs[pl];
    }
  }

  m_outcome_revision = RevisionNumber();
}


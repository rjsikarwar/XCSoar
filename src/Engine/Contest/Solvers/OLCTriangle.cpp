/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "OLCTriangle.hpp"
#include "Cast.hpp"
#include "Trace/Trace.hpp"

/*
 @todo potential to use 3d convex hull to speed search

 2nd point inclusion rules:
   if min leg length is 25%, max is 45%
   pmin = 0.25
   pmax = 0.45
   ptot = 2*pmin+pmax+0.05
   with 2 legs:
      ptot = pmin+pmax


  0: start
  1: first leg start
  2: second leg start
  3: third leg start
  4: end
*/

/**
 * Maximum allowed distance between start end finish.  According to
 * FAI-OLC 2012 rules, this is 1 km.
 *
 * TODO: due to trace thinning, our TracePoints are sometimes not
 * close enough for this check to succeed.  To work around this for
 * now, we allow up to 5 km until this library has been implemented
 * properly.
 */
static constexpr fixed max_distance(1000);

OLCTriangle::OLCTriangle(const Trace &_trace,
                         const bool _is_fai, bool _predict,
                         const unsigned _finish_alt_diff)
  : AbstractContest(_finish_alt_diff),
   TraceManager(_trace),
   is_fai(_is_fai), predict(_predict),
   is_closed(false),
   is_complete(false)
{
}

void
OLCTriangle::Reset()
{
  is_complete = false;
  is_closed = false;
  best_d = 0;
  closing_pairs.clear();
  search_point_tree.clear();
  branch_and_bound.clear();
  ClearTrace();

  AbstractContest::Reset();
}

gcc_pure
static fixed
CalcLegDistance(const ContestTraceVector &solution, const unsigned index)
{
  // leg 0: 1-2
  // leg 1: 2-3
  // leg 2: 3-1

  const GeoPoint &p_start = solution[index + 1].GetLocation();
  const GeoPoint &p_dest = solution[index < 2 ? index + 2 : 1].GetLocation();

  return p_start.Distance(p_dest);
}

void
OLCTriangle::UpdateTrace(bool force)
{
  if (IsMasterAppended()) return; /* unmodified */

  if (force || IsMasterUpdated(false)) {
    UpdateTraceFull();

    is_complete = false;

    best_d = 0;

    closing_pairs.clear();
    is_closed = FindClosingPairs(0);

   } else if (is_complete && incremental) {
    const unsigned old_size = n_points;
    if (UpdateTraceTail()) {
      is_complete = false;
      is_closed = FindClosingPairs(old_size);
    }
  }
}


SolverResult
OLCTriangle::Solve(bool exhaustive)
{
  if (trace_master.size() < 3) {
    ClearTrace();
    is_complete = false;
    return SolverResult::FAILED;
  }

  UpdateTrace(exhaustive);

  if (!is_complete) {
    if (n_points < 3)
      return SolverResult::FAILED;

    if (is_closed)
      SolveTriangle();

    if (!SaveSolution())
      return SolverResult::FAILED;

    return SolverResult::VALID;
  } else {
    return SolverResult::FAILED;
  }
}

void
OLCTriangle::SolveTriangle()
{
  unsigned tp1 = 0,
           tp2 = 0,
           tp3 = 0,
           start = 0,
           finish = 0;

  ClosingPairs relaxed_pairs;

  unsigned relax = n_points * 0.03;

  // for all closed trace loops
  for (auto closing_pair = closing_pairs.closing_pairs.begin();
       closing_pair != closing_pairs.closing_pairs.end();
       ++closing_pair) {

    auto already_relaxed = relaxed_pairs.findRange(*closing_pair);
    if (already_relaxed.first != 0 || already_relaxed.second != 0)
      // this pair is already relaxed... continue with next
      continue;

    unsigned relax_first = closing_pair->first;
    unsigned relax_last = closing_pair->second;

    for (auto relaxed = closing_pair;
         relaxed != closing_pairs.closing_pairs.end() &&
         relaxed->first <= closing_pair->first + relax &&
         relaxed->second <= closing_pair->second + relax;
         ++relaxed)
      relax_last = relaxed->second;

    relaxed_pairs.insert(ClosingPair(relax_first, relax_last));
  }

  // TODO: reverse sort relaxed pairs according to number of contained points

  ClosingPairs close_look;

  for (auto relaxed_pair = relaxed_pairs.closing_pairs.begin();
       relaxed_pair != relaxed_pairs.closing_pairs.end();
       ++relaxed_pair) {

    std::tuple<unsigned, unsigned, unsigned, unsigned> triangle;

    triangle = RunBranchAndBound(relaxed_pair->first, relaxed_pair->second, best_d);

    if (std::get<3>(triangle) > best_d) {
      // solution is better than best_d
      // only if triangle is inside a unrelaxed pair...

      auto unrelaxed = closing_pairs.findRange(ClosingPair(std::get<0>(triangle), std::get<2>(triangle)));
      if (unrelaxed.first != 0 || unrelaxed.second != 0) {
        // fortunately it is inside a unrelaxed closing pair :-)
        start = unrelaxed.first;
        tp1 = std::get<0>(triangle);
        tp2 = std::get<1>(triangle);
        tp3 = std::get<2>(triangle);
        finish = unrelaxed.second;

        best_d = std::get<3>(triangle);
      } else {
        // otherwise we should solve the triangle again for every unrelaxed pair
        // contained inside the current relaxed pair. *damn!*
        for (auto closing_pair = closing_pairs.closing_pairs.begin();
             closing_pair != closing_pairs.closing_pairs.end();
              ++closing_pair) {
          if (closing_pair->first >= relaxed_pair->first && closing_pair->second <= relaxed_pair->second)
            close_look.insert(*closing_pair);
        }
      }
    }
  }

  for (auto close_look_pair = close_look.closing_pairs.begin();
       close_look_pair != close_look.closing_pairs.end();
       ++close_look_pair) {

    std::tuple<unsigned, unsigned, unsigned, unsigned> triangle;

    triangle = RunBranchAndBound(close_look_pair->first, close_look_pair->second, best_d);

    if (std::get<3>(triangle) > best_d) {
      // solution is better than best_d

      start = close_look_pair->first;
      tp1 = std::get<0>(triangle);
      tp2 = std::get<1>(triangle);
      tp3 = std::get<2>(triangle);
      finish = close_look_pair->second;

      best_d = std::get<3>(triangle);
    }
  }

  if (best_d > 0) {
    solution.resize(5);

    solution[0] = TraceManager::GetPoint(start);
    solution[1] = TraceManager::GetPoint(tp1);
    solution[2] = TraceManager::GetPoint(tp2);
    solution[3] = TraceManager::GetPoint(tp3);
    solution[4] = TraceManager::GetPoint(finish);

    is_complete = true;
  }
}


std::tuple<unsigned, unsigned, unsigned, unsigned>
OLCTriangle::RunBranchAndBound(unsigned from, unsigned to, unsigned worst_d)
{
  /* Some general information about the branch and bound method can be found here:
   * http://eaton.math.rpi.edu/faculty/Mitchell/papers/leeejem.html
   *
   * How to use this method for solving FAI triangles is described here:
   * http://www.penguin.cz/~ondrap/algorithm.pdf
   */

  bool integral_feasible = false;
  unsigned best_d = 0,
           tp1 = 0,
           tp2 = 0,
           tp3 = 0;

  // note: this is _not_ the breakepoint between small and large triangles,
  // but a slightly lower value used for relaxed large triangle checking.
  const unsigned large_triangle_check =
    trace_master.ProjectRange(GetPoint(from).GetLocation(), fixed(500000)) * 0.99;

  // initialize bound-and-branch tree with root node
  CandidateSet root_candidates(this, from, to);
  if (root_candidates.isFeasible(is_fai, large_triangle_check) && root_candidates.df_max >= worst_d)
    branch_and_bound.insert(std::pair<unsigned, CandidateSet>(root_candidates.df_max, root_candidates));

  while (branch_and_bound.size() != 0) {
    /* now loop over the tree, branching each found candidate set, adding the branch if it's feasible.
     * remove all candidate sets with d_max smaller than d_min of the largest integral candidate set
     * always work on the node with largest d_min
     */

    // first clean up tree, removeing all nodes with d_max < worst_d
    branch_and_bound.erase(branch_and_bound.begin(), branch_and_bound.lower_bound(worst_d));
    assert(branch_and_bound.size() != 0);

    // get node to work on
    auto node = --branch_and_bound.end();

    if (node->second.df_min >= worst_d && node->second.integral(this, is_fai, large_triangle_check)) {
      // node is integral feasible -> a possible solution

      worst_d = node->second.df_min;

      tp1 = node->second.tp1.index_min;
      tp2 = node->second.tp2.index_min;
      tp3 = node->second.tp3.index_min;
      best_d = node->first;

      integral_feasible = true;

    } else {
      // split largest bounding box of node and create child nodes

      const unsigned tp1_diag = node->second.tp1.diagonal();
      const unsigned tp2_diag = node->second.tp2.diagonal();
      const unsigned tp3_diag = node->second.tp3.diagonal();

      const unsigned max_diag = std::max({tp1_diag, tp2_diag, tp3_diag});

      CandidateSet left, right;

      if (tp1_diag == max_diag && node->second.tp1.size() != 1) {
        // split tp1 range
        const unsigned split = (node->second.tp1.index_min + node->second.tp1.index_max) / 2;

        left = CandidateSet(TurnPointRange(this, node->second.tp1.index_min, split),
                            node->second.tp2, node->second.tp3);

        right = CandidateSet(TurnPointRange(this, split, node->second.tp1.index_max),
                             node->second.tp2, node->second.tp3);

      } else if (tp2_diag == max_diag && node->second.tp2.size() != 1) {
        // split tp2 range
        const unsigned split = (node->second.tp2.index_min + node->second.tp2.index_max) / 2;

        left = CandidateSet(node->second.tp1,
                            TurnPointRange(this, node->second.tp2.index_min, split),
                            node->second.tp3);

        right = CandidateSet(node->second.tp1,
                             TurnPointRange(this, split, node->second.tp2.index_max),
                             node->second.tp3);

      } else if (node->second.tp3.size() != 1) {
        // split tp3 range
        const unsigned split = (node->second.tp3.index_min + node->second.tp3.index_max) / 2;

        left = CandidateSet(node->second.tp1, node->second.tp2,
                            TurnPointRange(this, node->second.tp3.index_min, split));

        right = CandidateSet(node->second.tp1, node->second.tp2,
                             TurnPointRange(this, split, node->second.tp3.index_max));
      }

      // add the new candidate set only if it it's feasible and has d_min >= worst_d
      if (left.df_max >= worst_d && left.isFeasible(is_fai, large_triangle_check)) {
        branch_and_bound.insert(std::pair<unsigned, CandidateSet>(left.df_max, left));
      }

      if (right.df_max >= worst_d && right.isFeasible(is_fai, large_triangle_check)) {
        branch_and_bound.insert(std::pair<unsigned, CandidateSet>(right.df_max, right));
      }
    }

    // remove current node
    branch_and_bound.erase(node);
  }

  if (integral_feasible) {
    if (tp1 > tp2) std::swap(tp1, tp2);
    if (tp2 > tp3) std::swap(tp2, tp3);
    if (tp1 > tp2) std::swap(tp1, tp2);

    return std::tuple<unsigned, unsigned, unsigned, unsigned>(tp1, tp2, tp3, best_d);
  } else {
    return std::tuple<unsigned, unsigned, unsigned, unsigned>(0, 0, 0, 0);
  }
}

ContestResult
OLCTriangle::CalculateResult() const
{
  ContestResult result;
  result.time = (is_complete && is_closed)
    ? fixed(solution[4].DeltaTime(solution[0]))
    : fixed(0);
  result.distance = (is_complete && is_closed)
    ? CalcLegDistance(solution, 0) + CalcLegDistance(solution, 1) + CalcLegDistance(solution, 2)
    : fixed(0);
  result.score = ApplyHandicap(result.distance * fixed(0.001));
  return result;
}

bool
OLCTriangle::FindClosingPairs(unsigned old_size)
{
  if (predict) {
    return closing_pairs.insert(ClosingPair(0, n_points-1));
  }

  for (unsigned i = old_size; i < n_points; ++i) {
    TracePointNode node;
    node.point = &GetPoint(i);
    node.index = i;

    search_point_tree.insert(node);
  }

  bool new_pair = false;

  for (unsigned i = old_size; i < n_points; ++i) {
    std::vector<TracePointNode> how_close;
    TracePointNode point;
    point.point = &GetPoint(i);
    point.index = i;

    const unsigned max_range =
      trace_master.ProjectRange(GetPoint(i).GetLocation(), max_distance);

    search_point_tree.find_within_range(point, max_range,
      std::back_insert_iterator<std::vector<TracePointNode>>(how_close));

    const SearchPoint start = GetPoint(i);
    const int min_altitude = GetMinimumFinishAltitude(GetPoint(i));
    const int max_altitude = GetMaximumStartAltitude(GetPoint(i));

    unsigned last = 0, first = i;

    for (unsigned n = 0; n < how_close.size(); ++n) {
      const SearchPoint dest = GetPoint(how_close[n].index);

      if (how_close[n].index + 2 < i &&
          GetPoint(how_close[n].index).GetIntegerAltitude() <= max_altitude &&
          start.GetLocation().Distance(dest.GetLocation()) <= max_distance) {
        // point i is last point
        first = std::min(how_close[n].index, first);
        last = i;
      } else if (how_close[n].index > i + 2 &&
                 GetPoint(how_close[n].index).GetIntegerAltitude() >= min_altitude &&
                 start.GetLocation().Distance(dest.GetLocation()) <= max_distance) {
        // point i is first point
        first = i;
        last = std::max(how_close[n].index, last);
      }
    }

    if (last != 0 && closing_pairs.insert(ClosingPair(first, last)))
      new_pair = true;
  }

  return new_pair;
}

bool
OLCTriangle::UpdateScore()
{
  return false;
}

void
OLCTriangle::CopySolution(ContestTraceVector &result) const
{
  result = solution;
  assert(result.size() == 5);
}

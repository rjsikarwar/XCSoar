/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "AATPoint.hpp"
#include "Navigation/TaskProjection.hpp"
#include "Navigation/Flat/FlatLine.hpp"
#include "Navigation/ConvexHull/PolygonInterior.hpp"
#include <math.h>
#include <assert.h>

const GeoPoint&
AATPoint::get_location_remaining() const
{
  if (getActiveState() == BEFORE_ACTIVE) {
    if (has_sampled()) {
      return get_location_max();
    } else {
      return get_location_min();
    }
  } else {
    return m_target_location;
  }
}

bool 
AATPoint::update_sample_near(const AIRCRAFT_STATE& state,
                             TaskEvents &task_events,
                             const TaskProjection &projection)
{
  bool retval = OrderedTaskPoint::update_sample_near(state, task_events,
                                                     projection);

  if (retval) {
    // deadzone must be updated

    assert(get_next());

    // the deadzone is the convex hull formed from the sampled points
    // with the inclusion of the destination of the next turnpoint.

    m_deadzone = SearchPointVector(get_sample_points().begin(),
                                   get_sample_points().end());
    SearchPoint destination(get_next()->get_location_remaining(), 
                            projection);
    m_deadzone.push_back(destination);
    prune_interior(m_deadzone);
  }

  retval |= check_target(state, false);

  return retval;
}


bool 
AATPoint::update_sample_far(const AIRCRAFT_STATE& state,
                            TaskEvents &task_events,
                            const TaskProjection &projection)
{
  // the orderedtaskpoint::update_sample_far does nothing for now
  // but we are calling this in case that changes.
  return OrderedTaskPoint::update_sample_far(state, task_events,
                                             projection)
    || check_target(state, true);
}


bool
AATPoint::check_target(const AIRCRAFT_STATE& state, const bool known_outside) 
{
  if ((getActiveState() == CURRENT_ACTIVE) && (!m_target_locked)) {
    return false;
  }
  bool moved = false;
  if (!known_outside && isInSector(state)) {
    moved = check_target_inside(state);
  } else {
    moved = check_target_outside(state);
  }

  return moved;
}

bool 
AATPoint::close_to_target(const AIRCRAFT_STATE& state, const fixed threshold) const
{
  if (!valid())
    return false;

  return (double_leg_distance(state.Location)
          -double_leg_distance(m_target_location)
          > -threshold);
}

bool
AATPoint::check_target_inside(const AIRCRAFT_STATE& state) 
{
  // target must be moved if d(p_last,t)+d(t,p_next) 
  //    < d(p_last,state)+d(state,p_next)

  if (close_to_target(state)) {
    const fixed d_to_max = state.Location.distance(get_location_max());
    if (d_to_max <= fixed_zero) {
      // no improvement available
      return false;
    } else {
      m_target_location = state.Location;
      return true;
    }
  } else {
    return false;
  }
}

bool
AATPoint::check_target_outside(const AIRCRAFT_STATE& state) 
{
  return false;
/*
  // this is optional, to be replaced!
  
  // now uses TaskOptTarget

  if (!get_previous()->isInSector(state)) {
    double b0s = get_previous()->get_location_remaining()
      .bearing(state.Location);
    GeoVector vst(state.Location,m_target_location);
    double da = ::AngleLimit180(b0s-vst.Bearing);
    if ((fabs(da)>2.0) && (vst.Distance>1.0)) {
      AATIsolineIntercept ai(*this);
      AIRCRAFT_STATE si;
      if (ai.intercept(*this, state, 0.0, si.Location)
          && isInSector(si)) {

        // Note that this fights with auto-target

        m_target_location = si.Location;

        return true;
      }
    }
  }
  return false;
*/
}


bool
AATPoint::set_range(const fixed p, const bool force_if_current)
{
  if (m_target_locked) {
    return false;
  }

  switch (getActiveState()) {
  case CURRENT_ACTIVE:
    if (!has_entered() || force_if_current) {
      m_target_location = get_location_min().interpolate(get_location_max(),p);
      return true;
    }
    return false;
  case AFTER_ACTIVE:
    if (getActiveState() == AFTER_ACTIVE) {
      m_target_location = get_location_min().interpolate(get_location_max(),p);
      return true;
    }
  default:
    return false;
  }
  return false;
}


void 
AATPoint::set_target(const GeoPoint &loc, const bool override_lock)
{
  if (override_lock || !m_target_locked) {
    m_target_location = loc;
  }
}

void
AATPoint::set_target(const fixed range, const fixed radial,
                     const TaskProjection &proj)
{
  fixed oldrange = fixed_zero;
  fixed oldradial = fixed_zero;
  get_target_range_radial(oldrange, oldradial);

  const FlatPoint fprev = proj.fproject(get_previous()->get_location_remaining());
  const FlatPoint floc = proj.fproject(get_location());
  const FlatLine flb (fprev,floc);
  const FlatLine fradius (floc,proj.fproject(get_location_min()));
  const fixed bearing = fixed_minus_one * flb.angle().value_degrees();
  const fixed radius = fradius.d();

  fixed swapquadrants = fixed_zero;
  if (positive(range) != positive(oldrange))
    swapquadrants = fixed(180);
  const FlatPoint ftarget1 (fabs(range) * radius *
        cos((bearing + radial + swapquadrants)
            / fixed(360) * fixed_two_pi),
      fabs(range) * radius *
        sin( fixed_minus_one * (bearing + radial + swapquadrants)
            / fixed(360) * fixed_two_pi));

  const FlatPoint ftarget2 = floc + ftarget1;
  const GeoPoint targetG = proj.funproject(ftarget2);

  set_target(targetG, true);
}

void
AATPoint::get_target_range_radial(fixed &range, fixed &radial) const
{
  const fixed oldrange = range;

  const GeoPoint fprev = get_previous()->get_location_remaining();
  const GeoPoint floc = get_location();
  const Angle radialraw = (floc.bearing(get_location_target()) -
      fprev.bearing(floc)).as_bearing();

  const fixed d = floc.distance(get_location_target());
  const fixed radius = floc.distance(get_location_min());
  const fixed rangeraw = min(fixed_one, d / radius);

  radial = radialraw.as_delta().value_degrees();
  const fixed rangesign = (fabs(radial) > fixed(90)) ?
      fixed_minus_one : fixed_one;
  range = rangeraw * rangesign;

  if ((oldrange == fixed_zero) && (range == fixed_zero))
    radial = fixed_zero;
}


bool
AATPoint::equals(const OrderedTaskPoint* other) const
{
  const AATPoint &tp = (const AATPoint &)*other;

  return OrderedTaskPoint::equals(other) &&
    m_target_locked == tp.m_target_locked &&
    m_target_location == tp.m_target_location;
}

void
AATPoint::reset()
{
  IntermediateTaskPoint::reset();
  m_deadzone.clear();
}

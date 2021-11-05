//
// utility functions for particle trajectores
//
#ifndef TrackToy_Detector_TrajUtilities_hh
#define TrackToy_Detector_TrajUtilities_hh
#include "KinKal/General/ParticleState.hh"
#include "KinKal/General/TimeRange.hh"
#include "KinKal/General/BFieldMap.hh"
#include "KinKal/Trajectory/ParticleTrajectory.hh"
#include <stdexcept>
#include <stdio.h>

namespace TrackToy {
  //  Update the state of a trajectory for a change in the energy.  If this energy is still physical, this will append a new
  //  trajectory on a piecetraj at the point of the energy loss assignment and return true.  If not, it will terminate the trajectory and return false
  template<class KTRAJ> bool updateEnergy(KinKal::ParticleTrajectory<KTRAJ>& pktraj, double time, double newe) {
    auto const& ktraj = pktraj.nearestPiece(time);
    if(newe > pktraj.mass()) {
      // sample the momentum and position at this time
      auto dir = ktraj.direction(time);
      auto endpos = ktraj.position3(time);
      double mass = ktraj.mass();
      // correct the momentum for the energy change
      auto newmom = sqrt(newe*newe - mass*mass)*dir;
      // convert to particle state
      KinKal::ParticleState pstate(endpos,newmom,time,mass,ktraj.charge());
      // convert to trajectory, using the piece reference field
      KinKal::TimeRange range(time,pktraj.range().end());
      KTRAJ newtraj(pstate,ktraj.bnom(),range);
      // append this back, allowing removal
      //      std::cout << "2 appending " << range << " to range " << pktraj.range() << std::endl;
      pktraj.append(newtraj,true);
      return true;
    } else {
      // terminate the particle here
      KinKal::TimeRange range(pktraj.range().begin(),time);
      pktraj.setRange(range, true);
      return false;
    }
  }

  // extend a trajectory forwards (in z) to the given z value for the given BField to the given z value
  template<class KTRAJ> bool extendZ(KinKal::ParticleTrajectory<KTRAJ>& pktraj, KinKal::BFieldMap const& bfield, double zmax,double tol) {
    double tstart = pktraj.pieces().back().range().begin();
    auto pos = pktraj.position3(tstart);
    KinKal::TimeRange range(tstart, pktraj.range().end());
    while(pos.Z() < zmax && pos.Z() > bfield.zMin() && pos.Z() < bfield.zMax() && range.begin() < range.end() ){
      range.begin() = bfield.rangeInTolerance(pktraj.back(),range.begin(),tol);
      if(range.begin() < range.end()){
        // Predict new position and momentum at this end, making linear correction for BField effects
        auto pstate = pktraj.back().state(range.begin());
        pos = pstate.position3();
        auto bend = bfield.fieldVect(pos);
        KTRAJ endtraj(pstate,bend,range);
        //        std::cout << "appending " << range << " to range " << pktraj.range() << std::endl;
        pktraj.append(endtraj);
        //        cout << "appended helix at point " << pos << " time " << range.begin() << endl;
      } else {
        pos = pktraj.position3(range.end());
      }
    }
    return pos.Z() >= zmax;
  }

  template <class KTRAJ> void extendTraj(KinKal::BFieldMap const& bfield, KinKal::ParticleTrajectory<KTRAJ>& pktraj,double extime,double tol) {
    double tstart = pktraj.back().range().begin();
    KinKal::TimeRange range(tstart, pktraj.range().end());
    auto pos = pktraj.position3(tstart);
    while(range.begin() < extime){
      range.begin() = bfield.rangeInTolerance(pktraj.back(),range.begin(),tol);
      if(range.begin() < range.end()){
        auto pstate = pktraj.back().state(range.begin());
        pos = pstate.position3();
        auto bend = bfield.fieldVect(pos);
        KTRAJ endtraj(pstate,bend,range);
        //        std::cout << "appending " << range << " to range " << pktraj.range() << std::endl;
        pktraj.append(endtraj);
      } else {
        break;
      }
    }
  }

  template <class KTRAJ> double ztime(KinKal::ParticleTrajectory<KTRAJ>const& pktraj, double tstart, double zpos) {
    auto istart = pktraj.nearestIndex(tstart);
// advance till we're going in the correct direction
    auto index = istart;
    auto retval = tstart;
    while(index < pktraj.pieces().size()){
      auto const& piece = pktraj.piece(index);
      auto pos = piece.position3(piece.range().begin());
      auto vel = piece.velocity(piece.range().begin());
      double dt =(zpos-pos.Z())/vel.Z();
      if(dt > 0.0){
        break;
      }
      index++;
    }
    // now iteratively search for the solution
    size_t oldindex = index;
    size_t oldoldindex = index;
    size_t ntries(0);
    do {
      ++ntries;
      auto const& traj = pktraj.piece(index);
      retval = traj.ztime(zpos);
      oldoldindex = oldindex;
      oldindex = index;
      index = pktraj.nearestIndex(retval);
      // protext against osccilation and divergence
    } while (retval < pktraj.range().end() && oldindex != index && oldoldindex != index && ntries < pktraj.pieces().size());
    // last check for failure
    if(retval < tstart)
      retval = pktraj.range().end()+1.0e-6;
    return retval;
  }

}
#endif

//____________________________________________________________________________
/*!

\class    genie::NievesSimoVacasMECPXSec2016

\brief    Computes the Valencia MEC model differential cross section.
          Uses precomputed hadon tensor tables.
          Is a concrete implementation of the XSecAlgorithmI interface.

\author   Code contributed by J. Schwehr, D. Cherdack, R. Gran and described
          in arXiv:1601.02038 and some of the refereces there-in,
          in particular PRD 88 (2013) 113007

          Substantial code refactorizations by the core GENIE group.

          Refactored in 2018 by S. Gardiner to use the new hadron tensor
          framework

\ref      J. Nieves, I. Ruiz Simo, M.J. Vicente Vacas,
          Inclusive quasi-elastic neutrino reactions, PRC 83 (2011) 045501

\created  Mar 22, 2016

\cpright  Copyright (c) 2003-2020, The GENIE Collaboration
          For the full text of the license visit http://copyright.genie-mc.org
*/
//____________________________________________________________________________

#ifndef _NIEVES_SIMO_VACAS_MEC_PXSEC_2016_H_
#define _NIEVES_SIMO_VACAS_MEC_PXSEC_2016_H_

#include "Framework/EventGen/XSecAlgorithmI.h"
#include "Physics/HadronTensors/HadronTensorModelI.h"

namespace genie {

class XSecIntegratorI;

class NievesSimoVacasMECPXSec2016 : public XSecAlgorithmI {

public:
  NievesSimoVacasMECPXSec2016();
  NievesSimoVacasMECPXSec2016(string config);
  virtual ~NievesSimoVacasMECPXSec2016();

  // XSecAlgorithmI interface implementation
  double XSec            (const Interaction * i, KinePhaseSpace_t k) const;
  double Integral        (const Interaction * i) const;
  bool   ValidProcess    (const Interaction * i) const;
  double ScaleFunction   (const double W, const double W1, const double W2, const double s1, const double s2) const;
  double GetScaling      (const double Q0, const double Q3 ) const ;
  // override the Algorithm::Configure methods to load configuration
  // data to private data members
  void Configure (const Registry & config);
  void Configure (string config);

private:

  // Load algorithm configuration
  void LoadConfig (void);

  double fXSecScale; ///< external xsec scaling factor
  double fXSecScaleQELRegion; ///< external xsec scaling factor applied to the QEL region
  double fXSecScaleRESRegion; ///< external xsec scaling factor applied to the RES region

  const HadronTensorModelI* fHadronTensorModel;

  const XSecIntegratorI *  fXSecIntegrator; // Numerical integrator (GSL)

};

}       // genie namespace
#endif  // _NIEVES_SIMO_VACAS_MEC_PXSEC_2016_H_

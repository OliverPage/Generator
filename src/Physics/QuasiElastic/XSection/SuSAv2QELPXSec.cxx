//_________________________________________________________________________
/*
 Copyright (c) 2003-2019, The GENIE Collaboration
 For the full text of the license visit http://copyright.genie-mc.org
 or see $GENIE/LICENSE

 For the class documentation see the corresponding header file.
*/
//_________________________________________________________________________

#include "Framework/Algorithm/AlgConfigPool.h"
#include "Framework/Conventions/Units.h"
#include "Framework/Messenger/Messenger.h"
#include "Framework/ParticleData/PDGCodes.h"
#include "Framework/ParticleData/PDGLibrary.h"
#include "Framework/ParticleData/PDGUtils.h"
#include "Framework/Utils/KineUtils.h"
#include "Physics/HadronTensors/LabFrameHadronTensorI.h"
#include "Physics/HadronTensors/SuSAv2QELHadronTensorModel.h"
#include "Physics/QuasiElastic/XSection/SuSAv2QELPXSec.h"
#include "Physics/Multinucleon/XSection/MECUtils.h"
#include "Physics/XSectionIntegration/XSecIntegratorI.h"
#include "Physics/NuclearState/FermiMomentumTablePool.h"
#include "Physics/NuclearState/FermiMomentumTable.h"

using namespace genie;

//_________________________________________________________________________
SuSAv2QELPXSec::SuSAv2QELPXSec() : XSecAlgorithmI("genie::SuSAv2QELPXSec")
{
}
//_________________________________________________________________________
SuSAv2QELPXSec::SuSAv2QELPXSec(string config)
  : XSecAlgorithmI("genie::SuSAv2QELPXSec", config)
{
}
//_________________________________________________________________________
SuSAv2QELPXSec::~SuSAv2QELPXSec()
{
}
//_________________________________________________________________________
double SuSAv2QELPXSec::XSec(const Interaction* interaction,
  KinePhaseSpace_t kps) const
{
  if ( !this->ValidProcess(interaction) ) return 0.;

  // Get the hadron tensor for the selected nuclide. Check the probe PDG code
  // to know whether to use the tensor for CC neutrino scattering or for
  // electron scattering
  int target_pdg = interaction->InitState().Tgt().Pdg();
  int probe_pdg = interaction->InitState().ProbePdg();
  int tensor_pdg = target_pdg;
  int A_request = pdg::IonPdgCodeToA(target_pdg);
  bool need_to_scale = false;
  int modelConfig = 1; // 0 is HF, 1 is CRPA, 2 will be SuSA (TBD)

  HadronTensorType_t tensor_type = kHT_Undefined;
  if ( pdg::IsNeutrino(probe_pdg) || pdg::IsAntiNeutrino(probe_pdg) ) {
    tensor_type = kHT_QE_Full;
  }
  else {
    // If the probe is not a neutrino, assume that it's an electron
    tensor_type = kHT_QE_EM;
  }

  double Eb_tgt=0;
  double Eb_ten=0;

  if ( A_request <= 4 ) {
    // Use carbon tensor for very light nuclei. This is not ideal . . .
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbHe; Eb_ten=fEbC;
  }
  else if (A_request < 9) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbLi; Eb_ten=fEbC;
  }
  else if (A_request >= 9 && A_request < 15) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbC; Eb_ten=fEbC;
  }
  else if(A_request >= 15 && A_request < 22) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbO; Eb_ten=fEbC;
  }
  else if(A_request >= 22 && A_request < 40) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbMg; Eb_ten=fEbC;
  }
  else if(A_request >= 40 && A_request < 56) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbAr; Eb_ten=fEbC;
  }
  else if(A_request >= 56 && A_request < 119) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbFe; Eb_ten=fEbC;
  }
  else if(A_request >= 119 && A_request < 206) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbSn; Eb_ten=fEbC;
  }
  else if(A_request >= 206) {
    tensor_pdg = kPdgTgtC12;
    Eb_tgt=fEbPb; Eb_ten=fEbC;
  }

  if (tensor_pdg != target_pdg) need_to_scale = true;

  // The SuSAv2-1p1h hadron tensors are defined using the same conventions
  // as the Valencia MEC (and SuSAv2-MEC) model, so we can use the same sort of tensor
  // object to describe them.
  // const LabFrameHadronTensorI* tensor
  //   = dynamic_cast<const LabFrameHadronTensorI*>( fHadronTensorModel->GetTensor(tensor_pdg,
  //   tensor_type) );



  // Check that the input kinematical point is within the range
  // in which hadron tensors are known (for chosen target)
  double Ev    = interaction->InitState().ProbeE(kRfLab);
  double Tl    = interaction->Kine().GetKV(kKVTl);
  double costl = interaction->Kine().GetKV(kKVctl);
  double ml    = interaction->FSPrimLepton()->Mass();
  double Q0    = 0.;
  double Q3    = 0.;


  // SD: The Q-Value essentially corrects q0 to account for nuclear
  // binding energy in the Valencia model but this effect is already
  // in Guille's tensors so I'll set it to 0.
  // However, if I want to scale I need to account for the altered
  // binding energy. To first order I can use the Q_value for this
  double Q_value = Eb_tgt-Eb_ten;

  // Apply Qvalue relative shift if needed:
  if( fQvalueShifter ) Q_value += Q_value * fQvalueShifter -> Shift( interaction->InitState().Tgt() ) ;

  genie::utils::mec::Getq0q3FromTlCostl(Tl, costl, Ev, ml, Q0, Q3);


  if (Q0<0.060 && pdg::IsNeutrino(probe_pdg) && modelConfig==1 ) {
    tensor_type = kHT_QE_CRPA_Low;
  }
  else if (Q0<0.150 && pdg::IsNeutrino(probe_pdg) && modelConfig==1 ){
    tensor_type = kHT_QE_CRPA_Medium;
  }
  else if (Q0>=0.150 && pdg::IsNeutrino(probe_pdg) && modelConfig==1 ){
    tensor_type = kHT_QE_CRPA_High;
  }
  else if (pdg::IsAntiNeutrino(probe_pdg) && Q0<0.060 && modelConfig==1 ) {
    tensor_type = kHT_QE_CRPA_antiMuNu_Low;
  }
  else if (pdg::IsAntiNeutrino(probe_pdg) && Q0<0.150 && modelConfig==1 ) {
    tensor_type = kHT_QE_CRPA_antiMuNu_Medium;
  }
  else if (pdg::IsAntiNeutrino(probe_pdg) && Q0>=0.150 && modelConfig==1 ) {
    tensor_type = kHT_QE_CRPA_antiMuNu_High;
  }

    else if (pdg::IsNeutrino(probe_pdg) && Q0<0.060 && modelConfig==0 ) {
    tensor_type = kHT_QE_HF_Low;
  }
  else if (pdg::IsNeutrino(probe_pdg) && Q0<0.150 && modelConfig==0 ) {
    tensor_type = kHT_QE_HF_Medium;
  }
  else if (pdg::IsNeutrino(probe_pdg) && Q0>=0.150 && modelConfig==0 ) {
    tensor_type = kHT_QE_HF_High;
  }

  // tensor_type = kPdgTgtC12;
  if(modelConfig==1 && (tensor_type == kHT_QE_CRPA_Low || tensor_type == kHT_QE_CRPA_Medium || tensor_type == kHT_QE_CRPA_High) && (A_request >= 15 && A_request < 22) ) tensor_pdg = kPdgTgtO16;

  if( modelConfig==1 && (tensor_type == kHT_QE_CRPA_Low || tensor_type == kHT_QE_CRPA_Medium || tensor_type == kHT_QE_CRPA_High) && (A_request >= 22 && A_request < 56) ) tensor_pdg = 1000180400;

  if( modelConfig==1 && (tensor_type == kHT_QE_CRPA_antiMuNu_Low || tensor_type == kHT_QE_CRPA_antiMuNu_Medium || tensor_type == kHT_QE_CRPA_antiMuNu_High) && (A_request >= 15 && A_request < 22) ) tensor_pdg = kPdgTgtO16;

  if( modelConfig==1 && (tensor_type == kHT_QE_CRPA_antiMuNu_Low || tensor_type == kHT_QE_CRPA_antiMuNu_Medium || tensor_type == kHT_QE_CRPA_antiMuNu_High) && (A_request >= 22 && A_request < 56) ) tensor_pdg = 1000180400;

  if( modelConfig==0 && (tensor_type == kHT_QE_HF_Low || tensor_type == kHT_QE_HF_Medium || tensor_type == kHT_QE_HF_High) && (A_request >= 15 && A_request < 22) ) tensor_pdg = kPdgTgtO16;

  if( modelConfig==0 && (tensor_type == kHT_QE_HF_Low || tensor_type == kHT_QE_HF_Medium || tensor_type == kHT_QE_HF_High) && (A_request >= 22 && A_request < 56) ) tensor_pdg = 1000180400;
  // std::cout << "tensor type is " << tensor_type << std::endl;
  // std::cout << "target pdg code is " << target_pdg << std::endl;
  // std::cout << "probe pdg code is " << probe_pdg << std::endl;



  const LabFrameHadronTensorI* tensor
  = dynamic_cast<const LabFrameHadronTensorI*>( fHadronTensorModel->GetTensor(tensor_pdg,
  tensor_type) );

    // If retrieving the tensor failed, complain and return zero
  if ( !tensor ) {
    LOG("SuSAv2QE", pWARN) << "Failed to load a hadronic tensor for the"
      " nuclide " << tensor_pdg;
    return 0.;
  }

  double Q0min = tensor->q0Min();
  double Q0max = tensor->q0Max();
  double Q3min = tensor->qMagMin();
  double Q3max = tensor->qMagMax();
  if (Q0-Q_value < Q0min || Q0-Q_value > Q0max || Q3 < Q3min || Q3 > Q3max) {
    return 0.0;
  }

  // *** Enforce the global Q^2 cut (important for EM scattering) ***
  // Choose the appropriate minimum Q^2 value based on the interaction
  // mode (this is important for EM interactions since the differential
  // cross section blows up as Q^2 --> 0)
  double Q2min = genie::controls::kMinQ2Limit; // CC/NC limit
  if ( interaction->ProcInfo().IsEM() ) Q2min = genie::utils::kinematics
    ::electromagnetic::kMinQ2Limit; // EM limit

  // Neglect shift due to binding energy. The cut is on the actual
  // value of Q^2, not the effective one to use in the tensor contraction.
  double Q2 = Q3*Q3 - Q0*Q0;
  if ( Q2 < Q2min ) return 0.;

  // Compute the cross section using the hadron tensor
  double xsec = tensor->dSigma_dT_dCosTheta_rosenbluth(interaction, Q_value);
  LOG("SuSAv2QE", pDEBUG) << "XSec in cm2 / neutron is  " << xsec/(units::cm2);

  // Currently the SuSAv2 QE hadron tensors are given per active nucleon, but
  // the calculation above assumes they are per atom. Need to adjust for this.
  const ProcessInfo& proc_info = interaction->ProcInfo();

  // Neutron, proton, and mass numbers of the target
  const Target& tgt = interaction->InitState().Tgt();
  if ( proc_info.IsWeakCC() ) {
    if ( pdg::IsNeutrino(probe_pdg) ) xsec *= 1; //*= tgt.N();  
    else if ( pdg::IsAntiNeutrino(probe_pdg) ) xsec *= 1; //*= tgt.Z();
    else {
      // We should never get here if ValidProcess() is working correctly
      LOG("SuSAv2QE", pERROR) << "Unrecognized probe " << probe_pdg
        << " encountered for a WeakCC process";
      xsec = 0.;
    }
  }
  else if ( proc_info.IsEM() || proc_info.IsWeakNC() ) {
    // For EM processes, scale by the number of nucleons of the same type
    // as the struck one. This ensures the correct ratio of initial-state
    // p vs. n when making splines. The nuclear cross section is obtained
    // by scaling by A/2 for an isoscalar target, so we can get the right
    // behavior for all targets by scaling by Z/2 or N/2 as appropriate.
    // Do the same for NC. TODO: double-check that this is the right
    // thing to do when we SuSAv2 NC hadronic tensors are added to GENIE.
    int hit_nuc_pdg = tgt.HitNucPdg();
    if ( pdg::IsProton(hit_nuc_pdg) ) xsec *= tgt.Z() / 2.;
    else if ( pdg::IsNeutron(hit_nuc_pdg) ) xsec *= tgt.N() / 2.;
    // We should never get here if ValidProcess() is working correctly
    else return 0.;
  }
  else {
    // We should never get here if ValidProcess() is working correctly
    LOG("SuSAv2QE", pERROR) << "Unrecognized process " << proc_info.AsString()
      << " encountered in SuSAv2QELPXSec::XSec()";
    xsec = 0.;
  }

  LOG("SuSAv2QE", pDEBUG) << "XSec in cm2 / atom is  " << xsec / units::cm2;

  // This scaling should be okay-ish for the total xsec, but it misses
  // the energy shift. To get this we should really just build releveant
  // hadron tensors but there may be some ways to approximate it.
  // For more details see Guille's thesis: https://idus.us.es/xmlui/handle/11441/74826
  if ( need_to_scale ) {
    FermiMomentumTablePool * kftp = FermiMomentumTablePool::Instance();
    const FermiMomentumTable * kft = kftp->GetTable(fKFTable);
    double KF_tgt = kft->FindClosestKF(target_pdg, kPdgProton);
    double KF_ten = kft->FindClosestKF(tensor_pdg, kPdgProton);
    LOG("SuSAv2QE", pDEBUG) << "KF_tgt = " << KF_tgt;
    LOG("SuSAv2QE", pDEBUG) << "KF_ten = " << KF_ten;
    double scaleFact = (KF_ten/KF_tgt); // A-scaling already applied in section above
    xsec *= scaleFact;
  }

  // Apply given overall scaling factor
  xsec *= fXSecScale;

  if ( kps != kPSTlctl ) {
    LOG("SuSAv2QE", pWARN)
      << "Doesn't support transformation from "
      << KinePhaseSpace::AsString(kPSTlctl) << " to "
      << KinePhaseSpace::AsString(kps);
    xsec = 0.;
  }

  return xsec;
}
//_________________________________________________________________________
double SuSAv2QELPXSec::Integral(const Interaction* interaction) const
{
  double xsec = fXSecIntegrator->Integrate(this, interaction);
  return xsec;
}
//_________________________________________________________________________
bool SuSAv2QELPXSec::ValidProcess(const Interaction* interaction) const
{
  if ( interaction->TestBit(kISkipProcessChk) ) return true;

  const InitialState & init_state = interaction->InitState();
  const ProcessInfo &  proc_info  = interaction->ProcInfo();

  if ( !proc_info.IsQuasiElastic() ) return false;

  // The SuSAv2 calculation is only appropriate for complex nuclear targets,
  // not free nucleons.
  if ( !init_state.Tgt().IsNucleus() ) return false;

  int  nuc = init_state.Tgt().HitNucPdg();
  int  nu  = init_state.ProbePdg();

  bool isP   = pdg::IsProton(nuc);
  bool isN   = pdg::IsNeutron(nuc);
  bool isnu  = pdg::IsNeutrino(nu);
  bool isnub = pdg::IsAntiNeutrino(nu);
  bool is_chgl = pdg::IsChargedLepton(nu);

  bool prcok = ( proc_info.IsWeakCC() && ((isP && isnub) || (isN && isnu)) )
    || ( proc_info.IsEM() && is_chgl && (isP || isN) );
  if ( !prcok ) return false;

  return true;
}
//_________________________________________________________________________
void SuSAv2QELPXSec::Configure(const Registry& config)
{
  Algorithm::Configure(config);
  this->LoadConfig();
}
//____________________________________________________________________________
void SuSAv2QELPXSec::Configure(std::string config)
{
  Algorithm::Configure(config);
  this->LoadConfig();
}
//_________________________________________________________________________
void SuSAv2QELPXSec::LoadConfig(void)
{
  bool good_config = true ;

  // Cross section scaling factor
  GetParam( "QEL-CC-XSecScale", fXSecScale ) ;

  fHadronTensorModel = dynamic_cast< const SuSAv2QELHadronTensorModel* >(
    this->SubAlg("HadronTensorAlg") );
  assert( fHadronTensorModel );

  // Load XSec Integrator
  fXSecIntegrator = dynamic_cast<const XSecIntegratorI *>(
    this->SubAlg("XSec-Integrator") );
  assert( fXSecIntegrator );

  // Fermi momentum tables for scaling
  this->GetParam( "FermiMomentumTable", fKFTable);

  // Binding energy lookups for scaling
  this->GetParam( "RFG-NucRemovalE@Pdg=1000020040", fEbHe );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000030060", fEbLi );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000060120", fEbC  );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000080160", fEbO  );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000120240", fEbMg );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000180400", fEbAr );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000200400", fEbCa );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000260560", fEbFe );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000280580", fEbNi );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000501190", fEbSn );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000791970", fEbAu );
  this->GetParam( "RFG-NucRemovalE@Pdg=1000822080", fEbPb );


// Create variables
  double pmu = 0.000;
  double m_probe = 0.0;
  double Ml = PDGLibrary::Instance()->Find(13)->Mass();

  double Q_value = 0.0;
  double xsec_r = 0;

  double w00 = 0;
  double w03 = 0;
  double w11 = 0;
  double w12 = 0;
  double w33 = 0;

  // Hard coded point to assess: 
  int nu_pdg = 14;

// Set Enu
  double Enu = 1.0;
// Specify w and q
  double myq = 0.28119130473;
  double myw = 0.0599000015;
// obtain Tl and cos 
  double Tl = 0.0;
  double costhl = 0.0;
  genie::utils::mec::GetTlCostlFromq0q3(myw, myq, Enu, Ml, Tl, costhl);


  //double Tl = sqrt(pmu*pmu + Ml*Ml) - Ml;
  //genie::utils::mec::Getq0q3FromTlCostl(Tl, costhl, Enu, Ml, myw, myq);

  // chose correct data table
  int tensor_pdg = kPdgTgtC12;

  HadronTensorType_t tensor_type;
  if (myw<0.060) {
    tensor_type = kHT_QE_CRPA_Low;
  }
  else if (myw<0.150){
    tensor_type = kHT_QE_CRPA_Medium;
  }
  else{
    tensor_type = kHT_QE_CRPA_High;
  }

  const LabFrameHadronTensorI* tensor = dynamic_cast<const LabFrameHadronTensorI*>( fHadronTensorModel->GetTensor(tensor_pdg, tensor_type) );


  xsec_r = tensor->dSigma_dT_dCosTheta_rosenbluth(nu_pdg, Enu, m_probe, Tl, costhl, Ml, Q_value);
  xsec_r = xsec_r*1E+38/units::cm2;  
  w00 = (tensor->tt(myw,myq)).real();
  w03 = (tensor->tz(myw,myq)).real();
  w11 = (tensor->xx(myw,myq)).real();
  w12 = (tensor->xy(myw,myq)).imag();
  w33 = (tensor->zz(myw,myq)).real();

  std::cout << "\n" << std::endl;
  std::cout << "Calculate some fixed point xsec: " << std::endl;
  std::cout << "w = " << myw << ", q = " << myq << ", costhl = " << costhl << std::endl;
  std::cout << "Enu, pmu, Tl, costhl, w, q: xsec" << std::endl;
  std::cout << Enu << ", " << pmu << ", " << Tl << ", " << costhl << ", " << myw << ", " << myq << ": " << xsec_r << std::endl;
  
  std::cout << "       HTele: w00, w03, w11, w12, w33" << std::endl;
  std::cout << "       HTele: " << w00 << ", " << w03 << ", " << w11 << ", " << w12 << ", " << w33 <<  std::endl;

  std::cout << "Wait 5 seconds before continuing ..." << std::endl;
  sleep(5);

  // Read optional QvalueShifter:
  // Read optional QvalueShifter:                                                                                   
  fQvalueShifter = nullptr;                                                                                        
  if( GetConfig().Exists("QvalueShifterAlg") ) {            
    
    fQvalueShifter = dynamic_cast<const QvalueShifter *> ( this->SubAlg("QvalueShifterAlg") );      

    if( !fQvalueShifter ) {                                                      

      good_config = false ;                                                    
                                  
      LOG("SuSAv2QE", pERROR) << "The required QvalueShifterAlg is not valid. AlgID is : " 
			      << SubAlg("QvalueShifterAlg")->Id() ;    
    }                                                                                                               
  }  // if there is a requested QvalueShifteralgo                                                                                                                
  if( ! good_config ) {
    LOG("SuSAv2QE", pERROR) << "Configuration has failed.";
    exit(78) ;
  }
 

}

/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    testManifoldPreintegration.cpp
 * @brief   Unit test for the ManifoldPreintegration
 * @author  Luca Carlone
 */

#include <gtsam/navigation/ManifoldPreintegration.h>
#include <gtsam/base/numericalDerivative.h>
#include <gtsam/nonlinear/expressions.h>
#include <gtsam/nonlinear/ExpressionFactor.h>
#include <gtsam/nonlinear/expressionTesting.h>

#include <CppUnitLite/TestHarness.h>
#include <boost/bind.hpp>

#include "imuFactorTesting.h"

static const double kDt = 0.1;

boost::function<NavState(const NavState, const Vector3&, const Vector3&)> deltaXij =
    [=](const NavState inputState, const Vector3& a, const Vector3& w) {
      //        ManifoldPreintegration pim(testing::Params());
      Matrix9 aH1;
      Matrix93 aH2, aH3;
      //        pim.update(a, w, kDt,&aH1,&aH2,&aH3);
      //        return pim.deltaXij();
      NavState outputState = inputState.update(a, w, kDt, aH1, aH2, aH3);
      return outputState;
    };

NavState f(const NavState& inputState, const Vector3& a, const Vector3& w) {
  Matrix9 aH1;
  Matrix93 aH2, aH3;
  NavState outputState = inputState.update(a, w, kDt, aH1, aH2, aH3);
  return outputState;
}

namespace testing {
// Create default parameters with Z-down and above noise parameters
static boost::shared_ptr<PreintegrationParams> Params() {
  auto p = PreintegrationParams::MakeSharedD(kGravity);
  p->gyroscopeCovariance = kGyroSigma * kGyroSigma * I_3x3;
  p->accelerometerCovariance = kAccelSigma * kAccelSigma * I_3x3;
  p->integrationCovariance = 0.0001 * I_3x3;
  return p;
}
}

/* ************************************************************************* */
TEST(ManifoldPreintegration, UpdateEstimate1) {
  ManifoldPreintegration pim0(testing::Params());
  ManifoldPreintegration pim(pim0);
  const Vector3 acc(0.1, 0.2, 10), omega(0.1, 0.2, 0.3);
  Matrix9 aH1;
  Matrix93 aH2, aH3;
  pim.update(acc, omega, kDt, &aH1, &aH2, &aH3);
  NavState state;

//  boost::function<NavState(const NavState, const Vector3&, const Vector3&)> deltaXij =
//      [=](const NavState inputState, const Vector3& a, const Vector3& w) {
//        //        ManifoldPreintegration pim(testing::Params());
//        Matrix9 aH1;
//        Matrix93 aH2, aH3;
//        //        pim.update(a, w, kDt,&aH1,&aH2,&aH3);
//        //        return pim.deltaXij();
//        NavState outputState = inputState.update(a, w, kDt, aH1, aH2, aH3);
//        return outputState;
//      };
    EXPECT(assert_equal(numericalDerivative31(f, state, acc, omega), aH1, 1e-9));
    EXPECT(assert_equal(numericalDerivative32(f, state, acc, omega), aH2, 1e-9));
    EXPECT(assert_equal(numericalDerivative33(f, state, acc, omega), aH3, 1e-9));

//  EXPECT(assert_equal(numericalDerivative31(f, pim0, zeta, acc, omega), aH1, 1e-9));
//  EXPECT(assert_equal(numericalDerivative32(f, pim0, zeta, acc, omega), aH2, 1e-9));
//  EXPECT(assert_equal(numericalDerivative33(f, pim0, zeta, acc, omega), aH3, 1e-9));
}

/* ************************************************************************* *
TEST(ManifoldPreintegration, BiasCorrectionJacobians) {
  testing::SomeMeasurements measurements;

  boost::function<Rot3(const Vector3&, const Vector3&)> deltaRij =
      [=](const Vector3& a, const Vector3& w) {
        ManifoldPreintegration pim(testing::Params(), Bias(a, w));
        testing::integrateMeasurements(measurements, &pim);
        return pim.deltaRij();
      };

  boost::function<Point3(const Vector3&, const Vector3&)> deltaPij =
      [=](const Vector3& a, const Vector3& w) {
        ManifoldPreintegration pim(testing::Params(), Bias(a, w));
        testing::integrateMeasurements(measurements, &pim);
        return pim.deltaPij();
      };

  boost::function<Vector3(const Vector3&, const Vector3&)> deltaVij =
      [=](const Vector3& a, const Vector3& w) {
        ManifoldPreintegration pim(testing::Params(), Bias(a, w));
        testing::integrateMeasurements(measurements, &pim);
        return pim.deltaVij();
      };

  // Actual pre-integrated values
  ManifoldPreintegration pim(testing::Params());
  testing::integrateMeasurements(measurements, &pim);

  EXPECT(
      assert_equal(numericalDerivative21(deltaRij, kZero, kZero),
          Matrix3(Z_3x3)));
  EXPECT(
      assert_equal(numericalDerivative22(deltaRij, kZero, kZero),
          pim.delRdelBiasOmega(), 1e-3));

  EXPECT(
      assert_equal(numericalDerivative21(deltaPij, kZero, kZero),
          pim.delPdelBiasAcc()));
  EXPECT(
      assert_equal(numericalDerivative22(deltaPij, kZero, kZero),
          pim.delPdelBiasOmega(), 1e-3));

  EXPECT(
      assert_equal(numericalDerivative21(deltaVij, kZero, kZero),
          pim.delVdelBiasAcc()));
  EXPECT(
      assert_equal(numericalDerivative22(deltaVij, kZero, kZero),
          pim.delVdelBiasOmega(), 1e-3));
}

/* ************************************************************************* */
TEST(ManifoldPreintegration, computeError) {
  ManifoldPreintegration pim(testing::Params());
  NavState x1, x2;
  imuBias::ConstantBias bias;
  Matrix9 aH1, aH2;
  Matrix96 aH3;
  pim.computeError(x1, x2, bias, aH1, aH2, aH3);
  boost::function<Vector9(const NavState&, const NavState&,
                          const imuBias::ConstantBias&)> f =
      boost::bind(&ManifoldPreintegration::computeError, pim, _1, _2, _3,
                  boost::none, boost::none, boost::none);
  // NOTE(frank): tolerance of 1e-3 on H1 because approximate away from 0
  EXPECT(assert_equal(numericalDerivative31(f, x1, x2, bias), aH1, 1e-9));
  EXPECT(assert_equal(numericalDerivative32(f, x1, x2, bias), aH2, 1e-9));
  EXPECT(assert_equal(numericalDerivative33(f, x1, x2, bias), aH3, 1e-9));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */

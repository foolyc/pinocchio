//
// Copyright (c) 2019 INRIA
//

#include "pinocchio/multibody/joint/joint-generic.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/utility/binary.hpp>

using namespace pinocchio;

BOOST_AUTO_TEST_SUITE(BOOST_TEST_MODULE)

typedef Eigen::Matrix<double,6,Eigen::Dynamic> Matrix6x;

template<typename JointModel>
void test_constraint_mimic(const JointModelBase<JointModel> & jmodel)
{
  typedef typename traits<JointModel>::JointDerived Joint;
  typedef typename traits<Joint>::Constraint_t ConstraintType;
  typedef typename traits<Joint>::JointDataDerived JointData;
  typedef ScaledConstraint<ConstraintType> ScaledConstraintType;
  
  JointData jdata = jmodel.createData();
  
  const double scaling_factor = 2.;
  ConstraintType constraint_ref(jdata.S), constraint_ref_shared(jdata.S);
  ScaledConstraintType scaled_constraint(constraint_ref_shared,scaling_factor);
  
  BOOST_CHECK(constraint_ref.nv() == scaled_constraint.nv());
  
  typedef typename JointModel::TangentVector_t TangentVector_t;
  TangentVector_t v = TangentVector_t::Random();
  
  Motion m = scaled_constraint * v;
  Motion m_ref = scaling_factor * (Motion)(constraint_ref * v);
  
  BOOST_CHECK(m.isApprox(m_ref));
  
  {
    SE3 M = SE3::Random();
    typename ScaledConstraintType::DenseBase S = M.act(scaled_constraint);
    typename ScaledConstraintType::DenseBase S_ref = scaling_factor * M.act(constraint_ref);
    
    BOOST_CHECK(S.isApprox(S_ref));
  }
  
  {
    typename ScaledConstraintType::DenseBase S = scaled_constraint.matrix();
    typename ScaledConstraintType::DenseBase S_ref = scaling_factor * constraint_ref.matrix();
    
    BOOST_CHECK(S.isApprox(S_ref));
  }
  
  {
    Motion v = Motion::Random();
    typename ScaledConstraintType::DenseBase S = v.cross(scaled_constraint);
    typename ScaledConstraintType::DenseBase S_ref = scaling_factor * v.cross(constraint_ref);
    
    BOOST_CHECK(S.isApprox(S_ref));
  }
  
  // Test transpose operations
  {
    const Eigen::DenseIndex dim = 20;
    const Matrix6x Fin = Matrix6x::Random(6,dim);
    Eigen::MatrixXd Fout = scaled_constraint.transpose() * Fin;
    Eigen::MatrixXd Fout_ref = scaling_factor * (constraint_ref.transpose() * Fin);
    BOOST_CHECK(Fout.isApprox(Fout_ref));
    
    Force force_in(Force::Random());
    Eigen::MatrixXd Stf = (scaled_constraint.transpose() * force_in);
    Eigen::MatrixXd Stf_ref = scaling_factor * (constraint_ref.transpose() * force_in);
    BOOST_CHECK(Stf_ref.isApprox(Stf));
  }
  
}

struct TestJointConstraint
{
  
  template <typename JointModel>
  void operator()(const JointModelBase<JointModel> &) const
  {
    JointModel jmodel;
    jmodel.setIndexes(0,0,0);
    
    test_constraint_mimic(jmodel);
  }
  
  void operator()(const JointModelBase<JointModelRevoluteUnaligned> &) const
  {
    JointModelRevoluteUnaligned jmodel(1.5, 1., 0.);
    jmodel.setIndexes(0,0,0);

    test_constraint_mimic(jmodel);
  }

  void operator()(const JointModelBase<JointModelPrismaticUnaligned> &) const
  {
    JointModelPrismaticUnaligned jmodel(1.5, 1., 0.);
    jmodel.setIndexes(0,0,0);

    test_constraint_mimic(jmodel);
  }
  
};

BOOST_AUTO_TEST_CASE(test_constraint)
{
  using namespace pinocchio;
  typedef boost::variant<
  JointModelRX, JointModelRY, JointModelRZ
  , JointModelRevoluteUnaligned
  , JointModelPX, JointModelPY, JointModelPZ
  , JointModelPrismaticUnaligned
  , JointModelRUBX, JointModelRUBY, JointModelRUBZ
  > Variant;
  
  boost::mpl::for_each<Variant::types>(TestJointConstraint());
}

BOOST_AUTO_TEST_CASE(test_transform_linear_affine)
{
  typedef JointModelRX::ConfigVector_t ConfigVectorType;
  double scaling = 1., offset = 0.;
  
  ConfigVectorType q0 = ConfigVectorType::Random();
  ConfigVectorType q1;
  LinearAffineTransform::run(q0,scaling,offset,q1);
  BOOST_CHECK(q0 == q1);
  
  offset = 2.;
  LinearAffineTransform::run(ConfigVectorType::Zero(),scaling,offset,q1);
  BOOST_CHECK(q1 == ConfigVectorType::Constant(offset));
}

BOOST_AUTO_TEST_CASE(test_transform_linear_revolute)
{
  typedef JointModelRUBX::ConfigVector_t ConfigVectorType;
  double scaling = 1., offset = 0.;
  
  ConfigVectorType q0 = ConfigVectorType::Random().normalized();
  ConfigVectorType q1;
  UnboundedRevoluteAffineTransform::run(q0,scaling,offset,q1);
  BOOST_CHECK(q0.isApprox(q1));
  
  offset = 2.;
  UnboundedRevoluteAffineTransform::run(ConfigVectorType::Zero(),scaling,offset,q1);
  BOOST_CHECK(q1 == ConfigVectorType(math::cos(offset),math::sin(offset)));
}

BOOST_AUTO_TEST_SUITE_END()

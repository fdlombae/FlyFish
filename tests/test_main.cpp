#include <flyfish/FlyFish.h>

#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <numbers>
#include <sstream>
#include <string>

using namespace flyfish;

// Compile-time constexpr verification
static_assert(
    [] {
      constexpr Vector v{1, 2, 3, 4};
      constexpr BiVector bv{1, 2, 3, 4, 5, 6};
      constexpr auto gp = v * bv;
      return gp[0] != 0.0f || gp[1] != 0.0f; // just check it compiles
    }(),
    "Geometric product must be constexpr");

static_assert(
    [] {
      constexpr Motor m{1, 0, 0, 0, 0, 0, 0, 0};
      constexpr auto inv = ~m;
      return inv[0] == 1.0f;
    }(),
    "Motor inverse must be constexpr");

static_assert(
    [] {
      constexpr PseudoScalar ps{2};
      constexpr Vector v{1, 2, 3, 4};
      constexpr TriVector gp = ps * v;
      constexpr Motor exp = ps.Gexp();
      return gp[0] == -4.0f && exp[7] == 2.0f && !ps == 2.0f;
    }(),
    "PseudoScalar operations must be constexpr");

static_assert(
    [] {
      constexpr Motor rotor = BiVector{0, 0, 0, 0, 0, 1}.Gexp();
      constexpr Motor motor = Motor{1, 0, 0, 0, 0, 0, 0, 0}.Gexp();
      constexpr MultiVector series = MultiVector{1.0f}.Gexp();
      constexpr MultiVector ideal = TriVector::Direction(1, 2, 3).Gexp();
      constexpr Motor rotation = Motor::Rotation(90_deg, BiVector{0, 0, 0, 0, 0, 1});
      return rotor[6] > 0.84f && rotor[6] < 0.85f &&      // sin(1)
             motor[0] > 2.71f && motor[0] < 2.72f &&      // e
             series[0] > 2.71f && series[0] < 2.72f &&    // e
             ideal[0] == 1.0f && ideal[11] == 1.0f &&     // 1 + t
             rotation[0] > 0.707f && rotation[0] < 0.708f; // cos(45 deg)
    }(),
    "Exponentials and motor factories must be constexpr");

// Result types of products involving the pseudoscalar
static_assert(std::is_same_v<decltype(PseudoScalar{} * PseudoScalar{}), GANull>);
static_assert(std::is_same_v<decltype(PseudoScalar{} * Vector{}), TriVector>);
static_assert(std::is_same_v<decltype(PseudoScalar{} * BiVector{}), BiVector>);
static_assert(std::is_same_v<decltype(PseudoScalar{} * TriVector{}), Vector>);
static_assert(std::is_same_v<decltype(PseudoScalar{} * Motor{}), Motor>);
static_assert(std::is_same_v<decltype(PseudoScalar{} & Vector{}), Vector>);
static_assert(std::is_same_v<decltype(PseudoScalar{} ^ Motor{}), PseudoScalar>);
static_assert(std::is_same_v<decltype(Vector{} ^ TriVector{}), PseudoScalar>);
static_assert(std::is_same_v<decltype(TriVector{} ^ Vector{}), PseudoScalar>);
static_assert(std::is_same_v<decltype(BiVector{} ^ BiVector{}), PseudoScalar>);
static_assert(std::is_same_v<decltype(!PseudoScalar{}), float>);
static_assert(std::is_same_v<decltype(GA::HodgeDual(1.0f)), PseudoScalar>);
static_assert(std::is_same_v<decltype(GA::HodgeDual(1.0)), PseudoScalar>);

// Embed any element in a full MultiVector so typed results can be checked
// against the general MultiVector products.
template <typename T> MultiVector ToMV(const T &element) {
  MultiVector res{};
  res = element;
  return res;
}
inline MultiVector ToMV(const GANull &) { return MultiVector{}; }
inline MultiVector ToMV(float scalar) { return MultiVector{scalar}; }

static_assert(std::is_same_v<decltype(PseudoScalar{} | Motor{}), Motor>);
static_assert(std::is_same_v<decltype(Motor{} | PseudoScalar{}), Motor>);

template <typename A, typename B>
void ExpectInnerMatchesMultiVector(const A &a, const B &b) {
  EXPECT_EQ(ToMV(a | b), ToMV(a) | ToMV(b));
}

template <typename A, typename... Bs>
void ExpectInnerMatchesMultiVectorForAll(const A &a, const Bs &...bs) {
  (ExpectInnerMatchesMultiVector(a, bs), ...);
}

template <typename A, typename B>
void ExpectProductsMatchMultiVector(const A &a, const B &b) {
  const MultiVector ma{ToMV(a)};
  const MultiVector mb{ToMV(b)};
  EXPECT_EQ(ToMV(a * b), ma * mb);
  EXPECT_EQ(ToMV(a | b), ma | mb);
  EXPECT_EQ(ToMV(a ^ b), ma ^ mb);
  EXPECT_EQ(ToMV(a & b), ma & mb);
}

// Example class to be tested
class Elements {
public:
  Elements() = default;

  static MultiVector MultiVectorA() {
    return {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
  }
  static MultiVector MultiVectorB() {
    return {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
  }
  static MultiVector MultiVectorC() {
    return {-4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16, 17, -18, 19};
  }
  static Vector VectorA() { return {2, 3, 4, 5}; }
  static Vector VectorB() { return {3, 4, 5, 6}; }
  static Vector VectorC() { return {-4, 5, -6, 7}; }

  static BiVector BiVectorA() { return {2, 3, 4, 5, 6, 7}; }
  static BiVector BiVectorB() { return {3, 4, 5, 6, 7, 8}; }
  static BiVector BiVectorC() { return {-4, 5, -6, 7, -8, 9}; }

  static TriVector TriVectorA() { return {2, 3, 4, 5}; }
  static TriVector TriVectorB() { return {3, 4, 5, 6}; }
  static TriVector TriVectorC() { return {-4, 5, -6, 7}; }

  static Motor MotorA() { return Motor{2, 3, 4, 5, 6, 7, 8, 9}; }
  static Motor MotorB() { return Motor{3, 4, 5, 6, 7, 8, 9, 10}; }
  static Motor MotorC() { return Motor{-4, 5, -6, 7, -8, 9, -10, 11}; }

  static PseudoScalar PseudoScalarA() { return PseudoScalar{2}; }
  static PseudoScalar PseudoScalarB() { return PseudoScalar{3}; }
  static PseudoScalar PseudoScalarC() { return PseudoScalar{-4}; }
};

// Test Fixture
class ElementsTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Initialize the calculator before each test
    elements = new Elements();
  }

  void TearDown() override {
    // Clean up after each test
    delete elements;
  }

  Elements *elements = nullptr; // Shared object for all tests
};

// Named functions
TEST_F(ElementsTest, GANamedFunctions) {
  // Test data
  const MultiVector multi{Elements::MultiVectorA()};
  const TriVector trivector{Elements::TriVectorA()};
  const BiVector bivector{Elements::BiVectorA()};
  const Vector vector{Elements::VectorA()};
  const Motor motor{Elements::MotorA()};

  // Test Gep
  EXPECT_EQ(GA::Gep(multi, bivector), multi * bivector);
  EXPECT_EQ(GA::Gep(multi, vector), multi * vector);
  EXPECT_EQ(GA::Gep(multi, motor), multi * motor);

  EXPECT_EQ(GA::Gep(trivector, multi), trivector * multi);
  EXPECT_EQ(GA::Gep(trivector, bivector), trivector * bivector);
  EXPECT_EQ(GA::Gep(trivector, vector), trivector * vector);
  EXPECT_EQ(GA::Gep(trivector, motor), trivector * motor);

  EXPECT_EQ(GA::Gep(bivector, multi), bivector * multi);
  EXPECT_EQ(GA::Gep(bivector, trivector), bivector * trivector);
  EXPECT_EQ(GA::Gep(bivector, vector), bivector * vector);
  EXPECT_EQ(GA::Gep(bivector, motor), bivector * motor);

  EXPECT_EQ(GA::Gep(vector, multi), vector * multi);
  EXPECT_EQ(GA::Gep(vector, trivector), vector * trivector);
  EXPECT_EQ(GA::Gep(vector, bivector), vector * bivector);
  EXPECT_EQ(GA::Gep(vector, motor), vector * motor);

  EXPECT_EQ(GA::Gep(motor, multi), motor * multi);
  EXPECT_EQ(GA::Gep(motor, trivector), motor * trivector);
  EXPECT_EQ(GA::Gep(motor, bivector), motor * bivector);
  EXPECT_EQ(GA::Gep(motor, vector), motor * vector);

  // Test Inner product
  EXPECT_EQ(GA::Inner(multi, bivector), multi | bivector);
  EXPECT_EQ(GA::Inner(multi, vector), multi | vector);
  EXPECT_EQ(GA::Inner(multi, motor), multi | motor);

  EXPECT_EQ(GA::Inner(trivector, multi), trivector | multi);
  EXPECT_EQ(GA::Inner(trivector, bivector), trivector | bivector);
  EXPECT_EQ(GA::Inner(trivector, vector), trivector | vector);
  EXPECT_EQ(GA::Inner(trivector, motor), trivector | motor);

  EXPECT_EQ(GA::Inner(bivector, multi), bivector | multi);
  EXPECT_EQ(GA::Inner(bivector, trivector), bivector | trivector);
  EXPECT_EQ(GA::Inner(bivector, vector), bivector | vector);
  EXPECT_EQ(GA::Inner(bivector, motor), bivector | motor);

  EXPECT_EQ(GA::Inner(vector, multi), vector | multi);
  EXPECT_EQ(GA::Inner(vector, trivector), vector | trivector);
  EXPECT_EQ(GA::Inner(vector, bivector), vector | bivector);
  EXPECT_EQ(GA::Inner(vector, motor), vector | motor);

  EXPECT_EQ(GA::Inner(motor, multi), motor | multi);
  EXPECT_EQ(GA::Inner(motor, trivector), motor | trivector);
  EXPECT_EQ(GA::Inner(motor, bivector), motor | bivector);
  EXPECT_EQ(GA::Inner(motor, vector), motor | vector);

  // Test Outer product
  EXPECT_EQ(GA::Outer(multi, bivector), multi ^ bivector);
  EXPECT_EQ(GA::Outer(multi, vector), multi ^ vector);
  EXPECT_EQ(GA::Outer(multi, motor), multi ^ motor);

  EXPECT_EQ(GA::Outer(trivector, multi), trivector ^ multi);
  EXPECT_EQ(GA::Outer(trivector, bivector), trivector ^ bivector);
  EXPECT_EQ(GA::Outer(trivector, vector), trivector ^ vector);
  EXPECT_EQ(GA::Outer(trivector, motor), trivector ^ motor);

  EXPECT_EQ(GA::Outer(bivector, multi), bivector ^ multi);
  EXPECT_EQ(GA::Outer(bivector, trivector), bivector ^ trivector);
  EXPECT_EQ(GA::Outer(bivector, vector), bivector ^ vector);
  EXPECT_EQ(GA::Outer(bivector, motor), bivector ^ motor);

  EXPECT_EQ(GA::Outer(vector, multi), vector ^ multi);
  EXPECT_EQ(GA::Outer(vector, trivector), vector ^ trivector);
  EXPECT_EQ(GA::Outer(vector, bivector), vector ^ bivector);
  EXPECT_EQ(GA::Outer(vector, motor), vector ^ motor);

  EXPECT_EQ(GA::Outer(motor, multi), motor ^ multi);
  EXPECT_EQ(GA::Outer(motor, trivector), motor ^ trivector);
  EXPECT_EQ(GA::Outer(motor, bivector), motor ^ bivector);
  EXPECT_EQ(GA::Outer(motor, vector), motor ^ vector);

  // Test Join
  EXPECT_EQ(GA::Join(trivector, bivector), trivector & bivector);
  EXPECT_EQ(GA::Join(trivector, vector), trivector & vector);
  EXPECT_EQ(GA::Join(bivector, trivector), bivector & trivector);
  EXPECT_EQ(GA::Join(bivector, vector), bivector & vector);
  EXPECT_EQ(GA::Join(vector, trivector), vector & trivector);
  EXPECT_EQ(GA::Join(vector, bivector), vector & bivector);

  // Test Inverse
  EXPECT_EQ(GA::Inverse(multi), ~multi);
  EXPECT_EQ(GA::Inverse(trivector), ~trivector);
  EXPECT_EQ(GA::Inverse(bivector), ~bivector);
  EXPECT_EQ(GA::Inverse(vector), ~vector);
  EXPECT_EQ(GA::Inverse(motor), ~motor);

  // Test Gexp
  EXPECT_EQ(GA::Gexp(trivector), trivector.Gexp());
  EXPECT_EQ(GA::Gexp(vector), vector.Gexp());
}

// GAElement
TEST_F(ElementsTest, GAElementToString) {
  const MultiVector multi{-2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5.2f, -10};
  const TriVector three{5, 0, -3, 2.5f};
  const BiVector two{1, 2, 0, 0, 0, 0};
  const Vector one{-1, 2.5f, 0, 0};
  const Motor motor{1, 3, 0, 0, 0, 0, 0, -1.0f};

  EXPECT_EQ(multi.ToString(), "-2 + 5.2*e123 - 10*e0123");
  EXPECT_EQ(three.ToString(), "5*e032 - 3*e021 + 2.5*e123");
  EXPECT_EQ(two.ToString(), "e01 + 2*e02");
  EXPECT_EQ(one.ToString(), "-e0 + 2.5*e1");
  EXPECT_EQ(motor.ToString(), "1 + 3*e01 - e0123");
  EXPECT_EQ(GANull{}.ToString(), "GANull");
}

TEST_F(ElementsTest, GAElementOStreamOutput) {
  MultiVector multi{-2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5.2f, -10};

  std::ostringstream oss1;
  oss1 << multi;
  std::string output1 = oss1.str();

  std::ostringstream oss2;
  oss2 << GANull{};
  std::string output2 = oss2.str();

  EXPECT_EQ(output1, "-2 + 5.2*e123 - 10*e0123");
  EXPECT_EQ(output2, "GANull");
}

TEST_F(ElementsTest, GAElementPlus) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};

  const MultiVector res1{a + b};
  const MultiVector res2{b + Elements::MultiVectorA()};

  const MultiVector correct{5,  7,  9,  11, 13, 15, 17, 19,
                            21, 23, 25, 27, 29, 31, 33, 35};

  EXPECT_EQ(res1, correct);
  EXPECT_EQ(res2, correct);
}

TEST_F(ElementsTest, GAElementPlusEqual) {
  MultiVector a{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};

  a += b;

  const MultiVector correct{5,  7,  9,  11, 13, 15, 17, 19,
                            21, 23, 25, 27, 29, 31, 33, 35};

  EXPECT_EQ(a, correct);
}

TEST_F(ElementsTest, GAElementMinus) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};

  const MultiVector res1{a - b};
  const MultiVector res2{b - Elements::MultiVectorA()};

  const MultiVector correct1{-1, -1, -1, -1, -1, -1, -1, -1,
                             -1, -1, -1, -1, -1, -1, -1, -1};
  const MultiVector correct2{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
}

TEST_F(ElementsTest, GAElementMinusEqual) {
  MultiVector a{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};

  a -= b;

  const MultiVector correct{-1, -1, -1, -1, -1, -1, -1, -1,
                            -1, -1, -1, -1, -1, -1, -1, -1};

  EXPECT_EQ(a, correct);
}

TEST_F(ElementsTest, GAElementProxyReferencing) {
  MultiVector multiVector{Elements::MultiVectorA()};
  TriVector TriVector{Elements::TriVectorA()};
  BiVector BiVector{Elements::BiVectorA()};
  Vector Vector{Elements::VectorA()};
  Motor motor{Elements::MotorA()};

  EXPECT_EQ(multiVector.s(), multiVector[0]);
  EXPECT_EQ(multiVector.e0(), multiVector[1]);

  EXPECT_EQ(TriVector.e032(), TriVector[0]);
  EXPECT_EQ(TriVector.e123(), TriVector[3]);

  EXPECT_EQ(BiVector.e01(), BiVector[0]);
  EXPECT_EQ(BiVector.e23(), BiVector[3]);

  EXPECT_EQ(Vector.e0(), Vector[0]);
  EXPECT_EQ(Vector.e3(), Vector[3]);

  EXPECT_EQ(motor.s(), motor[0]);
  EXPECT_EQ(motor.e0123(), motor[7]);
}

// MultiVector
TEST_F(ElementsTest, MultiVectorInverse) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector mult1{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  EXPECT_TRUE(mult1.RoundedEqual(a * ~a));
  EXPECT_TRUE(mult1.RoundedEqual(b * ~b));
  EXPECT_TRUE(mult1.RoundedEqual(c * ~c));
}

TEST_F(ElementsTest, MultiVectorEqual) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};

  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a == aDuplicate);
  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorRoundedEqual) {
  MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};

  EXPECT_FALSE(a.RoundedEqual(b, 0.00001f));
  EXPECT_TRUE(a.RoundedEqual(aDuplicate, 0.00001f));

  a[4] += 0.0000001f;
  EXPECT_TRUE(a.RoundedEqual(aDuplicate, 0.00001f));

  a[5] += 0.001f;
  EXPECT_FALSE(a.RoundedEqual(aDuplicate, 0.00001f));
}

TEST_F(ElementsTest, MultiVectorScalarProduct) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};

  const MultiVector res1{2.f * a};
  const MultiVector res2{a * 2.f};

  const MultiVector correct{4,  6,  8,  10, 12, 14, 16, 18,
                            20, 22, 24, 26, 28, 30, 32, 34};

  EXPECT_EQ(res1, correct);
  EXPECT_EQ(res2, correct);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorScalarDivision) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};

  const MultiVector res1{a / 2.f};

  const MultiVector correct{1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5,
                            5, 5.5, 6, 6.5, 7, 7.5, 8, 8.5};

  EXPECT_EQ(res1, correct);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorMultiVectorGeometricProduct) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector bDuplicate{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};
  const MultiVector res3{b * a};

  const MultiVector correct1{-572, 1028, -322, -356, -378, -314, -404, -374,
                             200,  238,  276,  -14,  -4,   -18,  464,  636};
  const MultiVector correct2{388, 492, 664,  -8,  152,  -918, -478, -274,
                             -4,  4,   -516, 666, -226, 350,  -246, 24};
  const MultiVector correct3{-572, 1012, -326, -348, -382, -310, -388, -370,
                             200,  238,  276,  -10,  12,   -14,  464,  716};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);

  EXPECT_EQ(a, aDuplicate);
  EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorTriVectorGeometricProduct) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const TriVector b{Elements::TriVectorB()};
  const TriVector c{Elements::TriVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{-96, 32, -60, -66, -72, 29,  22, 9,
                             24,  30, 36,  -43, -26, -51, 12, 80};
  const MultiVector correct2{-112, -176, -70, -77, -84, 215,  18, 161,
                             28,   35,   42,  69,  -58, -169, 14, -6};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorBiVectorGeometricProduct) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const BiVector b{Elements::BiVectorB()};
  const BiVector c{Elements::BiVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{-233, 234, -94, -116, -126, -104, -95, -134,
                             8,    22,  12,  24,   55,   50,   107, 304};
  const MultiVector correct2{-90,  141, -205, 122,  -77, -145, 134, -147,
                             -181, -10, 175,  -391, 116, 119,  42,  9};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorVectorGeometricProduct) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{77, 128, 2,  22,   6,    9,    -18, 9,
                             64, 80,  96, -101, -112, -141, 167, -260};
  const MultiVector correct2{32,  42,  -139, -2,   129, 219, -14, -103,
                             151, -94, 63,   -155, 150, 11,  68,  -22};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorMotorGeometricProduct) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const Motor b{Elements::MotorB()};
  const Motor c{Elements::MotorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{-260, 430, -97, -119, -123, -196, -200, -240,
                             41,   55,  51,  116,  160,  168,  170,  432};
  const MultiVector correct2{93,  4,   216,  -156, 60,   20,   -306, 4,
                             162, -30, -246, 442,  -132, -130, -111, -52};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorWedge) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector bDuplicate{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a ^ b};
  const MultiVector res2{a ^ c};
  const MultiVector res3{b ^ a};

  const MultiVector correct1{6,  17, 22, 27,  32, 36,  40,  44,
                             51, 59, 61, -12, 4,  -16, 464, 636};
  const MultiVector correct2{-8,   -2,  -28, -6,  -40,  -48,  -56,  -68,
                             -146, -22, -18, 206, -174, -226, -246, 24};
  const MultiVector correct3{6,  17, 22, 27,  32, 38,  44,  50,
                             53, 55, 63, -12, 4,  -16, 464, 716};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);

  EXPECT_EQ(a, aDuplicate);
  EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorDot) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector bDuplicate{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a | b};
  const MultiVector res2{a | c};
  const MultiVector res3{b | a};

  const MultiVector correct1{-572, 1028, -322, -356, -378, -312, -396, -372,
                             200,  238,  276,  54,   60,   66,   82,   87};
  const MultiVector correct2{388,  492, 664,  -8,  152,  -394, -490, 310,
                             -232, 4,   -312, 156, -112, 224,  -100, -30};
  const MultiVector correct3{-572, 1012, -326, -348, -382, -312, -396, -372,
                             200,  238,  276,  80,   84,   88,   82,   87};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);

  EXPECT_EQ(a, aDuplicate);
  EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorJoin) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector bDuplicate{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a & b};
  const MultiVector res2{a & c};
  const MultiVector res3{b & a};

  const MultiVector correct1{716, 864, -84, -100, -80, 261, 299, 331,
                             370, 404, 438, 472,  507, 542, 577, 306};
  const MultiVector correct2{-72, 414, -814, 510, 626, 764, -14, -60,
                             460, 426, 532,  502, -6,  574, -2,  323};
  const MultiVector correct3{636, 864, -84, -100, -80, 263, 295, 333,
                             364, 400, 436, 472,  507, 542, 577, 306};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);

  EXPECT_EQ(a, aDuplicate);
  EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorNorm) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector c{Elements::MultiVectorC()};

  const float res1{a.Norm()};
  const float res2{c.Norm()};

  EXPECT_NEAR(res1, 26.4953f, 0.0001f);
  EXPECT_NEAR(res2, 31.5911f, 0.0001f);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorVNorm) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};
  const MultiVector c{Elements::MultiVectorC()};

  const float res1{a.VNorm()};
  const float res2{c.VNorm()};

  EXPECT_NEAR(res1, 32.8938f, 0.0001f);
  EXPECT_NEAR(res2, 38.1838f, 0.0001f);

  EXPECT_EQ(a, aDuplicate);
}

// Vector
TEST_F(ElementsTest, VectorMultiVectorGeometricProduct) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{74, -104, 17, -4, 23, 9,  -26, 5,
                             49, 72,   83, 27, 58, 59, 146, 216};
  const MultiVector correct2{-30,  -50, 109, 2,   -107, -175, -30, 67,
                             -121, -78, -45, 175, 62,   57,   -54, 30};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorTriVectorGeometricProduct) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const TriVector b{Elements::TriVectorB()};
  const TriVector c{Elements::TriVectorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};

  const Motor correct1{0, 0, 0, 0, 18, 24, 30, 62};
  const Motor correct2{0, 49, 2, -31, 21, 28, 35, -8};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorMotorGeometricProduct) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const Motor b{Elements::MotorB()};
  const Motor c{Elements::MotorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{0, -56, 13, 4,  19, 0,  0,  0,
                             0, 0,   0,  15, 26, 31, 98, 0};
  const MultiVector correct2{0, -34, 73, -6,  -79, 0,  0,   0,
                             0, 0,   0,  107, 30,  37, -38, 0};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorWedgeMultiVector) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a ^ b};
  const MultiVector res2{a ^ c};

  const MultiVector correct1{0,  6, 9,  12,  15,  -2,  -4,  -6,
                             -2, 4, -2, -27, -14, -31, 146, 216};
  const MultiVector correct2{0,   -8, -12, -16, -20, -27, -6,  -41,
                             -67, -6, 45,  118, -14, -38, -54, 30};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorWedgeBiVector) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const BiVector b{Elements::BiVectorB()};
  const BiVector c{Elements::BiVectorC()};

  const TriVector res1{a ^ b};
  const TriVector res2{a ^ c};

  const TriVector correct1{-12, -14, -16, 86};
  const TriVector correct2{-63, 14, 13, 34};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorWedgeVector) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};

  const BiVector res1{a ^ b};
  const BiVector res2{a ^ c};

  const BiVector correct1{-1, -2, -3, -1, 2, -1};
  const BiVector correct2{22, 4, 34, 58, 4, -38};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorGepVector) {
  const Vector a{Elements::VectorA()};
  const Vector aDuplicate{Elements::VectorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};

  const Motor correct1{62, -1, -2, -3, -1, 2, -1, 0};
  const Motor correct2{26, 22, 4, 34, 58, 4, -38, 0};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorGexp) {
  const Vector a{Elements::VectorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};

  const MultiVector res1{a.Gexp()};
  const MultiVector res2{b.Gexp()};
  const MultiVector res3{c.Gexp()};

  const MultiVector correct1{588.7027, 166.51, 249.7651, 333.0201, 416.2751};
  const MultiVector correct2{3235.1068, 1106.0239, 1474.6985, 1843.3729,
                             2212.0477};
  const MultiVector correct3{17942.75, -6843.0956, 8553.8695, -10264.6434,
                             11975.4173};

  EXPECT_TRUE(res1.RoundedEqual(correct1, 0.001f));
  EXPECT_TRUE(res2.RoundedEqual(correct2, 0.001f));
  EXPECT_TRUE(res3.RoundedEqual(correct3, 0.001f));
}

// TriVector
TEST_F(ElementsTest, TriVectorJoinTriVector) {
  const TriVector a{Elements::TriVectorA()};
  const TriVector aDuplicate{Elements::TriVectorA()};
  const TriVector b{Elements::TriVectorB()};
  const TriVector c{Elements::TriVectorC()};

  const BiVector res1{a & b};
  const BiVector res2{a & c};

  const BiVector correct1{-1, 2, -1, 3, 2, 1};
  const BiVector correct2{-38, -4, 22, -34, 4, -58};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  // Practical case
  const TriVector pointA{TriVector::Point(100, 150, 1)};
  const TriVector pointB{TriVector::Point(150, 150, 1)};

  const BiVector line{pointA & pointB};

  const BiVector expectedLine{0, 50, -7500, 50, 0, 0};

  EXPECT_EQ(line, expectedLine);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorJoinBiVector) {
  const TriVector a{Elements::TriVectorA()};
  const TriVector aDuplicate{Elements::TriVectorA()};
  const BiVector b{Elements::BiVectorB()};
  const BiVector c{Elements::BiVectorC()};

  const Vector res1{a & b};
  const Vector res2{a & c};

  const Vector correct1{38, -19, -12, -29};
  const Vector correct2{-17, 79, -15, -7};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorJoinPlane) {
  const TriVector a{1, 2, 3, 1};
  const Vector m{-4, float(std::sqrt(3) / 4), 1 / 2.f, 3 / 4.f};

  EXPECT_NEAR(a & m, -0.3169, 0.0001);
}

TEST_F(ElementsTest, TriVectorMultiVectorGeometricProduct) {
  const TriVector a{Elements::TriVectorA()};
  const TriVector aDuplicate{Elements::TriVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{-85, 200, -55, -60, -65, -39, -18, -15,
                             25,  30,  35,  55,  36,  71,  15,  -76};
  const MultiVector correct2{90,  54, 60,  -65, 70,  -163, 18,  -125,
                             -30, 35, -40, 131, -42, -23,  -20, -2};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorTriVectorGeometricProduct) {
  const TriVector a{Elements::TriVectorA()};
  const TriVector aDuplicate{Elements::TriVectorA()};
  const TriVector b{Elements::TriVectorB()};
  const TriVector c{Elements::TriVectorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};

  const Motor correct1{-30, -3, -2, -1, 0, 0, 0, 0};
  const Motor correct2{-35, 34, -4, 58, 0, 0, 0, 0};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorGexp) {
  const TriVector a{Elements::TriVectorA()};
  const TriVector c{Elements::TriVectorC()};

  const MultiVector res1{a.Gexp()};
  const MultiVector res2{c.Gexp()};

  const MultiVector correct1{0.28366, 0,        0,        0,        0,
                             0,       0,        0,        0,        0,
                             0,       -0.38357, -0.57535, -0.76714, -0.95892};
  const MultiVector correct2{0.7539, 0,        0,       0,        0,
                             0,      0,        0,       0,        0,
                             0,      -0.37542, 0.46928, -0.56313, 0.65699};

  EXPECT_TRUE(res1.RoundedEqual(correct1, 0.001f));
  EXPECT_TRUE(res2.RoundedEqual(correct2, 0.001f));
}

// BiVector
TEST_F(ElementsTest, BiVectorMultiVectorGeometricProduct) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector aDuplicate{Elements::BiVectorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{-218, 328, -85, -102, -119, -72, -123, -102,
                             21,   6,   27,  -42,  -99,  -84, 110,  274};
  const MultiVector correct2{80,  75,  187,  110,  55, -145, -114, -107,
                             155, -10, -165, -151, 12, 175,  -44,  21};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorMotorGeometricProduct) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector aDuplicate{Elements::BiVectorA()};
  const Motor b{Elements::MotorB()};
  const Motor c{Elements::MotorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};

  const Motor correct1{-146, -40, -59, -54, 17, 14, 23, 166};
  const Motor correct2{56, -81, -66, -75, 103, -18, -121, 9};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorBiVectorGeometricProduct) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector aDuplicate{Elements::BiVectorA()};
  const BiVector b{Elements::BiVectorB()};
  const BiVector c{Elements::BiVectorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};

  const Motor correct1{-128, 2, -4, 2, 1, -2, 1, 139};
  const Motor correct2{-50, 12, -12, -12, -110, -4, 82, -6};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorVectorGeometricProduct) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector aDuplicate{Elements::BiVectorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{0, 47, -1, 2,   -1,  0,   0,  0,
                             0, 0,  0,  -13, -22, -19, 92, 0};
  const MultiVector correct2{0, 20, -84, 0,   60, 0,  0,  0,
                             0, 0,  0,   -25, 18, 55, 38, 0};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorWedgeVector) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector aDuplicate{Elements::BiVectorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};

  const TriVector res1{a ^ b};
  const TriVector res2{a ^ c};

  const TriVector correct1{-13, -22, -19, 92};
  const TriVector correct2{-25, 18, 55, 38};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorGexp) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector c{Elements::BiVectorC()};

  const Motor res1{a.Gexp()};
  const Motor res2{c.Gexp()};

  const Motor correct1{-0.48598, -1.1916,  -1.4799,  -1.7682,
                       -0.41665, -0.49998, -0.58331, -4.6665};
  const Motor correct2{0.20727, -0.88415, 1.0406,  -1.197,
                       0.49166, -0.56189, 0.63213, -8.5689};

  EXPECT_TRUE(res1.RoundedEqual(correct1, 0.001f));
  EXPECT_TRUE(res2.RoundedEqual(correct2, 0.001f));

  // A line at infinity squares to 0: exp(B) = 1 + B, a translator.
  const BiVector ideal{1, -2, 3, 0, 0, 0};
  EXPECT_EQ(ideal.Gexp(), Motor(1, 1, -2, 3, 0, 0, 0, 0));
  EXPECT_EQ(BiVector{}.Gexp(), Motor(1, 0, 0, 0, 0, 0, 0, 0));
}

// Motor
TEST_F(ElementsTest, MotorMultiVectorGeometricProduct) {
  const Motor a{Elements::MotorA()};
  const Motor aDuplicate{Elements::MotorA()};
  const MultiVector b{Elements::MultiVectorB()};
  const MultiVector bDuplicate{Elements::MultiVectorB()};
  const MultiVector c{Elements::MultiVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{-248, 246, -93, -105, -123, -172, -224, -216,
                             45,   35,  55,  -82,  -140, -138, 162,  400};
  const MultiVector correct2{85,  256, 208,  140, 44,  -36, -274, 12,
                             154, 10,  -222, -72, -70, 312, -87,  20};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
  EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MotorMotorGeometricProduct) {
  const Motor a{Elements::MotorA()};
  const Motor aDuplicate{Elements::MotorA()};
  const Motor b{Elements::MotorB()};
  const Motor bDuplicate{Elements::MotorB()};
  const Motor c{Elements::MotorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};
  const Motor res3{b * a};

  const Motor correct1{-164, -104, -124, -132, 33, 35, 43, 252};
  const Motor correct2{57, -8, -174, 8, 102, -6, -162, -8};
  const Motor correct3{-164, -108, -116, -136, 31, 39, 41, 252};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);

  EXPECT_EQ(a, aDuplicate);
  EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MotorTriVectorGeometricProduct) {
  const Motor a{Elements::MotorA()};
  const Motor aDuplicate{Elements::MotorA()};
  const TriVector b{Elements::TriVectorB()};
  const TriVector c{Elements::TriVectorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};

  const MultiVector correct1{0, 32, -36, -42, -48, 0,   0,  0,
                             0, 0,  0,   -15, -10, -23, 12, 0};
  const MultiVector correct2{0, -100, -42, -49, -56, 0,    0,  0,
                             0, 0,    0,   53,  -22, -105, 14, 0};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);

  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MotorBiVectorGeometricProduct) {
  const Motor a{Elements::MotorA()};
  const Motor aDuplicate{Elements::MotorA()};
  const BiVector b{Elements::BiVectorB()};
  const BiVector c{Elements::BiVectorC()};
  const Motor motorC{Elements::MotorC()};

  const Motor res1{a * b};
  const Motor res2{a * c};
  const Motor res3{motorC * b};

  const Motor correct1{-149, -48, -55, -62, 12, 14, 16, 172};
  const Motor correct2{-58, -65, 70, -99, -113, -18, 115, -3};
  const Motor correct3{65, -66, -105, -120, -166, -32, 78, 6};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);
  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MotorVectorGeometricProduct) {
  const Motor a{Elements::MotorA()};
  const Motor aDuplicate{Elements::MotorA()};
  const Vector b{Elements::VectorB()};
  const Vector c{Elements::VectorC()};
  const Motor motorC{Elements::MotorC()};

  const MultiVector res1{a * b};
  const MultiVector res2{a * c};
  const MultiVector res3{motorC * c};

  const MultiVector correct1{0, 68, 6, 14,  10,  0,   0,   0,
                             0, 0,  0, -53, -68, -77, 107, 0};
  const MultiVector correct2{0, 18, -87, -10, 85, 0, 0,  0,
                             0, 0,  0,   -79, 78, 7, 44, 0};
  const MultiVector correct3{0, 126, -23, 18,  -31, 0,    0,    0,
                             0, 0,   0,   -87, 102, -117, -164, 0};

  EXPECT_EQ(res1, correct1);
  EXPECT_EQ(res2, correct2);
  EXPECT_EQ(res3, correct3);
  EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, Translation) {
  const BiVector a{Elements::BiVectorA()};
  const BiVector aDuplicate{Elements::BiVectorA()};
  const BiVector b{Elements::BiVectorB()};

  const Motor translation1 = Motor::Translation(7.0f, a);
  BiVector result1{((translation1 * b) * ~translation1).Grade2()};
  BiVector correct1{-2.1994f, 14.399f, -0.19956f, 6, 7, 8};

  EXPECT_TRUE(result1.RoundedEqual(correct1, 0.0001f));
  EXPECT_EQ(a, aDuplicate);
}

// GANull
TEST_F(ElementsTest, GANullOperations) {
  const MultiVector a{Elements::MultiVectorA()};
  const MultiVector aDuplicate{Elements::MultiVectorA()};

  const GAElement res1{GANull{} * a};
  const GAElement res2{a * GANull{}};

  const GAElement res3{GANull{} | a};
  const GAElement res4{a | GANull{}};

  const GAElement res5{GANull{} ^ a};
  const GAElement res6{a ^ GANull{}};

  const GAElement res7{GANull{} & a};
  const GAElement res8{a & GANull{}};

  const GAElement correct{GANull{}};

  EXPECT_EQ(res1, correct);
  EXPECT_EQ(res2, correct);

  EXPECT_EQ(res3, correct);
  EXPECT_EQ(res4, correct);

  EXPECT_EQ(res5, correct);
  EXPECT_EQ(res6, correct);

  EXPECT_EQ(res7, correct);
  EXPECT_EQ(res8, correct);

  EXPECT_EQ(a, aDuplicate);
}

// PseudoScalar
TEST_F(ElementsTest, PseudoScalarBasics) {
  PseudoScalar a{Elements::PseudoScalarA()};
  const PseudoScalar b{Elements::PseudoScalarB()};

  EXPECT_EQ(a.e0123(), a[0]);
  EXPECT_EQ(a.e0123(), 2.0f);

  EXPECT_EQ(a + b, PseudoScalar{5});
  EXPECT_EQ(a - b, PseudoScalar{-1});
  EXPECT_EQ(-a, PseudoScalar{-2});
  EXPECT_EQ(a * 3.0f, PseudoScalar{6});
  EXPECT_EQ(3.0f * a, PseudoScalar{6});
  EXPECT_EQ(a / 2.0f, PseudoScalar{1});

  a += b;
  EXPECT_EQ(a, PseudoScalar{5});
  a -= b;
  EXPECT_EQ(a, PseudoScalar{2});

  EXPECT_EQ(a.Norm(), 0.0f);
  EXPECT_EQ(Elements::PseudoScalarC().VNorm(), 4.0f);

  EXPECT_EQ(PseudoScalar{}.ToString(), "0");
  EXPECT_EQ(PseudoScalar{1}.ToString(), "e0123");
  EXPECT_EQ(PseudoScalar{-10}.ToString(), "-10*e0123");
  EXPECT_EQ(Elements::PseudoScalarC().ToString(), "-4*e0123");
}

TEST_F(ElementsTest, PseudoScalarGradeExtraction) {
  EXPECT_EQ(Elements::MultiVectorA().Grade4(), PseudoScalar{17});
  EXPECT_EQ(Elements::MotorA().Grade4(), PseudoScalar{9});

  MultiVector multi{Elements::MultiVectorA()};
  multi = Elements::PseudoScalarA();
  EXPECT_EQ(multi,
            MultiVector(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2));

  Motor motor{Elements::MotorA()};
  motor += Elements::PseudoScalarA();
  EXPECT_EQ(motor, Motor(2, 3, 4, 5, 6, 7, 8, 11));
  motor -= Elements::PseudoScalarB();
  EXPECT_EQ(motor, Motor(2, 3, 4, 5, 6, 7, 8, 8));
}

TEST_F(ElementsTest, PseudoScalarGeometricProduct) {
  const PseudoScalar ps{Elements::PseudoScalarA()};

  // e1 * e0123 = e032 and e0123 * e1 = -e032
  EXPECT_EQ(Vector(0, 1, 0, 0) * PseudoScalar{1}, TriVector(1, 0, 0, 0));
  EXPECT_EQ(PseudoScalar{1} * Vector(0, 1, 0, 0), TriVector(-1, 0, 0, 0));
  // e123 * e0123 = e0 and e0123 * e123 = -e0
  EXPECT_EQ(TriVector(0, 0, 0, 1) * PseudoScalar{1}, Vector(1, 0, 0, 0));
  EXPECT_EQ(PseudoScalar{1} * TriVector(0, 0, 0, 1), Vector(-1, 0, 0, 0));
  // e23 * e0123 = e0123 * e23 = -e01
  EXPECT_EQ(BiVector(0, 0, 0, 1, 0, 0) * PseudoScalar{1},
            BiVector(-1, 0, 0, 0, 0, 0));
  EXPECT_EQ(PseudoScalar{1} * BiVector(0, 0, 0, 1, 0, 0),
            BiVector(-1, 0, 0, 0, 0, 0));

  EXPECT_EQ(ps * Elements::VectorA(), TriVector(-6, -8, -10, 0));
  EXPECT_EQ(Elements::VectorA() * ps, TriVector(6, 8, 10, 0));
  EXPECT_EQ(ps * Elements::BiVectorA(), BiVector(-10, -12, -14, 0, 0, 0));
  EXPECT_EQ(Elements::BiVectorA() * ps, BiVector(-10, -12, -14, 0, 0, 0));
  EXPECT_EQ(ps * Elements::TriVectorA(), Vector(-10, 0, 0, 0));
  EXPECT_EQ(Elements::TriVectorA() * ps, Vector(10, 0, 0, 0));
  EXPECT_EQ(ps * Elements::MotorA(), Motor(0, -12, -14, -16, 0, 0, 0, 4));
  EXPECT_EQ(Elements::MotorA() * ps, Motor(0, -12, -14, -16, 0, 0, 0, 4));

  // e0123 squares to zero
  EXPECT_EQ(ps * Elements::PseudoScalarB(), GANull{});
}

TEST_F(ElementsTest, PseudoScalarProductsMatchMultiVector) {
  const PseudoScalar pss[]{Elements::PseudoScalarA(),
                           Elements::PseudoScalarB(),
                           Elements::PseudoScalarC()};

  for (const PseudoScalar &ps : pss) {
    for (const MultiVector &x :
         {Elements::MultiVectorA(), Elements::MultiVectorB(),
          Elements::MultiVectorC()}) {
      ExpectProductsMatchMultiVector(ps, x);
      ExpectProductsMatchMultiVector(x, ps);
    }
    for (const TriVector &x : {Elements::TriVectorA(), Elements::TriVectorB(),
                               Elements::TriVectorC()}) {
      ExpectProductsMatchMultiVector(ps, x);
      ExpectProductsMatchMultiVector(x, ps);
    }
    for (const BiVector &x : {Elements::BiVectorA(), Elements::BiVectorB(),
                              Elements::BiVectorC()}) {
      ExpectProductsMatchMultiVector(ps, x);
      ExpectProductsMatchMultiVector(x, ps);
    }
    for (const Vector &x :
         {Elements::VectorA(), Elements::VectorB(), Elements::VectorC()}) {
      ExpectProductsMatchMultiVector(ps, x);
      ExpectProductsMatchMultiVector(x, ps);
    }
    for (const Motor &x :
         {Elements::MotorA(), Elements::MotorB(), Elements::MotorC()}) {
      ExpectProductsMatchMultiVector(ps, x);
      ExpectProductsMatchMultiVector(x, ps);
    }
    for (const PseudoScalar &x : pss) {
      ExpectProductsMatchMultiVector(ps, x);
    }
  }
}

TEST_F(ElementsTest, PseudoScalarJoinIsIdentity) {
  const PseudoScalar one{1};

  EXPECT_EQ(one & Elements::MultiVectorA(), Elements::MultiVectorA());
  EXPECT_EQ(Elements::MultiVectorA() & one, Elements::MultiVectorA());
  EXPECT_EQ(one & Elements::TriVectorA(), Elements::TriVectorA());
  EXPECT_EQ(Elements::TriVectorA() & one, Elements::TriVectorA());
  EXPECT_EQ(one & Elements::BiVectorA(), Elements::BiVectorA());
  EXPECT_EQ(Elements::BiVectorA() & one, Elements::BiVectorA());
  EXPECT_EQ(one & Elements::VectorA(), Elements::VectorA());
  EXPECT_EQ(Elements::VectorA() & one, Elements::VectorA());
  EXPECT_EQ(one & Elements::MotorA(), Elements::MotorA());
  EXPECT_EQ(Elements::MotorA() & one, Elements::MotorA());
  EXPECT_EQ(one & Elements::PseudoScalarB(), Elements::PseudoScalarB());

  EXPECT_EQ(Elements::PseudoScalarA() & Elements::VectorA(),
            Elements::VectorA() * 2.0f);
}

TEST_F(ElementsTest, PseudoScalarWedge) {
  const PseudoScalar ps{Elements::PseudoScalarA()};

  EXPECT_EQ(ps ^ Elements::MotorA(), PseudoScalar{4});
  EXPECT_EQ(Elements::MotorA() ^ ps, PseudoScalar{4});
  EXPECT_EQ(ps ^ Elements::VectorA(), GANull{});
  EXPECT_EQ(Elements::BiVectorA() ^ ps, GANull{});

  // Grade 4 results of other wedges are now pseudoscalars
  const Vector v{Elements::VectorA()};
  const BiVector b{Elements::BiVectorA()};
  const BiVector c{Elements::BiVectorB()};
  const TriVector t{Elements::TriVectorB()};

  EXPECT_EQ(ToMV(v ^ t), ToMV(v) ^ ToMV(t));
  EXPECT_EQ(ToMV(t ^ v), ToMV(t) ^ ToMV(v));
  EXPECT_EQ(ToMV(b ^ c), ToMV(b) ^ ToMV(c));
  EXPECT_EQ(v ^ t, -(t ^ v));
  EXPECT_EQ(b ^ c, c ^ b);
}

TEST_F(ElementsTest, PseudoScalarHodgeDual) {
  const PseudoScalar ps{Elements::PseudoScalarC()};

  EXPECT_EQ(!ps, -4.0f);
  EXPECT_EQ(MultiVector(!ps), !ToMV(ps));
  EXPECT_EQ(GA::HodgeDual(ps), -4.0f);
  EXPECT_EQ(GA::HodgeDual(-4.0f), ps);
  EXPECT_EQ(ToMV(GA::HodgeDual(-4.0f)), !MultiVector(-4.0f));
  EXPECT_EQ(GA::HodgeDual(GA::HodgeDual(ps)), ps);
}

TEST_F(ElementsTest, PseudoScalarGexp) {
  const PseudoScalar a{Elements::PseudoScalarA()};
  const PseudoScalar b{Elements::PseudoScalarB()};

  EXPECT_EQ(a.Gexp(), Motor(1, 0, 0, 0, 0, 0, 0, 2));
  EXPECT_EQ(PseudoScalar{}.Gexp(), Motor(1, 0, 0, 0, 0, 0, 0, 0));
  // e0123 commutes with itself, so exp(a) * exp(b) = exp(a + b)
  EXPECT_EQ(a.Gexp() * b.Gexp(), (a + b).Gexp());
}

TEST_F(ElementsTest, PseudoScalarNamedFunctions) {
  const PseudoScalar ps{Elements::PseudoScalarA()};
  const MultiVector multi{Elements::MultiVectorA()};
  const TriVector trivector{Elements::TriVectorA()};
  const BiVector bivector{Elements::BiVectorA()};
  const Vector vector{Elements::VectorA()};
  const Motor motor{Elements::MotorA()};

  EXPECT_EQ(GA::Gep(ps, multi), ps * multi);
  EXPECT_EQ(GA::Gep(trivector, ps), trivector * ps);
  EXPECT_EQ(GA::Inner(ps, bivector), ps | bivector);
  EXPECT_EQ(GA::Inner(vector, ps), vector | ps);
  EXPECT_EQ(GA::Outer(ps, motor), ps ^ motor);
  EXPECT_EQ(GA::Outer(multi, ps), multi ^ ps);
  EXPECT_EQ(GA::Join(ps, vector), ps & vector);
  EXPECT_EQ(GA::Join(motor, ps), motor & ps);
  EXPECT_EQ(GA::Gexp(ps), ps.Gexp());
}

// Fat dot inner product
TEST_F(ElementsTest, InnerProductBasisBladesAreFatDot) {
  // Grade of each MultiVector component: s, e0..e3, e01..e12, e032..e123, e0123
  constexpr int grade[16]{0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4};

  for (size_t i = 0; i < 16; ++i) {
    for (size_t j = 0; j < 16; ++j) {
      MultiVector a{}, b{};
      a[i] = 1;
      b[j] = 1;

      // Fat dot: <AB>_|r-s|, scalars included
      MultiVector expected{};
      const MultiVector gp{a * b};
      for (size_t k = 0; k < 16; ++k) {
        if (grade[k] == std::abs(grade[i] - grade[j])) {
          expected[k] = gp[k];
        }
      }

      EXPECT_EQ(a | b, expected) << MultiVector::names()[i] << " | "
                                 << MultiVector::names()[j];
    }
  }
}

TEST_F(ElementsTest, InnerProductWithScalarScales) {
  // With the fat dot a scalar contracts with everything: s | X = X | s = s * X
  const MultiVector scalar{5};
  const Motor scalarMotor{5, 0, 0, 0, 0, 0, 0, 0};

  for (const MultiVector &x : {Elements::MultiVectorA(),
                               Elements::MultiVectorC(), scalar}) {
    EXPECT_EQ(scalar | x, x * 5.0f);
    EXPECT_EQ(x | scalar, x * 5.0f);
    EXPECT_EQ(scalarMotor | x, x * 5.0f);
    EXPECT_EQ(x | scalarMotor, x * 5.0f);
  }
  EXPECT_EQ(scalarMotor | Elements::MotorA(), Elements::MotorA() * 5.0f);
  EXPECT_EQ(Elements::MotorA() | scalarMotor, Elements::MotorA() * 5.0f);
  EXPECT_EQ(ToMV(scalarMotor | Elements::BiVectorA()),
            ToMV(Elements::BiVectorA() * 5.0f));
  EXPECT_EQ(ToMV(Elements::BiVectorA() | scalarMotor),
            ToMV(Elements::BiVectorA() * 5.0f));
  EXPECT_EQ(ToMV(scalarMotor | Elements::VectorA()),
            ToMV(Elements::VectorA() * 5.0f));
  EXPECT_EQ(ToMV(Elements::VectorA() | scalarMotor),
            ToMV(Elements::VectorA() * 5.0f));
  EXPECT_EQ(ToMV(scalarMotor | Elements::TriVectorA()),
            ToMV(Elements::TriVectorA() * 5.0f));
  EXPECT_EQ(ToMV(Elements::TriVectorA() | scalarMotor),
            ToMV(Elements::TriVectorA() * 5.0f));
  EXPECT_EQ(ToMV(scalarMotor | Elements::PseudoScalarA()),
            ToMV(Elements::PseudoScalarA() * 5.0f));
  EXPECT_EQ(ToMV(Elements::PseudoScalarA() | scalarMotor),
            ToMV(Elements::PseudoScalarA() * 5.0f));

  // With e0123, the fat dot is the full geometric product
  EXPECT_EQ(Elements::PseudoScalarA() | Elements::MotorA(),
            Motor(0, -12, -14, -16, 0, 0, 0, 4));
  EXPECT_EQ(Elements::MotorA() | Elements::PseudoScalarA(),
            Motor(0, -12, -14, -16, 0, 0, 0, 4));
}

TEST_F(ElementsTest, InnerProductTypedMatchesMultiVector) {
  const MultiVector m{Elements::MultiVectorC()};
  const Vector v{Elements::VectorC()};
  const BiVector b{Elements::BiVectorC()};
  const TriVector t{Elements::TriVectorC()};
  const Motor mo{Elements::MotorC()};
  const PseudoScalar p{Elements::PseudoScalarC()};

  ExpectInnerMatchesMultiVectorForAll(Elements::MultiVectorA(), m, v, b, t, mo,
                                      p);
  ExpectInnerMatchesMultiVectorForAll(Elements::VectorA(), m, v, b, t, mo, p);
  ExpectInnerMatchesMultiVectorForAll(Elements::BiVectorA(), m, v, b, t, mo, p);
  ExpectInnerMatchesMultiVectorForAll(Elements::TriVectorA(), m, v, b, t, mo,
                                      p);
  ExpectInnerMatchesMultiVectorForAll(Elements::MotorA(), m, v, b, t, mo, p);
  ExpectInnerMatchesMultiVectorForAll(Elements::PseudoScalarA(), m, v, b, t,
                                      mo, p);
}

///////////////////////////////////////////////////////////////////////////////
/// Exponentials
///////////////////////////////////////////////////////////////////////////////

// Coefficient-wise comparison with a tolerance relative to the largest
// expected coefficient, for exponentials whose coefficients run into the
// thousands (float round-off then leaves small noise in the zero components).
template <typename T>
::testing::AssertionResult NearlyEqual(const T &actual, const T &expected,
                                       float relTolerance = 1e-4f) {
  float magnitude{1.0f};
  for (const float coefficient : expected) {
    magnitude = std::max(magnitude, std::fabs(coefficient));
  }
  const float bound = relTolerance * magnitude;
  const auto size = static_cast<size_t>(expected.end() - expected.begin());
  for (size_t i = 0; i < size; ++i) {
    if (std::fabs(actual[i] - expected[i]) > bound) {
      return ::testing::AssertionFailure()
             << actual << " differs from " << expected << " at index " << i;
    }
  }
  return ::testing::AssertionSuccess();
}

TEST_F(ElementsTest, VectorGexpOfIdealVector) {
  // e0 squares to 0, so exp(a*e0) = 1 + a*e0.
  EXPECT_EQ(Vector(1, 0, 0, 0).Gexp(), MultiVector(1, 1));
  EXPECT_EQ(Vector(-3, 0, 0, 0).Gexp(), MultiVector(1, -3));
  EXPECT_EQ(Vector{}.Gexp(), MultiVector{1.0f});
}

TEST_F(ElementsTest, TriVectorGexpOfIdealPoint) {
  // A point at infinity squares to 0, so exp(t) = 1 + t.
  const TriVector direction{TriVector::Direction(1, 2, 3)};
  EXPECT_EQ(direction.Gexp(),
            MultiVector(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 0, 0));
  EXPECT_EQ(TriVector{}.Gexp(), MultiVector{1.0f});
}

TEST_F(ElementsTest, MotorGexp) {
  // Pure bivector motor: same as the bivector exponential.
  const BiVector b{Elements::BiVectorA()};
  Motor bivectorMotor{};
  bivectorMotor += b;
  EXPECT_EQ(bivectorMotor.Gexp(), b.Gexp());

  // exp(s) = e^s, exp(p*e0123) = 1 + p*e0123.
  EXPECT_TRUE(Motor(2, 0, 0, 0, 0, 0, 0, 0)
                  .Gexp()
                  .RoundedEqual(Motor(std::exp(2.0f), 0, 0, 0, 0, 0, 0, 0)));
  EXPECT_EQ(Motor(0, 0, 0, 0, 0, 0, 0, 5).Gexp(),
            Motor(1, 0, 0, 0, 0, 0, 0, 5));

  // A general motor against the power series of the MultiVector.
  const Motor m{Elements::MotorC() * 0.25f};
  EXPECT_TRUE(NearlyEqual(ToMV(m.Gexp()), ToMV(m).Gexp()));

  // exp(-angle/2 * L) is the rotation around the normalized line L.
  const BiVector line{BiVector::LineFromPoints(1, 2, 3, 4, 6, 5)};
  const float angle{1.2f};
  EXPECT_TRUE(
      ((line.Normalized() * (-0.5f * angle)).Gexp())
          .RoundedEqual(Motor::Rotation(Angle::FromRadians(angle), line)));

  EXPECT_EQ(GA::Gexp(m), m.Gexp());
  EXPECT_EQ(GA::Gexp(b), b.Gexp());
}

TEST_F(ElementsTest, MultiVectorGexp) {
  EXPECT_TRUE(
      NearlyEqual(MultiVector{1.5f}.Gexp(), MultiVector{std::exp(1.5f)}));
  EXPECT_EQ(MultiVector{}.Gexp(), MultiVector{1.0f});

  // The series agrees with the closed forms of every typed exponential.
  for (const Vector &v :
       {Elements::VectorA(), Elements::VectorB(), Elements::VectorC(),
        Vector(1, 0, 0, 0)}) {
    EXPECT_TRUE(NearlyEqual(ToMV(v).Gexp(), v.Gexp()));
  }
  for (const BiVector &b :
       {Elements::BiVectorA(), Elements::BiVectorB(), Elements::BiVectorC(),
        BiVector(1, -2, 3, 0, 0, 0)}) {
    EXPECT_TRUE(NearlyEqual(ToMV(b).Gexp(), ToMV(b.Gexp())));
  }
  for (const TriVector &t :
       {Elements::TriVectorA(), Elements::TriVectorC(),
        TriVector::Direction(1, 2, 3)}) {
    EXPECT_TRUE(NearlyEqual(ToMV(t).Gexp(), t.Gexp()));
  }
  EXPECT_TRUE(NearlyEqual(ToMV(Elements::PseudoScalarA()).Gexp(),
                          ToMV(Elements::PseudoScalarA().Gexp())));

  const MultiVector m{Elements::MultiVectorC() * 0.1f};
  EXPECT_EQ(GA::Gexp(m), m.Gexp());
}

///////////////////////////////////////////////////////////////////////////////
/// Scalar division
///////////////////////////////////////////////////////////////////////////////

template <typename T>
concept ScalarDivisible = requires(float s, const T &t) { s / t; };

static_assert(ScalarDivisible<Vector>);
static_assert(ScalarDivisible<MultiVector>);
// Not invertible, so dividing by them must not compile.
static_assert(!ScalarDivisible<PseudoScalar>);
static_assert(!ScalarDivisible<GANull>);

TEST_F(ElementsTest, ScalarDividedByElement) {
  const Vector v{Elements::VectorA()};
  const BiVector b{Elements::BiVectorA()};
  const TriVector t{Elements::TriVectorA()};
  const Motor m{Elements::MotorA()};
  const MultiVector mv{Elements::MultiVectorA()};

  EXPECT_EQ(2.0f / v, 2.0f * ~v);
  EXPECT_EQ(2.0f / b, 2.0f * ~b);
  EXPECT_EQ(2.0f / t, 2.0f * ~t);
  EXPECT_EQ(2.0f / m, 2.0f * ~m);
  EXPECT_EQ(2.0f / mv, 2.0f * ~mv);

  // 1 / v really is the inverse.
  EXPECT_TRUE(ToMV(v * (1.0f / v)).RoundedEqual(MultiVector{1.0f}));
}

///////////////////////////////////////////////////////////////////////////////
/// Constructors and factories
///////////////////////////////////////////////////////////////////////////////

// A lone float no longer converts to a MultiVector implicitly.
static_assert(!std::is_convertible_v<float, MultiVector>);
static_assert(std::is_constructible_v<MultiVector, float>);
template <typename A, typename B>
concept Wedgeable = requires(const A &a, const B &b) { a ^ b; };
static_assert(!Wedgeable<MultiVector, float>);
static_assert(!Wedgeable<float, MultiVector>);

// The 3-float point constructor is gone (one argument away from the 4-float
// one, which means something else): use TriVector::Point(x, y, z).
static_assert(!std::is_constructible_v<TriVector, float, float, float>);

TEST_F(ElementsTest, ElementFactories) {
  EXPECT_EQ(TriVector::Point(1, 2, 3), TriVector(1, 2, 3, 1));
  EXPECT_EQ(TriVector::Direction(1, 2, 3), TriVector(1, 2, 3, 0));

  // x + 2y + 3z - 4 = 0
  const Vector plane{Vector::Plane(1, 2, 3, -4)};
  EXPECT_EQ(plane, Vector(-4, 1, 2, 3));
  // (1, 0, 1) lies on the plane, (1, 1, 1) doesn't.
  EXPECT_EQ(plane ^ TriVector::Point(1, 0, 1), PseudoScalar{0});
  EXPECT_NE(plane ^ TriVector::Point(1, 1, 1), PseudoScalar{0});

  const TriVector p{TriVector::Point(1, 2, 3)};
  const TriVector q{TriVector::Point(4, 6, 5)};
  EXPECT_EQ(BiVector::LineFromPoints(p, q),
            BiVector::LineFromPoints(1, 2, 3, 4, 6, 5));
  EXPECT_EQ(BiVector::LineFromPoints(p, q), p & q);
}

///////////////////////////////////////////////////////////////////////////////
/// Motor factories
///////////////////////////////////////////////////////////////////////////////

TEST_F(ElementsTest, AngleUnits) {
  EXPECT_FLOAT_EQ(Angle::FromDegrees(180).Radians(), std::numbers::pi_v<float>);
  EXPECT_FLOAT_EQ(Angle::FromRadians(std::numbers::pi_v<float>).Degrees(), 180);
  EXPECT_EQ(90_deg, Angle::FromDegrees(90));
  EXPECT_EQ(90.0_deg, Angle::FromDegrees(90));
  EXPECT_EQ(1.5_rad, Angle::FromRadians(1.5f));
  EXPECT_EQ(-(1_rad), Angle::FromRadians(-1));

  // A plain float is not an angle: the unit has to be spelled out.
  static_assert(!std::is_convertible_v<float, Angle>);
}

TEST_F(ElementsTest, MotorTranslationByVector) {
  const Motor t{Motor::Translation(1, -2, 3)};
  const TriVector p{TriVector::Point(4, 5, 6)};
  EXPECT_EQ((t * p * ~t).Grade3(), TriVector::Point(5, 3, 9));

  // Directions are not affected by translations.
  const TriVector d{TriVector::Direction(1, 2, 3)};
  EXPECT_EQ((t * d * ~t).Grade3(), d);

  // Same as the exponential of the line at infinity -1/2 (dx e01 + ...).
  EXPECT_EQ(t, BiVector(-0.5f, 1, -1.5f, 0, 0, 0).Gexp());
  // Same as the line-based translation along e01.
  EXPECT_TRUE(Motor::Translation(7, 0, 0).RoundedEqual(
      Motor::Translation(7.0f, BiVector{1, 0, 0, 0, 0, 0})));
}

TEST_F(ElementsTest, MotorRotationTakesAngle) {
  const BiVector zAxis{BiVector::LineFromPoints(0, 0, 0, 0, 0, 1)};
  const TriVector p{TriVector::Point(1, 0, 0)};

  const Motor r{Motor::Rotation(90_deg, zAxis)};
  EXPECT_TRUE((r * p * ~r).Grade3().RoundedEqual(TriVector::Point(0, 1, 0)));
  EXPECT_EQ(r, Motor::Rotation(Angle::FromDegrees(90), zAxis));
  EXPECT_TRUE(r.RoundedEqual(
      Motor::Rotation(Angle::FromRadians(std::numbers::pi_v<float> / 2), zAxis)));

  // Rotation around a line that doesn't pass through the origin.
  const BiVector shifted{BiVector::LineFromPoints(1, 1, 0, 1, 1, 1)};
  const Motor r2{Motor::Rotation(180_deg, shifted)};
  EXPECT_TRUE((r2 * TriVector::Point(2, 1, 5) * ~r2)
                  .Grade3()
                  .RoundedEqual(TriVector::Point(0, 1, 5), 1e-5f));
}

///////////////////////////////////////////////////////////////////////////////
/// GANull
///////////////////////////////////////////////////////////////////////////////

// The catch-all operators only accept the library's types and numbers.
template <typename A, typename B>
concept Multipliable = requires(const A &a, const B &b) { a * b; };
static_assert(!Multipliable<GANull, std::string>);
static_assert(!Multipliable<std::string, GANull>);
static_assert(!Wedgeable<std::string, GANull>);
// No conversion to float, so GANull can't sneak into float arithmetic.
static_assert(!std::is_convertible_v<GANull, float>);
static_assert(std::is_same_v<decltype(GANull{} * 2.0f), GANull>);
static_assert(std::is_same_v<decltype(GANull{} * GANull{}), GANull>);
static_assert(std::is_same_v<decltype(Vector{} & GANull{}), GANull>);

TEST_F(ElementsTest, GANullConvertsToZero) {
  const Vector v = Elements::VectorA() & Elements::BiVectorA();
  EXPECT_EQ(v, Vector{});
  const BiVector b = GANull{};
  EXPECT_EQ(b, BiVector{});
  const TriVector t = Elements::TriVectorA() ^ Elements::TriVectorB();
  EXPECT_EQ(t, TriVector{});
  const Motor m = GANull{};
  EXPECT_EQ(m, Motor{});
  const PseudoScalar p = Elements::PseudoScalarA() * Elements::PseudoScalarB();
  EXPECT_EQ(p, PseudoScalar{});

  MultiVector mv{Elements::MultiVectorA()};
  mv = GANull{};
  EXPECT_EQ(mv, MultiVector{});
  const MultiVector mv2 = GANull{};
  EXPECT_EQ(mv2, MultiVector{});
}

///////////////////////////////////////////////////////////////////////////////
/// Asserts on ideal elements (only active without NDEBUG)
///////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
TEST(FlyFishDeathTest, NormalizeIdealElementAsserts) {
  EXPECT_DEATH((void)TriVector::Direction(1, 2, 3).Normalized(),
               "Euclidean norm is 0");
  const auto normalizeIdealLine = [] {
    BiVector ideal{1, 2, 3, 0, 0, 0};
    ideal.Normalize();
  };
  EXPECT_DEATH(normalizeIdealLine(), "Euclidean norm is 0");
  EXPECT_DEATH((void)Elements::PseudoScalarA().Normalized(),
               "Euclidean norm is 0");
  EXPECT_DEATH((void)Vector(1, 0, 0, 0).Normalized(), "Euclidean norm is 0");
}

TEST(FlyFishDeathTest, MotorFactoriesAssertOnDegenerateLines) {
  EXPECT_DEATH((void)Motor::Rotation(90_deg, BiVector{1, 0, 0, 0, 0, 0}),
               "line is ideal");
  EXPECT_DEATH((void)Motor::Translation(1.0f, BiVector{0, 0, 0, 1, 0, 0}),
               "no ideal part");
}
#endif

TEST_F(ElementsTest, NormalizeEuclideanElements) {
  EXPECT_FLOAT_EQ(TriVector(2, 4, 6, 2).Normalized().Norm(), 1);
  EXPECT_FLOAT_EQ(Elements::BiVectorA().Normalized().Norm(), 1);
  Vector v{Elements::VectorA()};
  v.Normalize();
  EXPECT_FLOAT_EQ(v.Norm(), 1);
}

///////////////////////////////////////////////////////////////////////////////
/// Inverse of non-simple bivectors and un-normalized motors
///////////////////////////////////////////////////////////////////////////////

TEST_F(ElementsTest, BiVectorInverseOfScrew) {
  // None of these is a single line (B ^ B != 0), so B^2 is not a scalar.
  for (const BiVector &b :
       {Elements::BiVectorA(), Elements::BiVectorB(), Elements::BiVectorC()}) {
    EXPECT_NE(b ^ b, PseudoScalar{0});
    EXPECT_TRUE((b * ~b).RoundedEqual(Motor(1, 0, 0, 0, 0, 0, 0, 0)));
    EXPECT_TRUE((~b * b).RoundedEqual(Motor(1, 0, 0, 0, 0, 0, 0, 0)));
    EXPECT_TRUE(ToMV(~b).RoundedEqual(~ToMV(b)));
  }

  // For an actual line it is still -B / |B|^2.
  const BiVector line{BiVector::LineFromPoints(1, 2, 3, 4, 6, 5)};
  EXPECT_TRUE((~line).RoundedEqual(-line / (line.Norm() * line.Norm())));
}

TEST_F(ElementsTest, MotorInverseOfGeneralMotor) {
  for (const Motor &m :
       {Elements::MotorA(), Elements::MotorB(), Elements::MotorC()}) {
    EXPECT_TRUE((m * ~m).RoundedEqual(Motor(1, 0, 0, 0, 0, 0, 0, 0)));
    EXPECT_TRUE((~m * m).RoundedEqual(Motor(1, 0, 0, 0, 0, 0, 0, 0)));
    EXPECT_TRUE(ToMV(~m).RoundedEqual(~ToMV(m)));
  }

  // For a normalized motor the inverse is the reverse.
  const Motor r{Motor::Translation(1, 2, 3) *
                Motor::Rotation(40_deg, BiVector::LineFromPoints(1, 2, 3, 4, 6, 5))};
  EXPECT_TRUE((~r).RoundedEqual(
      Motor(r.s(), -r.e01(), -r.e02(), -r.e03(), -r.e23(), -r.e31(), -r.e12(),
            r.e0123())));
}

///////////////////////////////////////////////////////////////////////////////
/// Conversions to MultiVector and sums of different types
///////////////////////////////////////////////////////////////////////////////

static_assert(std::is_convertible_v<Vector, MultiVector>);
static_assert(std::is_convertible_v<BiVector, MultiVector>);
static_assert(std::is_convertible_v<TriVector, MultiVector>);
static_assert(std::is_convertible_v<Motor, MultiVector>);
static_assert(std::is_convertible_v<PseudoScalar, MultiVector>);
static_assert(std::is_convertible_v<GANull, MultiVector>);

// Even elements add up to a Motor, everything else to a MultiVector.
static_assert(std::is_same_v<decltype(1.0f + BiVector{}), Motor>);
static_assert(std::is_same_v<decltype(BiVector{} + 1), Motor>);
static_assert(std::is_same_v<decltype(BiVector{} + PseudoScalar{}), Motor>);
static_assert(std::is_same_v<decltype(Motor{} - BiVector{}), Motor>);
static_assert(std::is_same_v<decltype(2.0f - Motor{}), Motor>);
static_assert(std::is_same_v<decltype(Vector{} + BiVector{}), MultiVector>);
static_assert(std::is_same_v<decltype(TriVector{} - Vector{}), MultiVector>);
static_assert(std::is_same_v<decltype(1.0f + Vector{}), MultiVector>);
static_assert(std::is_same_v<decltype(Motor{} + Vector{}), MultiVector>);
static_assert(std::is_same_v<decltype(MultiVector{} + Vector{}), MultiVector>);
// Same types keep their type, zero keeps the other operand's type.
static_assert(std::is_same_v<decltype(Vector{} + Vector{}), Vector>);
static_assert(std::is_same_v<decltype(Vector{} + GANull{}), Vector>);
static_assert(std::is_same_v<decltype(GANull{} - BiVector{}), BiVector>);
// Plain numbers are left alone.
static_assert(std::is_same_v<decltype(1.0f + 2.0f), float>);

TEST_F(ElementsTest, MultiVectorFromElements) {
  const TriVector t{Elements::TriVectorA()};
  const MultiVector direct{t};
  const MultiVector copy = t;
  MultiVector assigned{};
  assigned = t;
  EXPECT_EQ(direct, assigned);
  EXPECT_EQ(copy, assigned);
  EXPECT_EQ(direct.Grade3(), t);

  EXPECT_EQ(MultiVector{Elements::VectorA()}.Grade1(), Elements::VectorA());
  EXPECT_EQ(MultiVector{Elements::BiVectorA()}.Grade2(), Elements::BiVectorA());
  EXPECT_EQ(MultiVector{Elements::MotorA()}.ToMotor(), Elements::MotorA());
  EXPECT_EQ(MultiVector{Elements::PseudoScalarA()}.Grade4(),
            Elements::PseudoScalarA());
  EXPECT_EQ(MultiVector{GANull{}}, MultiVector{});
}

TEST_F(ElementsTest, SumsOfDifferentTypes) {
  const Vector v{Elements::VectorA()};
  const BiVector b{Elements::BiVectorA()};
  const TriVector t{Elements::TriVectorA()};
  const Motor m{Elements::MotorA()};
  const PseudoScalar p{Elements::PseudoScalarA()};

  EXPECT_EQ(1.0f + b, Motor(1, 2, 3, 4, 5, 6, 7, 0));
  EXPECT_EQ(b + 1.0f, Motor(1, 2, 3, 4, 5, 6, 7, 0));
  EXPECT_EQ(1 - b, Motor(1, -2, -3, -4, -5, -6, -7, 0));
  EXPECT_EQ(b + p, Motor(0, 2, 3, 4, 5, 6, 7, 2));
  EXPECT_EQ(m - b, Motor(2, 1, 1, 1, 1, 1, 1, 9));
  EXPECT_EQ(m + p, Motor(2, 3, 4, 5, 6, 7, 8, 11));
  EXPECT_EQ(3.0f + m, Motor(5, 3, 4, 5, 6, 7, 8, 9));

  EXPECT_EQ(v + b,
            MultiVector(0, 2, 3, 4, 5, 2, 3, 4, 5, 6, 7, 0, 0, 0, 0, 0));
  EXPECT_EQ(v - t,
            MultiVector(0, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, -2, -3, -4, -5, 0));
  EXPECT_EQ(1.0f + v, MultiVector(1, 2, 3, 4, 5));
  EXPECT_EQ(v + b + t + m + p, ToMV(v) + ToMV(b) + ToMV(t) + ToMV(m) + ToMV(p));
  EXPECT_EQ(Elements::MultiVectorA() - v,
            Elements::MultiVectorA() - ToMV(v));

  EXPECT_EQ(v + GANull{}, v);
  EXPECT_EQ(GANull{} - v, -v);

  // A translator written the textbook way: 1 - d/2 * e01.
  EXPECT_EQ(1.0f - BiVector(1, 0, 0, 0, 0, 0) * 3.5f,
            Motor::Translation(7, 0, 0));
}

///////////////////////////////////////////////////////////////////////////////
/// Printing and comparing float results
///////////////////////////////////////////////////////////////////////////////

TEST_F(ElementsTest, ToStringRoundsLikeItPrints) {
  // 0.9999999 prints as "1", so it must not show up as "1*e013".
  EXPECT_EQ(TriVector(0.9999999f, 1.0000001f, -0.9999999f, 1).ToString(),
            "e032 + e013 - e021 + e123");
  EXPECT_EQ(Motor(0.9999999f, 0, 0, 0, 0, 0, 0, 0).ToString(), "1");
  EXPECT_EQ(Motor(-1.0000001f, 2, 0, 0, 0, 0, 0, 0).ToString(), "-1 + 2*e01");
  EXPECT_EQ(Vector(1.5f, -1, 0, 0.999f).ToString(), "1.5*e0 - e1 + 0.999*e3");

  // The situation students actually hit: a point after a rotation.
  const Motor r{Motor::Rotation(90_deg, BiVector::LineFromPoints(0, 0, 0, 0, 0, 1))};
  const Motor m{Motor::Translation(1, 2, 3) * r};
  const TriVector moved{(m * TriVector::Point(1, 0, 0) * ~m).Grade3()};
  EXPECT_EQ(moved.ToString(), "e032 + 3*e013 + 3*e021 + e123");
}

TEST_F(ElementsTest, RoundedEqualScalesWithTheValues) {
  const Motor r{Motor::Rotation(90_deg, BiVector::LineFromPoints(0, 0, 0, 0, 0, 1))};

  // Exact == fails after float arithmetic, RoundedEqual is the way to compare.
  const TriVector moved{(r * TriVector::Point(1, 0, 0) * ~r).Grade3()};
  EXPECT_NE(moved, TriVector::Point(0, 1, 0));
  EXPECT_TRUE(moved.RoundedEqual(TriVector::Point(0, 1, 0)));

  // Large coordinates: the float error is far above 1e-5 in absolute terms.
  const TriVector far{(r * TriVector::Point(12345, 6789, 1000) * ~r).Grade3()};
  EXPECT_TRUE(far.RoundedEqual(TriVector::Point(-6789, 12345, 1000)));

  // Still a real comparison.
  EXPECT_FALSE(moved.RoundedEqual(TriVector::Point(0, 1.001f, 0)));
  EXPECT_FALSE(far.RoundedEqual(TriVector::Point(-6789, 12346, 1000)));
  EXPECT_TRUE(Vector(1, 2, 3, 4).RoundedEqual(Vector(1, 2, 3, 4.5f), 0.2f));
}

// Main entry point for Google Test
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
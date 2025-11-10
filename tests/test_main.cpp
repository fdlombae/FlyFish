//#include "../src/FlyFish.h"

#include "../src/FlyFish.h"

#include <gtest/gtest.h>
#include <cmath>
#include <string>
#include <sstream>

// Example class to be tested
class Elements {
public:
    Elements() = default;

    static MultiVector MultiVectorA()
    {
        return {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
    }
    static MultiVector MultiVectorB()
    {
        return {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
    }
    static MultiVector MultiVectorC()
    {
        return {-4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16, 17, -18, 19};
    }
    static Vector VectorA()
    {
        return {2, 3, 4, 5};
    }
    static Vector VectorB()
    {
        return {3, 4, 5, 6};
    }
    static Vector VectorC()
    {
        return {-4, 5, -6, 7};
    }

    static BiVector BiVectorA()
    {
        return {2, 3, 4, 5, 6, 7};
    }
    static BiVector BiVectorB()
    {
        return {3, 4, 5, 6, 7, 8};
    }
    static BiVector BiVectorC()
    {
        return {-4, 5, -6, 7, -8, 9};
    }

    static TriVector TriVectorA()
    {
        return {2, 3, 4, 5};
    }
    static TriVector TriVectorB()
    {
        return {3, 4, 5, 6};
    }
    static TriVector TriVectorC()
    {
        return {-4, 5, -6, 7};
    }

    static Motor MotorA()
    {
        return Motor{ 2, 3, 4, 5, 6, 7, 8, 9 };
    }
    static Motor MotorB()
    {
        return Motor{ 3, 4, 5, 6, 7, 8, 9, 10 };
    }
    static Motor MotorC()
    {
        return Motor{ -4, 5, -6, 7, -8, 9, -10, 11 };
    }
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

    Elements* elements = nullptr;  // Shared object for all tests
};

// GAElement
TEST_F(ElementsTest, GAElementToString) {
    const MultiVector multi{ -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5.2f, -10 };
    const TriVector three{ 5, 0, -3, 2.5f };
    const BiVector two{1, 2, 0, 0, 0, 0 };
    const Vector one{ -1, 2.5f, 0, 0 };
    const Motor motor{ 1, 3, 0, 0, 0, 0, 0, -1.0f };

    EXPECT_EQ(multi.ToString(), "-2 + 5.2*e123 - 10*e0123");
    EXPECT_EQ(three.ToString(), "5*e032 - 3*e021 + 2.5*e123");
    EXPECT_EQ(two.ToString(), "e01 + 2*e02");
    EXPECT_EQ(one.ToString(), "-e0 + 2.5*e1");
    EXPECT_EQ(motor.ToString(), "1 + 3*e01 - e0123");
    EXPECT_EQ(GANull{}.ToString(), "GANull");
}

TEST_F(ElementsTest, GAElementOStreamOutput) {
    MultiVector multi{ -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5.2f, -10 };

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
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };

    const MultiVector res1{ a + b };
    const MultiVector res2{ b + Elements::MultiVectorA() };

    const MultiVector correct{ 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35 };

    EXPECT_EQ(res1, correct);
    EXPECT_EQ(res2, correct);
}

TEST_F(ElementsTest, GAElementPlusEqual) {
    MultiVector a{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };

    a += b;

    const MultiVector correct{ 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35 };

    EXPECT_EQ(a, correct);
}

TEST_F(ElementsTest, GAElementMinus) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };

    const MultiVector res1{ a - b };
    const MultiVector res2{ b - Elements::MultiVectorA() };

    const MultiVector correct1{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
    const MultiVector correct2{ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
}

TEST_F(ElementsTest, GAElementMinusEqual) {
    MultiVector a{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };

    a -= b;

    const MultiVector correct{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    EXPECT_EQ(a, correct);
}

TEST_F(ElementsTest, GAElementProxyReferencing) {
    MultiVector multiVector{ Elements::MultiVectorA() };
    TriVector TriVector{ Elements::TriVectorA() };
    BiVector BiVector{ Elements::BiVectorA() };
    Vector Vector{ Elements::VectorA() };
    Motor motor{ Elements::MotorA() };
    
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
TEST_F(ElementsTest, MultiVectorInverse)
{
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector mult1{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    EXPECT_TRUE(mult1.RoundedEqual(a*~a));
    EXPECT_TRUE(mult1.RoundedEqual(b*~b));
    EXPECT_TRUE(mult1.RoundedEqual(c*~c));
}

TEST_F(ElementsTest, MultiVectorEqual) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a == aDuplicate);
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorRoundedEqual) {
    MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };

    EXPECT_FALSE(a.RoundedEqual(b, 0.00001f));
    EXPECT_TRUE(a.RoundedEqual(aDuplicate, 0.00001f));

    a[4] += 0.0000001f;
    EXPECT_TRUE(a.RoundedEqual(aDuplicate, 0.00001f));

    a[5] += 0.001f;
    EXPECT_FALSE(a.RoundedEqual(aDuplicate, 0.00001f));
}

TEST_F(ElementsTest, MultiVectorScalarProduct) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };

    const MultiVector res1{ 2.f * a };
    const MultiVector res2{ a * 2.f };

    const MultiVector correct{ 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34 };

    EXPECT_EQ(res1, correct);
    EXPECT_EQ(res2, correct);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorScalarDivision) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };

    const MultiVector res1{ a / 2.f };

    const MultiVector correct{ 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7, 7.5, 8, 8.5 };

    EXPECT_EQ(res1, correct);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorMultiVectorGeometricProduct) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector bDuplicate{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };
    const MultiVector res3{ b * a };

    const MultiVector correct1{ -572, 1028, -322, -356, -378, -314, -404, -374, 200, 238, 276, -14, -4, -18, 464, 636 };
    const MultiVector correct2{ 388, 492, 664, -8, 152, -918, -478, -274, -4, 4, -516, 666, -226, 350, -246, 24 };
    const MultiVector correct3{ -572, 1012, -326, -348, -382, -310, -388, -370, 200, 238, 276, -10, 12, -14, 464, 716 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorTriVectorGeometricProduct) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const TriVector b{ Elements::TriVectorB() };
    const TriVector c{ Elements::TriVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ -96, 32, -60, -66, -72, 29, 22, 9, 24, 30, 36, -43, -26, -51, 12, 80 };
    const MultiVector correct2{ -112, -176, -70, -77, -84, 215, 18, 161, 28, 35, 42, 69, -58, -169, 14, -6 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorBiVectorGeometricProduct) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const BiVector b{ Elements::BiVectorB() };
    const BiVector c{ Elements::BiVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ -233, 234, -94, -116, -126, -104, -95, -134, 8, 22, 12, 24, 55, 50, 107, 304 };
    const MultiVector correct2{ -90, 141, -205, 122, -77, -145, 134, -147, -181, -10, 175, -391, 116, 119, 42, 9 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorVectorGeometricProduct) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const Vector b{ Elements::VectorB() };
    const Vector c{ Elements::VectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ 77, 128, 2, 22, 6, 9, -18, 9, 64, 80, 96, -101, -112, -141, 167, -260 };
    const MultiVector correct2{ 32, 42, -139, -2, 129, 219, -14, -103, 151, -94, 63, -155, 150, 11, 68, -22 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorMotorGeometricProduct) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const Motor b{ Elements::MotorB() };
    const Motor c{ Elements::MotorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ -260, 430, -97, -119, -123, -196, -200, -240, 41, 55, 51, 116, 160, 168, 170, 432 };
    const MultiVector correct2{ 93, 4, 216, -156, 60, 20, -306, 4, 162, -30, -246, 442, -132, -130, -111, -52 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorWedge) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector bDuplicate{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a ^ b };
    const MultiVector res2{ a ^ c };
    const MultiVector res3{ b ^ a };

    const MultiVector correct1{ 6, 17, 22, 27, 32, 36, 40, 44, 51, 59, 61, -12, 4, -16, 464, 636 };
    const MultiVector correct2{ -8, -2, -28, -6, -40, -48, -56, -68, -146, -22, -18, 206, -174, -226, -246, 24 };
    const MultiVector correct3{ 6, 17, 22, 27, 32, 38, 44, 50, 53, 55, 63, -12, 4, -16, 464, 716 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorDot) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector bDuplicate{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a | b };
    const MultiVector res2{ a | c };
    const MultiVector res3{ b | a };

    const MultiVector correct1{ -572, 1028, -322, -356, -378, -312, -396, -372, 200, 238, 276, 54, 60, 66, 82, 87 };
    const MultiVector correct2{ 388, 492, 664, -8, 152, -394, -490, 310, -232, 4, -312, 156, -112, 224, -100, -30 };
    const MultiVector correct3{ -572, 1012, -326, -348, -382, -312, -396, -372, 200, 238, 276, 80, 84, 88, 82, 87 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorJoin) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector bDuplicate{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a & b };
    const MultiVector res2{ a & c };
    const MultiVector res3{ b & a };

    const MultiVector correct1{ 636, -620, 398, 484, 534, 261, 299, 331, 370, 404, 438, 472, 507, 542, 577, 306 };
    const MultiVector correct2{ 24, -130, 762, -82, -670, 764, -14, -60, 460, 426, 532, 502, -6, 574, -2, 323 };
    const MultiVector correct3{ 716, -620, 398, 484, 534, 263, 295, 333, 364, 400, 436, 472, 507, 542, 577, 306 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorNorm) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector c{ Elements::MultiVectorC() };

    const float res1{ a.Norm() };
    const float res2{ c.Norm() };

    EXPECT_NEAR(res1, 26.4953f, 0.0001f);
    EXPECT_NEAR(res2, 31.5911f, 0.0001f);
    
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorVNorm) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };
    const MultiVector c{ Elements::MultiVectorC() };

    const float res1{ a.VNorm() };
    const float res2{ c.VNorm() };

    EXPECT_NEAR(res1, 32.8938f, 0.0001f);
    EXPECT_NEAR(res2, 38.1838f, 0.0001f);

    EXPECT_EQ(a, aDuplicate);
}

// Vector
TEST_F(ElementsTest, VectorMultiVectorGeometricProduct) {
    const Vector a{ Elements::VectorA() };
    const Vector aDuplicate{ Elements::VectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ 74, -104, 17, -4, 23, 9, -26, 5, 49, 72, 83, 27, 58, 59, 146, 216 };
    const MultiVector correct2{ -30, -50, 109, 2, -107, -175, -30, 67, -121, -78, -45, 175, 62, 57, -54, 30 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorTriVectorGeometricProduct) {
    const Vector a{ Elements::VectorA() };
    const Vector aDuplicate{ Elements::VectorA() };
    const TriVector b{ Elements::TriVectorB() };
    const TriVector c{ Elements::TriVectorC() };

    const Motor res1{ a * b };
    const Motor res2{ a * c };

    const Motor correct1{ 0, 0, 0, 0, 18, 24, 30, 62 };
    const Motor correct2{ 0, 49, 2, -31, 21, 28, 35, -8 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorMotorGeometricProduct) {
    const Vector a{ Elements::VectorA() };
    const Vector aDuplicate{ Elements::VectorA() };
    const Motor b{ Elements::MotorB() };
    const Motor c{ Elements::MotorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ 0, -56, 13, 4, 19, 0, 0, 0, 0, 0, 0, 15, 26, 31, 98, 0 };
    const MultiVector correct2{ 0, -34, 73, -6, -79, 0, 0, 0, 0, 0, 0, 107, 30, 37, -38, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorWedgeMultiVector) {
    const Vector a{ Elements::VectorA() };
    const Vector aDuplicate{ Elements::VectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a ^ b };
    const MultiVector res2{ a ^ c };

    const MultiVector correct1{ 0, 6, 9, 12, 15, -2, -4, -6, -2, 4, -2, -27, -14, -31, 146, 216 };
    const MultiVector correct2{ 0, -8, -12, -16, -20, -27, -6, -41, -67, -6, 45, 118, -14, -38, -54, 30 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorWedgeBiVector) {
    const Vector a{ Elements::VectorA() };
    const Vector aDuplicate{ Elements::VectorA() };
    const BiVector b{ Elements::BiVectorB() };
    const BiVector c{ Elements::BiVectorC() };

    const TriVector res1{ a ^ b };
    const TriVector res2{ a ^ c };

    const TriVector correct1{ -12, -14, -16, 86 };
    const TriVector correct2{ -63, 14, 13, 34 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorWedgeVector) {
    const Vector a{ Elements::VectorA() };
    const Vector aDuplicate{ Elements::VectorA() };
    const Vector b{ Elements::VectorB() };
    const Vector c{ Elements::VectorC() };

    const BiVector res1{ a ^ b };
    const BiVector res2{ a ^ c };

    const BiVector correct1{ -1, -2, -3, -1, 2, -1 };
    const BiVector correct2{ 22, 4, 34, 58, 4, -38 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, VectorGexp) {
    const Vector a{ Elements::VectorA() };
    const Vector b{ Elements::VectorB() };
    const Vector c{ Elements::VectorC() };

    const MultiVector res1{ a.Gexp()};
    const MultiVector res2{ b.Gexp() };
    const MultiVector res3{ c.Gexp() };

    const MultiVector correct1{ 588.7027, 166.51, 249.7651, 333.0201, 416.2751};
    const MultiVector correct2{ 3235.1068, 1106.0239, 1474.6985, 1843.3729, 2212.0477 };
    const MultiVector correct3{ 17942.75 , -6843.0956, 8553.8695, -10264.6434, 11975.4173 };

    EXPECT_TRUE(res1.RoundedEqual(correct1, 0.001f));
    EXPECT_TRUE(res2.RoundedEqual(correct2, 0.001f));
    EXPECT_TRUE(res3.RoundedEqual(correct3, 0.001f));
}

// TriVector
TEST_F(ElementsTest, TriVectorJoinTriVector) {
    const TriVector a{ Elements::TriVectorA() };
    const TriVector aDuplicate{ Elements::TriVectorA() };
    const TriVector b{ Elements::TriVectorB() };
    const TriVector c{ Elements::TriVectorC() };

    const BiVector res1{ a & b };
    const BiVector res2{ a & c };

    const BiVector correct1{ -1, 2, -1, 3, 2, 1 };
    const BiVector correct2{ -38, -4, 22, -34, 4, -58 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    // Practical case
    const TriVector pointA{ 100, 150, 1 };
    const TriVector pointB{ 150, 150, 1 };

    const BiVector line{ pointA & pointB };

    const BiVector expectedLine{ 0, 50, -7500, 50, 0, 0 };

    EXPECT_EQ(line, expectedLine);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorJoinBiVector) {
    const TriVector a{ Elements::TriVectorA() };
    const TriVector aDuplicate{ Elements::TriVectorA() };
    const BiVector b{ Elements::BiVectorB() };
    const BiVector c{ Elements::BiVectorC() };

    const Vector res1{ a & b };
    const Vector res2{ a & c };

    const Vector correct1{ -38, 19, 12, 29 };
    const Vector correct2{ 17, -79, 15, 7 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorJoinPlane)
{
    const TriVector a{ 1, 2, 3, 1};
    const Vector m{-4, float(std::sqrt(3) / 4), 1/2.f, 3/4.f};

    float v = a & m;

    EXPECT_NEAR(a & m, -0.3169, 0.0001);
}

TEST_F(ElementsTest, TriVectorMultiVectorGeometricProduct) {
    const TriVector a{ Elements::TriVectorA() };
    const TriVector aDuplicate{ Elements::TriVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ -85, 200, -55, -60, -65, -39, -18, -15, 25, 30, 35, 55, 36, 71, 15, -76 };
    const MultiVector correct2{ 90, 54, 60, -65, 70, -163, 18, -125, -30, 35, -40, 131, -42, -23, -20, -2 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorTriVectorGeometricProduct) {
    const TriVector a{ Elements::TriVectorA() };
    const TriVector aDuplicate{ Elements::TriVectorA() };
    const TriVector b{ Elements::TriVectorB() };
    const TriVector c{ Elements::TriVectorC() };

    const Motor res1{ a * b };
    const Motor res2{ a * c };

    const Motor correct1{ -30, -3, -2, -1, 0, 0, 0, 0 };
    const Motor correct2{ -35, 34, -4, 58, 0, 0, 0, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TriVectorGexp) {
    const TriVector a{ Elements::TriVectorA() };
    const TriVector c{ Elements::TriVectorC() };

    const MultiVector res1{ a.Gexp()};
    const MultiVector res2{ c.Gexp() };

    const MultiVector correct1{ 0.28366, 0, 0, 0, 0, 0, 0, 0 ,0 ,0, 0, -0.38357, -0.57535, -0.76714, -0.95892};
    const MultiVector correct2{ 0.7539, 0, 0, 0, 0, 0, 0, 0 ,0 ,0, 0, -0.37542, 0.46928, -0.56313, 0.65699};

    EXPECT_TRUE(res1.RoundedEqual(correct1, 0.001f));
    EXPECT_TRUE(res2.RoundedEqual(correct2, 0.001f));
}

// BiVector
TEST_F(ElementsTest, BiVectorMultiVectorGeometricProduct) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ -218, 328, -85, -102, -119, -72, -123, -102, 21, 6, 27, -42, -99, -84, 110, 274 };
    const MultiVector correct2{ 80, 75, 187, 110, 55, -145, -114, -107, 155, -10, -165, -151, 12, 175, -44, 21 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorMotorGeometricProduct) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const Motor b{ Elements::MotorB() };
    const Motor c{ Elements::MotorC() };

    const Motor res1{ a * b };
    const Motor res2{ a * c };

    const Motor correct1{ -146, -40, -59, -54, 17, 14, 23, 166 };
    const Motor correct2{ 56, -81, -66, -75, 103, -18, -121, 9 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorBiVectorGeometricProduct) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const BiVector b{ Elements::BiVectorB() };
    const BiVector c{ Elements::BiVectorC() };

    const Motor res1{ a * b };
    const Motor res2{ a * c };

    const Motor correct1{ -128, 2, -4, 2, 1, -2, 1, 139 };
    const Motor correct2{ -50, 12, -12, -12, -110, -4, 82, -6 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorVectorGeometricProduct) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const Vector b{ Elements::VectorB() };
    const Vector c{ Elements::VectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ 0, 47, -1, 2, -1, 0, 0, 0, 0, 0, 0, -13, -22, -19, 92, 0 };
    const MultiVector correct2{ 0, 20, -84, 0, 60, 0, 0, 0, 0, 0, 0, -25, 18, 55, 38, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, BiVectorWedgeVector) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const Vector b{ Elements::VectorB() };
    const Vector c{ Elements::VectorC() };

    const TriVector res1{ a ^ b };
    const TriVector res2{ a ^ c };

    const TriVector correct1{ -13, -22, -19, 92 };
    const TriVector correct2{ -25, 18, 55, 38 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

/*
TEST_F(ElementsTest, BiVectorGexp) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector c{ Elements::BiVectorC() };

    const Motor res1{ a.Gexp()};
    const Motor res2{ c.Gexp() };

    const Motor correct1{ -0.48598, -1.1916, -1.4799, -1.7682, -0.41665, -0.49998, -0.58331, -4.6665};
    const Motor correct2{ 0.20727, -0.88415, 1.0406, -1.197, 0.49166, -0.56189,0.63213, -8.5689};

    EXPECT_TRUE(res1.RoundedEqual(correct1, 0.001f));
    EXPECT_TRUE(res2.RoundedEqual(correct2, 0.001f));
}
*/

// Motor
TEST_F(ElementsTest, MotorMultiVectorGeometricProduct) {
    const Motor a{ Elements::MotorA() };
    const Motor aDuplicate{ Elements::MotorA() };
    const MultiVector b{ Elements::MultiVectorB() };
    const MultiVector bDuplicate{ Elements::MultiVectorB() };
    const MultiVector c{ Elements::MultiVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ -248, 246, -93, -105, -123, -172, -224, -216, 45, 35, 55, -82, -140, -138, 162, 400};
    const MultiVector correct2{ 85, 256, 208, 140, 44, -36, -274, 12, 154, 10, -222, -72, -70, 312, -87, 20 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MotorMotorGeometricProduct) {
    const Motor a{ Elements::MotorA() };
    const Motor aDuplicate{ Elements::MotorA() };
    const Motor b{ Elements::MotorB() };
    const Motor bDuplicate{ Elements::MotorB() };
    const Motor c{ Elements::MotorC() };

    const Motor res1{ a * b };
    const Motor res2{ a * c };
    const Motor res3{ b * a };

    const Motor correct1{ -164, -104, -124, -132, 33, 35, 43, 252 };
    const Motor correct2{ 57, -8, -174, 8, 102, -6, -162, -8 };
    const Motor correct3{ -164, -108, -116, -136, 31, 39, 41, 252 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MotorTriVectorGeometricProduct) {
    const Motor a{ Elements::MotorA() };
    const Motor aDuplicate{ Elements::MotorA() };
    const TriVector b{ Elements::TriVectorB() };
    const TriVector c{ Elements::TriVectorC() };

    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };

    const MultiVector correct1{ 0, 32, -36, -42, -48, 0, 0, 0, 0, 0, 0, -15, -10, -23, 12, 0 };
    const MultiVector correct2{ 0, -100, -42, -49, -56, 0, 0, 0, 0, 0, 0, 53, -22, -105, 14, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MotorBiVectorGeometricProduct) {
    const Motor a{ Elements::MotorA() };
    const Motor aDuplicate{ Elements::MotorA() };
    const BiVector b{ Elements::BiVectorB() };
    const BiVector c{ Elements::BiVectorC() };
    const Motor motorC{ Elements::MotorC() };


    const Motor res1{ a * b };
    const Motor res2{ a * c };
    const Motor res3{ motorC * b };

    const Motor correct1{ -149, -48, -55, -62, 12, 14, 16, 172 };
    const Motor correct2{ -58, -65, 70, -99, -113, -18, 115, -3 };
    const Motor correct3{ 65, -66, -105, -120, -166, -32, 78, 6 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MotorVectorGeometricProduct) {
    const Motor a{ Elements::MotorA() };
    const Motor aDuplicate{ Elements::MotorA() };
    const Vector b{ Elements::VectorB() };
    const Vector c{ Elements::VectorC() };
    const Motor motorC{ Elements::MotorC() };


    const MultiVector res1{ a * b };
    const MultiVector res2{ a * c };
    const MultiVector res3{ motorC * c };

    const MultiVector correct1{ 0, 68, 6, 14, 10, 0, 0, 0, 0, 0, 0, -53, -68, -77, 107, 0 };
    const MultiVector correct2{ 0, 18, -87, -10, 85, 0, 0, 0, 0, 0, 0, -79, 78, 7, 44, 0 };
    const MultiVector correct3{ 0, 126, -23, 18, -31, 0, 0, 0, 0, 0, 0, -87, 102, -117, -164, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, Rotation) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const BiVector b{ Elements::BiVectorB() };
    BiVector c{ Elements::BiVectorC() };

    const Motor rotation1 = Motor::Rotation(50.0f, a);
    BiVector result1{ ((rotation1 * b) * ~rotation1).Grade2() };
    BiVector correct1{ 3.276f, 3.7208f, 5.0422f, 5.862f, 7.1396f, 7.9789f };

    for (size_t i = 0; i < 6; ++i) {
        EXPECT_NEAR(result1[i], correct1[i], 0.0001f);
    }
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, Translation) {
    const BiVector a{ Elements::BiVectorA() };
    const BiVector aDuplicate{ Elements::BiVectorA() };
    const BiVector b{ Elements::BiVectorB() };
    BiVector c{ Elements::BiVectorC() };

    const Motor translation1 = Motor::Translation(7.0f, a);
    BiVector result1{ ((translation1 * b) * ~translation1).Grade2() };
    BiVector correct1{ -2.1994f, 14.399f, -0.19956f, 6, 7, 8 };

    for (size_t i = 0; i < 6; ++i) {
        EXPECT_NEAR(result1[i], correct1[i], 0.0001f);
    }
    EXPECT_EQ(a, aDuplicate);
}

// GANull
TEST_F(ElementsTest, GANullOperations) {
    const MultiVector a{ Elements::MultiVectorA() };
    const MultiVector aDuplicate{ Elements::MultiVectorA() };

    const GAElement res1{ GANull{} * a };
    const GAElement res2{ a * GANull{} };

    const GAElement res3{ GANull{} | a };
    const GAElement res4{ a | GANull{} };

    const GAElement res5{ GANull{} ^ a };
    const GAElement res6{ a ^ GANull{} };

    const GAElement res7{ GANull{}  & a };
    const GAElement res8{ a  & GANull{} };

    const GAElement correct{ GANull{} };

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

// Main entry point for Google Test
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
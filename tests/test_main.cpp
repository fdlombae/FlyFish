#include "../src/FlyFish.h"
#include <gtest/gtest.h>
#include <cmath>

// Example class to be tested
class Elements {
public:
    Elements() {};

    MultiVector MultiVectorA()
    {
        return MultiVector(2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17);
    }
    MultiVector MultiVectorB()
    {
        return MultiVector(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18);
    }
    MultiVector MultiVectorC()
    {
        return MultiVector(-4, 5, -6, 7, -8, 9, -10, 11, -12, 13, -14, 15, -16, 17, -18, 19);
    }
    OneBlade OneBladeA()
    {
        return OneBlade(2, 3, 4, 5);
    }
    OneBlade OneBladeB()
    {
        return OneBlade(3, 4, 5, 6);
    }
    OneBlade OneBladeC()
    {
        return OneBlade(-4, 5, -6, 7);
    }
    TwoBlade TwoBladeA()
    {
        return TwoBlade(2, 3, 4, 5, 6, 7);
    }
    TwoBlade TwoBladeB()
    {
        return TwoBlade(3, 4, 5, 6, 7, 8);
    }
    TwoBlade TwoBladeC()
    {
        return TwoBlade(-4, 5, -6, 7, -8, 9);
    }
    ThreeBlade ThreeBladeA()
    {
        return ThreeBlade(2, 3, 4, 5);
    }
    ThreeBlade ThreeBladeB()
    {
        return ThreeBlade(3, 4, 5, 6);
    }
    ThreeBlade ThreeBladeC()
    {
        return ThreeBlade(-4, 5, -6, 7);
    }
    Motor MotorA()
    {
        return Motor{ 2, 3, 4, 5, 6, 7, 8, 9 };
    }
    Motor MotorB()
    {
        return Motor{ 3, 4, 5, 6, 7, 8, 9, 10 };
    }
    Motor MotorC()
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

    Elements* elements{};  // Shared object for all tests
};

// GAElement
TEST_F(ElementsTest, GAElementToString) {
    MultiVector multi{ -2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5.2f, -10 };
    ThreeBlade three{ 5, 0, -3, 2.5f };
    TwoBlade two{1, 2, 0, 0, 0, 0 };
    OneBlade one{ -1, 2.5f, 0, 0 };
    Motor motor{ 1, 3, 0, 0, 0, 0, 0, -1.0f };

    EXPECT_EQ(multi.toString(), "-2 + 5.2*e123 - 10*e0123");
    EXPECT_EQ(three.toString(), "5*e032 - 3*e021 + 2.5*e123");
    EXPECT_EQ(two.toString(), "e01 + 2*e02");
    EXPECT_EQ(one.toString(), "-e0 + 2.5*e1");
    EXPECT_EQ(motor.toString(), "1 + 3*e01 - e0123");
    EXPECT_EQ(GANull{}.toString(), "GANull");
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
    MultiVector a{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };

    MultiVector res1{ a + b };

    MultiVector correct{ 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35 };

    EXPECT_EQ(res1, correct);
}

TEST_F(ElementsTest, GAElementPlusEqual) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };

    a += b;

    MultiVector correct{ 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33, 35 };

    EXPECT_EQ(a, correct);
}

TEST_F(ElementsTest, GAElementMinus) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };

    MultiVector res1{ a - b };

    MultiVector correct{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    EXPECT_EQ(res1, correct);
}

TEST_F(ElementsTest, GAElementMinusEqual) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };

    a -= b;

    MultiVector correct{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

    EXPECT_EQ(a, correct);
}

TEST_F(ElementsTest, GAElementProxyReferencing) {
    MultiVector multiVector{ elements->MultiVectorA() };
    ThreeBlade threeBlade{ elements->ThreeBladeA() };
    TwoBlade twoBlade{ elements->TwoBladeA() };
    OneBlade oneBlade{ elements->OneBladeA() };
    Motor motor{ elements->MotorA() };
    
    EXPECT_EQ(multiVector.s, multiVector[0]);
    EXPECT_EQ(multiVector.e0, multiVector[1]);

    EXPECT_EQ(threeBlade.e032, threeBlade[0]);
    EXPECT_EQ(threeBlade.e123, threeBlade[3]);

    EXPECT_EQ(twoBlade.e01, twoBlade[0]);
    EXPECT_EQ(twoBlade.e23, twoBlade[3]);

    EXPECT_EQ(oneBlade.e0, oneBlade[0]);
    EXPECT_EQ(oneBlade.e3, oneBlade[3]);

    EXPECT_EQ(motor.s, motor[0]);
    EXPECT_EQ(motor.e0123, motor[7]);
}

// MultiVector
TEST_F(ElementsTest, MultiVectorEqual) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };

    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a == aDuplicate);
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorRoundedEqual) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };

    EXPECT_FALSE(a.RoundedEqual(b, 0.00001f));
    EXPECT_TRUE(a.RoundedEqual(aDuplicate, 0.00001f));

    a[4] += 0.0000001f;
    EXPECT_TRUE(a.RoundedEqual(aDuplicate, 0.00001f));

    a[5] += 0.001f;
    EXPECT_FALSE(a.RoundedEqual(aDuplicate, 0.00001f));
}

TEST_F(ElementsTest, MultiVectorScalarProduct) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };

    MultiVector res1{ 2.f * a };
    MultiVector res2{ a * 2.f };

    MultiVector correct{ 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34 };

    EXPECT_EQ(res1, correct);
    EXPECT_EQ(res2, correct);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorScalarDivision) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };

    MultiVector res1{ a / 2.f };

    MultiVector correct{ 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5, 6, 6.5, 7, 7.5, 8, 8.5 };

    EXPECT_EQ(res1, correct);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorMultiVectorGeometricProduct) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector bDuplicate{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };
    MultiVector res3{ b * a };

    MultiVector correct1{ -572, 1028, -322, -356, -378, -314, -404, -374, 200, 238, 276, -14, -4, -18, 464, 636 };
    MultiVector correct2{ 388, 492, 664, -8, 152, -918, -478, -274, -4, 4, -516, 666, -226, 350, -246, 24 };
    MultiVector correct3{ -572, 1012, -326, -348, -382, -310, -388, -370, 200, 238, 276, -10, 12, -14, 464, 716 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorThreeBladeGeometricProduct) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    ThreeBlade b{ elements->ThreeBladeB() };
    ThreeBlade c{ elements->ThreeBladeC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ -96, 32, -60, -66, -72, 29, 22, 9, 24, 30, 36, -43, -26, -51, 12, 80 };
    MultiVector correct2{ -112, -176, -70, -77, -84, 215, 18, 161, 28, 35, 42, 69, -58, -169, 14, -6 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorTwoBladeGeometricProduct) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    TwoBlade b{ elements->TwoBladeB() };
    TwoBlade c{ elements->TwoBladeC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ -233, 234, -94, -116, -126, -104, -95, -134, 8, 22, 12, 24, 55, 50, 107, 304 };
    MultiVector correct2{ -90, 141, -205, 122, -77, -145, 134, -147, -181, -10, 175, -391, 116, 119, 42, 9 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorOneBladeGeometricProduct) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    OneBlade b{ elements->OneBladeB() };
    OneBlade c{ elements->OneBladeC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ 77, 128, 2, 22, 6, 9, -18, 9, 64, 80, 96, -101, -112, -141, 167, -260 };
    MultiVector correct2{ 32, 42, -139, -2, 129, 219, -14, -103, 151, -94, 63, -155, 150, 11, 68, -22 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorMotorGeometricProduct) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    Motor b{ elements->MotorB() };
    Motor c{ elements->MotorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ -260, 430, -97, -119, -123, -196, -200, -240, 41, 55, 51, 116, 160, 168, 170, 432 };
    MultiVector correct2{ 93, 4, 216, -156, 60, 20, -306, 4, 162, -30, -246, 442, -132, -130, -111, -52 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorWedge) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector bDuplicate{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a ^ b };
    MultiVector res2{ a ^ c };
    MultiVector res3{ b ^ a };

    MultiVector correct1{ 6, 17, 22, 27, 32, 36, 40, 44, 51, 59, 61, -12, 4, -16, 464, 636 };
    MultiVector correct2{ -8, -2, -28, -6, -40, -48, -56, -68, -146, -22, -18, 206, -174, -226, -246, 24 };
    MultiVector correct3{ 6, 17, 22, 27, 32, 38, 44, 50, 53, 55, 63, -12, 4, -16, 464, 716 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorDot) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector bDuplicate{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a | b };
    MultiVector res2{ a | c };
    MultiVector res3{ b | a };

    MultiVector correct1{ -572, 1028, -322, -356, -378, -312, -396, -372, 200, 238, 276, 54, 60, 66, 82, 87 };
    MultiVector correct2{ 388, 492, 664, -8, 152, -394, -490, 310, -232, 4, -312, 156, -112, 224, -100, -30 };
    MultiVector correct3{ -572, 1012, -326, -348, -382, -312, -396, -372, 200, 238, 276, 80, 84, 88, 82, 87 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MultiVectorJoin) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector bDuplicate{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a & b };
    MultiVector res2{ a & c };
    MultiVector res3{ b & a };

    MultiVector correct1{ 636, -620, 398, 484, 534, 261, 299, 331, 370, 404, 438, 472, 507, 542, 577, 306 };
    MultiVector correct2{ 24, -130, 762, -82, -670, 764, -14, -60, 460, 426, 532, 502, -6, 574, -2, 323 };
    MultiVector correct3{ 716, -620, 398, 484, 534, 263, 295, 333, 364, 400, 436, 472, 507, 542, 577, 306 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

// TODO
TEST_F(ElementsTest, MultiVectorInverse) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ ~a };
    MultiVector res2{ ~c };

    MultiVector correct1{
        -0.057254f, -0.05906f, 0.034449f, 0.037603f, 0.040757f, 0.041053f, 0.041673f, 0.055879f,
        -0.019408f, -0.023532f, -0.027656f, 0.026875f, 0.041619f, 0.04666f, 0.015041f, 0.0034234f
    };

    MultiVector correct2{
        0.042378f, -0.033246f, -0.027217f, 0.02933f, -0.031443f, 0.029903f, -0.03863f, 0.038761f,
        0.018264f, -0.020998f, 0.023732f, 0.028972f, -0.03278f, 0.041745f, -0.01528f, 0.0092276f
    };

    // EXPECT_TRUE(res1.RoundedEqual(correct1, 0.0001));
    // EXPECT_TRUE(res2.RoundedEqual(correct2, 0.0001));

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorNorm) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector c{ elements->MultiVectorC() };

    float res1{ a.Norm() };
    float res2{ c.Norm() };

    EXPECT_TRUE(std::fabs(res1 - 26.4953f) < 0.0001f);
    EXPECT_TRUE(std::fabs(res2 - 31.5911f) < 0.0001f);
    
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MultiVectorVNorm) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };
    MultiVector c{ elements->MultiVectorC() };

    float res1{ a.VNorm() };
    float res2{ c.VNorm() };

    EXPECT_TRUE(std::fabs(res1 - 32.8938f) < 0.0001f);
    EXPECT_TRUE(std::fabs(res2 - 38.1838f) < 0.0001f);

    EXPECT_EQ(a, aDuplicate);
}

// OneBlade
TEST_F(ElementsTest, OneBladeMultiVectorGeometricProduct) {
    OneBlade a{ elements->OneBladeA() };
    OneBlade aDuplicate{ elements->OneBladeA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ 74, -104, 17, -4, 23, 9, -26, 5, 49, 72, 83, 27, 58, 59, 146, 216 };
    MultiVector correct2{ -30, -50, 109, 2, -107, -175, -30, 67, -121, -78, -45, 175, 62, 57, -54, 30 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, OneBladeMotorGeometricProduct) {
    OneBlade a{ elements->OneBladeA() };
    OneBlade aDuplicate{ elements->OneBladeA() };
    Motor b{ elements->MotorB() };
    Motor c{ elements->MotorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ 0, -56, 13, 4, 19, 0, 0, 0, 0, 0, 0, 15, 26, 31, 98, 0 };
    MultiVector correct2{ 0, -34, 73, -6, -79, 0, 0, 0, 0, 0, 0, 107, 30, 37, -38, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, OneBladeWedgeMultiVector) {
    OneBlade a{ elements->OneBladeA() };
    OneBlade aDuplicate{ elements->OneBladeA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a ^ b };
    MultiVector res2{ a ^ c };

    MultiVector correct1{ 0, 6, 9, 12, 15, -2, -4, -6, -2, 4, -2, -27, -14, -31, 146, 216 };
    MultiVector correct2{ 0, -8, -12, -16, -20, -27, -6, -41, -67, -6, 45, 118, -14, -38, -54, 30 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, OneBladeWedgeTwoBlade) {
    OneBlade a{ elements->OneBladeA() };
    OneBlade aDuplicate{ elements->OneBladeA() };
    TwoBlade b{ elements->TwoBladeB() };
    TwoBlade c{ elements->TwoBladeC() };

    ThreeBlade res1{ a ^ b };
    ThreeBlade res2{ a ^ c };

    ThreeBlade correct1{ -12, -14, -16, 86 };
    ThreeBlade correct2{ -63, 14, 13, 34 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, OneBladeWedgeOneBlade) {
    OneBlade a{ elements->OneBladeA() };
    OneBlade aDuplicate{ elements->OneBladeA() };
    OneBlade b{ elements->OneBladeB() };
    OneBlade c{ elements->OneBladeC() };

    TwoBlade res1{ a ^ b };
    TwoBlade res2{ a ^ c };

    TwoBlade correct1{ -1, -2, -3, -1, 2, -1 };
    TwoBlade correct2{ 22, 4, 34, 58, 4, -38 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

// ThreeBlade
TEST_F(ElementsTest, ThreeBladeJoinThreeBlade) {
    ThreeBlade a{ elements->ThreeBladeA() };
    ThreeBlade aDuplicate{ elements->ThreeBladeA() };
    ThreeBlade b{ elements->ThreeBladeB() };
    ThreeBlade c{ elements->ThreeBladeC() };

    TwoBlade res1{ a & b };
    TwoBlade res2{ a & c };

    TwoBlade correct1{ -1, 2, -1, 3, 2, 1 };
    TwoBlade correct2{ -38, -4, 22, -34, 4, -58 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    // Practical case
    ThreeBlade pointA{ 100, 150, 1 };
    ThreeBlade pointB{ 150, 150, 1 };

    TwoBlade line{ pointA & pointB };

    TwoBlade expectedLine{ 0, 50, -7500, 50, 0, 0 };

    EXPECT_EQ(line, expectedLine);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, ThreeBladeMultiVectorGeometricProduct) {
    ThreeBlade a{ elements->ThreeBladeA() };
    ThreeBlade aDuplicate{ elements->ThreeBladeA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ -85, 200, -55, -60, -65, -39, -18, -15, 25, 30, 35, 55, 36, 71, 15, -76 };
    MultiVector correct2{ 90, 54, 60, -65, 70, -163, 18, -125, -30, 35, -40, 131, -42, -23, -20, -2 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, ThreeBladeThreeBladeGeometricProduct) {
    ThreeBlade a{ elements->ThreeBladeA() };
    ThreeBlade aDuplicate{ elements->ThreeBladeA() };
    ThreeBlade b{ elements->ThreeBladeB() };
    ThreeBlade c{ elements->ThreeBladeC() };

    Motor res1{ a * b };
    Motor res2{ a * c };

    Motor correct1{ -30, -3, -2, -1, 0, 0, 0, 0 };
    Motor correct2{ -35, 34, -4, 58, 0, 0, 0, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

// TwoBlade
TEST_F(ElementsTest, TwoBladeMultiVectorGeometricProduct) {
    TwoBlade a{ elements->TwoBladeA() };
    TwoBlade aDuplicate{ elements->TwoBladeA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ -218, 328, -85, -102, -119, -72, -123, -102, 21, 6, 27, -42, -99, -84, 110, 274 };
    MultiVector correct2{ 80, 75, 187, 110, 55, -145, -114, -107, 155, -10, -165, -151, 12, 175, -44, 21 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TwoBladeMotorGeometricProduct) {
    TwoBlade a{ elements->TwoBladeA() };
    TwoBlade aDuplicate{ elements->TwoBladeA() };
    Motor b{ elements->MotorB() };
    Motor c{ elements->MotorC() };

    Motor res1{ a * b };
    Motor res2{ a * c };

    Motor correct1{ -146, -40, -59, -54, 17, 14, 23, 166 };
    Motor correct2{ 56, -81, -66, -75, 103, -18, -121, 9 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TwoBladeOneBladeGeometricProduct) {
    TwoBlade a{ elements->TwoBladeA() };
    TwoBlade aDuplicate{ elements->TwoBladeA() };
    OneBlade b{ elements->OneBladeB() };
    OneBlade c{ elements->OneBladeC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ 0, 47, -1, 2, -1, 0, 0, 0, 0, 0, 0, -13, -22, -19, 92, 0 };
    MultiVector correct2{ 0, 20, -84, 0, 60, 0, 0, 0, 0, 0, 0, -25, 18, 55, 38, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, TwoBladePermutedDotTwoBladed) {
    TwoBlade a{ elements->TwoBladeA() };
    TwoBlade aDuplicate{ elements->TwoBladeA() };
    TwoBlade b{ elements->TwoBladeB() };
    TwoBlade c{ elements->TwoBladeC() };

    float res1{ a.PermutedDot(b) };
    float res2{ a.PermutedDot(c) };

    // TODO

    EXPECT_EQ(a, aDuplicate);
}

// Motor
TEST_F(ElementsTest, MotorMultiVectorGeometricProduct) {
    Motor a{ elements->MotorA() };
    Motor aDuplicate{ elements->MotorA() };
    MultiVector b{ elements->MultiVectorB() };
    MultiVector bDuplicate{ elements->MultiVectorB() };
    MultiVector c{ elements->MultiVectorC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ -248, 246, -93, -105, -123, -172, -224, -216, 45, 35, 55, -82, -140, -138, 162, 400};
    MultiVector correct2{ 85, 256, 208, 140, 44, -36, -274, 12, 154, 10, -222, -72, -70, 312, -87, 20 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MotorMotorGeometricProduct) {
    Motor a{ elements->MotorA() };
    Motor aDuplicate{ elements->MotorA() };
    Motor b{ elements->MotorB() };
    Motor bDuplicate{ elements->MotorB() };
    Motor c{ elements->MotorC() };

    Motor res1{ a * b };
    Motor res2{ a * c };
    Motor res3{ b * a };

    Motor correct1{ -164, -104, -124, -132, 33, 35, 43, 252 };
    Motor correct2{ 57, -8, -174, 8, 102, -6, -162, -8 };
    Motor correct3{ -164, -108, -116, -136, 31, 39, 41, 252 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);

    EXPECT_EQ(a, aDuplicate);
    EXPECT_EQ(b, bDuplicate);
}

TEST_F(ElementsTest, MotorThreeBladeGeometricProduct) {
    Motor a{ elements->MotorA() };
    Motor aDuplicate{ elements->MotorA() };
    ThreeBlade b{ elements->ThreeBladeB() };
    ThreeBlade c{ elements->ThreeBladeC() };

    MultiVector res1{ a * b };
    MultiVector res2{ a * c };

    MultiVector correct1{ 0, 32, -36, -42, -48, 0, 0, 0, 0, 0, 0, -15, -10, -23, 12, 0 };
    MultiVector correct2{ 0, -100, -42, -49, -56, 0, 0, 0, 0, 0, 0, 53, -22, -105, 14, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);

    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MotorTwoBladeGeometricProduct) {
    Motor a{ elements->MotorA() };
    Motor aDuplicate{ elements->MotorA() };
    TwoBlade b{ elements->TwoBladeB() };
    TwoBlade c{ elements->TwoBladeC() };
    Motor motorC{ elements->MotorC() };


    Motor res1{ a * b };
    Motor res2{ a * c };
    Motor res3{ motorC * b };

    Motor correct1{ -149, -48, -55, -62, 12, 14, 16, 172 };
    Motor correct2{ -58, -65, 70, -99, -113, -18, 115, -3 };
    Motor correct3{ 65, -66, -105, -120, -166, -32, 78, 6 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, MotorOneBladeGeometricProduct) {
    Motor a{ elements->MotorA() };
    Motor aDuplicate{ elements->MotorA() };
    OneBlade b{ elements->OneBladeB() };
    OneBlade c{ elements->OneBladeC() };
    Motor motorC{ elements->MotorC() };


    MultiVector res1{ a * b };
    MultiVector res2{ a * c };
    MultiVector res3{ motorC * c };

    MultiVector correct1{ 0, 68, 6, 14, 10, 0, 0, 0, 0, 0, 0, -53, -68, -77, 107, 0 };
    MultiVector correct2{ 0, 18, -87, -10, 85, 0, 0, 0, 0, 0, 0, -79, 78, 7, 44, 0 };
    MultiVector correct3{ 0, 126, -23, 18, -31, 0, 0, 0, 0, 0, 0, -87, 102, -117, -164, 0 };

    EXPECT_EQ(res1, correct1);
    EXPECT_EQ(res2, correct2);
    EXPECT_EQ(res3, correct3);
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, Rotation) {
    TwoBlade a{ elements->TwoBladeA() };
    TwoBlade aDuplicate{ elements->TwoBladeA() };
    TwoBlade b{ elements->TwoBladeB() };
    TwoBlade c{ elements->TwoBladeC() };

    Motor rotation1 = Motor::Rotation(50.f, a);
    TwoBlade result1{ ((rotation1 * b) * ~rotation1).Grade2() };
    TwoBlade correct1{ 3.276f, 3.7208f, 5.0422f, 5.862f, 7.1396f, 7.9789f };

    EXPECT_TRUE(result1.RoundedEqual(correct1, 0.0001f));
    EXPECT_EQ(a, aDuplicate);
}

TEST_F(ElementsTest, Translation) {
    TwoBlade a{ elements->TwoBladeA() };
    TwoBlade aDuplicate{ elements->TwoBladeA() };
    TwoBlade b{ elements->TwoBladeB() };
    TwoBlade c{ elements->TwoBladeC() };

    Motor translation1 = Motor::Translation(7.f, a);
    TwoBlade result1{ ((translation1 * b) * ~translation1).Grade2() };
    TwoBlade correct1{ -2.1994f, 14.399f, -0.19956f, 6, 7, 8 };

    EXPECT_TRUE(result1.RoundedEqual(correct1, 0.0001f));
    EXPECT_EQ(a, aDuplicate);
}

// GANull
TEST_F(ElementsTest, GANullOperations) {
    MultiVector a{ elements->MultiVectorA() };
    MultiVector aDuplicate{ elements->MultiVectorA() };

    GAElement res1{ GANull{} * a };
    GAElement res2{ a * GANull{} };

    GAElement res3{ GANull{} | a };
    GAElement res4{ a | GANull{} };

    GAElement res5{ GANull{} ^ a };
    GAElement res6{ a ^ GANull{} };

    GAElement res7{ GANull{}  & a };
    GAElement res8{ a  & GANull{} };

    GAElement correct{ GANull{} };

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
#pragma once

#include <cmath>
#include <array>
#include <sstream>

class OneBlade;
class TwoBlade;
class ThreeBlade;
class Motor;
class GANull;

constexpr float DEG_TO_RAD = 3.141592f / 180.0f;

template <typename Derived, int DataSize>
class GAElement
{
public:
    [[nodiscard]] GAElement() noexcept
    {
    }

    inline float& operator [] (size_t idx) { return data[idx]; }
    inline const float& operator [] (size_t idx) const { return data[idx]; }

    inline float& get(size_t index) { return data[index]; }
    inline const float& get(size_t index) const { return data[index]; }

    GAElement(const GAElement& other) noexcept {
        data = other.data;
    }

    GAElement(GAElement&& other) noexcept {
        data = std::move(other.data);
    }

    GAElement& operator=(const GAElement& other) noexcept {
        if (this != &other) {
            data = other.data;
        }
        return *this;
    }

    GAElement& operator=(GAElement&& other) noexcept {
        if (this != &other) {
            data = std::move(other.data);
        }
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const Derived& element) {
        os << element.toString();
        return os;
    }

    std::string toString() const {
        std::ostringstream output;
        const auto& names = Derived::names();
        bool first = true;

        for (size_t i = 0; i < data.size(); ++i) 
        {
            if (std::fabs(data[i]) > 1e-6) 
            {
                if (!first) 
                {
                    output << (data[i] > 0 ? " + " : " - ");
                }
                else if (data[i] < 0) 
                {
                    output << "-";
                }
                first = false;

                if (std::fabs(data[i]) != 1) 
                {
                    output << std::fabs(data[i]);
                    if (names[i] != "")
                    {
                        output << "*";
                    }
                }

                if (names[i] != "") 
                {                        
                   output << names[i];
                }
                else if (std::fabs(data[i]) == 1)
                {
                    output << '1';
                }
            }
        }

        return first ? "0" : output.str(); // Return "0" if all coefficients are zero.
    }

    // Iterator support
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }

    bool operator== (const GAElement& b) const
    {
        return data == b.data;
    }
    bool RoundedEqual(const GAElement& b, float tolerance) const
    {
        for (size_t i = 0; i < DataSize; ++i) {
            if (std::fabs(data[i] - b[i]) > tolerance) {
                return false;
            }
        }
        return true;
    }

    Derived& operator += (const Derived& b)
    {
        for (size_t idx{}; idx < DataSize; idx++)
        {
            data[idx] += b[idx];
        }

        return static_cast<Derived&>(*this);
    }
    Derived& operator -= (const Derived& b)
    {
        for (size_t idx{}; idx < DataSize; idx++)
        {
            data[idx] -= b[idx];
        }

        return static_cast<Derived&>(*this);
    }
    Derived& operator *= (float s)
    {
        for (size_t idx{}; idx < DataSize; idx++)
        {
            data[idx] *= s;
        }
        return static_cast<Derived&>(*this);
    }
    Derived& operator /= (float s)
    {
        float reciprocal = 1 / s;
        for (size_t idx{}; idx < DataSize; idx++)
        {
            data[idx] *= reciprocal;
        }
        return static_cast<Derived&>(*this);
    }

    [[nodiscard]] Derived operator * (float s) const
    {
        Derived d{};
        for (size_t idx{}; idx < DataSize; idx++)
        {
            d[idx] = s * data[idx];
        }
        return d;
    }
    [[nodiscard]] Derived operator / (float s) const
    {
        Derived d{};
        float mult = 1 / s;
        for (size_t idx{}; idx < DataSize; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }
    [[nodiscard]] Derived operator-() const {
        Derived d{};
        for (size_t idx{}; idx < DataSize; idx++)
        {
            d[idx] = -data[idx];
        }
        return d;
    }
    [[nodiscard]] Derived operator + (const Derived& b) const
    {
        Derived d{};
        for (size_t idx{}; idx < DataSize; idx++)
        {
            d[idx] = data[idx] + b[idx];
        }
        return d;
    }
    [[nodiscard]] Derived operator - (const Derived& b) const
    {
        Derived d{};
        for (size_t idx{}; idx < DataSize; idx++)
        {
            d[idx] = data[idx] - b[idx];
        }
        return d;
    }

    friend [[nodiscard]] Derived operator*(float scalar, const Derived& element) {
        return element * scalar;
    }

protected:
    std::array<float, DataSize> data{};
};

class MultiVector : public GAElement<MultiVector, 16>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& s() { return get(0); }
    inline float& e0() { return get(1); }
    inline float& e1() { return get(2); }
    inline float& e2() { return get(3); }
    inline float& e3() { return get(4); }
    inline float& e01() { return get(5); }
    inline float& e02() { return get(6); }
    inline float& e03() { return get(7); }
    inline float& e23() { return get(8); }
    inline float& e31() { return get(9); }
    inline float& e12() { return get(10); }
    inline float& e032() { return get(11); }
    inline float& e013() { return get(12); }
    inline float& e021() { return get(13); }
    inline float& e123() { return get(14); }
    inline float& e0123() { return get(15); }

    inline const float& s() const { return get(0); }
    inline const float& e0() const { return get(1); }
    inline const float& e1() const { return get(2); }
    inline const float& e2() const { return get(3); }
    inline const float& e3() const { return get(4); }
    inline const float& e01() const { return get(5); }
    inline const float& e02() const { return get(6); }
    inline const float& e03() const { return get(7); }
    inline const float& e23() const { return get(8); }
    inline const float& e31() const { return get(9); }
    inline const float& e12() const { return get(10); }
    inline const float& e032() const { return get(11); }
    inline const float& e013() const { return get(12); }
    inline const float& e021() const { return get(13); }
    inline const float& e123() const { return get(14); }
    inline const float& e0123() const { return get(15); }

    [[nodiscard]] MultiVector() noexcept : GAElement()
    {
    }

    [[nodiscard]] MultiVector(float s, float e0, float e1, float e2, float e3, float e01, float e02, float e03, float e23, float e31, float e12, float e032, float e013, float e021, float e123, float e0123) noexcept
    {
        data[0] = s;
        data[1] = e0;
        data[2] = e1;
        data[3] = e2;
        data[4] = e3;
        data[5] = e01;
        data[6] = e02;
        data[7] = e03;
        data[8] = e23;
        data[9] = e31;
        data[10] = e12;
        data[11] = e032;
        data[12] = e013;
        data[13] = e021;
        data[14] = e123;
        data[15] = e0123;
    }

    static constexpr std::array<const char*, 16> names() {
        return { "", "e0", "e1", "e2", "e3", "e01", "e02", "e03",
                 "e23", "e31", "e12", "e032", "e013", "e021", "e123", "e0123" };
    }

    MultiVector& Normalize()
    {
        return (*this) /= Norm();
    }
    [[nodiscard]] MultiVector Normalized() const
    {
        MultiVector d{};
        float mult = 1 / Norm();
        for (size_t idx{}; idx < 16; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }
    
    MultiVector& operator=(const ThreeBlade& b);
    MultiVector& operator=(ThreeBlade&& b) noexcept;
    MultiVector& operator=(const TwoBlade& b);
    MultiVector& operator=(TwoBlade&& b) noexcept;
    MultiVector& operator=(const OneBlade& b);
    MultiVector& operator=(OneBlade&& b) noexcept;
    MultiVector& operator=(const Motor& b);
    MultiVector& operator=(Motor&& b) noexcept;

    [[nodiscard]] float Norm() const
    {
        return std::sqrt(data[0] * data[0] + data[2] * data[2] + data[3] * data[3] + data[4] * data[4] + data[8] * data[8] + data[9] * data[9] + data[10] * data[10] + data[14] * data[14]);
    }
    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[1] * data[1] + data[5] * data[5] + data[6] * data[6] + data[7] * data[7] + data[11] * data[11] + data[12] * data[12] + data[13] * data[13] + data[15] * data[15]);
    }

    [[nodiscard]] OneBlade Grade1() const;
    [[nodiscard]] TwoBlade Grade2() const;
    [[nodiscard]] ThreeBlade Grade3() const;
    [[nodiscard]] Motor ToMotor() const;

    [[nodiscard]] MultiVector operator ~() const{
        float norm{ Norm() };
        float normSquared{ norm };
        return MultiVector(
            data[0] / normSquared,
            data[1] / normSquared,
            data[2] / normSquared,
            data[3] / normSquared,
            data[4] / normSquared,
            -data[5] / normSquared,
            -data[6] / normSquared,
            -data[7] / normSquared,
            -data[8] / normSquared,
            -data[9] / normSquared,
            -data[10] / normSquared,
            -data[11] / normSquared,
            -data[12] / normSquared,
            -data[13] / normSquared,
            -data[14] / normSquared,
            data[15] / normSquared
            );
    };

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator* (const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator* (const Motor& b) const;
    [[nodiscard]] MultiVector operator* (const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator* (const OneBlade& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator| (const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator| (const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator| (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator| (const Motor& b) const;


    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator& (const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator& (const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator& (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] MultiVector operator^(const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator^(const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator^(const OneBlade& b) const;
    [[nodiscard]] MultiVector operator^(const Motor& b) const;


    [[nodiscard]] MultiVector operator! () const;
};

class OneBlade : public GAElement<OneBlade, 4>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& e0() { return get(0); }
    inline float& e1() { return get(1); }
    inline float& e2() { return get(2); }
    inline float& e3() { return get(3); }

    inline const float& e0() const { return get(0); }
    inline const float& e1() const { return get(1); }
    inline const float& e2() const { return get(2); }
    inline const float& e3() const { return get(3); }

    OneBlade() : GAElement()
    {
    }

    [[nodiscard]] OneBlade(float e0, float e1, float e2, float e3) : GAElement()
    {
        data[0] = e0;
        data[1] = e1;
        data[2] = e2;
        data[3] = e3;
    }

    static constexpr std::array<const char*, 4> names() {
        return { "e0", "e1", "e2", "e3" };
    }

    [[nodiscard]] float Norm() const
    {
        return std::sqrt(data[1] * data[1] + data[2] * data[2] + data[3] * data[3]);
    }

    OneBlade& Normalize()
    {
        return (*this) /= Norm();
    }
    [[nodiscard]] OneBlade Normalized() const
    {
        OneBlade d{};
        float mult = 1 / Norm();
        for (size_t idx{}; idx < 4; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }

    [[nodiscard]] OneBlade operator ~() const
    {
        float norm{ Norm() };
        float normSquared{ norm * norm };
        return OneBlade(
            data[0] / normSquared,
            data[1] / normSquared,
            data[2] / normSquared,
            data[3] / normSquared
        );
    }

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] Motor operator* (const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator* (const TwoBlade& b) const;
    [[nodiscard]] Motor operator* (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] float operator& (const ThreeBlade& b) const;
    [[nodiscard]] GANull operator& (const TwoBlade& b) const;
    [[nodiscard]] GANull operator& (const OneBlade& b) const;
    [[nodiscard]] OneBlade operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] TwoBlade operator| (const ThreeBlade& b) const;
    [[nodiscard]] OneBlade operator| (const TwoBlade& b) const;
    [[nodiscard]] float operator| (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] MultiVector operator^(const ThreeBlade& b) const;
    [[nodiscard]] ThreeBlade operator^ (const TwoBlade& b) const;
    [[nodiscard]] TwoBlade operator^(const OneBlade& b) const;
    [[nodiscard]] MultiVector operator^(const Motor& b) const;


    [[nodiscard]] ThreeBlade operator! () const;
};

class TwoBlade : public GAElement<TwoBlade, 6>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& e01() { return get(0); }
    inline float& e02() { return get(1); }
    inline float& e03() { return get(2); }
    inline float& e23() { return get(3); }
    inline float& e31() { return get(4); }
    inline float& e12() { return get(5); }

    inline const float& e01() const { return get(0); }
    inline const float& e02() const { return get(1); }
    inline const float& e03() const { return get(2); }
    inline const float& e23() const { return get(3); }
    inline const float& e31() const { return get(4); }
    inline const float& e12() const { return get(5); }

    TwoBlade() : GAElement()
    {
    }

    [[nodiscard]] TwoBlade(float e01, float e02, float e03, float e23, float e31, float e12) : GAElement()
    {
        data[0] = e01;
        data[1] = e02;
        data[2] = e03;
        data[3] = e23;
        data[4] = e31;
        data[5] = e12;
    }

    static constexpr std::array<const char*, 6> names() {
        return { "e01", "e02", "e03", "e23", "e31", "e12"};
    }

    [[nodiscard]] float PermutedDot(const TwoBlade& b) const {
        return data[3] * b[0] + data[4] * b[1] + data[5] * b[2] + data[2] * b[5] + data[1] * b[4] + data[0] * b[3];
    }

    [[nodiscard]] static TwoBlade LineFromPoints(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        return TwoBlade(
            y1 * z2 - y2 * z1,
            z1 * x2 - z2 * x1,
            x1 * y2 - x2 * y1,
            x2 - x1,
            y2 - y1,
            z2 - z1
            );
    }

    TwoBlade& Normalize()
    {
        return (*this) /= Norm();
    }
    [[nodiscard]] TwoBlade Normalized() const
    {
        TwoBlade d{};
        float mult = 1 / Norm();
        for (size_t idx{}; idx < 6; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }

    [[nodiscard]] float Norm() const
    {
        return std::sqrt(data[3] * data[3] + data[4] * data[4] + data[5] * data[5]);
    }
    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]);
    }

    [[nodiscard]] TwoBlade operator ~() const {
        float squareNorm{ Norm() * Norm() };
        return TwoBlade(
            -data[0] / squareNorm,
            -data[1] / squareNorm,
            -data[2] / squareNorm,
            -data[3] / squareNorm,
            -data[4] / squareNorm,
            -data[5] / squareNorm
        );
    };

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator* (const ThreeBlade& b) const;
    [[nodiscard]] Motor operator* (const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator* (const OneBlade& b) const;
    [[nodiscard]] Motor operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] OneBlade operator| (const ThreeBlade& b) const;
    [[nodiscard]] float operator| (const TwoBlade& b) const;
    [[nodiscard]] OneBlade operator| (const OneBlade& b) const;
    [[nodiscard]] Motor operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] OneBlade operator & (const ThreeBlade& b) const;
    [[nodiscard]] float operator & (const TwoBlade& b) const;
    [[nodiscard]] GANull operator& (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator ^ (const MultiVector& b) const;
    [[nodiscard]] GANull operator ^ (const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator ^ (const TwoBlade& b) const;
    [[nodiscard]] ThreeBlade operator ^ (const OneBlade& b) const;
    [[nodiscard]] Motor operator ^ (const Motor& b) const;
    
    [[nodiscard]] TwoBlade operator! () const;
};

class ThreeBlade : public GAElement<ThreeBlade, 4>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& e032() { return get(0); }
    inline float& e013() { return get(1); }
    inline float& e021() { return get(2); }
    inline float& e123() { return get(3); }

    inline const float& e032() const { return get(0); }
    inline const float& e013() const { return get(1); }
    inline const float& e021() const { return get(2); }
    inline const float& e123() const { return get(3); }

    [[nodiscard]] ThreeBlade() : GAElement()
    {
    }

    [[nodiscard]] ThreeBlade(float x, float y, float z) : GAElement()
    {
        data[0] = x;
        data[1] = y;
        data[2] = z;
        data[3] = 1;
    }

    [[nodiscard]] ThreeBlade(float e032, float e013, float e021, float e123) : GAElement()
    {
        data[0] = e032;
        data[1] = e013;
        data[2] = e021;
        data[3] = e123;
    }

    static constexpr std::array<const char*, 4> names() {
        return { "e032", "e013", "e021", "e123" };
    }

    ThreeBlade& Normalize()
    {
        return (*this) /= Norm();
    }
    [[nodiscard]] ThreeBlade Normalized() const
    {
        ThreeBlade d{};
        float mult = 1 / Norm();
        for (size_t idx{}; idx < 4; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }

    [[nodiscard]] float Norm() const
    {
        return data[3];
    }

    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]);
    }

    [[nodiscard]] ThreeBlade operator ~() const
    {
        float norm{ Norm() };
        float normSquared{ norm * norm };
        return ThreeBlade(
            -data[0] / normSquared,
            -data[1] / normSquared,
            -data[2] / normSquared,
            -data[3] / normSquared
        );
    }

    [[nodiscard]] OneBlade operator! () const;

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] Motor operator* (const ThreeBlade& b) const;
    [[nodiscard]] MultiVector operator* (const TwoBlade& b) const;
    [[nodiscard]] Motor operator* (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] float operator| (const ThreeBlade& b) const;
    [[nodiscard]] OneBlade operator| (const TwoBlade& b) const;
    [[nodiscard]] TwoBlade operator| (const OneBlade& b) const;
    [[nodiscard]] OneBlade operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] TwoBlade operator& (const ThreeBlade& b) const;
    [[nodiscard]] OneBlade operator& (const TwoBlade& b) const;
    [[nodiscard]] float operator& (const OneBlade& b) const;
    [[nodiscard]] MultiVector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] GANull operator^(const ThreeBlade& b) const;
    [[nodiscard]] GANull operator^(const TwoBlade& b) const;
    [[nodiscard]] float operator^(const OneBlade& b) const;
    [[nodiscard]] ThreeBlade operator^(const Motor& b) const;
};

class Motor : public GAElement<Motor, 8>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& s() { return get(0); }
    inline float& e01() { return get(1); }
    inline float& e02() { return get(2); }
    inline float& e03() { return get(3); }
    inline float& e23() { return get(4); }
    inline float& e31() { return get(5); }
    inline float& e12() { return get(6); }
    inline float& e0123() { return get(7); }

    inline const float& s() const { return get(0); }
    inline const float& e01() const { return get(1); }
    inline const float& e02() const { return get(2); }
    inline const float& e03() const { return get(3); }
    inline const float& e23() const { return get(4); }
    inline const float& e31() const { return get(5); }
    inline const float& e12() const { return get(6); }
    inline const float& e0123() const { return get(7); }

    [[nodiscard]] Motor() : GAElement()
    {
    }

    [[nodiscard]] Motor(float s, float e01, float e02, float e03, float e23, float e31, float e12, float e0123) : GAElement()
    {
        data[0] = s;
        data[1] = e01;
        data[2] = e02;
        data[3] = e03;
        data[4] = e23;
        data[5] = e31;
        data[6] = e12;
        data[7] = e0123;
    }

    static constexpr std::array<const char*, 8> names() {
        return { "", "e01", "e02", "e03", "e23", "e31", "e12", "e0123"};
    }

    // Todo
    //[[nodiscard]] Motor(float angle, float translation, const TwoBlade line) : GAElement()
    //{
    //    *this = Translation(translation, line) * Rotation(angle, line) * ~Translation(translation, line);
    //}

    [[nodiscard]] static Motor Translation(float translation, const TwoBlade line)
    {
        float d{ -translation / (2 * line.VNorm()) };
        return Motor{
            1,
            d * line[0],
            d * line[1],
            d * line[2],
            0,
            0,
            0,
            0
        };
    }

    [[nodiscard]] static Motor Rotation(float angle, const TwoBlade line)
    {
        float mult{ -sin(angle * DEG_TO_RAD / 2) / line.Norm() };
        return Motor{
            cos(angle * DEG_TO_RAD /2),
            0,
            0,
            0,
            mult * line[3],
            mult * line[4],
            mult * line[5],
            0
        };
    }

    Motor& Normalize()
    {
        return (*this) /= Norm();
    }
    [[nodiscard]] Motor Normalized() const
    {
        Motor d{};
        float mult = 1 / Norm();
        for (size_t idx{}; idx < 8; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }

    [[nodiscard]] float Norm() const
    {
        return std::sqrt(data[0] * data[0] + data[4] * data[4] + data[5] * data[5] + data[6] * data[6]);
    }

    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[1] * data[1] + data[2] * data[2] + data[3] * data[3] + data[7] * data[7]);
    }

    [[nodiscard]] TwoBlade Grade2() const;

    [[nodiscard]] Motor operator ~() const {
        float norm{ Norm() };
        float normSquared{ norm * norm };
        return Motor(
            data[0] / normSquared ,
            -data[1] / normSquared,
            -data[2] / normSquared,
            -data[3] / normSquared,
            -data[4] / normSquared,
            -data[5] / normSquared,
            -data[6] / normSquared,
            data[7] / normSquared
        );
    };

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator* (const ThreeBlade& b) const;
    [[nodiscard]] Motor operator* (const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator* (const OneBlade& b) const;
    [[nodiscard]] Motor operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator| (const ThreeBlade& b) const;
    [[nodiscard]] Motor operator| (const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator| (const OneBlade& b) const;
    [[nodiscard]] Motor operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator& (const ThreeBlade& b) const;
    [[nodiscard]] Motor operator& (const TwoBlade& b) const;
    [[nodiscard]] OneBlade operator& (const OneBlade& b) const;
    [[nodiscard]] Motor operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] ThreeBlade operator^(const ThreeBlade& b) const;
    [[nodiscard]] Motor operator^(const TwoBlade& b) const;
    [[nodiscard]] MultiVector operator^(const OneBlade& b) const;
    [[nodiscard]] MultiVector operator^(const Motor& b) const;

    Motor& operator += (const TwoBlade& b)
    {
        for (size_t idx{}; idx < 6; idx++)
        {
            data[idx + 1] += b[idx];
        }

        return (*this);
    }
    Motor& operator -= (const TwoBlade& b)
    {
        for (size_t idx{}; idx < 6; idx++)
        {
            data[idx + 1] -= b[idx];
        }

        return (*this);
    }

    [[nodiscard]] Motor operator! () const;
};

class GANull : public GAElement<GANull, 0>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    std::string toString() const {
        return "GANull";
    }

    template <typename Derived>
    [[nodiscard]] GANull operator* (const Derived& b) const
    {
        return GANull{};
    }
    template <typename Derived>
    [[nodiscard]] GANull operator| (const Derived& b) const
    {
        return GANull{};
    }
    template <typename Derived>
    [[nodiscard]] GANull operator^ (const Derived& b) const
    {
        return GANull{};
    }
    template <typename Derived>
    [[nodiscard]] GANull operator& (const Derived& b) const
    {
        return GANull{};
    }

    template <typename Derived>
    friend [[nodiscard]] GANull operator* (const Derived& b, const GANull& element)
    {
        return GANull{};
    }
    template <typename Derived>
    friend [[nodiscard]] GANull operator| (const Derived& b, const GANull& element)
    {
        return GANull{};
    }
    template <typename Derived>
    friend [[nodiscard]] GANull operator^ (const Derived& b, const GANull& element)
    {
        return GANull{};
    }
    template <typename Derived>
    friend [[nodiscard]] GANull operator& (const Derived& b, const GANull& element)
    {
        return GANull{};
    }
};

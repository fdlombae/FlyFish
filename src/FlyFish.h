#pragma once

#include <cmath>
#include <array>
#include <cassert>
#include <sstream>
#include <numbers>

class Vector;
class BiVector;
class TriVector;
class Motor;
class GANull;

constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;

template <typename Derived, std::size_t DataSize>
class GAElement
{
public:
    [[nodiscard]] GAElement() noexcept = default;
    virtual ~GAElement() = default;


    [[nodiscard]] constexpr float& operator [] (size_t idx) noexcept
    {
        assert(idx < DataSize);
        return data[idx];
    }
    [[nodiscard]] constexpr const float& operator [] (size_t idx) const noexcept
    {
        assert(idx < DataSize);
        return data[idx];
    }

    [[nodiscard]] constexpr float& get(size_t index) { return data[index]; }
    [[nodiscard]] constexpr const float& get(size_t index) const { return data[index]; }

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
        os << element.ToString();
        return os;
    }

    [[nodiscard]] std::string ToString() const {
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
    [[nodiscard]] constexpr auto begin() { return data.begin(); }
    [[nodiscard]] constexpr auto end() { return data.end(); }
    [[nodiscard]] constexpr auto begin() const { return data.begin(); }
    [[nodiscard]] constexpr auto end() const { return data.end(); }

    [[nodiscard]] constexpr bool operator== (const GAElement& b) const
    {
        return data == b.data;
    }

    [[nodiscard]] constexpr bool RoundedEqual(const GAElement& b, float tolerance = 1e-6f) const noexcept
    {
        for (size_t i = 0; i < DataSize; ++i) {
            if (std::abs(data[i] - b[i]) > tolerance) {
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

    friend Derived operator*(float scalar, const Derived& element) {
        return element * scalar;
    }

    [[nodiscard]] virtual constexpr float Norm() const
    {
        return 0;
    }

    Derived& Normalize()
    {
        return (*this) /= (*this).Norm();
    }

    [[nodiscard]] Derived Normalized() const
    {
        Derived d{};
        const float mult = 1 / Norm();
        for (size_t idx{}; idx < DataSize; idx++)
        {
            d[idx] = mult * data[idx];
        }
        return d;
    }

protected:
    std::array<float, DataSize> data{};
};

class MultiVector final : public GAElement<MultiVector, 16>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    [[nodiscard]] constexpr float& s() { return get(0); }
    [[nodiscard]] constexpr float& e0() { return get(1); }
    [[nodiscard]] constexpr float& e1() { return get(2); }
    [[nodiscard]] constexpr float& e2() { return get(3); }
    [[nodiscard]] constexpr float& e3() { return get(4); }
    [[nodiscard]] constexpr float& e01() { return get(5); }
    [[nodiscard]] constexpr float& e02() { return get(6); }
    [[nodiscard]] constexpr float& e03() { return get(7); }
    [[nodiscard]] constexpr float& e23() { return get(8); }
    [[nodiscard]] constexpr float& e31() { return get(9); }
    [[nodiscard]] constexpr float& e12() { return get(10); }
    [[nodiscard]] constexpr float& e032() { return get(11); }
    [[nodiscard]] constexpr float& e013() { return get(12); }
    [[nodiscard]] constexpr float& e021() { return get(13); }
    [[nodiscard]] constexpr float& e123() { return get(14); }
    [[nodiscard]] constexpr float& e0123() { return get(15); }

    [[nodiscard]] constexpr const float& s() const { return get(0); }
    [[nodiscard]] constexpr const float& e0() const { return get(1); }
    [[nodiscard]] constexpr const float& e1() const { return get(2); }
    [[nodiscard]] constexpr const float& e2() const { return get(3); }
    [[nodiscard]] constexpr const float& e3() const { return get(4); }
    [[nodiscard]] constexpr const float& e01() const { return get(5); }
    [[nodiscard]] constexpr const float& e02() const { return get(6); }
    [[nodiscard]] constexpr const float& e03() const { return get(7); }
    [[nodiscard]] constexpr const float& e23() const { return get(8); }
    [[nodiscard]] constexpr const float& e31() const { return get(9); }
    [[nodiscard]] constexpr const float& e12() const { return get(10); }
    [[nodiscard]] constexpr const float& e032() const { return get(11); }
    [[nodiscard]] constexpr const float& e013() const { return get(12); }
    [[nodiscard]] constexpr const float& e021() const { return get(13); }
    [[nodiscard]] constexpr const float& e123() const { return get(14); }
    [[nodiscard]] constexpr const float& e0123() const { return get(15); }

    [[nodiscard]] constexpr MultiVector() noexcept : GAElement()
    {
    }

    [[nodiscard]] constexpr MultiVector(float s, float e0, float e1, float e2, float e3, float e01, float e02, float e03, float e23, float e31, float e12, float e032, float e013, float e021, float e123, float e0123) noexcept
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

    MultiVector& operator=(const TriVector& b);
    MultiVector& operator=(TriVector&& b) noexcept;
    MultiVector& operator=(const BiVector& b);
    MultiVector& operator=(BiVector&& b) noexcept;
    MultiVector& operator=(const Vector& b);
    MultiVector& operator=(Vector&& b) noexcept;
    MultiVector& operator=(const Motor& b);
    MultiVector& operator=(Motor&& b) noexcept;

    [[nodiscard]] float Norm() const override
    {
        return std::sqrt(data[0] * data[0] + data[2] * data[2] + data[3] * data[3] + data[4] * data[4] + data[8] * data[8] + data[9] * data[9] + data[10] * data[10] + data[14] * data[14]);
    }
    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[1] * data[1] + data[5] * data[5] + data[6] * data[6] + data[7] * data[7] + data[11] * data[11] + data[12] * data[12] + data[13] * data[13] + data[15] * data[15]);
    }

    [[nodiscard]] Vector Grade1() const;
    [[nodiscard]] BiVector Grade2() const;
    [[nodiscard]] TriVector Grade3() const;
    [[nodiscard]] Motor ToMotor() const;

    [[nodiscard]] MultiVector operator ~() const{
        float norm{ Norm() };
        float normSquared{ norm };
        return {
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
            };
    };

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator* (const TriVector& b) const;
    [[nodiscard]] MultiVector operator* (const Motor& b) const;
    [[nodiscard]] MultiVector operator* (const BiVector& b) const;
    [[nodiscard]] MultiVector operator* (const Vector& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator| (const TriVector& b) const;
    [[nodiscard]] MultiVector operator| (const BiVector& b) const;
    [[nodiscard]] MultiVector operator| (const Vector& b) const;
    [[nodiscard]] MultiVector operator| (const Motor& b) const;


    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator& (const TriVector& b) const;
    [[nodiscard]] MultiVector operator& (const BiVector& b) const;
    [[nodiscard]] MultiVector operator& (const Vector& b) const;
    [[nodiscard]] MultiVector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] MultiVector operator^(const TriVector& b) const;
    [[nodiscard]] MultiVector operator^(const BiVector& b) const;
    [[nodiscard]] MultiVector operator^(const Vector& b) const;
    [[nodiscard]] MultiVector operator^(const Motor& b) const;


    [[nodiscard]] MultiVector operator! () const;
};

class Vector final : public GAElement<Vector, 4>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& e0() { return get(0); }
    inline float& e1() { return get(1); }
    inline float& e2() { return get(2); }
    inline float& e3() { return get(3); }

    [[nodiscard]] inline const float& e0() const { return get(0); }
    [[nodiscard]] inline const float& e1() const { return get(1); }
    [[nodiscard]] inline const float& e2() const { return get(2); }
    [[nodiscard]] inline const float& e3() const { return get(3); }

    Vector() : GAElement()
    {
    }

    [[nodiscard]] Vector(float e0, float e1, float e2, float e3) : GAElement()
    {
        data[0] = e0;
        data[1] = e1;
        data[2] = e2;
        data[3] = e3;
    }

    static constexpr std::array<const char*, 4> names() {
        return { "e0", "e1", "e2", "e3" };
    }

    [[nodiscard]] float Norm() const override
    {
        return std::sqrt(data[1] * data[1] + data[2] * data[2] + data[3] * data[3]);
    }

    [[nodiscard]] Vector operator ~() const
    {
        float norm{ Norm() };
        float normSquared{ norm * norm };
        return {
            data[0] / normSquared,
            data[1] / normSquared,
            data[2] / normSquared,
            data[3] / normSquared
        };
    }

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] Motor operator* (const TriVector& b) const;
    [[nodiscard]] MultiVector operator* (const BiVector& b) const;
    [[nodiscard]] Motor operator* (const Vector& b) const;
    [[nodiscard]] MultiVector operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] float operator& (const TriVector& b) const;
    [[nodiscard]] GANull operator& (const BiVector& b) const;
    [[nodiscard]] GANull operator& (const Vector& b) const;
    [[nodiscard]] Vector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] BiVector operator| (const TriVector& b) const;
    [[nodiscard]] Vector operator| (const BiVector& b) const;
    [[nodiscard]] float operator| (const Vector& b) const;
    [[nodiscard]] MultiVector operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] MultiVector operator^(const TriVector& b) const;
    [[nodiscard]] TriVector operator^ (const BiVector& b) const;
    [[nodiscard]] BiVector operator^(const Vector& b) const;
    [[nodiscard]] MultiVector operator^(const Motor& b) const;


    [[nodiscard]] TriVector operator! () const;
};

class BiVector final : public GAElement<BiVector, 6>
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

    [[nodiscard]] inline const float& e01() const { return get(0); }
    [[nodiscard]] inline const float& e02() const { return get(1); }
    [[nodiscard]] inline const float& e03() const { return get(2); }
    [[nodiscard]] inline const float& e23() const { return get(3); }
    [[nodiscard]] inline const float& e31() const { return get(4); }
    [[nodiscard]] inline const float& e12() const { return get(5); }

    BiVector() : GAElement()
    {
    }

    [[nodiscard]] BiVector(float e01, float e02, float e03, float e23, float e31, float e12) : GAElement()
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

    [[nodiscard]] float PermutedDot(const BiVector& b) const {
        return data[3] * b[0] + data[4] * b[1] + data[5] * b[2] + data[2] * b[5] + data[1] * b[4] + data[0] * b[3];
    }

    [[nodiscard]] static BiVector LineFromPoints(float x1, float y1, float z1, float x2, float y2, float z2)
    {
        return {
            y1 * z2 - y2 * z1,
            z1 * x2 - z2 * x1,
            x1 * y2 - x2 * y1,
            x2 - x1,
            y2 - y1,
            z2 - z1
            };
    }

    [[nodiscard]] float Norm() const override
    {
        return std::sqrt(data[3] * data[3] + data[4] * data[4] + data[5] * data[5]);
    }
    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]);
    }

    [[nodiscard]] BiVector operator ~() const {
        float squareNorm{ Norm() * Norm() };
        return {
            -data[0] / squareNorm,
            -data[1] / squareNorm,
            -data[2] / squareNorm,
            -data[3] / squareNorm,
            -data[4] / squareNorm,
            -data[5] / squareNorm
        };
    };

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator* (const TriVector& b) const;
    [[nodiscard]] Motor operator* (const BiVector& b) const;
    [[nodiscard]] MultiVector operator* (const Vector& b) const;
    [[nodiscard]] Motor operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] Vector operator| (const TriVector& b) const;
    [[nodiscard]] float operator| (const BiVector& b) const;
    [[nodiscard]] Vector operator| (const Vector& b) const;
    [[nodiscard]] Motor operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] Vector operator & (const TriVector& b) const;
    [[nodiscard]] float operator & (const BiVector& b) const;
    [[nodiscard]] GANull operator& (const Vector& b) const;
    [[nodiscard]] MultiVector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator ^ (const MultiVector& b) const;
    [[nodiscard]] GANull operator ^ (const TriVector& b) const;
    [[nodiscard]] MultiVector operator ^ (const BiVector& b) const;
    [[nodiscard]] TriVector operator ^ (const Vector& b) const;
    [[nodiscard]] Motor operator ^ (const Motor& b) const;
    
    [[nodiscard]] BiVector operator! () const;
};

class TriVector final : public GAElement<TriVector, 4>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    inline float& e032() { return get(0); }
    inline float& e013() { return get(1); }
    inline float& e021() { return get(2); }
    inline float& e123() { return get(3); }

    [[nodiscard]] inline const float& e032() const { return get(0); }
    [[nodiscard]] inline const float& e013() const { return get(1); }
    [[nodiscard]] inline const float& e021() const { return get(2); }
    [[nodiscard]] inline const float& e123() const { return get(3); }

    [[nodiscard]] TriVector() : GAElement()
    {
    }

    [[nodiscard]] TriVector(float x, float y, float z) : GAElement()
    {
        data[0] = x;
        data[1] = y;
        data[2] = z;
        data[3] = 1;
    }

    [[nodiscard]] TriVector(float e032, float e013, float e021, float e123) : GAElement()
    {
        data[0] = e032;
        data[1] = e013;
        data[2] = e021;
        data[3] = e123;
    }

    static constexpr std::array<const char*, 4> names() {
        return { "e032", "e013", "e021", "e123" };
    }

    [[nodiscard]] constexpr float Norm() const override
    {
        return data[3];
    }

    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[0] * data[0] + data[1] * data[1] + data[2] * data[2]);
    }

    [[nodiscard]] TriVector operator ~() const
    {
        float norm{ Norm() };
        float normSquared{ norm * norm };
        return {
            -data[0] / normSquared,
            -data[1] / normSquared,
            -data[2] / normSquared,
            -data[3] / normSquared
        };
    }

    [[nodiscard]] Vector operator! () const;

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] Motor operator* (const TriVector& b) const;
    [[nodiscard]] MultiVector operator* (const BiVector& b) const;
    [[nodiscard]] Motor operator* (const Vector& b) const;
    [[nodiscard]] MultiVector operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] float operator| (const TriVector& b) const;
    [[nodiscard]] Vector operator| (const BiVector& b) const;
    [[nodiscard]] BiVector operator| (const Vector& b) const;
    [[nodiscard]] Vector operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] BiVector operator& (const TriVector& b) const;
    [[nodiscard]] Vector operator& (const BiVector& b) const;
    [[nodiscard]] float operator& (const Vector& b) const;
    [[nodiscard]] MultiVector operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] GANull operator^(const TriVector& b) const;
    [[nodiscard]] GANull operator^(const BiVector& b) const;
    [[nodiscard]] float operator^(const Vector& b) const;
    [[nodiscard]] TriVector operator^(const Motor& b) const;
};

class Motor final : public GAElement<Motor, 8>
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

    [[nodiscard]] inline const float& s() const { return get(0); }
    [[nodiscard]] inline const float& e01() const { return get(1); }
    [[nodiscard]] inline const float& e02() const { return get(2); }
    [[nodiscard]] inline const float& e03() const { return get(3); }
    [[nodiscard]] inline const float& e23() const { return get(4); }
    [[nodiscard]] inline const float& e31() const { return get(5); }
    [[nodiscard]] inline const float& e12() const { return get(6); }
    [[nodiscard]] inline const float& e0123() const { return get(7); }

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

    [[nodiscard]] static Motor Translation(float translation, const BiVector& line)
    {
        const float d{ -translation / (2 * line.VNorm()) };
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

    [[nodiscard]] static Motor Rotation(float angle, const BiVector& line)
    {
        const float mult{ -sin(angle * DEG_TO_RAD / 2) / line.Norm() };
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

    [[nodiscard]] float Norm() const override
    {
        return std::sqrt(data[0] * data[0] + data[4] * data[4] + data[5] * data[5] + data[6] * data[6]);
    }

    [[nodiscard]] float VNorm() const
    {
        return std::sqrt(data[1] * data[1] + data[2] * data[2] + data[3] * data[3] + data[7] * data[7]);
    }

    [[nodiscard]] BiVector Grade2() const;

    [[nodiscard]] Motor operator ~() const {
        float norm{ Norm() };
        float normSquared{ norm * norm };
        return {
            data[0] / normSquared ,
            -data[1] / normSquared,
            -data[2] / normSquared,
            -data[3] / normSquared,
            -data[4] / normSquared,
            -data[5] / normSquared,
            -data[6] / normSquared,
            data[7] / normSquared
        };
    };

    [[nodiscard]] MultiVector operator* (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator* (const TriVector& b) const;
    [[nodiscard]] Motor operator* (const BiVector& b) const;
    [[nodiscard]] MultiVector operator* (const Vector& b) const;
    [[nodiscard]] Motor operator* (const Motor& b) const;

    [[nodiscard]] MultiVector operator| (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator| (const TriVector& b) const;
    [[nodiscard]] Motor operator| (const BiVector& b) const;
    [[nodiscard]] MultiVector operator| (const Vector& b) const;
    [[nodiscard]] Motor operator| (const Motor& b) const;

    [[nodiscard]] MultiVector operator& (const MultiVector& b) const;
    [[nodiscard]] MultiVector operator& (const TriVector& b) const;
    [[nodiscard]] Motor operator& (const BiVector& b) const;
    [[nodiscard]] Vector operator& (const Vector& b) const;
    [[nodiscard]] Motor operator& (const Motor& b) const;

    [[nodiscard]] MultiVector operator^(const MultiVector& b) const;
    [[nodiscard]] TriVector operator^(const TriVector& b) const;
    [[nodiscard]] Motor operator^(const BiVector& b) const;
    [[nodiscard]] MultiVector operator^(const Vector& b) const;
    [[nodiscard]] MultiVector operator^(const Motor& b) const;

    Motor& operator += (const BiVector& b)
    {
        for (size_t idx{}; idx < 6; idx++)
        {
            data[idx + 1] += b[idx];
        }

        return (*this);
    }
    Motor& operator -= (const BiVector& b)
    {
        for (size_t idx{}; idx < 6; idx++)
        {
            data[idx + 1] -= b[idx];
        }

        return (*this);
    }

    [[nodiscard]] Motor operator! () const;
};

class GANull final : public GAElement<GANull, 0>
{
public:
    using GAElement::GAElement;
    using GAElement::operator*;
    using GAElement::operator/;

    [[nodiscard]] static std::string ToString() {
        return "GANull";
    }

    template <typename Derived>
    [[nodiscard]] GANull operator* (const Derived&) const
    {
        return GANull{};
    }
    template <typename Derived>
    [[nodiscard]] GANull operator| (const Derived&) const
    {
        return GANull{};
    }
    template <typename Derived>
    [[nodiscard]] GANull operator^ (const Derived&) const
    {
        return GANull{};
    }
    template <typename Derived>
    [[nodiscard]] GANull operator& (const Derived&) const
    {
        return GANull{};
    }

    template <typename Derived>
    friend GANull operator* (const Derived&, const GANull&)
    {
        return GANull{};
    }
    template <typename Derived>
    friend GANull operator| (const Derived&, const GANull&)
    {
        return GANull{};
    }
    template <typename Derived>
    friend GANull operator^ (const Derived&, const GANull&)
    {
        return GANull{};
    }
    template <typename Derived>
    friend GANull operator& (const Derived&, const GANull&)
    {
        return GANull{};
    }
};

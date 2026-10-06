#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include <sstream>
#include <stdexcept>

class Vector;
class BiVector;
class TriVector;
class Motor;
class GANull;

constexpr float DEG_TO_RAD = std::numbers::pi_v<float> / 180.0f;
constexpr float RAD_TO_DEG = 1 / DEG_TO_RAD;

///////////////////////////////////////////////////////////////////////////////////
/// Constexpr math helpers — compile-time via series, runtime via hardware
/// intrinsics
///////////////////////////////////////////////////////////////////////////////////

namespace detail {
constexpr float ce_abs(float x) { return x < 0 ? -x : x; }

constexpr float ce_sqrt(float x) {
  if consteval {
    if (x == 0.0f)
      return 0.0f;
    float guess = x;
    for (int i = 0; i < 30; ++i)
      guess = 0.5f * (guess + x / guess);
    return guess;
  } else {
    return std::sqrt(x);
  }
}

constexpr float ce_fmod(float x, float y) {
  return x - static_cast<float>(static_cast<int>(x / y)) * y;
}

constexpr float ce_sin(float x) {
  if consteval {
    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float two_pi = 2.0f * pi;
    x = ce_fmod(x, two_pi);
    if (x > pi)
      x -= two_pi;
    if (x < -pi)
      x += two_pi;
    float result = 0.0f;
    float term = x;
    for (int i = 0; i < 12; ++i) {
      result += term;
      term *= -x * x / static_cast<float>((2 * i + 2) * (2 * i + 3));
    }
    return result;
  } else {
    return std::sin(x);
  }
}

constexpr float ce_cos(float x) {
  if consteval {
    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float two_pi = 2.0f * pi;
    x = ce_fmod(x, two_pi);
    if (x > pi)
      x -= two_pi;
    if (x < -pi)
      x += two_pi;
    float result = 0.0f;
    float term = 1.0f;
    for (int i = 0; i < 12; ++i) {
      result += term;
      term *= -x * x / static_cast<float>((2 * i + 1) * (2 * i + 2));
    }
    return result;
  } else {
    return std::cos(x);
  }
}

constexpr float ce_sinh(float x) {
  if consteval {
    float result = 0.0f;
    float term = x;
    for (int i = 0; i < 12; ++i) {
      result += term;
      term *= x * x / static_cast<float>((2 * i + 2) * (2 * i + 3));
    }
    return result;
  } else {
    return std::sinh(x);
  }
}

constexpr float ce_cosh(float x) {
  if consteval {
    float result = 0.0f;
    float term = 1.0f;
    for (int i = 0; i < 12; ++i) {
      result += term;
      term *= x * x / static_cast<float>((2 * i + 1) * (2 * i + 2));
    }
    return result;
  } else {
    return std::cosh(x);
  }
}
} // namespace detail

template <typename Derived, std::size_t DataSize> class GAElement {
public:
  [[nodiscard]] constexpr GAElement() noexcept = default;
  constexpr GAElement(const GAElement &) noexcept = default;
  constexpr GAElement(GAElement &&) noexcept = default;
  constexpr GAElement &operator=(const GAElement &) noexcept = default;
  constexpr GAElement &operator=(GAElement &&) noexcept = default;

  [[nodiscard]] constexpr float &operator[](size_t idx) noexcept {
    assert(idx < DataSize);
    return data[idx];
  }
  [[nodiscard]] constexpr const float &operator[](size_t idx) const noexcept {
    assert(idx < DataSize);
    return data[idx];
  }

  friend std::ostream &operator<<(std::ostream &os, const Derived &element) {
    os << element.ToString();
    return os;
  }

  [[nodiscard]] std::string ToString() const {
    std::ostringstream output;
    const auto &names = Derived::names();
    bool first = true;

    for (size_t i = 0; i < data.size(); ++i) {
      if (std::fabs(data[i]) > 1e-6) {
        if (!first) {
          output << (data[i] > 0 ? " + " : " - ");
        } else if (data[i] < 0) {
          output << "-";
        }
        first = false;

        if (std::fabs(data[i]) != 1) {
          output << std::fabs(data[i]);
          if (names[i] != "") {
            output << "*";
          }
        }

        if (names[i] != "") {
          output << names[i];
        } else if (std::fabs(data[i]) == 1) {
          output << '1';
        }
      }
    }

    return first ? "0"
                 : output.str(); // Return "0" if all coefficients are zero.
  }

  // Iterator support
  [[nodiscard]] constexpr auto begin() { return data.begin(); }
  [[nodiscard]] constexpr auto end() { return data.end(); }
  [[nodiscard]] constexpr auto begin() const { return data.begin(); }
  [[nodiscard]] constexpr auto end() const { return data.end(); }

  [[nodiscard]] constexpr bool operator==(const GAElement &b) const {
    return data == b.data;
  }

  [[nodiscard]] constexpr bool
  RoundedEqual(const GAElement &b, float tolerance = 1e-6f) const noexcept {
    for (size_t i = 0; i < DataSize; ++i) {
      if (detail::ce_abs(data[i] - b[i]) > tolerance) {
        return false;
      }
    }
    return true;
  }

  constexpr Derived &operator+=(const Derived &b) {
    for (size_t idx{}; idx < DataSize; idx++) {
      data[idx] += b[idx];
    }

    return static_cast<Derived &>(*this);
  }
  constexpr Derived &operator-=(const Derived &b) {
    for (size_t idx{}; idx < DataSize; idx++) {
      data[idx] -= b[idx];
    }

    return static_cast<Derived &>(*this);
  }
  constexpr Derived &operator*=(float s) {
    for (size_t idx{}; idx < DataSize; idx++) {
      data[idx] *= s;
    }
    return static_cast<Derived &>(*this);
  }
  constexpr Derived &operator/=(float s) {
    float reciprocal = 1 / s;
    for (size_t idx{}; idx < DataSize; idx++) {
      data[idx] *= reciprocal;
    }
    return static_cast<Derived &>(*this);
  }

  [[nodiscard]] constexpr Derived operator*(float s) const {
    Derived d{};
    for (size_t idx{}; idx < DataSize; idx++) {
      d[idx] = s * data[idx];
    }
    return d;
  }
  [[nodiscard]] constexpr Derived operator/(float s) const {
    Derived d{};
    float mult = 1 / s;
    for (size_t idx{}; idx < DataSize; idx++) {
      d[idx] = mult * data[idx];
    }
    return d;
  }
  [[nodiscard]] constexpr Derived operator-() const {
    Derived d{};
    for (size_t idx{}; idx < DataSize; idx++) {
      d[idx] = -data[idx];
    }
    return d;
  }
  [[nodiscard]] constexpr Derived operator+(const Derived &b) const {
    Derived d{};
    for (size_t idx{}; idx < DataSize; idx++) {
      d[idx] = data[idx] + b[idx];
    }
    return d;
  }
  [[nodiscard]] constexpr Derived operator-(const Derived &b) const {
    Derived d{};
    for (size_t idx{}; idx < DataSize; idx++) {
      d[idx] = data[idx] - b[idx];
    }
    return d;
  }

  friend constexpr Derived operator*(float scalar, const Derived &element) {
    return element * scalar;
  }
  friend constexpr Derived operator/(float scalar, const Derived &element) {
    return scalar * Inverse(element);
  }

  constexpr Derived &Normalize() {
    return (*this) /= static_cast<const Derived *>(this)->Norm();
  }

  [[nodiscard]] constexpr Derived Normalized() const {
    Derived d{};
    const float mult = 1 / static_cast<const Derived *>(this)->Norm();
    for (size_t idx{}; idx < DataSize; idx++) {
      d[idx] = mult * data[idx];
    }
    return d;
  }

protected:
  std::array<float, DataSize> data{};
};

class MultiVector final : public GAElement<MultiVector, 16> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] constexpr float &s() { return data[0]; }
  [[nodiscard]] constexpr float &e0() { return data[1]; }
  [[nodiscard]] constexpr float &e1() { return data[2]; }
  [[nodiscard]] constexpr float &e2() { return data[3]; }
  [[nodiscard]] constexpr float &e3() { return data[4]; }
  [[nodiscard]] constexpr float &e01() { return data[5]; }
  [[nodiscard]] constexpr float &e02() { return data[6]; }
  [[nodiscard]] constexpr float &e03() { return data[7]; }
  [[nodiscard]] constexpr float &e23() { return data[8]; }
  [[nodiscard]] constexpr float &e31() { return data[9]; }
  [[nodiscard]] constexpr float &e12() { return data[10]; }
  [[nodiscard]] constexpr float &e032() { return data[11]; }
  [[nodiscard]] constexpr float &e013() { return data[12]; }
  [[nodiscard]] constexpr float &e021() { return data[13]; }
  [[nodiscard]] constexpr float &e123() { return data[14]; }
  [[nodiscard]] constexpr float &e0123() { return data[15]; }

  [[nodiscard]] constexpr const float &s() const { return data[0]; }
  [[nodiscard]] constexpr const float &e0() const { return data[1]; }
  [[nodiscard]] constexpr const float &e1() const { return data[2]; }
  [[nodiscard]] constexpr const float &e2() const { return data[3]; }
  [[nodiscard]] constexpr const float &e3() const { return data[4]; }
  [[nodiscard]] constexpr const float &e01() const { return data[5]; }
  [[nodiscard]] constexpr const float &e02() const { return data[6]; }
  [[nodiscard]] constexpr const float &e03() const { return data[7]; }
  [[nodiscard]] constexpr const float &e23() const { return data[8]; }
  [[nodiscard]] constexpr const float &e31() const { return data[9]; }
  [[nodiscard]] constexpr const float &e12() const { return data[10]; }
  [[nodiscard]] constexpr const float &e032() const { return data[11]; }
  [[nodiscard]] constexpr const float &e013() const { return data[12]; }
  [[nodiscard]] constexpr const float &e021() const { return data[13]; }
  [[nodiscard]] constexpr const float &e123() const { return data[14]; }
  [[nodiscard]] constexpr const float &e0123() const { return data[15]; }

  [[nodiscard]] constexpr MultiVector() noexcept : GAElement() {}

  [[nodiscard]] constexpr MultiVector(
      float s, float e0 = 0, float e1 = 0, float e2 = 0, float e3 = 0,
      float e01 = 0, float e02 = 0, float e03 = 0, float e23 = 0, float e31 = 0,
      float e12 = 0, float e032 = 0, float e013 = 0, float e021 = 0,
      float e123 = 0, float e0123 = 0) noexcept {
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

  static constexpr std::array<const char *, 16> names() {
    return {"",    "e0",  "e1",  "e2",   "e3",   "e01",  "e02",  "e03",
            "e23", "e31", "e12", "e032", "e013", "e021", "e123", "e0123"};
  }

  constexpr MultiVector &operator=(const TriVector &b);
  constexpr MultiVector &operator=(const BiVector &b);
  constexpr MultiVector &operator=(const Vector &b);
  constexpr MultiVector &operator=(const Motor &b);

  [[nodiscard]] constexpr float Norm() const {
    return detail::ce_sqrt(data[0] * data[0] + data[2] * data[2] +
                           data[3] * data[3] + data[4] * data[4] +
                           data[8] * data[8] + data[9] * data[9] +
                           data[10] * data[10] + data[14] * data[14]);
  }
  [[nodiscard]] constexpr float VNorm() const {
    return detail::ce_sqrt(data[1] * data[1] + data[5] * data[5] +
                           data[6] * data[6] + data[7] * data[7] +
                           data[11] * data[11] + data[12] * data[12] +
                           data[13] * data[13] + data[15] * data[15]);
  }

  [[nodiscard]] constexpr Vector Grade1() const;
  [[nodiscard]] constexpr BiVector Grade2() const;
  [[nodiscard]] constexpr TriVector Grade3() const;
  [[nodiscard]] constexpr Motor ToMotor() const;

  [[nodiscard]] constexpr MultiVector operator~() const;

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Motor &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Vector &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator!() const;

  friend MultiVector Gexp([[maybe_unused]] MultiVector &m) {
    throw std::runtime_error("Not implemented");
  }
};

class Vector final : public GAElement<Vector, 4> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] constexpr float &e0() { return data[0]; }
  [[nodiscard]] constexpr float &e1() { return data[1]; }
  [[nodiscard]] constexpr float &e2() { return data[2]; }
  [[nodiscard]] constexpr float &e3() { return data[3]; }

  [[nodiscard]] constexpr const float &e0() const { return data[0]; }
  [[nodiscard]] constexpr const float &e1() const { return data[1]; }
  [[nodiscard]] constexpr const float &e2() const { return data[2]; }
  [[nodiscard]] constexpr const float &e3() const { return data[3]; }

  [[nodiscard]] constexpr Vector() noexcept : GAElement() {}

  [[nodiscard]] constexpr Vector(float e0, float e1, float e2,
                                 float e3) noexcept
      : GAElement() {
    data[0] = e0;
    data[1] = e1;
    data[2] = e2;
    data[3] = e3;
  }

  static constexpr std::array<const char *, 4> names() {
    return {"e0", "e1", "e2", "e3"};
  }

  [[nodiscard]] constexpr float Norm() const {
    return detail::ce_sqrt(data[1] * data[1] + data[2] * data[2] +
                           data[3] * data[3]);
  }

  [[nodiscard]] constexpr Vector operator~() const {
    float inv{1.0f /
              (data[1] * data[1] + data[2] * data[2] + data[3] * data[3])};
    return {data[0] * inv, data[1] * inv, data[2] * inv, data[3] * inv};
  }

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const BiVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr float operator&(const TriVector &b) const;
  [[nodiscard]] constexpr GANull operator&(const BiVector &b) const;
  [[nodiscard]] constexpr GANull operator&(const Vector &b) const;
  [[nodiscard]] constexpr Vector operator&(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr BiVector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr float operator|(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const TriVector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const BiVector &b) const;
  [[nodiscard]] constexpr BiVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Motor &b) const;

  [[nodiscard]] constexpr TriVector operator!() const;

  [[nodiscard]] constexpr MultiVector Gexp() const;
};

class BiVector final : public GAElement<BiVector, 6> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] constexpr float &e01() { return data[0]; }
  [[nodiscard]] constexpr float &e02() { return data[1]; }
  [[nodiscard]] constexpr float &e03() { return data[2]; }
  [[nodiscard]] constexpr float &e23() { return data[3]; }
  [[nodiscard]] constexpr float &e31() { return data[4]; }
  [[nodiscard]] constexpr float &e12() { return data[5]; }

  [[nodiscard]] constexpr const float &e01() const { return data[0]; }
  [[nodiscard]] constexpr const float &e02() const { return data[1]; }
  [[nodiscard]] constexpr const float &e03() const { return data[2]; }
  [[nodiscard]] constexpr const float &e23() const { return data[3]; }
  [[nodiscard]] constexpr const float &e31() const { return data[4]; }
  [[nodiscard]] constexpr const float &e12() const { return data[5]; }

  [[nodiscard]] constexpr BiVector() noexcept : GAElement() {}

  [[nodiscard]] constexpr BiVector(float e01, float e02, float e03, float e23,
                                   float e31, float e12) noexcept
      : GAElement() {
    data[0] = e01;
    data[1] = e02;
    data[2] = e03;
    data[3] = e23;
    data[4] = e31;
    data[5] = e12;
  }

  static constexpr std::array<const char *, 6> names() {
    return {"e01", "e02", "e03", "e23", "e31", "e12"};
  }

  [[nodiscard]] constexpr float PermutedDot(const BiVector &b) const {
    return data[3] * b[0] + data[4] * b[1] + data[5] * b[2] + data[2] * b[5] +
           data[1] * b[4] + data[0] * b[3];
  }

  [[nodiscard]] static constexpr BiVector
  LineFromPoints(float x1, float y1, float z1, float x2, float y2, float z2) {
    return {y1 * z2 - y2 * z1, z1 * x2 - z2 * x1, x1 * y2 - x2 * y1,
            x2 - x1,           y2 - y1,           z2 - z1};
  }

  [[nodiscard]] constexpr float Norm() const {
    return detail::ce_sqrt(data[3] * data[3] + data[4] * data[4] +
                           data[5] * data[5]);
  }
  [[nodiscard]] constexpr float VNorm() const {
    return detail::ce_sqrt(data[0] * data[0] + data[1] * data[1] +
                           data[2] * data[2]);
  }

  [[nodiscard]] constexpr BiVector operator~() const {
    float inv{1.0f /
              (data[3] * data[3] + data[4] * data[4] + data[5] * data[5])};
    return {-data[0] * inv, -data[1] * inv, -data[2] * inv,
            -data[3] * inv, -data[4] * inv, -data[5] * inv};
  }

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr float operator|(const BiVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator|(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr float operator&(const BiVector &b) const;
  [[nodiscard]] constexpr GANull operator&(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const BiVector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator^(const Motor &b) const;

  [[nodiscard]] constexpr BiVector operator!() const;

  [[nodiscard]] constexpr Motor Gexp() const;
};

class TriVector final : public GAElement<TriVector, 4> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] constexpr float &e032() { return data[0]; }
  [[nodiscard]] constexpr float &e013() { return data[1]; }
  [[nodiscard]] constexpr float &e021() { return data[2]; }
  [[nodiscard]] constexpr float &e123() { return data[3]; }

  [[nodiscard]] constexpr const float &e032() const { return data[0]; }
  [[nodiscard]] constexpr const float &e013() const { return data[1]; }
  [[nodiscard]] constexpr const float &e021() const { return data[2]; }
  [[nodiscard]] constexpr const float &e123() const { return data[3]; }

  [[nodiscard]] constexpr TriVector() noexcept : GAElement() {}

  [[nodiscard]] constexpr TriVector(float x, float y, float z) noexcept
      : GAElement() {
    data[0] = x;
    data[1] = y;
    data[2] = z;
    data[3] = 1;
  }

  [[nodiscard]] constexpr TriVector(float e032, float e013, float e021,
                                    float e123) noexcept
      : GAElement() {
    data[0] = e032;
    data[1] = e013;
    data[2] = e021;
    data[3] = e123;
  }

  static constexpr std::array<const char *, 4> names() {
    return {"e032", "e013", "e021", "e123"};
  }

  [[nodiscard]] constexpr float Norm() const { return detail::ce_abs(data[3]); }

  [[nodiscard]] constexpr float VNorm() const {
    return detail::ce_sqrt(data[0] * data[0] + data[1] * data[1] +
                           data[2] * data[2]);
  }

  [[nodiscard]] constexpr TriVector operator~() const {
    float inv{1.0f / (data[3] * data[3])};
    return {-data[0] * inv, -data[1] * inv, -data[2] * inv, -data[3] * inv};
  }

  [[nodiscard]] constexpr Vector operator!() const;

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const BiVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr float operator|(const TriVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr BiVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr BiVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const BiVector &b) const;
  [[nodiscard]] constexpr float operator&(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const TriVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const BiVector &b) const;
  [[nodiscard]] constexpr float operator^(const Vector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector Gexp() const;
};

class Motor final : public GAElement<Motor, 8> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;
  using GAElement::operator+=;
  using GAElement::operator-=;

  [[nodiscard]] constexpr float &s() { return data[0]; }
  [[nodiscard]] constexpr float &e01() { return data[1]; }
  [[nodiscard]] constexpr float &e02() { return data[2]; }
  [[nodiscard]] constexpr float &e03() { return data[3]; }
  [[nodiscard]] constexpr float &e23() { return data[4]; }
  [[nodiscard]] constexpr float &e31() { return data[5]; }
  [[nodiscard]] constexpr float &e12() { return data[6]; }
  [[nodiscard]] constexpr float &e0123() { return data[7]; }

  [[nodiscard]] constexpr const float &s() const { return data[0]; }
  [[nodiscard]] constexpr const float &e01() const { return data[1]; }
  [[nodiscard]] constexpr const float &e02() const { return data[2]; }
  [[nodiscard]] constexpr const float &e03() const { return data[3]; }
  [[nodiscard]] constexpr const float &e23() const { return data[4]; }
  [[nodiscard]] constexpr const float &e31() const { return data[5]; }
  [[nodiscard]] constexpr const float &e12() const { return data[6]; }
  [[nodiscard]] constexpr const float &e0123() const { return data[7]; }

  [[nodiscard]] constexpr Motor() noexcept : GAElement() {}

  [[nodiscard]] constexpr Motor(float s, float e01, float e02, float e03,
                                float e23, float e31, float e12,
                                float e0123) noexcept
      : GAElement() {
    data[0] = s;
    data[1] = e01;
    data[2] = e02;
    data[3] = e03;
    data[4] = e23;
    data[5] = e31;
    data[6] = e12;
    data[7] = e0123;
  }

  static constexpr std::array<const char *, 8> names() {
    return {"", "e01", "e02", "e03", "e23", "e31", "e12", "e0123"};
  }

  [[nodiscard]] static constexpr Motor Translation(float translation,
                                                   const BiVector &line) {
    const float d{-translation / (2 * line.VNorm())};
    return Motor{1, d * line[0], d * line[1], d * line[2], 0, 0, 0, 0};
  }

  [[nodiscard]] static constexpr Motor Rotation(float angle,
                                                const BiVector &line) {
    const float mult{-detail::ce_sin(0.5f * angle * DEG_TO_RAD) / line.Norm()};
    return Motor{detail::ce_cos(0.5f * angle * DEG_TO_RAD),
                 mult * line[0],
                 mult * line[1],
                 mult * line[2],
                 mult * line[3],
                 mult * line[4],
                 mult * line[5],
                 0};
  }

  [[nodiscard]] constexpr float Norm() const {
    return detail::ce_sqrt(data[0] * data[0] + data[4] * data[4] +
                           data[5] * data[5] + data[6] * data[6]);
  }

  [[nodiscard]] constexpr float VNorm() const {
    return detail::ce_sqrt(data[1] * data[1] + data[2] * data[2] +
                           data[3] * data[3] + data[7] * data[7]);
  }

  [[nodiscard]] constexpr BiVector Grade2() const;

  [[nodiscard]] constexpr Motor operator~() const {
    float inv{1.0f / (data[0] * data[0] + data[4] * data[4] +
                      data[5] * data[5] + data[6] * data[6])};
    return {data[0] * inv,  -data[1] * inv, -data[2] * inv, -data[3] * inv,
            -data[4] * inv, -data[5] * inv, -data[6] * inv, data[7] * inv};
  }

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator|(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator|(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator&(const BiVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator&(const Motor &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator^(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Motor &b) const;

  constexpr Motor &operator+=(const BiVector &b) {
    for (size_t idx{}; idx < 6; idx++) {
      data[idx + 1] += b[idx];
    }

    return (*this);
  }
  constexpr Motor &operator-=(const BiVector &b) {
    for (size_t idx{}; idx < 6; idx++) {
      data[idx + 1] -= b[idx];
    }

    return (*this);
  }

  [[nodiscard]] constexpr Motor operator!() const;

  [[nodiscard]] constexpr Motor Gexp() const;
};

class GANull final : public GAElement<GANull, 0> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] static std::string ToString() { return "GANull"; }

  template <typename Derived>
  [[nodiscard]] constexpr GANull operator*(const Derived &) const {
    return GANull{};
  }
  template <typename Derived>
  [[nodiscard]] constexpr GANull operator|(const Derived &) const {
    return GANull{};
  }
  template <typename Derived>
  [[nodiscard]] constexpr GANull operator^(const Derived &) const {
    return GANull{};
  }
  template <typename Derived>
  [[nodiscard]] constexpr GANull operator&(const Derived &) const {
    return GANull{};
  }

  template <typename Derived>
  friend constexpr GANull operator*(const Derived &, const GANull &) {
    return GANull{};
  }
  template <typename Derived>
  friend constexpr GANull operator|(const Derived &, const GANull &) {
    return GANull{};
  }
  template <typename Derived>
  friend constexpr auto operator^(const Derived &, const GANull &) -> GANull {
    return GANull{};
  }
  template <typename Derived>
  friend constexpr GANull operator&(const Derived &, const GANull &) {
    return GANull{};
  }
};

///////////////////////////////////////////////////////////////////////////////////
/// Out-of-line constexpr definitions
///////////////////////////////////////////////////////////////////////////////////

// Type conversions
constexpr Vector MultiVector::Grade1() const {
  return Vector{data[1], data[2], data[3], data[4]};
}
constexpr BiVector MultiVector::Grade2() const {
  return BiVector{data[5], data[6], data[7], data[8], data[9], data[10]};
}
constexpr TriVector MultiVector::Grade3() const {
  return TriVector{data[11], data[12], data[13], data[14]};
}
constexpr Motor MultiVector::ToMotor() const {
  return Motor{data[0], data[5], data[6],  data[7],
               data[8], data[9], data[10], data[15]};
}
constexpr BiVector Motor::Grade2() const {
  return {data[1], data[2], data[3], data[4], data[5], data[6]};
}

// Cross-type assignments
constexpr MultiVector &MultiVector::operator=(const TriVector &b) {
  data.fill(0);
  data[11] = b[0];
  data[12] = b[1];
  data[13] = b[2];
  data[14] = b[3];
  return *this;
}
constexpr MultiVector &MultiVector::operator=(const BiVector &b) {
  data.fill(0);
  data[5] = b[0];
  data[6] = b[1];
  data[7] = b[2];
  data[8] = b[3];
  data[9] = b[4];
  data[10] = b[5];
  return *this;
}
constexpr MultiVector &MultiVector::operator=(const Vector &b) {
  data.fill(0);
  data[1] = b[0];
  data[2] = b[1];
  data[3] = b[2];
  data[4] = b[3];
  return *this;
}
constexpr MultiVector &MultiVector::operator=(const Motor &b) {
  data.fill(0);
  data[0] = b[0];
  data[5] = b[1];
  data[6] = b[2];
  data[7] = b[3];
  data[8] = b[4];
  data[9] = b[5];
  data[10] = b[6];
  data[15] = b[7];
  return *this;
}

// MultiVector inverse
constexpr MultiVector MultiVector::operator~() const {
  float s{}, t0{}, t1{}, t2{}, t3{}, ps{};
  s = data[0] * data[0] - data[2] * data[2] - data[3] * data[3] -
      data[4] * data[4] + data[10] * data[10] + data[9] * data[9] +
      data[8] * data[8] - data[14] * data[14];
  t0 = -2 * (data[11] * data[0] + data[8] * data[1] + data[15] * data[2] -
             data[7] * data[3] + data[6] * data[4] - data[14] * data[5] +
             data[12] * data[10] - data[13] * data[9]);
  t1 = -2 * (data[12] * data[0] + data[9] * data[1] + data[7] * data[2] +
             data[15] * data[3] - data[5] * data[4] - data[14] * data[6] -
             data[11] * data[10] + data[13] * data[8]);
  t2 = -2 * (data[13] * data[0] + data[10] * data[1] - data[6] * data[2] +
             data[5] * data[3] + data[15] * data[4] - data[14] * data[7] +
             data[11] * data[9] - data[12] * data[8]);
  t3 = -2 * (data[14] * data[0] - data[8] * data[2] - data[9] * data[3] -
             data[10] * data[4]);
  ps = -2 * (data[15] * data[0] + data[14] * data[1] + data[11] * data[2] +
             data[12] * data[3] + data[13] * data[4] - data[8] * data[5] -
             data[9] * data[6] - data[10] * data[7]);
  float denom{s * s + t3 * t3};
  MultiVector numer{s * data[0] - t3 * data[14],
                    -s * data[1] - t2 * data[10] - t1 * data[9] - t0 * data[8] +
                        ps * data[14] - t3 * data[15],
                    -s * data[2] + t3 * data[8],
                    -s * data[3] + t3 * data[9],
                    -s * data[4] + t3 * data[10],
                    t2 * data[3] - t1 * data[4] - s * data[5] + ps * data[8] +
                        t3 * data[11] - t0 * data[14],
                    -t2 * data[2] + t0 * data[4] - s * data[6] + ps * data[9] +
                        t3 * data[12] - t1 * data[14],
                    t1 * data[2] - t0 * data[3] - s * data[7] + ps * data[10] +
                        t3 * data[13] - t2 * data[14],
                    -t3 * data[2] - s * data[8],
                    -t3 * data[3] - s * data[9],
                    -t3 * data[4] - s * data[10],
                    t0 * data[0] - ps * data[2] + t3 * data[5] - t1 * data[10] +
                        t2 * data[9] + s * data[11],
                    t1 * data[0] - ps * data[3] + t3 * data[6] + t0 * data[10] -
                        t2 * data[8] + s * data[12],
                    t2 * data[0] - ps * data[4] + t3 * data[7] - t0 * data[9] +
                        t1 * data[8] + s * data[13],
                    t3 * data[0] + s * data[14],
                    ps * data[0] - t3 * data[1] - t0 * data[2] - t1 * data[3] -
                        t2 * data[4] + s * data[15]};
  return numer / denom;
}

///////////////////////////////////////////////////////////////////////////////////
/// Geometric Product
///////////////////////////////////////////////////////////////////////////////////

constexpr MultiVector MultiVector::operator*(const MultiVector &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0] + b[2] * data[2] + b[3] * data[3] + b[4] * data[4] -
           b[10] * data[10] - b[9] * data[9] - b[8] * data[8] -
           b[14] * data[14];
  res[1] = b[1] * data[0] + b[0] * data[1] - b[5] * data[2] - b[6] * data[3] -
           b[7] * data[4] + b[2] * data[5] + b[3] * data[6] + b[4] * data[7] +
           b[13] * data[10] + b[12] * data[9] + b[11] * data[8] +
           b[10] * data[13] + b[9] * data[12] + b[8] * data[11] +
           b[15] * data[14] - b[14] * data[15];
  res[2] = b[2] * data[0] + b[0] * data[2] - b[10] * data[3] + b[9] * data[4] +
           b[3] * data[10] - b[4] * data[9] - b[14] * data[8] - b[8] * data[14];
  res[3] = b[3] * data[0] + b[10] * data[2] + b[0] * data[3] - b[8] * data[4] -
           b[2] * data[10] - b[14] * data[9] + b[4] * data[8] - b[9] * data[14];
  res[4] = b[4] * data[0] - b[9] * data[2] + b[8] * data[3] + b[0] * data[4] -
           b[14] * data[10] + b[2] * data[9] - b[3] * data[8] -
           b[10] * data[14];
  res[5] = b[5] * data[0] + b[2] * data[1] - b[1] * data[2] - b[13] * data[3] +
           b[12] * data[4] + b[0] * data[5] - b[10] * data[6] + b[9] * data[7] +
           b[6] * data[10] - b[7] * data[9] - b[15] * data[8] -
           b[3] * data[13] + b[4] * data[12] + b[14] * data[11] -
           b[11] * data[14] - b[8] * data[15];
  res[6] = b[6] * data[0] + b[3] * data[1] + b[13] * data[2] - b[1] * data[3] -
           b[11] * data[4] + b[10] * data[5] + b[0] * data[6] - b[8] * data[7] -
           b[5] * data[10] - b[15] * data[9] + b[7] * data[8] +
           b[2] * data[13] + b[14] * data[12] - b[4] * data[11] -
           b[12] * data[14] - b[9] * data[15];
  res[7] = b[7] * data[0] + b[4] * data[1] - b[12] * data[2] + b[11] * data[3] -
           b[1] * data[4] - b[9] * data[5] + b[8] * data[6] + b[0] * data[7] -
           b[15] * data[10] + b[5] * data[9] - b[6] * data[8] +
           b[14] * data[13] - b[2] * data[12] + b[3] * data[11] -
           b[13] * data[14] - b[10] * data[15];
  res[8] = b[8] * data[0] + b[14] * data[2] + b[4] * data[3] - b[3] * data[4] +
           b[9] * data[10] - b[10] * data[9] + b[0] * data[8] + b[2] * data[14];
  res[9] = b[9] * data[0] - b[4] * data[2] + b[14] * data[3] + b[2] * data[4] -
           b[8] * data[10] + b[0] * data[9] + b[10] * data[8] + b[3] * data[14];
  res[10] = b[10] * data[0] + b[3] * data[2] - b[2] * data[3] +
            b[14] * data[4] + b[0] * data[10] + b[8] * data[9] -
            b[9] * data[8] + b[4] * data[14];
  res[11] = b[11] * data[0] - b[8] * data[1] + b[15] * data[2] +
            b[7] * data[3] - b[6] * data[4] - b[14] * data[5] - b[4] * data[6] +
            b[3] * data[7] + b[12] * data[10] - b[13] * data[9] -
            b[1] * data[8] + b[9] * data[13] - b[10] * data[12] +
            b[0] * data[11] + b[5] * data[14] - b[2] * data[15];
  res[12] =
      b[12] * data[0] - b[9] * data[1] - b[7] * data[2] + b[15] * data[3] +
      b[5] * data[4] + b[4] * data[5] - b[14] * data[6] - b[2] * data[7] -
      b[11] * data[10] - b[1] * data[9] + b[13] * data[8] - b[8] * data[13] +
      b[0] * data[12] + b[10] * data[11] + b[6] * data[14] - b[3] * data[15];
  res[13] = b[13] * data[0] - b[10] * data[1] + b[6] * data[2] -
            b[5] * data[3] + b[15] * data[4] - b[3] * data[5] + b[2] * data[6] -
            b[14] * data[7] - b[1] * data[10] + b[11] * data[9] -
            b[12] * data[8] + b[0] * data[13] + b[8] * data[12] -
            b[9] * data[11] + b[7] * data[14] - b[4] * data[15];
  res[14] = b[14] * data[0] + b[8] * data[2] + b[9] * data[3] +
            b[10] * data[4] + b[4] * data[10] + b[3] * data[9] +
            b[2] * data[8] + b[0] * data[14];
  res[15] =
      b[15] * data[0] + b[14] * data[1] + b[11] * data[2] + b[12] * data[3] +
      b[13] * data[4] + b[8] * data[5] + b[9] * data[6] + b[10] * data[7] +
      b[7] * data[10] + b[6] * data[9] + b[5] * data[8] - b[4] * data[13] -
      b[3] * data[12] - b[2] * data[11] - b[1] * data[14] + b[0] * data[15];
  return res;
}
constexpr MultiVector MultiVector::operator*(const TriVector &b) const {
  MultiVector res{};
  res[0] = -b[3] * data[14];
  res[1] = b[2] * data[10] + b[1] * data[9] + b[0] * data[8] - b[3] * data[15];
  res[2] = -b[3] * data[8];
  res[3] = -b[3] * data[9];
  res[4] = -b[3] * data[10];
  res[5] = -b[2] * data[3] + b[1] * data[4] + b[3] * data[11] - b[0] * data[14];
  res[6] = b[2] * data[2] - b[0] * data[4] + b[3] * data[12] - b[1] * data[14];
  res[7] = -b[1] * data[2] + b[0] * data[3] + b[3] * data[13] - b[2] * data[14];
  res[8] = b[3] * data[2];
  res[9] = b[3] * data[3];
  res[10] = b[3] * data[4];
  res[11] = b[0] * data[0] - b[3] * data[5] + b[1] * data[10] - b[2] * data[9];
  res[12] = b[1] * data[0] - b[3] * data[6] - b[0] * data[10] + b[2] * data[8];
  res[13] = b[2] * data[0] - b[3] * data[7] + b[0] * data[9] - b[1] * data[8];
  res[14] = b[3] * data[0];
  res[15] = b[3] * data[1] + b[0] * data[2] + b[1] * data[3] + b[2] * data[4];
  return res;
}
constexpr MultiVector MultiVector::operator*(const BiVector &b) const {
  MultiVector res{};
  res[0] = -b[5] * data[10] - b[4] * data[9] - b[3] * data[8];
  res[1] = -b[0] * data[2] - b[1] * data[3] - b[2] * data[4] + b[5] * data[13] +
           b[4] * data[12] + b[3] * data[11];
  res[2] = -b[5] * data[3] + b[4] * data[4] - b[3] * data[14];
  res[3] = b[5] * data[2] - b[3] * data[4] - b[4] * data[14];
  res[4] = -b[4] * data[2] + b[3] * data[3] - b[5] * data[14];
  res[5] = b[0] * data[0] - b[5] * data[6] + b[4] * data[7] + b[1] * data[10] -
           b[2] * data[9] - b[3] * data[15];
  res[6] = b[1] * data[0] + b[5] * data[5] - b[3] * data[7] - b[0] * data[10] +
           b[2] * data[8] - b[4] * data[15];
  res[7] = b[2] * data[0] - b[4] * data[5] + b[3] * data[6] + b[0] * data[9] -
           b[1] * data[8] - b[5] * data[15];
  res[8] = b[3] * data[0] + b[4] * data[10] - b[5] * data[9];
  res[9] = b[4] * data[0] - b[3] * data[10] + b[5] * data[8];
  res[10] = b[5] * data[0] + b[3] * data[9] - b[4] * data[8];
  res[11] = -b[3] * data[1] + b[2] * data[3] - b[1] * data[4] +
            b[4] * data[13] - b[5] * data[12] + b[0] * data[14];
  res[12] = -b[4] * data[1] - b[2] * data[2] + b[0] * data[4] -
            b[3] * data[13] + b[5] * data[11] + b[1] * data[14];
  res[13] = -b[5] * data[1] + b[1] * data[2] - b[0] * data[3] +
            b[3] * data[12] - b[4] * data[11] + b[2] * data[14];
  res[14] = b[3] * data[2] + b[4] * data[3] + b[5] * data[4];
  res[15] = b[3] * data[5] + b[4] * data[6] + b[5] * data[7] + b[2] * data[10] +
            b[1] * data[9] + b[0] * data[8];
  return res;
}
constexpr MultiVector MultiVector::operator*(const Vector &b) const {
  MultiVector res{};
  res[0] = b[1] * data[2] + b[2] * data[3] + b[3] * data[4];
  res[1] = b[0] * data[0] + b[1] * data[5] + b[2] * data[6] + b[3] * data[7];
  res[2] = b[1] * data[0] + b[2] * data[10] - b[3] * data[9];
  res[3] = b[2] * data[0] - b[1] * data[10] + b[3] * data[8];
  res[4] = b[3] * data[0] + b[1] * data[9] - b[2] * data[8];
  res[5] = b[1] * data[1] - b[0] * data[2] - b[2] * data[13] + b[3] * data[12];
  res[6] = b[2] * data[1] - b[0] * data[3] + b[1] * data[13] - b[3] * data[11];
  res[7] = b[3] * data[1] - b[0] * data[4] - b[1] * data[12] + b[2] * data[11];
  res[8] = b[3] * data[3] - b[2] * data[4] + b[1] * data[14];
  res[9] = -b[3] * data[2] + b[1] * data[4] + b[2] * data[14];
  res[10] = b[2] * data[2] - b[1] * data[3] + b[3] * data[14];
  res[11] = -b[3] * data[6] + b[2] * data[7] - b[0] * data[8] - b[1] * data[15];
  res[12] = b[3] * data[5] - b[1] * data[7] - b[0] * data[9] - b[2] * data[15];
  res[13] =
      -b[2] * data[5] + b[1] * data[6] - b[0] * data[10] - b[3] * data[15];
  res[14] = b[3] * data[10] + b[2] * data[9] + b[1] * data[8];
  res[15] =
      -b[3] * data[13] - b[2] * data[12] - b[1] * data[11] - b[0] * data[14];
  return res;
}
constexpr MultiVector MultiVector::operator*(const Motor &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0] - b[6] * data[10] - b[5] * data[9] - b[4] * data[8];
  res[1] = b[0] * data[1] - b[1] * data[2] - b[2] * data[3] - b[3] * data[4] +
           b[6] * data[13] + b[5] * data[12] + b[4] * data[11] +
           b[7] * data[14];
  res[2] = b[0] * data[2] - b[6] * data[3] + b[5] * data[4] - b[4] * data[14];
  res[3] = b[6] * data[2] + b[0] * data[3] - b[4] * data[4] - b[5] * data[14];
  res[4] = -b[5] * data[2] + b[4] * data[3] + b[0] * data[4] - b[6] * data[14];
  res[5] = b[1] * data[0] + b[0] * data[5] - b[6] * data[6] + b[5] * data[7] +
           b[2] * data[10] - b[3] * data[9] - b[7] * data[8] - b[4] * data[15];
  res[6] = b[2] * data[0] + b[6] * data[5] + b[0] * data[6] - b[4] * data[7] -
           b[1] * data[10] - b[7] * data[9] + b[3] * data[8] - b[5] * data[15];
  res[7] = b[3] * data[0] - b[5] * data[5] + b[4] * data[6] + b[0] * data[7] -
           b[7] * data[10] + b[1] * data[9] - b[2] * data[8] - b[6] * data[15];
  res[8] = b[4] * data[0] + b[5] * data[10] - b[6] * data[9] + b[0] * data[8];
  res[9] = b[5] * data[0] - b[4] * data[10] + b[0] * data[9] + b[6] * data[8];
  res[10] = b[6] * data[0] + b[0] * data[10] + b[4] * data[9] - b[5] * data[8];
  res[11] = -b[4] * data[1] + b[7] * data[2] + b[3] * data[3] - b[2] * data[4] +
            b[5] * data[13] - b[6] * data[12] + b[0] * data[11] +
            b[1] * data[14];
  res[12] = -b[5] * data[1] - b[3] * data[2] + b[7] * data[3] + b[1] * data[4] -
            b[4] * data[13] + b[0] * data[12] + b[6] * data[11] +
            b[2] * data[14];
  res[13] = -b[6] * data[1] + b[2] * data[2] - b[1] * data[3] + b[7] * data[4] +
            b[0] * data[13] + b[4] * data[12] - b[5] * data[11] +
            b[3] * data[14];
  res[14] = b[4] * data[2] + b[5] * data[3] + b[6] * data[4] + b[0] * data[14];
  res[15] = b[7] * data[0] + b[4] * data[5] + b[5] * data[6] + b[6] * data[7] +
            b[3] * data[10] + b[2] * data[9] + b[1] * data[8] + b[0] * data[15];
  return res;
}
constexpr MultiVector TriVector::operator*(const MultiVector &b) const {
  MultiVector res{};
  res[0] = -b[14] * data[3];
  res[1] = b[10] * data[2] + b[9] * data[1] + b[8] * data[0] + b[15] * data[3];
  res[2] = -b[8] * data[3];
  res[3] = -b[9] * data[3];
  res[4] = -b[10] * data[3];
  res[5] = -b[3] * data[2] + b[4] * data[1] + b[14] * data[0] - b[11] * data[3];
  res[6] = b[2] * data[2] + b[14] * data[1] - b[4] * data[0] - b[12] * data[3];
  res[7] = b[14] * data[2] - b[2] * data[1] + b[3] * data[0] - b[13] * data[3];
  res[8] = b[2] * data[3];
  res[9] = b[3] * data[3];
  res[10] = b[4] * data[3];
  res[11] = b[9] * data[2] - b[10] * data[1] + b[0] * data[0] + b[5] * data[3];
  res[12] = -b[8] * data[2] + b[0] * data[1] + b[10] * data[0] + b[6] * data[3];
  res[13] = b[0] * data[2] + b[8] * data[1] - b[9] * data[0] + b[7] * data[3];
  res[14] = b[0] * data[3];
  res[15] = -b[4] * data[2] - b[3] * data[1] - b[2] * data[0] - b[1] * data[3];
  return res;
}
constexpr Motor TriVector::operator*(const TriVector &b) const {
  Motor res{};
  res[0] = -b[3] * data[3];
  res[1] = b[3] * data[0] - b[0] * data[3];
  res[2] = b[3] * data[1] - b[1] * data[3];
  res[3] = b[3] * data[2] - b[2] * data[3];
  return res;
}
constexpr MultiVector TriVector::operator*(const BiVector &b) const {
  MultiVector res{};
  res[1] = b[5] * data[2] + b[4] * data[1] + b[3] * data[0];
  res[2] = -b[3] * data[3];
  res[3] = -b[4] * data[3];
  res[4] = -b[5] * data[3];
  res[11] = b[4] * data[2] - b[5] * data[1] + b[0] * data[3];
  res[12] = -b[3] * data[2] + b[5] * data[0] + b[1] * data[3];
  res[13] = b[3] * data[1] - b[4] * data[0] + b[2] * data[3];
  return res;
}
constexpr Motor TriVector::operator*(const Vector &b) const {
  Motor res{};
  res[1] = -b[2] * data[2] + b[3] * data[1];
  res[2] = b[1] * data[2] - b[3] * data[0];
  res[3] = -b[1] * data[1] + b[2] * data[0];
  res[4] = b[1] * data[3];
  res[5] = b[2] * data[3];
  res[6] = b[3] * data[3];
  res[7] = -b[3] * data[2] - b[2] * data[1] - b[1] * data[0] - b[0] * data[3];
  return res;
}
constexpr MultiVector TriVector::operator*(const Motor &b) const {
  MultiVector res{};
  res[1] = b[6] * data[2] + b[5] * data[1] + b[4] * data[0] + b[7] * data[3];
  res[2] = -b[4] * data[3];
  res[3] = -b[5] * data[3];
  res[4] = -b[6] * data[3];
  res[11] = b[5] * data[2] - b[6] * data[1] + b[0] * data[0] + b[1] * data[3];
  res[12] = -b[4] * data[2] + b[0] * data[1] + b[6] * data[0] + b[2] * data[3];
  res[13] = b[0] * data[2] + b[4] * data[1] - b[5] * data[0] + b[3] * data[3];
  res[14] = b[0] * data[3];
  return res;
}
constexpr MultiVector BiVector::operator*(const MultiVector &b) const {
  MultiVector res{};
  res[0] = -b[10] * data[5] - b[9] * data[4] - b[8] * data[3];
  res[1] = b[2] * data[0] + b[3] * data[1] + b[4] * data[2] + b[13] * data[5] +
           b[12] * data[4] + b[11] * data[3];
  res[2] = b[3] * data[5] - b[4] * data[4] - b[14] * data[3];
  res[3] = -b[2] * data[5] - b[14] * data[4] + b[4] * data[3];
  res[4] = -b[14] * data[5] + b[2] * data[4] - b[3] * data[3];
  res[5] = b[0] * data[0] - b[10] * data[1] + b[9] * data[2] + b[6] * data[5] -
           b[7] * data[4] - b[15] * data[3];
  res[6] = b[10] * data[0] + b[0] * data[1] - b[8] * data[2] - b[5] * data[5] -
           b[15] * data[4] + b[7] * data[3];
  res[7] = -b[9] * data[0] + b[8] * data[1] + b[0] * data[2] - b[15] * data[5] +
           b[5] * data[4] - b[6] * data[3];
  res[8] = b[9] * data[5] - b[10] * data[4] + b[0] * data[3];
  res[9] = -b[8] * data[5] + b[0] * data[4] + b[10] * data[3];
  res[10] = b[0] * data[5] + b[8] * data[4] - b[9] * data[3];
  res[11] = -b[14] * data[0] - b[4] * data[1] + b[3] * data[2] +
            b[12] * data[5] - b[13] * data[4] - b[1] * data[3];
  res[12] = b[4] * data[0] - b[14] * data[1] - b[2] * data[2] -
            b[11] * data[5] - b[1] * data[4] + b[13] * data[3];
  res[13] = -b[3] * data[0] + b[2] * data[1] - b[14] * data[2] -
            b[1] * data[5] + b[11] * data[4] - b[12] * data[3];
  res[14] = b[4] * data[5] + b[3] * data[4] + b[2] * data[3];
  res[15] = b[8] * data[0] + b[9] * data[1] + b[10] * data[2] + b[7] * data[5] +
            b[6] * data[4] + b[5] * data[3];
  return res;
}
constexpr MultiVector BiVector::operator*(const TriVector &b) const {
  MultiVector res{};
  res[1] = b[2] * data[5] + b[1] * data[4] + b[0] * data[3];
  res[2] = -b[3] * data[3];
  res[3] = -b[3] * data[4];
  res[4] = -b[3] * data[5];
  res[11] = -b[3] * data[0] + b[1] * data[5] - b[2] * data[4];
  res[12] = -b[3] * data[1] - b[0] * data[5] + b[2] * data[3];
  res[13] = -b[3] * data[2] + b[0] * data[4] - b[1] * data[3];
  return res;
}
constexpr Motor BiVector::operator*(const BiVector &b) const {
  Motor res{};
  res[0] = -b[5] * data[5] - b[4] * data[4] - b[3] * data[3];
  res[1] = -b[5] * data[1] + b[4] * data[2] + b[1] * data[5] - b[2] * data[4];
  res[2] = b[5] * data[0] - b[3] * data[2] - b[0] * data[5] + b[2] * data[3];
  res[3] = -b[4] * data[0] + b[3] * data[1] + b[0] * data[4] - b[1] * data[3];
  res[4] = b[4] * data[5] - b[5] * data[4];
  res[5] = -b[3] * data[5] + b[5] * data[3];
  res[6] = b[3] * data[4] - b[4] * data[3];
  res[7] = b[3] * data[0] + b[4] * data[1] + b[5] * data[2] + b[2] * data[5] +
           b[1] * data[4] + b[0] * data[3];
  return res;
}
constexpr MultiVector BiVector::operator*(const Vector &b) const {
  MultiVector res{};
  res[1] = b[1] * data[0] + b[2] * data[1] + b[3] * data[2];
  res[2] = b[2] * data[5] - b[3] * data[4];
  res[3] = -b[1] * data[5] + b[3] * data[3];
  res[4] = b[1] * data[4] - b[2] * data[3];
  res[11] = -b[3] * data[1] + b[2] * data[2] - b[0] * data[3];
  res[12] = b[3] * data[0] - b[1] * data[2] - b[0] * data[4];
  res[13] = -b[2] * data[0] + b[1] * data[1] - b[0] * data[5];
  res[14] = b[3] * data[5] + b[2] * data[4] + b[1] * data[3];
  return res;
}
constexpr Motor BiVector::operator*(const Motor &b) const {
  Motor res{};
  res[0] = -b[6] * data[5] - b[5] * data[4] - b[4] * data[3];
  res[1] = b[0] * data[0] - b[6] * data[1] + b[5] * data[2] + b[2] * data[5] -
           b[3] * data[4] - b[7] * data[3];
  res[2] = b[6] * data[0] + b[0] * data[1] - b[4] * data[2] - b[1] * data[5] -
           b[7] * data[4] + b[3] * data[3];
  res[3] = -b[5] * data[0] + b[4] * data[1] + b[0] * data[2] - b[7] * data[5] +
           b[1] * data[4] - b[2] * data[3];
  res[4] = b[5] * data[5] - b[6] * data[4] + b[0] * data[3];
  res[5] = -b[4] * data[5] + b[0] * data[4] + b[6] * data[3];
  res[6] = b[0] * data[5] + b[4] * data[4] - b[5] * data[3];
  res[7] = b[4] * data[0] + b[5] * data[1] + b[6] * data[2] + b[3] * data[5] +
           b[2] * data[4] + b[1] * data[3];
  return res;
}
constexpr MultiVector Vector::operator*(const MultiVector &b) const {
  MultiVector res{};
  res[0] = b[2] * data[1] + b[3] * data[2] + b[4] * data[3];
  res[1] = b[0] * data[0] - b[5] * data[1] - b[6] * data[2] - b[7] * data[3];
  res[2] = b[0] * data[1] - b[10] * data[2] + b[9] * data[3];
  res[3] = b[10] * data[1] + b[0] * data[2] - b[8] * data[3];
  res[4] = -b[9] * data[1] + b[8] * data[2] + b[0] * data[3];
  res[5] = b[2] * data[0] - b[1] * data[1] - b[13] * data[2] + b[12] * data[3];
  res[6] = b[3] * data[0] + b[13] * data[1] - b[1] * data[2] - b[11] * data[3];
  res[7] = b[4] * data[0] - b[12] * data[1] + b[11] * data[2] - b[1] * data[3];
  res[8] = b[14] * data[1] + b[4] * data[2] - b[3] * data[3];
  res[9] = -b[4] * data[1] + b[14] * data[2] + b[2] * data[3];
  res[10] = b[3] * data[1] - b[2] * data[2] + b[14] * data[3];
  res[11] = -b[8] * data[0] + b[15] * data[1] + b[7] * data[2] - b[6] * data[3];
  res[12] = -b[9] * data[0] - b[7] * data[1] + b[15] * data[2] + b[5] * data[3];
  res[13] =
      -b[10] * data[0] + b[6] * data[1] - b[5] * data[2] + b[15] * data[3];
  res[14] = b[8] * data[1] + b[9] * data[2] + b[10] * data[3];
  res[15] =
      b[14] * data[0] + b[11] * data[1] + b[12] * data[2] + b[13] * data[3];
  return res;
}
constexpr Motor Vector::operator*(const TriVector &b) const {
  Motor res{};
  res[1] = -data[2] * b[2] + data[3] * b[1];
  res[2] = data[1] * b[2] - data[3] * b[0];
  res[3] = -data[1] * b[1] + data[2] * b[0];
  res[4] = data[1] * b[3];
  res[5] = data[2] * b[3];
  res[6] = data[3] * b[3];
  res[7] = data[3] * b[2] + data[2] * b[1] + data[1] * b[0] + data[0] * b[3];
  return res;
}
constexpr MultiVector Vector::operator*(const BiVector &b) const {
  MultiVector res{};
  res[1] = -b[0] * data[1] - b[1] * data[2] - b[2] * data[3];
  res[2] = -b[5] * data[2] + b[4] * data[3];
  res[3] = b[5] * data[1] - b[3] * data[3];
  res[4] = -b[4] * data[1] + b[3] * data[2];
  res[11] = -b[3] * data[0] + b[2] * data[2] - b[1] * data[3];
  res[12] = -b[4] * data[0] - b[2] * data[1] + b[0] * data[3];
  res[13] = -b[5] * data[0] + b[1] * data[1] - b[0] * data[2];
  res[14] = b[3] * data[1] + b[4] * data[2] + b[5] * data[3];
  return res;
}
constexpr Motor Vector::operator*(const Vector &b) const {
  Motor res{};
  res[0] = data[1] * b[1] + data[2] * b[2] + data[3] * b[3];
  res[1] = data[0] * b[1] - data[1] * b[0];
  res[2] = data[0] * b[2] - data[2] * b[0];
  res[3] = data[0] * b[3] - data[3] * b[0];
  res[4] = data[2] * b[3] - data[3] * b[2];
  res[5] = data[3] * b[1] - data[1] * b[3];
  res[6] = data[1] * b[2] - data[2] * b[1];
  return res;
}
constexpr MultiVector Vector::operator*(const Motor &b) const {
  MultiVector res{};
  res[1] = b[0] * data[0] - b[1] * data[1] - b[2] * data[2] - b[3] * data[3];
  res[2] = b[0] * data[1] - b[6] * data[2] + b[5] * data[3];
  res[3] = b[6] * data[1] + b[0] * data[2] - b[4] * data[3];
  res[4] = -b[5] * data[1] + b[4] * data[2] + b[0] * data[3];
  res[11] = -b[4] * data[0] + b[7] * data[1] + b[3] * data[2] - b[2] * data[3];
  res[12] = -b[5] * data[0] - b[3] * data[1] + b[7] * data[2] + b[1] * data[3];
  res[13] = -b[6] * data[0] + b[2] * data[1] - b[1] * data[2] + b[7] * data[3];
  res[14] = b[4] * data[1] + b[5] * data[2] + b[6] * data[3];
  return res;
}
constexpr MultiVector Motor::operator*(const MultiVector &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0] - b[10] * data[6] - b[9] * data[5] - b[8] * data[4];
  res[1] = b[1] * data[0] + b[2] * data[1] + b[3] * data[2] + b[4] * data[3] +
           b[13] * data[6] + b[12] * data[5] + b[11] * data[4] -
           b[14] * data[7];
  res[2] = b[2] * data[0] + b[3] * data[6] - b[4] * data[5] - b[14] * data[4];
  res[3] = b[3] * data[0] - b[2] * data[6] - b[14] * data[5] + b[4] * data[4];
  res[4] = b[4] * data[0] - b[14] * data[6] + b[2] * data[5] - b[3] * data[4];
  res[5] = b[5] * data[0] + b[0] * data[1] - b[10] * data[2] + b[9] * data[3] +
           b[6] * data[6] - b[7] * data[5] - b[15] * data[4] - b[8] * data[7];
  res[6] = b[6] * data[0] + b[10] * data[1] + b[0] * data[2] - b[8] * data[3] -
           b[5] * data[6] - b[15] * data[5] + b[7] * data[4] - b[9] * data[7];
  res[7] = b[7] * data[0] - b[9] * data[1] + b[8] * data[2] + b[0] * data[3] -
           b[15] * data[6] + b[5] * data[5] - b[6] * data[4] - b[10] * data[7];
  res[8] = b[8] * data[0] + b[9] * data[6] - b[10] * data[5] + b[0] * data[4];
  res[9] = b[9] * data[0] - b[8] * data[6] + b[0] * data[5] + b[10] * data[4];
  res[10] = b[10] * data[0] + b[0] * data[6] + b[8] * data[5] - b[9] * data[4];
  res[11] = b[11] * data[0] - b[14] * data[1] - b[4] * data[2] +
            b[3] * data[3] + b[12] * data[6] - b[13] * data[5] -
            b[1] * data[4] - b[2] * data[7];
  res[12] = b[12] * data[0] + b[4] * data[1] - b[14] * data[2] -
            b[2] * data[3] - b[11] * data[6] - b[1] * data[5] +
            b[13] * data[4] - b[3] * data[7];
  res[13] = b[13] * data[0] - b[3] * data[1] + b[2] * data[2] -
            b[14] * data[3] - b[1] * data[6] + b[11] * data[5] -
            b[12] * data[4] - b[4] * data[7];
  res[14] = b[14] * data[0] + b[4] * data[6] + b[3] * data[5] + b[2] * data[4];
  res[15] = b[15] * data[0] + b[8] * data[1] + b[9] * data[2] +
            b[10] * data[3] + b[7] * data[6] + b[6] * data[5] + b[5] * data[4] +
            b[0] * data[7];
  return res;
}
constexpr MultiVector Motor::operator*(const TriVector &b) const {
  MultiVector res{};
  res[1] = b[2] * data[6] + b[1] * data[5] + b[0] * data[4] - b[3] * data[7];
  res[2] = -data[4] * b[3];
  res[3] = -data[5] * b[3];
  res[4] = -data[6] * b[3];
  res[11] = b[0] * data[0] - b[3] * data[1] + b[1] * data[6] - b[2] * data[5];
  res[12] = b[1] * data[0] - b[3] * data[2] - b[0] * data[6] + b[2] * data[4];
  res[13] = b[2] * data[0] - b[3] * data[3] + b[0] * data[5] - b[1] * data[4];
  res[14] = data[0] * b[3];
  return res;
}
constexpr Motor Motor::operator*(const BiVector &b) const {
  Motor res{};
  res[0] = -b[5] * data[6] - b[4] * data[5] - b[3] * data[4];
  res[1] = b[0] * data[0] - b[5] * data[2] + b[4] * data[3] + b[1] * data[6] -
           b[2] * data[5] - b[3] * data[7];
  res[2] = b[1] * data[0] + b[5] * data[1] - b[3] * data[3] - b[0] * data[6] +
           b[2] * data[4] - b[4] * data[7];
  res[3] = b[2] * data[0] - b[4] * data[1] + b[3] * data[2] + b[0] * data[5] -
           b[1] * data[4] - b[5] * data[7];
  res[4] = b[3] * data[0] + b[4] * data[6] - b[5] * data[5];
  res[5] = b[4] * data[0] - b[3] * data[6] + b[5] * data[4];
  res[6] = b[5] * data[0] + b[3] * data[5] - b[4] * data[4];
  res[7] = b[3] * data[1] + b[4] * data[2] + b[5] * data[3] + b[2] * data[6] +
           b[1] * data[5] + b[0] * data[4];
  return res;
}
constexpr MultiVector Motor::operator*(const Vector &b) const {
  MultiVector res{};
  res[1] = b[0] * data[0] + b[1] * data[1] + b[2] * data[2] + b[3] * data[3];
  res[2] = b[1] * data[0] + b[2] * data[6] - b[3] * data[5];
  res[3] = b[2] * data[0] - b[1] * data[6] + b[3] * data[4];
  res[4] = b[3] * data[0] + b[1] * data[5] - b[2] * data[4];
  res[11] = -b[3] * data[2] + b[2] * data[3] - b[0] * data[4] - b[1] * data[7];
  res[12] = b[3] * data[1] - b[1] * data[3] - b[0] * data[5] - b[2] * data[7];
  res[13] = -b[2] * data[1] + b[1] * data[2] - b[0] * data[6] - b[3] * data[7];
  res[14] = b[3] * data[6] + b[2] * data[5] + b[1] * data[4];
  return res;
}
constexpr Motor Motor::operator*(const Motor &b) const {
  Motor res{};
  res[0] = b[0] * data[0] - b[6] * data[6] - b[5] * data[5] - b[4] * data[4];
  res[1] = b[1] * data[0] + b[0] * data[1] - b[6] * data[2] + b[5] * data[3] +
           b[2] * data[6] - b[3] * data[5] - b[7] * data[4] - b[4] * data[7];
  res[2] = b[2] * data[0] + b[6] * data[1] + b[0] * data[2] - b[4] * data[3] -
           b[1] * data[6] - b[7] * data[5] + b[3] * data[4] - b[5] * data[7];
  res[3] = b[3] * data[0] - b[5] * data[1] + b[4] * data[2] + b[0] * data[3] -
           b[7] * data[6] + b[1] * data[5] - b[2] * data[4] - b[6] * data[7];
  res[4] = b[4] * data[0] + b[5] * data[6] - b[6] * data[5] + b[0] * data[4];
  res[5] = b[5] * data[0] - b[4] * data[6] + b[0] * data[5] + b[6] * data[4];
  res[6] = b[6] * data[0] + b[0] * data[6] + b[4] * data[5] - b[5] * data[4];
  res[7] = b[7] * data[0] + b[4] * data[1] + b[5] * data[2] + b[6] * data[3] +
           b[3] * data[6] + b[2] * data[5] + b[1] * data[4] + b[0] * data[7];
  return res;
}

///////////////////////////////////////////////////////////////////////////////////
/// Inner Product
///////////////////////////////////////////////////////////////////////////////////

constexpr MultiVector MultiVector::operator|(const MultiVector &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0] + b[2] * data[2] + b[3] * data[3] + b[4] * data[4] -
           b[10] * data[10] - b[9] * data[9] - b[8] * data[8] -
           b[14] * data[14];
  res[1] = b[1] * data[0] + b[0] * data[1] - b[5] * data[2] - b[6] * data[3] -
           b[7] * data[4] + b[2] * data[5] + b[3] * data[6] + b[4] * data[7] +
           b[13] * data[10] + b[12] * data[9] + b[11] * data[8] +
           b[10] * data[13] + b[9] * data[12] + b[8] * data[11] +
           b[15] * data[14] - b[14] * data[15];
  res[2] = b[2] * data[0] + b[0] * data[2] - b[10] * data[3] + b[9] * data[4] +
           b[3] * data[10] - b[4] * data[9] - b[14] * data[8] - b[8] * data[14];
  res[3] = b[3] * data[0] + b[10] * data[2] + b[0] * data[3] - b[8] * data[4] -
           b[2] * data[10] - b[14] * data[9] + b[4] * data[8] - b[9] * data[14];
  res[4] = b[4] * data[0] - b[9] * data[2] + b[8] * data[3] + b[0] * data[4] -
           b[14] * data[10] + b[2] * data[9] - b[3] * data[8] -
           b[10] * data[14];
  res[5] = b[5] * data[0] - b[13] * data[3] + b[12] * data[4] + b[0] * data[5] -
           b[15] * data[8] - b[3] * data[13] + b[4] * data[12] -
           b[8] * data[15];
  res[6] = b[6] * data[0] + b[13] * data[2] - b[11] * data[4] + b[0] * data[6] -
           b[15] * data[9] + b[2] * data[13] - b[4] * data[11] -
           b[9] * data[15];
  res[7] = b[7] * data[0] - b[12] * data[2] + b[11] * data[3] + b[0] * data[7] -
           b[15] * data[10] - b[2] * data[12] + b[3] * data[11] -
           b[10] * data[15];
  res[10] =
      b[10] * data[0] + b[14] * data[4] + b[0] * data[10] + b[4] * data[14];
  res[9] = b[9] * data[0] + b[14] * data[3] + b[0] * data[9] + b[3] * data[14];
  res[8] = b[8] * data[0] + b[14] * data[2] + b[0] * data[8] + b[2] * data[14];
  res[13] =
      b[13] * data[0] + b[15] * data[4] + b[0] * data[13] - b[4] * data[15];
  res[12] =
      b[12] * data[0] + b[15] * data[3] + b[0] * data[12] - b[3] * data[15];
  res[11] =
      b[11] * data[0] + b[15] * data[2] + b[0] * data[11] - b[2] * data[15];
  res[14] = b[14] * data[0] + b[0] * data[14];
  res[15] = b[15] * data[0] + b[0] * data[15];
  return res;
}
constexpr MultiVector MultiVector::operator|(const TriVector &b) const {
  MultiVector res{};
  res[0] = -b[3] * data[14];
  res[1] = b[2] * data[10] + b[1] * data[9] + b[0] * data[8] - b[3] * data[15];
  res[2] = -b[3] * data[8];
  res[3] = -b[3] * data[9];
  res[4] = -b[3] * data[10];
  res[5] = -b[2] * data[3] + b[1] * data[4];
  res[6] = b[2] * data[2] - b[0] * data[4];
  res[7] = -b[1] * data[2] + b[0] * data[3];
  res[10] = b[3] * data[4];
  res[9] = b[3] * data[3];
  res[8] = b[3] * data[2];
  res[13] = b[2] * data[0];
  res[12] = b[1] * data[0];
  res[11] = b[0] * data[0];
  res[14] = b[3] * data[0];
  return res;
}
constexpr MultiVector MultiVector::operator|(const BiVector &b) const {
  MultiVector res{};
  res[0] = -b[5] * data[10] - b[4] * data[9] - b[3] * data[8];
  res[1] = -b[0] * data[2] - b[1] * data[3] - b[2] * data[4] + b[5] * data[13] +
           b[4] * data[12] + b[3] * data[11];
  res[2] = -b[5] * data[3] + b[4] * data[4] - b[3] * data[14];
  res[3] = b[5] * data[2] - b[3] * data[4] - b[4] * data[14];
  res[4] = -b[4] * data[2] + b[3] * data[3] - b[5] * data[14];
  res[5] = b[0] * data[0] - b[3] * data[15];
  res[6] = b[1] * data[0] - b[4] * data[15];
  res[7] = b[2] * data[0] - b[5] * data[15];
  res[10] = b[5] * data[0];
  res[9] = b[4] * data[0];
  res[8] = b[3] * data[0];
  return res;
}
constexpr MultiVector MultiVector::operator|(const Vector &b) const {
  MultiVector res{};
  res[0] = b[1] * data[2] + b[2] * data[3] + b[3] * data[4];
  res[1] = b[0] * data[0] + b[1] * data[5] + b[2] * data[6] + b[3] * data[7];
  res[2] = b[1] * data[0] + b[2] * data[10] - b[3] * data[9];
  res[3] = b[2] * data[0] - b[1] * data[10] + b[3] * data[8];
  res[4] = b[3] * data[0] + b[1] * data[9] - b[2] * data[8];
  res[5] = -b[2] * data[13] + b[3] * data[12];
  res[6] = b[1] * data[13] - b[3] * data[11];
  res[7] = -b[1] * data[12] + b[2] * data[11];
  res[10] = b[3] * data[14];
  res[9] = b[2] * data[14];
  res[8] = b[1] * data[14];
  res[13] = -b[3] * data[15];
  res[12] = -b[2] * data[15];
  res[11] = -b[1] * data[15];
  return res;
}
constexpr MultiVector MultiVector::operator|(const Motor &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0] - b[6] * data[10] - b[5] * data[9] - b[4] * data[8];
  res[1] = b[0] * data[1] - b[1] * data[2] - b[2] * data[3] - b[3] * data[4] +
           b[6] * data[13] + b[5] * data[12] + b[4] * data[11] +
           b[7] * data[14];
  res[2] = b[0] * data[2] - b[6] * data[3] + b[5] * data[4] - b[4] * data[14];
  res[3] = b[6] * data[2] + b[0] * data[3] - b[4] * data[4] - b[5] * data[14];
  res[4] = -b[5] * data[2] + b[4] * data[3] + b[0] * data[4] - b[6] * data[14];
  res[5] = b[1] * data[0] + b[0] * data[5] - b[7] * data[8] - b[4] * data[15];
  res[6] = b[2] * data[0] + b[0] * data[6] - b[7] * data[9] - b[5] * data[15];
  res[7] = b[3] * data[0] + b[0] * data[7] - b[7] * data[10] - b[6] * data[15];
  res[10] = b[6] * data[0] + b[0] * data[10];
  res[9] = b[5] * data[0] + b[0] * data[9];
  res[8] = b[4] * data[0] + b[0] * data[8];
  res[13] = b[7] * data[4] + b[0] * data[13];
  res[12] = b[7] * data[3] + b[0] * data[12];
  res[11] = b[7] * data[2] + b[0] * data[11];
  res[14] = b[0] * data[14];
  res[15] = b[0] * data[15] + b[7] * data[0];
  return res;
}
constexpr MultiVector TriVector::operator|(const MultiVector &b) const {
  MultiVector res{};
  res[0] = -data[3] * b[14];
  res[1] = b[8] * data[0] + b[9] * data[1] + b[10] * data[2] + b[15] * data[3];
  res[2] = -data[3] * b[8];
  res[3] = -data[3] * b[9];
  res[4] = -data[3] * b[10];
  res[5] = -data[2] * b[3] + data[1] * b[4];
  res[6] = data[2] * b[2] - data[0] * b[4];
  res[7] = -data[1] * b[2] + data[0] * b[3];
  res[10] = data[3] * b[4];
  res[9] = data[3] * b[3];
  res[8] = data[3] * b[2];
  res[13] = data[2] * b[0];
  res[12] = data[1] * b[0];
  res[11] = data[0] * b[0];
  res[14] = data[3] * b[0];
  return res;
}
constexpr float TriVector::operator|(const TriVector &b) const {
  return -data[3] * b[3];
}
constexpr Vector TriVector::operator|(const BiVector &b) const {
  Vector res{};
  res[0] = data[2] * b[5] + data[1] * b[4] + data[0] * b[3];
  res[1] = -data[3] * b[3];
  res[2] = -data[3] * b[4];
  res[3] = -data[3] * b[5];
  return res;
}
constexpr BiVector TriVector::operator|(const Vector &b) const {
  BiVector res{};
  res[0] = -data[2] * b[2] + data[1] * b[3];
  res[1] = data[2] * b[1] - data[0] * b[3];
  res[2] = -data[1] * b[1] + data[0] * b[2];
  res[3] = data[3] * b[1];
  res[4] = data[3] * b[2];
  res[5] = data[3] * b[3];
  return res;
}
constexpr MultiVector TriVector::operator|(const Motor &b) const {
  MultiVector res{};
  res[1] = b[4] * data[0] + b[5] * data[1] + b[6] * data[2] + b[7] * data[3];
  res[2] = -b[4] * data[3];
  res[3] = -b[5] * data[3];
  res[4] = -b[6] * data[3];
  res[11] = b[0] * data[0];
  res[12] = b[0] * data[1];
  res[13] = b[0] * data[2];
  res[14] = b[0] * data[3];
  return res;
}
constexpr MultiVector BiVector::operator|(const MultiVector &b) const {
  MultiVector res{};
  res[0] = -data[5] * b[10] - data[4] * b[9] - data[3] * b[8];
  res[1] = b[2] * data[0] + b[3] * data[1] + b[4] * data[2] + b[11] * data[3] +
           b[12] * data[4] + b[13] * data[5];
  res[2] = b[3] * data[5] - b[4] * data[4] - b[14] * data[3];
  res[3] = -b[2] * data[5] + b[4] * data[3] - b[14] * data[4];
  res[4] = b[2] * data[4] - b[3] * data[3] - b[14] * data[5];
  res[5] = data[0] * b[0] - data[3] * b[15];
  res[6] = data[1] * b[0] - data[4] * b[15];
  res[7] = data[2] * b[0] - data[5] * b[15];
  res[10] = data[5] * b[0];
  res[9] = data[4] * b[0];
  res[8] = data[3] * b[0];
  return res;
}
constexpr Vector BiVector::operator|(const TriVector &b) const {
  Vector res{};
  res[0] = data[5] * b[2] + data[4] * b[1] + data[3] * b[0];
  res[1] = -data[3] * b[3];
  res[2] = -data[4] * b[3];
  res[3] = -data[5] * b[3];
  return res;
}
constexpr float BiVector::operator|(const BiVector &b) const {
  return -b[5] * data[5] - b[4] * data[4] - b[3] * data[3];
}
constexpr Vector BiVector::operator|(const Vector &b) const {
  Vector res{};
  res[0] = b[1] * data[0] + b[2] * data[1] + b[3] * data[2];
  res[1] = b[2] * data[5] - b[3] * data[4];
  res[2] = -b[1] * data[5] + b[3] * data[3];
  res[3] = b[1] * data[4] - b[2] * data[3];
  return res;
}
constexpr Motor BiVector::operator|(const Motor &b) const {
  Motor res{};
  res[0] = -data[5] * b[6] - data[4] * b[5] - data[3] * b[4];
  res[1] = data[0] * b[0] - data[3] * b[7];
  res[2] = data[1] * b[0] - data[4] * b[7];
  res[3] = data[2] * b[0] - data[5] * b[7];
  res[6] = data[5] * b[0];
  res[5] = data[4] * b[0];
  res[4] = data[3] * b[0];
  return res;
}
constexpr MultiVector Vector::operator|(const MultiVector &b) const {
  MultiVector res{};
  res[0] = data[1] * b[2] + data[2] * b[3] + data[3] * b[4];
  res[1] = b[0] * data[0] - b[5] * data[1] - b[6] * data[2] - b[7] * data[3];
  res[2] = b[0] * data[1] + b[9] * data[3] - b[10] * data[2];
  res[3] = b[0] * data[2] - b[8] * data[3] + b[10] * data[1];
  res[4] = b[0] * data[3] + b[8] * data[2] - b[9] * data[1];
  res[5] = -data[2] * b[13] + data[3] * b[12];
  res[6] = data[1] * b[13] - data[3] * b[11];
  res[7] = -data[1] * b[12] + data[2] * b[11];
  res[10] = data[3] * b[14];
  res[9] = data[2] * b[14];
  res[8] = data[1] * b[14];
  res[13] = b[15] * data[3];
  res[12] = b[15] * data[2];
  res[11] = b[15] * data[1];
  return res;
}
constexpr BiVector Vector::operator|(const TriVector &b) const {
  BiVector res{};
  res[0] = -data[2] * b[2] + data[3] * b[1];
  res[1] = data[1] * b[2] - data[3] * b[0];
  res[2] = -data[1] * b[1] + data[2] * b[0];
  res[5] = data[3] * b[3];
  res[4] = data[2] * b[3];
  res[3] = data[1] * b[3];
  return res;
}
constexpr Vector Vector::operator|(const BiVector &b) const {
  Vector res{};
  res[0] = -b[0] * data[1] - b[1] * data[2] - b[2] * data[3];
  res[1] = -b[5] * data[2] + b[4] * data[3];
  res[2] = b[5] * data[1] - b[3] * data[3];
  res[3] = -b[4] * data[1] + b[3] * data[2];
  return res;
}
constexpr float Vector::operator|(const Vector &b) const {
  return data[1] * b[1] + data[2] * b[2] + data[3] * b[3];
}
constexpr MultiVector Vector::operator|(const Motor &b) const {
  MultiVector res{};
  res[1] = b[0] * data[0] - b[1] * data[1] - b[2] * data[2] - b[3] * data[3];
  res[2] = b[0] * data[1] + b[5] * data[3] - b[6] * data[2];
  res[3] = b[0] * data[2] - b[4] * data[3] + b[6] * data[1];
  res[4] = b[0] * data[3] + b[4] * data[2] - b[5] * data[1];
  res[13] = b[7] * data[3];
  res[12] = b[7] * data[2];
  res[11] = b[7] * data[1];
  return res;
}
constexpr MultiVector Motor::operator|(const MultiVector &b) const {
  MultiVector res{};
  res[0] = data[0] * b[0] - data[6] * b[10] - data[5] * b[9] - data[4] * b[8];
  res[1] = b[1] * data[0] + b[2] * data[1] + b[3] * data[2] + b[4] * data[3] +
           b[11] * data[4] + b[12] * data[5] + b[13] * data[6] -
           b[14] * data[7];
  res[2] = b[2] * data[0] + b[3] * data[6] - b[4] * data[5] - b[14] * data[4];
  res[3] = -b[2] * data[6] + b[3] * data[0] + b[4] * data[4] - b[14] * data[5];
  res[4] = b[2] * data[5] - b[3] * data[4] + b[4] * data[0] - b[14] * data[6];
  res[5] = data[1] * b[0] + data[0] * b[5] - data[7] * b[8] - data[4] * b[15];
  res[6] = data[2] * b[0] + data[0] * b[6] - data[7] * b[9] - data[5] * b[15];
  res[7] = data[3] * b[0] + data[0] * b[7] - data[7] * b[10] - data[6] * b[15];
  res[10] = data[6] * b[0] + data[0] * b[10];
  res[9] = data[5] * b[0] + data[0] * b[9];
  res[8] = data[4] * b[0] + data[0] * b[8];
  res[13] = -b[4] * data[7] + b[13] * data[0];
  res[12] = -b[3] * data[7] + b[12] * data[0];
  res[11] = -b[2] * data[7] + b[11] * data[0];
  res[14] = data[0] * b[14];
  res[15] = b[0] * data[7] + b[15] * data[0];
  return res;
}
constexpr MultiVector Motor::operator|(const TriVector &b) const {
  MultiVector res{};
  res[1] = b[0] * data[4] + b[1] * data[5] + b[2] * data[6] - b[3] * data[7];
  res[2] = -data[4] * b[3];
  res[3] = -data[5] * b[3];
  res[4] = -data[6] * b[3];
  res[13] = data[0] * b[2];
  res[12] = data[0] * b[1];
  res[11] = data[0] * b[0];
  res[14] = data[0] * b[3];
  return res;
}
constexpr Motor Motor::operator|(const BiVector &b) const {
  Motor res{};
  res[0] = -data[6] * b[5] - data[5] * b[4] - data[4] * b[3];
  res[1] = data[0] * b[0] - data[7] * b[3];
  res[2] = data[0] * b[1] - data[7] * b[4];
  res[3] = data[0] * b[2] - data[7] * b[5];
  res[6] = data[0] * b[5];
  res[5] = data[0] * b[4];
  res[4] = data[0] * b[3];
  return res;
}
constexpr MultiVector Motor::operator|(const Vector &b) const {
  MultiVector res{};
  res[1] = b[0] * data[0] + b[1] * data[1] + b[2] * data[2] + b[3] * data[3];
  res[2] = b[1] * data[0] + b[2] * data[6] - b[3] * data[5];
  res[3] = -b[1] * data[6] + b[2] * data[0] + b[3] * data[4];
  res[4] = b[1] * data[5] - b[2] * data[4] + b[3] * data[0];
  res[13] = -b[3] * data[7];
  res[12] = -b[2] * data[7];
  res[11] = -b[1] * data[7];
  return res;
}
constexpr Motor Motor::operator|(const Motor &b) const {
  Motor res{};
  res[0] = data[0] * b[0] - data[6] * b[6] - data[5] * b[5] - data[4] * b[4];
  res[1] = data[1] * b[0] + data[0] * b[1] - data[7] * b[4] - data[4] * b[7];
  res[2] = data[2] * b[0] + data[0] * b[2] - data[7] * b[5] - data[5] * b[7];
  res[3] = data[3] * b[0] + data[0] * b[3] - data[7] * b[6] - data[6] * b[7];
  res[6] = data[6] * b[0] + data[0] * b[6];
  res[5] = data[5] * b[0] + data[0] * b[5];
  res[4] = data[4] * b[0] + data[0] * b[4];
  res[7] = data[0] * b[7] + data[7] * b[0];
  return res;
}

///////////////////////////////////////////////////////////////////////////////////
/// Outer Product
///////////////////////////////////////////////////////////////////////////////////

constexpr MultiVector MultiVector::operator^(const MultiVector &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0];
  res[1] = b[1] * data[0] + b[0] * data[1];
  res[2] = b[2] * data[0] + b[0] * data[2];
  res[3] = b[3] * data[0] + b[0] * data[3];
  res[4] = b[4] * data[0] + b[0] * data[4];
  res[5] = b[5] * data[0] + b[2] * data[1] - b[1] * data[2] + b[0] * data[5];
  res[6] = b[6] * data[0] + b[3] * data[1] - b[1] * data[3] + b[0] * data[6];
  res[7] = b[7] * data[0] + b[4] * data[1] - b[1] * data[4] + b[0] * data[7];
  res[10] = b[10] * data[0] + b[3] * data[2] - b[2] * data[3] + b[0] * data[10];
  res[9] = b[9] * data[0] - b[4] * data[2] + b[2] * data[4] + b[0] * data[9];
  res[8] = b[8] * data[0] + b[4] * data[3] - b[3] * data[4] + b[0] * data[8];
  res[13] = b[13] * data[0] - b[10] * data[1] + b[6] * data[2] -
            b[5] * data[3] - b[3] * data[5] + b[2] * data[6] - b[1] * data[10] +
            b[0] * data[13];
  res[12] = b[12] * data[0] - b[9] * data[1] - b[7] * data[2] + b[5] * data[4] +
            b[4] * data[5] - b[2] * data[7] - b[1] * data[9] + b[0] * data[12];
  res[11] = b[11] * data[0] - b[8] * data[1] + b[7] * data[3] - b[6] * data[4] -
            b[4] * data[6] + b[3] * data[7] - b[1] * data[8] + b[0] * data[11];
  res[14] = b[14] * data[0] + b[8] * data[2] + b[9] * data[3] +
            b[10] * data[4] + b[4] * data[10] + b[3] * data[9] +
            b[2] * data[8] + b[0] * data[14];
  res[15] =
      b[15] * data[0] + b[14] * data[1] + b[11] * data[2] + b[12] * data[3] +
      b[13] * data[4] + b[8] * data[5] + b[9] * data[6] + b[10] * data[7] +
      b[7] * data[10] + b[6] * data[9] + b[5] * data[8] - b[4] * data[13] -
      b[3] * data[12] - b[2] * data[11] - b[1] * data[14] + b[0] * data[15];
  return res;
}
constexpr MultiVector MultiVector::operator^(const TriVector &b) const {
  MultiVector res{};
  res[13] = b[2] * data[0];
  res[12] = b[1] * data[0];
  res[11] = b[0] * data[0];
  res[14] = b[3] * data[0];
  res[15] = b[3] * data[1] + b[0] * data[2] + b[1] * data[3] + b[2] * data[4];
  return res;
}
constexpr MultiVector MultiVector::operator^(const BiVector &b) const {
  MultiVector res{};
  res[5] = b[0] * data[0];
  res[6] = b[1] * data[0];
  res[7] = b[2] * data[0];
  res[10] = b[5] * data[0];
  res[9] = b[4] * data[0];
  res[8] = b[3] * data[0];
  res[13] = -b[5] * data[1] + b[1] * data[2] - b[0] * data[3];
  res[12] = -b[4] * data[1] - b[2] * data[2] + b[0] * data[4];
  res[11] = -b[3] * data[1] + b[2] * data[3] - b[1] * data[4];
  res[14] = b[3] * data[2] + b[4] * data[3] + b[5] * data[4];
  res[15] = b[3] * data[5] + b[4] * data[6] + b[5] * data[7] + b[2] * data[10] +
            b[1] * data[9] + b[0] * data[8];
  return res;
}
constexpr MultiVector MultiVector::operator^(const Vector &b) const {
  MultiVector res{};
  res[1] = b[0] * data[0];
  res[2] = b[1] * data[0];
  res[3] = b[2] * data[0];
  res[4] = b[3] * data[0];
  res[5] = b[1] * data[1] - b[0] * data[2];
  res[6] = b[2] * data[1] - b[0] * data[3];
  res[7] = b[3] * data[1] - b[0] * data[4];
  res[10] = b[2] * data[2] - b[1] * data[3];
  res[9] = -b[3] * data[2] + b[1] * data[4];
  res[8] = b[3] * data[3] - b[2] * data[4];
  res[13] = -b[2] * data[5] + b[1] * data[6] - b[0] * data[10];
  res[12] = b[3] * data[5] - b[1] * data[7] - b[0] * data[9];
  res[11] = -b[3] * data[6] + b[2] * data[7] - b[0] * data[8];
  res[14] = b[3] * data[10] + b[2] * data[9] + b[1] * data[8];
  res[15] =
      -b[3] * data[13] - b[2] * data[12] - b[1] * data[11] - b[0] * data[14];
  return res;
}
constexpr MultiVector MultiVector::operator^(const Motor &b) const {
  MultiVector res{};
  res[0] = b[0] * data[0];
  res[1] = b[0] * data[1];
  res[2] = b[0] * data[2];
  res[3] = b[0] * data[3];
  res[4] = b[0] * data[4];
  res[5] = b[1] * data[0] + b[0] * data[5];
  res[6] = b[2] * data[0] + b[0] * data[6];
  res[7] = b[3] * data[0] + b[0] * data[7];
  res[10] = b[6] * data[0] + b[0] * data[10];
  res[9] = b[5] * data[0] + b[0] * data[9];
  res[8] = b[4] * data[0] + b[0] * data[8];
  res[13] = b[0] * data[13] - b[1] * data[3] + b[2] * data[2] - b[6] * data[1];
  res[12] = b[0] * data[12] + b[1] * data[4] - b[3] * data[2] - b[5] * data[1];
  res[11] = b[0] * data[11] - b[2] * data[4] + b[3] * data[3] - b[4] * data[1];
  res[14] = b[4] * data[2] + b[5] * data[3] + b[6] * data[4] + b[0] * data[14];
  res[15] = b[7] * data[0] + b[4] * data[5] + b[5] * data[6] + b[6] * data[7] +
            b[3] * data[10] + b[2] * data[9] + b[1] * data[8] + b[0] * data[15];
  return res;
}
constexpr MultiVector TriVector::operator^(const MultiVector &b) const {
  MultiVector res{};
  res[13] = data[2] * b[0];
  res[12] = data[1] * b[0];
  res[11] = data[0] * b[0];
  res[14] = data[3] * b[0];
  res[15] = -b[1] * data[3] - b[2] * data[0] - b[3] * data[1] - b[4] * data[2];
  return res;
}
constexpr GANull TriVector::operator^(const TriVector &) const {
  return GANull{};
}
constexpr GANull TriVector::operator^(const BiVector &) const {
  return GANull{};
}
constexpr float TriVector::operator^(const Vector &b) const {
  return -b[0] * data[3] - b[1] * data[0] - b[2] * data[1] - b[3] * data[2];
}
constexpr TriVector TriVector::operator^(const Motor &b) const {
  TriVector res{};
  res[2] = data[2] * b[0];
  res[1] = data[1] * b[0];
  res[0] = data[0] * b[0];
  res[3] = data[3] * b[0];
  return res;
}
constexpr MultiVector BiVector::operator^(const MultiVector &b) const {
  MultiVector res{};
  res[5] = data[0] * b[0];
  res[6] = data[1] * b[0];
  res[7] = data[2] * b[0];
  res[10] = data[5] * b[0];
  res[9] = data[4] * b[0];
  res[8] = data[3] * b[0];
  res[13] = -data[5] * b[1] + data[1] * b[2] - data[0] * b[3];
  res[12] = -data[4] * b[1] - data[2] * b[2] + data[0] * b[4];
  res[11] = -data[3] * b[1] + data[2] * b[3] - data[1] * b[4];
  res[14] = data[3] * b[2] + data[4] * b[3] + data[5] * b[4];
  res[15] = data[3] * b[5] + data[4] * b[6] + data[5] * b[7] + data[2] * b[10] +
            data[1] * b[9] + data[0] * b[8];
  return res;
}
constexpr GANull BiVector::operator^(const TriVector &) const {
  return GANull{};
}
constexpr MultiVector BiVector::operator^(const BiVector &b) const {
  MultiVector res{};
  res[15] = b[3] * data[0] + b[4] * data[1] + b[5] * data[2] + b[2] * data[5] +
            b[1] * data[4] + b[0] * data[3];
  return res;
}
constexpr TriVector BiVector::operator^(const Vector &b) const {
  TriVector res{};
  res[2] = -data[5] * b[0] + data[1] * b[1] - data[0] * b[2];
  res[1] = -data[4] * b[0] - data[2] * b[1] + data[0] * b[3];
  res[0] = -data[3] * b[0] + data[2] * b[2] - data[1] * b[3];
  res[3] = data[3] * b[1] + data[4] * b[2] + data[5] * b[3];
  return res;
}
constexpr Motor BiVector::operator^(const Motor &b) const {
  Motor res{};
  res[1] = data[0] * b[0];
  res[2] = data[1] * b[0];
  res[3] = data[2] * b[0];
  res[6] = data[5] * b[0];
  res[5] = data[4] * b[0];
  res[4] = data[3] * b[0];
  res[7] = data[3] * b[1] + data[4] * b[2] + data[5] * b[3] + data[2] * b[6] +
           data[1] * b[5] + data[0] * b[4];
  return res;
}
constexpr MultiVector Vector::operator^(const MultiVector &b) const {
  MultiVector res{};
  res[1] = b[0] * data[0];
  res[2] = b[0] * data[1];
  res[3] = b[0] * data[2];
  res[4] = b[0] * data[3];
  res[5] = b[2] * data[0] - b[1] * data[1];
  res[6] = b[3] * data[0] - b[1] * data[2];
  res[7] = b[4] * data[0] - b[1] * data[3];
  res[10] = b[3] * data[1] - b[2] * data[2];
  res[9] = -b[4] * data[1] + b[2] * data[3];
  res[8] = b[4] * data[2] - b[3] * data[3];
  res[13] = -b[10] * data[0] + b[6] * data[1] - b[5] * data[2];
  res[12] = -b[9] * data[0] - b[7] * data[1] + b[5] * data[3];
  res[11] = -b[8] * data[0] + b[7] * data[2] - b[6] * data[3];
  res[14] = b[8] * data[1] + b[9] * data[2] + b[10] * data[3];
  res[15] =
      b[14] * data[0] + b[11] * data[1] + b[12] * data[2] + b[13] * data[3];
  return res;
}
constexpr MultiVector Vector::operator^(const TriVector &b) const {
  MultiVector res{};
  res[15] = b[0] * data[1] + b[1] * data[2] + b[2] * data[3] + b[3] * data[0];
  return res;
}
constexpr TriVector Vector::operator^(const BiVector &b) const {
  TriVector res{};
  res[2] = -b[5] * data[0] + b[1] * data[1] - b[0] * data[2];
  res[1] = -b[4] * data[0] - b[2] * data[1] + b[0] * data[3];
  res[0] = -b[3] * data[0] + b[2] * data[2] - b[1] * data[3];
  res[3] = b[3] * data[1] + b[4] * data[2] + b[5] * data[3];
  return res;
}
constexpr BiVector Vector::operator^(const Vector &b) const {
  BiVector res{};
  res[0] = b[1] * data[0] - b[0] * data[1];
  res[1] = b[2] * data[0] - b[0] * data[2];
  res[2] = b[3] * data[0] - b[0] * data[3];
  res[5] = b[2] * data[1] - b[1] * data[2];
  res[4] = -b[3] * data[1] + b[1] * data[3];
  res[3] = b[3] * data[2] - b[2] * data[3];
  return res;
}
constexpr MultiVector Vector::operator^(const Motor &b) const {
  MultiVector res{};
  res[1] = data[0] * b[0];
  res[2] = data[1] * b[0];
  res[3] = data[2] * b[0];
  res[4] = data[3] * b[0];
  res[13] = -data[2] * b[1] + data[1] * b[2] - data[0] * b[6];
  res[12] = data[3] * b[1] - data[1] * b[3] - data[0] * b[5];
  res[11] = -data[3] * b[2] + data[2] * b[3] - data[0] * b[4];
  res[14] = data[3] * b[6] + data[2] * b[5] + data[1] * b[4];
  return res;
}
constexpr MultiVector Motor::operator^(const MultiVector &b) const {
  MultiVector res{};
  res[0] = data[0] * b[0];
  res[1] = data[0] * b[1];
  res[2] = data[0] * b[2];
  res[3] = data[0] * b[3];
  res[4] = data[0] * b[4];
  res[5] = data[1] * b[0] + data[0] * b[5];
  res[6] = data[2] * b[0] + data[0] * b[6];
  res[7] = data[3] * b[0] + data[0] * b[7];
  res[10] = data[6] * b[0] + data[0] * b[10];
  res[9] = data[5] * b[0] + data[0] * b[9];
  res[8] = data[4] * b[0] + data[0] * b[8];
  res[13] = -b[1] * data[6] + b[2] * data[2] - b[3] * data[1] + b[13] * data[0];
  res[12] = -b[1] * data[5] - b[2] * data[3] + b[4] * data[1] + b[12] * data[0];
  res[11] = -b[1] * data[4] + b[3] * data[3] - b[4] * data[2] + b[11] * data[0];
  res[14] = data[4] * b[2] + data[5] * b[3] + data[6] * b[4] + data[0] * b[14];
  res[15] = data[7] * b[0] + data[4] * b[5] + data[5] * b[6] + data[6] * b[7] +
            data[3] * b[10] + data[2] * b[9] + data[1] * b[8] + data[0] * b[15];
  return res;
}
constexpr TriVector Motor::operator^(const TriVector &b) const {
  TriVector res{};
  res[2] = data[0] * b[2];
  res[1] = data[0] * b[1];
  res[0] = data[0] * b[0];
  res[3] = data[0] * b[3];
  return res;
}
constexpr Motor Motor::operator^(const BiVector &b) const {
  Motor res{};
  res[1] = data[0] * b[0];
  res[2] = data[0] * b[1];
  res[3] = data[0] * b[2];
  res[6] = data[0] * b[5];
  res[5] = data[0] * b[4];
  res[4] = data[0] * b[3];
  res[7] = data[4] * b[0] + data[5] * b[1] + data[6] * b[2] + data[3] * b[5] +
           data[2] * b[4] + data[1] * b[3];
  return res;
}
constexpr MultiVector Motor::operator^(const Vector &b) const {
  MultiVector res{};
  res[1] = data[0] * b[0];
  res[2] = data[0] * b[1];
  res[3] = data[0] * b[2];
  res[4] = data[0] * b[3];
  res[13] = -b[0] * data[6] + b[1] * data[2] - b[2] * data[1];
  res[12] = -b[0] * data[5] - b[1] * data[3] + b[3] * data[1];
  res[11] = -b[0] * data[4] + b[2] * data[3] - b[3] * data[2];
  res[14] = data[4] * b[1] + data[5] * b[2] + data[6] * b[3];
  return res;
}
constexpr MultiVector Motor::operator^(const Motor &b) const {
  MultiVector res{};
  res[0] = data[0] * b[0];
  res[5] = data[1] * b[0] + data[0] * b[1];
  res[6] = data[2] * b[0] + data[0] * b[2];
  res[7] = data[3] * b[0] + data[0] * b[3];
  res[10] = data[6] * b[0] + data[0] * b[6];
  res[9] = data[5] * b[0] + data[0] * b[5];
  res[8] = data[4] * b[0] + data[0] * b[4];
  res[15] = data[7] * b[0] + data[4] * b[1] + data[5] * b[2] + data[6] * b[3] +
            data[3] * b[6] + data[2] * b[5] + data[1] * b[4] + data[0] * b[7];
  return res;
}

///////////////////////////////////////////////////////////////////////////////////
/// Regressive Product / Join
///////////////////////////////////////////////////////////////////////////////////

constexpr MultiVector MultiVector::operator&(const MultiVector &b) const {
  MultiVector res{};
  res[15] = b[15] * data[15];
  res[14] = b[14] * data[15] + b[15] * data[14];
  res[11] = b[11] * data[15] + b[15] * data[11];
  res[12] = b[12] * data[15] + b[15] * data[12];
  res[13] = b[13] * data[15] + b[15] * data[13];
  res[8] =
      b[8] * data[15] + b[11] * data[14] - b[14] * data[11] + b[15] * data[8];
  res[9] =
      b[9] * data[15] + b[12] * data[14] - b[14] * data[12] + b[15] * data[9];
  res[10] =
      b[10] * data[15] + b[13] * data[14] - b[14] * data[13] + b[15] * data[10];
  res[7] =
      b[7] * data[15] + b[12] * data[11] - b[11] * data[12] + b[15] * data[7];
  res[6] =
      b[6] * data[15] - b[13] * data[11] + b[11] * data[13] + b[15] * data[6];
  res[5] =
      b[5] * data[15] + b[13] * data[12] - b[12] * data[13] + b[15] * data[5];
  res[4] = b[4] * data[15] - b[7] * data[14] + b[9] * data[11] -
           b[8] * data[12] - b[12] * data[8] + b[11] * data[9] -
           b[14] * data[7] + b[15] * data[4];
  res[3] = b[3] * data[15] - b[6] * data[14] - b[10] * data[11] +
           b[8] * data[13] + b[13] * data[8] - b[11] * data[10] -
           b[14] * data[6] + b[15] * data[3];
  res[2] = b[2] * data[15] - b[5] * data[14] + b[10] * data[12] -
           b[9] * data[13] - b[13] * data[9] + b[12] * data[10] -
           b[14] * data[5] + b[15] * data[2];
  res[1] = b[1] * data[15] + b[5] * data[11] + b[6] * data[12] +
           b[7] * data[13] + b[13] * data[7] + b[12] * data[6] +
           b[11] * data[5] + b[15] * data[1];
  res[0] = b[0] * data[15] + b[1] * data[14] + b[2] * data[11] +
           b[3] * data[12] + b[4] * data[13] + b[5] * data[8] + b[6] * data[9] +
           b[7] * data[10] + b[10] * data[7] + b[9] * data[6] + b[8] * data[5] -
           b[13] * data[4] - b[12] * data[3] - b[11] * data[2] -
           b[14] * data[1] + b[15] * data[0];
  return res;
}
constexpr MultiVector MultiVector::operator&(const TriVector &b) const {
  MultiVector res{};
  res[14] = b[3] * data[15];
  res[11] = b[0] * data[15];
  res[12] = b[1] * data[15];
  res[13] = b[2] * data[15];
  res[8] = b[0] * data[14] - b[3] * data[11];
  res[9] = b[1] * data[14] - b[3] * data[12];
  res[10] = b[2] * data[14] - b[3] * data[13];
  res[7] = b[1] * data[11] - b[0] * data[12];
  res[6] = -b[2] * data[11] + b[0] * data[13];
  res[5] = b[2] * data[12] - b[1] * data[13];
  res[4] = -b[1] * data[8] + b[0] * data[9] - b[3] * data[7];
  res[3] = b[2] * data[8] - b[0] * data[10] - b[3] * data[6];
  res[2] = -b[2] * data[9] + b[1] * data[10] - b[3] * data[5];
  res[1] = b[2] * data[7] + b[1] * data[6] + b[0] * data[5];
  res[0] = -b[2] * data[4] - b[1] * data[3] - b[0] * data[2] - b[3] * data[1];
  return res;
}
constexpr MultiVector MultiVector::operator&(const BiVector &b) const {
  MultiVector res{};
  res[8] = b[3] * data[15];
  res[9] = b[4] * data[15];
  res[10] = b[5] * data[15];
  res[7] = b[2] * data[15];
  res[6] = b[1] * data[15];
  res[5] = b[0] * data[15];
  res[4] = -b[2] * data[14] + b[4] * data[11] - b[3] * data[12];
  res[3] = -b[1] * data[14] - b[5] * data[11] + b[3] * data[13];
  res[2] = -b[0] * data[14] + b[5] * data[12] - b[4] * data[13];
  res[1] = b[0] * data[11] + b[1] * data[12] + b[2] * data[13];
  res[0] = b[0] * data[8] + b[1] * data[9] + b[2] * data[10] + b[5] * data[7] +
           b[4] * data[6] + b[3] * data[5];
  return res;
}
constexpr MultiVector MultiVector::operator&(const Vector &b) const {
  MultiVector res{};
  res[4] = b[3] * data[15];
  res[3] = b[2] * data[15];
  res[2] = b[1] * data[15];
  res[1] = b[0] * data[15];
  res[0] =
      b[0] * data[14] + b[1] * data[11] + b[2] * data[12] + b[3] * data[13];
  return res;
}
constexpr MultiVector MultiVector::operator&(const Motor &b) const {
  MultiVector res{};
  res[15] = b[7] * data[15];
  res[14] = b[7] * data[14];
  res[11] = b[7] * data[11];
  res[12] = b[7] * data[12];
  res[13] = b[7] * data[13];
  res[8] = b[4] * data[15] + b[7] * data[8];
  res[9] = b[5] * data[15] + b[7] * data[9];
  res[10] = b[6] * data[15] + b[7] * data[10];
  res[7] = b[3] * data[15] + b[7] * data[7];
  res[6] = b[2] * data[15] + b[7] * data[6];
  res[5] = b[1] * data[15] + b[7] * data[5];
  res[4] =
      -b[3] * data[14] + b[5] * data[11] - b[4] * data[12] + b[7] * data[4];
  res[3] =
      -b[2] * data[14] - b[6] * data[11] + b[4] * data[13] + b[7] * data[3];
  res[2] =
      -b[1] * data[14] + b[6] * data[12] - b[5] * data[13] + b[7] * data[2];
  res[1] = b[1] * data[11] + b[2] * data[12] + b[3] * data[13] + b[7] * data[1];
  res[0] = b[0] * data[15] + b[1] * data[8] + b[2] * data[9] + b[3] * data[10] +
           b[6] * data[7] + b[5] * data[6] + b[4] * data[5] + b[7] * data[0];
  return res;
}
constexpr MultiVector TriVector::operator&(const MultiVector &b) const {
  MultiVector res{};
  res[14] = data[3] * b[15];
  res[11] = data[0] * b[15];
  res[12] = data[1] * b[15];
  res[13] = data[2] * b[15];
  res[8] = -data[0] * b[14] + data[3] * b[11];
  res[9] = -data[1] * b[14] + data[3] * b[12];
  res[10] = -data[2] * b[14] + data[3] * b[13];
  res[7] = -data[1] * b[11] + data[0] * b[12];
  res[6] = data[2] * b[11] - data[0] * b[13];
  res[5] = -data[2] * b[12] + data[1] * b[13];
  res[4] = -data[1] * b[8] + data[0] * b[9] - data[3] * b[7];
  res[3] = data[2] * b[8] - data[0] * b[10] - data[3] * b[6];
  res[2] = -data[2] * b[9] + data[1] * b[10] - data[3] * b[5];
  res[1] = data[2] * b[7] + data[1] * b[6] + data[0] * b[5];
  res[0] = data[2] * b[4] + data[1] * b[3] + data[0] * b[2] + data[3] * b[1];
  return res;
}
constexpr BiVector TriVector::operator&(const TriVector &b) const {
  BiVector res{};
  res[0] = b[2] * data[1] - b[1] * data[2];
  res[1] = -b[2] * data[0] + b[0] * data[2];
  res[2] = b[1] * data[0] - b[0] * data[1];
  res[3] = b[0] * data[3] - b[3] * data[0];
  res[4] = b[1] * data[3] - b[3] * data[1];
  res[5] = b[2] * data[3] - b[3] * data[2];
  return res;
}
constexpr Vector TriVector::operator&(const BiVector &b) const {
  Vector res{};
  res[3] = -data[1] * b[3] + data[0] * b[4] - data[3] * b[2];
  res[2] = data[2] * b[3] - data[0] * b[5] - data[3] * b[1];
  res[1] = -data[2] * b[4] + data[1] * b[5] - data[3] * b[0];
  res[0] = data[2] * b[2] + data[1] * b[1] + data[0] * b[0];
  return res;
}
constexpr float TriVector::operator&(const Vector &b) const {
  return data[2] * b[3] + data[1] * b[2] + data[0] * b[1] + data[3] * b[0];
}
constexpr MultiVector TriVector::operator&(const Motor &b) const {
  MultiVector res{};
  res[14] = data[3] * b[7];
  res[11] = data[0] * b[7];
  res[12] = data[1] * b[7];
  res[13] = data[2] * b[7];
  res[4] = -data[1] * b[4] + data[0] * b[5] - data[3] * b[3];
  res[3] = data[2] * b[4] - data[0] * b[6] - data[3] * b[2];
  res[2] = -data[2] * b[5] + data[1] * b[6] - data[3] * b[1];
  res[1] = data[2] * b[3] + data[1] * b[2] + data[0] * b[1];
  return res;
}
constexpr MultiVector BiVector::operator&(const MultiVector &b) const {
  MultiVector res{};
  res[8] = b[15] * data[3];
  res[9] = b[15] * data[4];
  res[10] = b[15] * data[5];
  res[7] = b[15] * data[2];
  res[6] = b[15] * data[1];
  res[5] = b[15] * data[0];
  res[4] = -b[12] * data[3] + b[11] * data[4] - b[14] * data[2];
  res[3] = b[13] * data[3] - b[11] * data[5] - b[14] * data[1];
  res[2] = -b[13] * data[4] + b[12] * data[5] - b[14] * data[0];
  res[1] = b[13] * data[2] + b[12] * data[1] + b[11] * data[0];
  res[0] = b[5] * data[3] + b[6] * data[4] + b[7] * data[5] + b[10] * data[2] +
           b[9] * data[1] + b[8] * data[0];
  return res;
}
constexpr Vector BiVector::operator&(const TriVector &b) const {
  Vector res{};
  res[3] = -b[1] * data[3] + b[0] * data[4] - b[3] * data[2];
  res[2] = b[2] * data[3] - b[0] * data[5] - b[3] * data[1];
  res[1] = -b[2] * data[4] + b[1] * data[5] - b[3] * data[0];
  res[0] = b[2] * data[2] + b[1] * data[1] + b[0] * data[0];
  return res;
}
constexpr float BiVector::operator&(const BiVector &b) const {
  return b[0] * data[3] + b[1] * data[4] + b[2] * data[5] + b[5] * data[2] +
         b[4] * data[1] + b[3] * data[0];
}
constexpr GANull BiVector::operator&(const Vector &) const { return GANull{}; }
constexpr MultiVector BiVector::operator&(const Motor &b) const {
  MultiVector res{};
  res[8] = b[7] * data[3];
  res[9] = b[7] * data[4];
  res[10] = b[7] * data[5];
  res[7] = b[7] * data[2];
  res[6] = b[7] * data[1];
  res[5] = b[7] * data[0];
  res[0] = b[1] * data[3] + b[2] * data[4] + b[3] * data[5] + b[6] * data[2] +
           b[5] * data[1] + b[4] * data[0];
  return res;
}
constexpr MultiVector Vector::operator&(const MultiVector &b) const {
  MultiVector res{};
  res[4] = b[15] * data[3];
  res[3] = b[15] * data[2];
  res[2] = b[15] * data[1];
  res[1] = b[15] * data[0];
  res[0] =
      -b[13] * data[3] - b[12] * data[2] - b[11] * data[1] - b[14] * data[0];
  return res;
}
constexpr float Vector::operator&(const TriVector &b) const {
  return -b[2] * data[3] - b[1] * data[2] - b[0] * data[1] - b[3] * data[0];
}
constexpr GANull Vector::operator&(const BiVector &) const { return GANull{}; }
constexpr GANull Vector::operator&(const Vector &) const { return GANull{}; }
constexpr Vector Vector::operator&(const Motor &b) const {
  Vector res{};
  res[3] = b[7] * data[3];
  res[2] = b[7] * data[2];
  res[1] = b[7] * data[1];
  res[0] = b[7] * data[0];
  return res;
}
constexpr MultiVector Motor::operator&(const MultiVector &b) const {
  MultiVector res{};
  res[15] = b[15] * data[7];
  res[14] = b[14] * data[7];
  res[11] = b[11] * data[7];
  res[12] = b[12] * data[7];
  res[13] = b[13] * data[7];
  res[8] = b[8] * data[7] + b[15] * data[4];
  res[9] = b[9] * data[7] + b[15] * data[5];
  res[10] = b[10] * data[7] + b[15] * data[6];
  res[7] = b[7] * data[7] + b[15] * data[3];
  res[6] = b[6] * data[7] + b[15] * data[2];
  res[5] = b[5] * data[7] + b[15] * data[1];
  res[4] = b[4] * data[7] - b[12] * data[4] + b[11] * data[5] - b[14] * data[3];
  res[3] = b[3] * data[7] + b[13] * data[4] - b[11] * data[6] - b[14] * data[2];
  res[2] = b[2] * data[7] - b[13] * data[5] + b[12] * data[6] - b[14] * data[1];
  res[1] = b[1] * data[7] + b[13] * data[3] + b[12] * data[2] + b[11] * data[1];
  res[0] = b[0] * data[7] + b[5] * data[4] + b[6] * data[5] + b[7] * data[6] +
           b[10] * data[3] + b[9] * data[2] + b[8] * data[1] + b[15] * data[0];
  return res;
}
constexpr MultiVector Motor::operator&(const TriVector &b) const {
  MultiVector res{};
  res[14] = b[3] * data[7];
  res[11] = b[0] * data[7];
  res[12] = b[1] * data[7];
  res[13] = b[2] * data[7];
  res[4] = -b[1] * data[4] + b[0] * data[5] - b[3] * data[3];
  res[3] = b[2] * data[4] - b[0] * data[6] - b[3] * data[2];
  res[2] = -b[2] * data[5] + b[1] * data[6] - b[3] * data[1];
  res[1] = b[2] * data[3] + b[1] * data[2] + b[0] * data[1];
  return res;
}
constexpr Motor Motor::operator&(const BiVector &b) const {
  Motor res{};
  res[4] = b[3] * data[7];
  res[5] = b[4] * data[7];
  res[6] = b[5] * data[7];
  res[3] = b[2] * data[7];
  res[2] = b[1] * data[7];
  res[1] = b[0] * data[7];
  res[0] = b[0] * data[4] + b[1] * data[5] + b[2] * data[6] + b[5] * data[3] +
           b[4] * data[2] + b[3] * data[1];
  return res;
}
constexpr Vector Motor::operator&(const Vector &b) const {
  Vector res{};
  res[3] = b[3] * data[7];
  res[2] = b[2] * data[7];
  res[1] = b[1] * data[7];
  res[0] = b[0] * data[7];
  return res;
}
constexpr Motor Motor::operator&(const Motor &b) const {
  Motor res{};
  res[7] = b[7] * data[7];
  res[4] = b[4] * data[7] + b[7] * data[4];
  res[5] = b[5] * data[7] + b[7] * data[5];
  res[6] = b[6] * data[7] + b[7] * data[6];
  res[3] = b[3] * data[7] + b[7] * data[3];
  res[2] = b[2] * data[7] + b[7] * data[2];
  res[1] = b[1] * data[7] + b[7] * data[1];
  res[0] = b[0] * data[7] + b[1] * data[4] + b[2] * data[5] + b[3] * data[6] +
           b[6] * data[3] + b[5] * data[2] + b[4] * data[1] + b[7] * data[0];
  return res;
}

///////////////////////////////////////////////////////////////////////////////////
/// Hodge dual
///////////////////////////////////////////////////////////////////////////////////

constexpr MultiVector MultiVector::operator!() const {
  return {data[15], -data[14], -data[11], -data[12], -data[13], data[8],
          data[9],  data[10],  data[5],   data[6],   data[7],   -data[2],
          -data[3], -data[4],  -data[1],  data[0]};
}
constexpr Vector TriVector::operator!() const {
  return {-data[3], -data[0], -data[1], -data[2]};
}
constexpr BiVector BiVector::operator!() const {
  return {data[3], data[4], data[5], data[0], data[1], data[2]};
}
constexpr TriVector Vector::operator!() const {
  return {-data[1], -data[2], -data[3], -data[0]};
}
constexpr Motor Motor::operator!() const {
  return {data[7], data[4], data[5], data[6],
          data[1], data[2], data[3], data[0]};
}

///////////////////////////////////////////////////////////////////////////////////
/// Exponential
///////////////////////////////////////////////////////////////////////////////////

constexpr MultiVector Vector::Gexp() const {
  float vectorNorm{this->Norm()};
  MultiVector res{};
  if (vectorNorm != 0) {
    float const factor{detail::ce_sinh(vectorNorm) / vectorNorm};
    res[0] = detail::ce_cosh(vectorNorm);
    res[1] = data[0] * factor;
    res[2] = data[1] * factor;
    res[3] = data[2] * factor;
    res[4] = data[3] * factor;
  } else {
    res[0] = 1;
  }
  return res;
}

constexpr MultiVector TriVector::Gexp() const {
  float const trivectorNorm{this->Norm()};
  MultiVector res{};
  if (trivectorNorm != 0) {
    float const factor{detail::ce_sin(trivectorNorm) / trivectorNorm};
    res = MultiVector{detail::ce_cos(trivectorNorm),
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      0,
                      data[0] * factor,
                      data[1] * factor,
                      data[2] * factor,
                      data[3] * factor,
                      0};
  }
  return res;
}

///////////////////////////////////////////////////////////////////////////////////
/// Named functions
///////////////////////////////////////////////////////////////////////////////////

namespace GA {
// Inverse
template <typename A>
  requires requires(const A &a) { ~a; }
[[nodiscard]] constexpr auto Inverse(const A &a) {
  return ~a;
}

// Exponential
template <typename T>
  requires requires(const T &value) { value.Gexp(); }
[[nodiscard]] constexpr auto Gexp(const T &value) {
  return value.Gexp();
}

// Gep
template <typename A, typename B>
  requires requires(const A &a, const B &b) { a * b; }
[[nodiscard]] constexpr auto Gep(const A &a, const B &b) {
  return a * b;
}

// Inner
template <typename A, typename B>
  requires requires(const A &a, const B &b) { a | b; }
[[nodiscard]] constexpr auto Inner(const A &a, const B &b) {
  return a | b;
}

// Outer
template <typename A, typename B>
  requires requires(const A &a, const B &b) { a ^ b; }
[[nodiscard]] constexpr auto Outer(const A &a, const B &b) {
  return a ^ b;
}

// Join
template <typename A, typename B>
  requires requires(const A &a, const B &b) { a & b; }
[[nodiscard]] constexpr auto Join(const A &a, const B &b) {
  return a & b;
}

// Hodge dual
template <typename T>
  requires requires(const T &value) { !value; }
[[nodiscard]] constexpr auto HodgeDual(const T &value) {
  return !value;
}
} // namespace GA

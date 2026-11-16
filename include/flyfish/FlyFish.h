#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <numbers>
#include <sstream>
#include <concepts>
#include <string>
#include <type_traits>

namespace flyfish {

class MultiVector;
class Vector;
class BiVector;
class TriVector;
class Motor;
class PseudoScalar;
class GANull;

/// Any value that can appear as an operand of a GA operator: a plain number or
/// one of the library's element types.
template <typename T>
concept GAOperand =
    std::is_arithmetic_v<T> || std::same_as<T, MultiVector> ||
    std::same_as<T, Vector> || std::same_as<T, BiVector> ||
    std::same_as<T, TriVector> || std::same_as<T, Motor> ||
    std::same_as<T, PseudoScalar> || std::same_as<T, GANull>;

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

constexpr float ce_exp(float x) {
  if consteval {
    // Scale into [-0.5, 0.5], sum the series, then square back up.
    int halvings = 0;
    while (ce_abs(x) > 0.5f && halvings < 64) {
      x *= 0.5f;
      ++halvings;
    }
    float result = 0.0f;
    float term = 1.0f;
    for (int i = 1; i < 14; ++i) {
      result += term;
      term *= x / static_cast<float>(i);
    }
    for (int i = 0; i < halvings; ++i)
      result *= result;
    return result;
  } else {
    return std::exp(x);
  }
}
} // namespace detail

/// An angle with an explicit unit, so Motor::Rotation can't be handed degrees
/// where radians are expected (or the other way around).
///   Angle::FromDegrees(90), Angle::FromRadians(pi / 2), 90.0_deg, 1.5_rad
class Angle final {
public:
  [[nodiscard]] static constexpr Angle FromRadians(float radians) noexcept {
    return Angle{radians};
  }
  [[nodiscard]] static constexpr Angle FromDegrees(float degrees) noexcept {
    return Angle{degrees * DEG_TO_RAD};
  }

  [[nodiscard]] constexpr float Radians() const noexcept { return radians; }
  [[nodiscard]] constexpr float Degrees() const noexcept {
    return radians * RAD_TO_DEG;
  }

  [[nodiscard]] constexpr Angle operator-() const noexcept {
    return Angle{-radians};
  }
  [[nodiscard]] constexpr bool operator==(const Angle &) const = default;

private:
  constexpr explicit Angle(float value) noexcept : radians{value} {}
  float radians;
};

inline namespace literals {
[[nodiscard]] constexpr Angle operator""_deg(long double degrees) noexcept {
  return Angle::FromDegrees(static_cast<float>(degrees));
}
[[nodiscard]] constexpr Angle operator""_deg(unsigned long long degrees) noexcept {
  return Angle::FromDegrees(static_cast<float>(degrees));
}
[[nodiscard]] constexpr Angle operator""_rad(long double radians) noexcept {
  return Angle::FromRadians(static_cast<float>(radians));
}
[[nodiscard]] constexpr Angle operator""_rad(unsigned long long radians) noexcept {
  return Angle::FromRadians(static_cast<float>(radians));
}
} // namespace literals

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
      const float magnitude = std::fabs(data[i]);
      if (magnitude > 1e-6f) {
        if (!first) {
          output << (data[i] > 0 ? " + " : " - ");
        } else if (data[i] < 0) {
          output << "-";
        }
        first = false;

        // Decide on the printed digits, not the exact float, so a coefficient
        // like 0.9999999 (which prints as "1") is shown as "e1", not "1*e1".
        std::ostringstream number;
        number << magnitude;
        const std::string digits = number.str();
        const std::string name = names[i];

        if (name.empty()) {
          output << digits;
        } else if (digits == "1") {
          output << name;
        } else {
          output << digits << "*" << name;
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

  /// Exact comparison of every coefficient. Results of float arithmetic
  /// (anything that went through a rotation, a normalization, ...) are rarely
  /// exactly equal: compare those with RoundedEqual.
  [[nodiscard]] constexpr bool operator==(const GAElement &b) const {
    return data == b.data;
  }

  /// Approximate comparison: every coefficient may differ by at most
  /// `tolerance`, scaled up by the largest coefficient when that is above 1
  /// (floats only carry about 7 digits, so the error grows with the values).
  [[nodiscard]] constexpr bool
  RoundedEqual(const GAElement &b, float tolerance = 1e-5f) const noexcept {
    float scale = 1.0f;
    for (size_t i = 0; i < DataSize; ++i) {
      const float largest = detail::ce_abs(data[i]) > detail::ce_abs(b[i])
                                ? detail::ce_abs(data[i])
                                : detail::ce_abs(b[i]);
      if (largest > scale) {
        scale = largest;
      }
    }
    for (size_t i = 0; i < DataSize; ++i) {
      if (detail::ce_abs(data[i] - b[i]) > tolerance * scale) {
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
  // s / X = s * X^-1, for every type that has an inverse (operator~).
  friend constexpr Derived operator/(float scalar, const Derived &element)
    requires requires { ~element; }
  {
    return scalar * ~element;
  }

  /// Scales the element to unit (Euclidean) norm. Ideal elements (points at
  /// infinity, lines at infinity, e0, the pseudoscalar) have a Euclidean norm
  /// of 0 and cannot be normalized this way; doing so asserts.
  constexpr Derived &Normalize() {
    const float norm = static_cast<const Derived *>(this)->Norm();
    assert(norm != 0.0f &&
           "Normalize: the Euclidean norm is 0 (ideal element), cannot divide");
    return (*this) /= norm;
  }

  [[nodiscard]] constexpr Derived Normalized() const {
    const float norm = static_cast<const Derived *>(this)->Norm();
    assert(norm != 0.0f &&
           "Normalized: the Euclidean norm is 0 (ideal element), cannot divide");
    Derived d{};
    const float mult = 1 / norm;
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

  // A lone float never turns into a MultiVector implicitly (so `mv ^ 2.0f`
  // does not compile); write MultiVector{2.0f} when you mean the scalar.
  [[nodiscard]] constexpr explicit MultiVector(float s) noexcept {
    data[0] = s;
  }

  // Every element is also a MultiVector (with the other grades zero), so
  // these conversions are implicit: MultiVector mv = point;
  [[nodiscard]] constexpr MultiVector(const Vector &b) noexcept;
  [[nodiscard]] constexpr MultiVector(const BiVector &b) noexcept;
  [[nodiscard]] constexpr MultiVector(const TriVector &b) noexcept;
  [[nodiscard]] constexpr MultiVector(const Motor &b) noexcept;
  [[nodiscard]] constexpr MultiVector(const PseudoScalar &b) noexcept;
  [[nodiscard]] constexpr MultiVector(const GANull &b) noexcept;

  [[nodiscard]] constexpr MultiVector(
      float s, float e0, float e1 = 0, float e2 = 0, float e3 = 0,
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
  constexpr MultiVector &operator=(const PseudoScalar &b);
  constexpr MultiVector &operator=(const GANull &b);

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
  [[nodiscard]] constexpr PseudoScalar Grade4() const;
  [[nodiscard]] constexpr Motor ToMotor() const;

  [[nodiscard]] constexpr MultiVector operator~() const;

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Motor &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Motor &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Motor &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const TriVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Motor &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator!() const;

  /// General exponential (scaling and squaring of the power series). For a
  /// single-grade element prefer the typed Gexp, which uses the closed form.
  [[nodiscard]] constexpr MultiVector Gexp() const;
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

  /// The plane a*x + b*y + c*z + d = 0, i.e. d*e0 + a*e1 + b*e2 + c*e3.
  [[nodiscard]] static constexpr Vector Plane(float a, float b, float c,
                                              float d) noexcept {
    return {d, a, b, c};
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
  [[nodiscard]] constexpr TriVector operator*(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr float operator&(const TriVector &b) const;
  [[nodiscard]] constexpr GANull operator&(const BiVector &b) const;
  [[nodiscard]] constexpr GANull operator&(const Vector &b) const;
  [[nodiscard]] constexpr Vector operator&(const Motor &b) const;
  [[nodiscard]] constexpr Vector operator&(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr BiVector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr float operator|(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Motor &b) const;
  [[nodiscard]] constexpr TriVector operator|(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr PseudoScalar operator^(const TriVector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const BiVector &b) const;
  [[nodiscard]] constexpr BiVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Motor &b) const;
  [[nodiscard]] constexpr GANull operator^(const PseudoScalar &b) const;

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

  /// The line through (x1, y1, z1) and (x2, y2, z2), oriented from the first
  /// point to the second.
  [[nodiscard]] static constexpr BiVector
  LineFromPoints(float x1, float y1, float z1, float x2, float y2, float z2) {
    return {y1 * z2 - y2 * z1, z1 * x2 - z2 * x1, x1 * y2 - x2 * y1,
            x2 - x1,           y2 - y1,           z2 - z1};
  }

  /// The line through two points (the join p & q). Also works when one point
  /// is a direction (ideal point): the line then runs through the other point
  /// in that direction.
  [[nodiscard]] static constexpr BiVector LineFromPoints(const TriVector &p,
                                                         const TriVector &q);

  [[nodiscard]] constexpr float Norm() const {
    return detail::ce_sqrt(data[3] * data[3] + data[4] * data[4] +
                           data[5] * data[5]);
  }
  [[nodiscard]] constexpr float VNorm() const {
    return detail::ce_sqrt(data[0] * data[0] + data[1] * data[1] +
                           data[2] * data[2]);
  }

  // B^2 = -l + 2*m*e0123, with l = B23^2 + B31^2 + B12^2 and
  // m = B23*B01 + B31*B02 + B12*B03. For a line m = 0 and B^-1 = -B / l; in
  // general (a screw, e.g. the sum of two skew lines)
  //   B^-1 = -(B + k * e0123 * B) / l,  k = 2*m / l,
  // where e0123 * B = -(B23*e01 + B31*e02 + B12*e03).
  [[nodiscard]] constexpr BiVector operator~() const {
    const float l{data[3] * data[3] + data[4] * data[4] + data[5] * data[5]};
    const float m{data[3] * data[0] + data[4] * data[1] + data[5] * data[2]};
    const float inv{1.0f / l};
    const float k{2 * m * inv};
    return {-(data[0] - k * data[3]) * inv,
            -(data[1] - k * data[4]) * inv,
            -(data[2] - k * data[5]) * inv,
            -data[3] * inv,
            -data[4] * inv,
            -data[5] * inv};
  }

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Motor &b) const;
  [[nodiscard]] constexpr BiVector operator*(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr float operator|(const BiVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator|(const Motor &b) const;
  [[nodiscard]] constexpr BiVector operator|(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr float operator&(const BiVector &b) const;
  [[nodiscard]] constexpr GANull operator&(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Motor &b) const;
  [[nodiscard]] constexpr BiVector operator&(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const TriVector &b) const;
  [[nodiscard]] constexpr PseudoScalar operator^(const BiVector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator^(const Motor &b) const;
  [[nodiscard]] constexpr GANull operator^(const PseudoScalar &b) const;

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

  /// The Euclidean point (x, y, z): x*e032 + y*e013 + z*e021 + e123.
  [[nodiscard]] static constexpr TriVector Point(float x, float y,
                                                 float z) noexcept {
    return {x, y, z, 1};
  }
  /// The direction (x, y, z), a point at infinity: e123 = 0.
  [[nodiscard]] static constexpr TriVector Direction(float x, float y,
                                                     float z) noexcept {
    return {x, y, z, 0};
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
  [[nodiscard]] constexpr Vector operator*(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr float operator|(const TriVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr BiVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Motor &b) const;
  [[nodiscard]] constexpr Vector operator|(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr BiVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const BiVector &b) const;
  [[nodiscard]] constexpr float operator&(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const Motor &b) const;
  [[nodiscard]] constexpr TriVector operator&(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const TriVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const BiVector &b) const;
  [[nodiscard]] constexpr PseudoScalar operator^(const Vector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const Motor &b) const;
  [[nodiscard]] constexpr GANull operator^(const PseudoScalar &b) const;

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

  /// Translation by the vector (dx, dy, dz): applied as M * X * ~M it moves
  /// the point (x, y, z) to (x + dx, y + dy, z + dz).
  [[nodiscard]] static constexpr Motor Translation(float dx, float dy,
                                                   float dz) noexcept {
    return Motor{1, -0.5f * dx, -0.5f * dy, -0.5f * dz, 0, 0, 0, 0};
  }

  /// Translation over `distance`, in the direction given by the ideal part
  /// (e01, e02, e03) of `line`: e01 moves along x, e02 along y, e03 along z.
  /// The Euclidean part of `line` is ignored; prefer Translation(dx, dy, dz).
  [[nodiscard]] static constexpr Motor Translation(float distance,
                                                   const BiVector &line) {
    const float idealNorm{line.VNorm()};
    assert(idealNorm != 0.0f &&
           "Translation: the line has no ideal part (e01, e02, e03)");
    const float d{-distance / (2 * idealNorm)};
    return Motor{1, d * line[0], d * line[1], d * line[2], 0, 0, 0, 0};
  }

  /// Rotation by `angle` around `line`, counterclockwise when looking against
  /// the line's direction (right-hand rule). Equal to Gexp(-angle/2 * L), with
  /// L the normalized line.
  ///   Motor::Rotation(Angle::FromDegrees(90), line) or
  ///   Motor::Rotation(90.0_deg, line)
  [[nodiscard]] static constexpr Motor Rotation(Angle angle,
                                                const BiVector &line) {
    const float norm{line.Norm()};
    assert(norm != 0.0f &&
           "Rotation: the line is ideal (Euclidean norm 0), use Translation");
    const float halfAngle{0.5f * angle.Radians()};
    const float mult{-detail::ce_sin(halfAngle) / norm};
    return Motor{detail::ce_cos(halfAngle),
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
  [[nodiscard]] constexpr PseudoScalar Grade4() const;

  // M^-1 = rev(M) / (M * rev(M)), and M * rev(M) = a + b*e0123 with
  //   a = s^2 + e23^2 + e31^2 + e12^2,
  //   b = 2 * (s*e0123 - e23*e01 - e31*e02 - e12*e03).
  // For a normalized motor b = 0 and the inverse is just the reverse; in
  // general (a + b*e0123)^-1 = (1 - (b/a)*e0123) / a.
  [[nodiscard]] constexpr Motor operator~() const {
    const float a{data[0] * data[0] + data[4] * data[4] + data[5] * data[5] +
                  data[6] * data[6]};
    const float b{2 * (data[0] * data[7] - data[4] * data[1] -
                       data[5] * data[2] - data[6] * data[3])};
    const float inv{1.0f / a};
    const float c{b * inv};
    return {data[0] * inv,
            (-data[1] - c * data[4]) * inv,
            (-data[2] - c * data[5]) * inv,
            (-data[3] - c * data[6]) * inv,
            -data[4] * inv,
            -data[5] * inv,
            -data[6] * inv,
            (data[7] - c * data[0]) * inv};
  }

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator*(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator*(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Motor &b) const;
  [[nodiscard]] constexpr Motor operator*(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator|(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator|(const Motor &b) const;
  [[nodiscard]] constexpr Motor operator|(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator&(const BiVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator&(const Motor &b) const;
  [[nodiscard]] constexpr Motor operator&(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr TriVector operator^(const TriVector &b) const;
  [[nodiscard]] constexpr Motor operator^(const BiVector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Vector &b) const;
  [[nodiscard]] constexpr MultiVector operator^(const Motor &b) const;
  [[nodiscard]] constexpr PseudoScalar operator^(const PseudoScalar &b) const;

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
  constexpr Motor &operator+=(const PseudoScalar &b);
  constexpr Motor &operator-=(const PseudoScalar &b);

  [[nodiscard]] constexpr Motor operator!() const;

  [[nodiscard]] constexpr Motor Gexp() const;
};

/// The PGA pseudoscalar e0123. It squares to zero (e0 * e0 = 0), so it is not
/// invertible and has no Euclidean norm. Its Hodge dual is the scalar, it is
/// the identity element of the regressive product (I & X = X), and its
/// exponential is the motor 1 + p*e0123.
class PseudoScalar final : public GAElement<PseudoScalar, 1> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] constexpr float &e0123() { return data[0]; }
  [[nodiscard]] constexpr const float &e0123() const { return data[0]; }

  [[nodiscard]] constexpr PseudoScalar() noexcept : GAElement() {}

  // Explicit so a plain float never silently turns into a pseudoscalar.
  [[nodiscard]] constexpr explicit PseudoScalar(float e0123) noexcept
      : GAElement() {
    data[0] = e0123;
  }

  static constexpr std::array<const char *, 1> names() { return {"e0123"}; }

  // e0123 is purely ideal: its Euclidean norm is 0, its ideal norm is |e0123|.
  [[nodiscard]] constexpr float Norm() const { return 0.0f; }
  [[nodiscard]] constexpr float VNorm() const { return detail::ce_abs(data[0]); }

  [[nodiscard]] constexpr MultiVector operator*(const MultiVector &b) const;
  [[nodiscard]] constexpr Vector operator*(const TriVector &b) const;
  [[nodiscard]] constexpr BiVector operator*(const BiVector &b) const;
  [[nodiscard]] constexpr TriVector operator*(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator*(const Motor &b) const;
  [[nodiscard]] constexpr GANull operator*(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator|(const MultiVector &b) const;
  [[nodiscard]] constexpr Vector operator|(const TriVector &b) const;
  [[nodiscard]] constexpr BiVector operator|(const BiVector &b) const;
  [[nodiscard]] constexpr TriVector operator|(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator|(const Motor &b) const;
  [[nodiscard]] constexpr GANull operator|(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator&(const MultiVector &b) const;
  [[nodiscard]] constexpr TriVector operator&(const TriVector &b) const;
  [[nodiscard]] constexpr BiVector operator&(const BiVector &b) const;
  [[nodiscard]] constexpr Vector operator&(const Vector &b) const;
  [[nodiscard]] constexpr Motor operator&(const Motor &b) const;
  [[nodiscard]] constexpr PseudoScalar operator&(const PseudoScalar &b) const;

  [[nodiscard]] constexpr MultiVector operator^(const MultiVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const TriVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const BiVector &b) const;
  [[nodiscard]] constexpr GANull operator^(const Vector &b) const;
  [[nodiscard]] constexpr PseudoScalar operator^(const Motor &b) const;
  [[nodiscard]] constexpr GANull operator^(const PseudoScalar &b) const;

  // Hodge dual of e0123 is the scalar 1.
  [[nodiscard]] constexpr float operator!() const { return data[0]; }

  [[nodiscard]] constexpr Motor Gexp() const;
};

class GANull final : public GAElement<GANull, 0> {
public:
  using GAElement::GAElement;
  using GAElement::operator*;
  using GAElement::operator/;

  [[nodiscard]] static std::string ToString() { return "GANull"; }

  // GANull is the zero of every type, so a result that is identically zero
  // can still be stored as the type generic code expects:
  //   Vector v = Vector{} & BiVector{};   // GANull -> zero Vector
  // (The conversion to MultiVector is MultiVector's own constructor.)
  constexpr operator Vector() const noexcept;
  constexpr operator BiVector() const noexcept;
  constexpr operator TriVector() const noexcept;
  constexpr operator Motor() const noexcept;
  constexpr operator PseudoScalar() const noexcept;
  // Deliberately no conversion to float: that would let `2 / GANull{}` or
  // `GANull{} + 1` silently compile as plain float arithmetic.

  // Any product with zero is zero. Restricted to the library's types (and
  // plain numbers) so these templates never hijack unrelated operators.
  template <GAOperand T>
  [[nodiscard]] constexpr GANull operator*(const T &) const {
    return GANull{};
  }
  template <GAOperand T>
  [[nodiscard]] constexpr GANull operator|(const T &) const {
    return GANull{};
  }
  template <GAOperand T>
  [[nodiscard]] constexpr GANull operator^(const T &) const {
    return GANull{};
  }
  template <GAOperand T>
  [[nodiscard]] constexpr GANull operator&(const T &) const {
    return GANull{};
  }

  template <GAOperand T>
    requires(!std::same_as<T, GANull>)
  friend constexpr GANull operator*(const T &, const GANull &) {
    return GANull{};
  }
  template <GAOperand T>
    requires(!std::same_as<T, GANull>)
  friend constexpr GANull operator|(const T &, const GANull &) {
    return GANull{};
  }
  template <GAOperand T>
    requires(!std::same_as<T, GANull>)
  friend constexpr GANull operator^(const T &, const GANull &) {
    return GANull{};
  }
  template <GAOperand T>
    requires(!std::same_as<T, GANull>)
  friend constexpr GANull operator&(const T &, const GANull &) {
    return GANull{};
  }
};

constexpr GANull::operator Vector() const noexcept { return {}; }
constexpr GANull::operator BiVector() const noexcept { return {}; }
constexpr GANull::operator TriVector() const noexcept { return {}; }
constexpr GANull::operator Motor() const noexcept { return {}; }
constexpr GANull::operator PseudoScalar() const noexcept { return {}; }

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
constexpr PseudoScalar MultiVector::Grade4() const {
  return PseudoScalar{data[15]};
}
constexpr BiVector Motor::Grade2() const {
  return {data[1], data[2], data[3], data[4], data[5], data[6]};
}
constexpr PseudoScalar Motor::Grade4() const { return PseudoScalar{data[7]}; }

constexpr Motor &Motor::operator+=(const PseudoScalar &b) {
  data[7] += b[0];
  return *this;
}
constexpr Motor &Motor::operator-=(const PseudoScalar &b) {
  data[7] -= b[0];
  return *this;
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
constexpr MultiVector &MultiVector::operator=(const PseudoScalar &b) {
  data.fill(0);
  data[15] = b[0];
  return *this;
}
constexpr MultiVector &MultiVector::operator=(const GANull &) {
  data.fill(0);
  return *this;
}

constexpr BiVector BiVector::LineFromPoints(const TriVector &p,
                                            const TriVector &q) {
  return p & q;
}

// Converting constructors
constexpr MultiVector::MultiVector(const Vector &b) noexcept { *this = b; }
constexpr MultiVector::MultiVector(const BiVector &b) noexcept { *this = b; }
constexpr MultiVector::MultiVector(const TriVector &b) noexcept { *this = b; }
constexpr MultiVector::MultiVector(const Motor &b) noexcept { *this = b; }
constexpr MultiVector::MultiVector(const PseudoScalar &b) noexcept {
  *this = b;
}
constexpr MultiVector::MultiVector(const GANull &) noexcept {}

///////////////////////////////////////////////////////////////////////////////////
/// Sums of different types
///////////////////////////////////////////////////////////////////////////////////

namespace detail {
// One of the library's element types (not a plain number, not GANull).
template <typename T>
concept Element =
    GAOperand<T> && !std::is_arithmetic_v<T> && !std::same_as<T, GANull>;

// Fits in a Motor: a scalar, a bivector, the pseudoscalar or a motor.
template <typename T>
concept EvenOperand =
    std::is_arithmetic_v<T> || std::same_as<T, BiVector> ||
    std::same_as<T, PseudoScalar> || std::same_as<T, Motor>;

template <typename T> constexpr Motor WidenToMotor(const T &value) {
  if constexpr (std::is_arithmetic_v<T>) {
    return {static_cast<float>(value), 0, 0, 0, 0, 0, 0, 0};
  } else if constexpr (std::same_as<T, BiVector>) {
    return {0, value[0], value[1], value[2], value[3], value[4], value[5], 0};
  } else if constexpr (std::same_as<T, PseudoScalar>) {
    return {0, 0, 0, 0, 0, 0, 0, value[0]};
  } else {
    return value;
  }
}

template <typename T> constexpr MultiVector WidenToMultiVector(const T &value) {
  if constexpr (std::is_arithmetic_v<T>) {
    return MultiVector{static_cast<float>(value)};
  } else {
    return value;
  }
}

// Sum (sign = 1) or difference (sign = -1) of two different types, in the
// smallest type that can hold the result.
template <typename A, typename B>
constexpr auto MixedSum(const A &a, const B &b, float sign) {
  if constexpr (std::same_as<A, GANull>) {
    return b * sign;
  } else if constexpr (std::same_as<B, GANull>) {
    return a;
  } else if constexpr (EvenOperand<A> && EvenOperand<B>) {
    return WidenToMotor(a) + WidenToMotor(b) * sign;
  } else {
    return WidenToMultiVector(a) + WidenToMultiVector(b) * sign;
  }
}
} // namespace detail

/// Adding two different types gives the smallest type that holds the result:
/// a Motor when both fit in one (scalar, BiVector, PseudoScalar, Motor), a
/// MultiVector otherwise.
///   Motor m = 1.0f + line;   MultiVector mv = plane + point;
template <GAOperand A, GAOperand B>
  requires(!std::same_as<A, B>) && (detail::Element<A> || detail::Element<B>)
[[nodiscard]] constexpr auto operator+(const A &a, const B &b) {
  return detail::MixedSum(a, b, 1.0f);
}
template <GAOperand A, GAOperand B>
  requires(!std::same_as<A, B>) && (detail::Element<A> || detail::Element<B>)
[[nodiscard]] constexpr auto operator-(const A &a, const B &b) {
  return detail::MixedSum(a, b, -1.0f);
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

// Pseudoscalar: e0123 annihilates every blade containing e0, so only the
// Euclidean components of the other operand survive.
constexpr MultiVector MultiVector::operator*(const PseudoScalar &b) const {
  MultiVector res{};
  res[1] = b[0] * data[14];
  res[5] = -b[0] * data[8];
  res[6] = -b[0] * data[9];
  res[7] = -b[0] * data[10];
  res[11] = b[0] * data[2];
  res[12] = b[0] * data[3];
  res[13] = b[0] * data[4];
  res[15] = b[0] * data[0];
  return res;
}
constexpr TriVector Vector::operator*(const PseudoScalar &b) const {
  return {b[0] * data[1], b[0] * data[2], b[0] * data[3], 0};
}
constexpr BiVector BiVector::operator*(const PseudoScalar &b) const {
  return {-b[0] * data[3], -b[0] * data[4], -b[0] * data[5], 0, 0, 0};
}
constexpr Vector TriVector::operator*(const PseudoScalar &b) const {
  return {b[0] * data[3], 0, 0, 0};
}
constexpr Motor Motor::operator*(const PseudoScalar &b) const {
  return {0, -b[0] * data[4], -b[0] * data[5], -b[0] * data[6],
          0, 0,               0,               b[0] * data[0]};
}
constexpr MultiVector PseudoScalar::operator*(const MultiVector &b) const {
  MultiVector res{};
  res[1] = -b[14] * data[0];
  res[5] = -b[8] * data[0];
  res[6] = -b[9] * data[0];
  res[7] = -b[10] * data[0];
  res[11] = -b[2] * data[0];
  res[12] = -b[3] * data[0];
  res[13] = -b[4] * data[0];
  res[15] = b[0] * data[0];
  return res;
}
constexpr Vector PseudoScalar::operator*(const TriVector &b) const {
  return {-b[3] * data[0], 0, 0, 0};
}
constexpr BiVector PseudoScalar::operator*(const BiVector &b) const {
  return {-b[3] * data[0], -b[4] * data[0], -b[5] * data[0], 0, 0, 0};
}
constexpr TriVector PseudoScalar::operator*(const Vector &b) const {
  return {-b[1] * data[0], -b[2] * data[0], -b[3] * data[0], 0};
}
constexpr Motor PseudoScalar::operator*(const Motor &b) const {
  return {0, -b[4] * data[0], -b[5] * data[0], -b[6] * data[0],
          0, 0,               0,               b[0] * data[0]};
}
constexpr GANull PseudoScalar::operator*(const PseudoScalar &) const {
  return GANull{};
}

///////////////////////////////////////////////////////////////////////////////////
/// Inner Product
///
/// Fat dot product: for blades of grade r and s it is the grade |r - s| part
/// of the geometric product. Unlike the Hestenes inner product, scalars are
/// included, so s | X = s * X.
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

// Pseudoscalar: with the fat dot, contracting anything with e0123 gives exactly
// the same result as the geometric product.
constexpr MultiVector MultiVector::operator|(const PseudoScalar &b) const {
  return *this * b;
}
constexpr TriVector Vector::operator|(const PseudoScalar &b) const {
  return *this * b;
}
constexpr BiVector BiVector::operator|(const PseudoScalar &b) const {
  return *this * b;
}
constexpr Vector TriVector::operator|(const PseudoScalar &b) const {
  return *this * b;
}
constexpr Motor Motor::operator|(const PseudoScalar &b) const {
  return *this * b;
}
constexpr MultiVector PseudoScalar::operator|(const MultiVector &b) const {
  return *this * b;
}
constexpr Vector PseudoScalar::operator|(const TriVector &b) const {
  return *this * b;
}
constexpr BiVector PseudoScalar::operator|(const BiVector &b) const {
  return *this * b;
}
constexpr TriVector PseudoScalar::operator|(const Vector &b) const {
  return *this * b;
}
constexpr Motor PseudoScalar::operator|(const Motor &b) const {
  return *this * b;
}
constexpr GANull PseudoScalar::operator|(const PseudoScalar &) const {
  return GANull{};
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
constexpr PseudoScalar TriVector::operator^(const Vector &b) const {
  return PseudoScalar{-b[0] * data[3] - b[1] * data[0] - b[2] * data[1] -
                      b[3] * data[2]};
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
constexpr PseudoScalar BiVector::operator^(const BiVector &b) const {
  return PseudoScalar{b[3] * data[0] + b[4] * data[1] + b[5] * data[2] +
                      b[2] * data[5] + b[1] * data[4] + b[0] * data[3]};
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
constexpr PseudoScalar Vector::operator^(const TriVector &b) const {
  return PseudoScalar{b[0] * data[1] + b[1] * data[2] + b[2] * data[3] +
                      b[3] * data[0]};
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

// Pseudoscalar: e0123 already has the maximum grade, so only the scalar part
// of the other operand survives the wedge.
constexpr MultiVector MultiVector::operator^(const PseudoScalar &b) const {
  MultiVector res{};
  res[15] = data[0] * b[0];
  return res;
}
constexpr GANull Vector::operator^(const PseudoScalar &) const {
  return GANull{};
}
constexpr GANull BiVector::operator^(const PseudoScalar &) const {
  return GANull{};
}
constexpr GANull TriVector::operator^(const PseudoScalar &) const {
  return GANull{};
}
constexpr PseudoScalar Motor::operator^(const PseudoScalar &b) const {
  return PseudoScalar{data[0] * b[0]};
}
constexpr MultiVector PseudoScalar::operator^(const MultiVector &b) const {
  MultiVector res{};
  res[15] = data[0] * b[0];
  return res;
}
constexpr GANull PseudoScalar::operator^(const TriVector &) const {
  return GANull{};
}
constexpr GANull PseudoScalar::operator^(const BiVector &) const {
  return GANull{};
}
constexpr GANull PseudoScalar::operator^(const Vector &) const {
  return GANull{};
}
constexpr PseudoScalar PseudoScalar::operator^(const Motor &b) const {
  return PseudoScalar{data[0] * b[0]};
}
constexpr GANull PseudoScalar::operator^(const PseudoScalar &) const {
  return GANull{};
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

// Pseudoscalar: e0123 is the identity of the regressive product, so joining
// with p*e0123 just scales the other operand by p.
constexpr MultiVector MultiVector::operator&(const PseudoScalar &b) const {
  return *this * b[0];
}
constexpr Vector Vector::operator&(const PseudoScalar &b) const {
  return *this * b[0];
}
constexpr BiVector BiVector::operator&(const PseudoScalar &b) const {
  return *this * b[0];
}
constexpr TriVector TriVector::operator&(const PseudoScalar &b) const {
  return *this * b[0];
}
constexpr Motor Motor::operator&(const PseudoScalar &b) const {
  return *this * b[0];
}
constexpr MultiVector PseudoScalar::operator&(const MultiVector &b) const {
  return b * data[0];
}
constexpr TriVector PseudoScalar::operator&(const TriVector &b) const {
  return b * data[0];
}
constexpr BiVector PseudoScalar::operator&(const BiVector &b) const {
  return b * data[0];
}
constexpr Vector PseudoScalar::operator&(const Vector &b) const {
  return b * data[0];
}
constexpr Motor PseudoScalar::operator&(const Motor &b) const {
  return b * data[0];
}
constexpr PseudoScalar PseudoScalar::operator&(const PseudoScalar &b) const {
  return PseudoScalar{data[0] * b[0]};
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

// v^2 = |v|^2 (e0 squares to 0), so exp(v) = cosh|v| + v * sinh|v| / |v|.
// For |v| = 0 (v = a*e0) the limit sinh(x)/x -> 1 gives exp(v) = 1 + v.
constexpr MultiVector Vector::Gexp() const {
  const float vectorNorm{this->Norm()};
  const float factor{vectorNorm != 0
                         ? detail::ce_sinh(vectorNorm) / vectorNorm
                         : 1.0f};
  MultiVector res{};
  res[0] = detail::ce_cosh(vectorNorm);
  res[1] = data[0] * factor;
  res[2] = data[1] * factor;
  res[3] = data[2] * factor;
  res[4] = data[3] * factor;
  return res;
}

// t^2 = -e123^2, so exp(t) = cos|t| + t * sin|t| / |t|. For an ideal point
// (e123 = 0) t^2 = 0 and the limit sin(x)/x -> 1 gives exp(t) = 1 + t.
constexpr MultiVector TriVector::Gexp() const {
  const float trivectorNorm{this->Norm()};
  const float factor{trivectorNorm != 0
                         ? detail::ce_sin(trivectorNorm) / trivectorNorm
                         : 1.0f};
  MultiVector res{};
  res[0] = detail::ce_cos(trivectorNorm);
  res[11] = data[0] * factor;
  res[12] = data[1] * factor;
  res[13] = data[2] * factor;
  res[14] = data[3] * factor;
  return res;
}

// Split B into its Euclidean part E (e23, e31, e12) and ideal part (e01, e02,
// e03). Then B^2 = -l + 2*m*e0123 with l = |E|^2 and
// m = B23*B01 + B31*B02 + B12*B03, and
//   exp(B) = c + s*B + t*(e0123 * B) + m*s*e0123
// with a = sqrt(l), c = cos(a), s = sin(a)/a, t = m/l * (s - c).
// For l = 0 (a line at infinity) B^2 = 0 and exp(B) = 1 + B, a translator.
constexpr Motor BiVector::Gexp() const {
  const float l{data[3] * data[3] + data[4] * data[4] + data[5] * data[5]};
  if (l == 0) {
    return {1, data[0], data[1], data[2], 0, 0, 0, 0};
  }
  const float m{data[3] * data[0] + data[4] * data[1] + data[5] * data[2]};
  const float a{detail::ce_sqrt(l)};
  const float c{detail::ce_cos(a)};
  const float s{detail::ce_sin(a) / a};
  const float t{m / l * (s - c)};
  // e0123 * B = -(B23*e01 + B31*e02 + B12*e03)
  return {c,
          s * data[0] - t * data[3],
          s * data[1] - t * data[4],
          s * data[2] - t * data[5],
          s * data[3],
          s * data[4],
          s * data[5],
          m * s};
}

// The scalar, bivector and pseudoscalar parts of a motor all commute, so
//   exp(s + B + p*e0123) = e^s * exp(B) * (1 + p*e0123).
constexpr Motor Motor::Gexp() const {
  const Motor bivectorExp{Grade2().Gexp()};
  const PseudoScalar pseudo{Grade4()};
  return (bivectorExp + bivectorExp * pseudo) * detail::ce_exp(data[0]);
}

// Scaling and squaring: exp(X) = exp(X / 2^k)^(2^k), with k chosen so that
// X / 2^k is small enough for a short power series. The sum of absolute
// coefficients bounds the size of every power, since each product of basis
// blades is plus or minus a single basis blade (or zero).
constexpr MultiVector MultiVector::Gexp() const {
  float size{};
  for (const float coefficient : data) {
    size += detail::ce_abs(coefficient);
  }
  int squarings{};
  float scale{1.0f};
  while (size * scale > 0.25f && squarings < 64) {
    scale *= 0.5f;
    ++squarings;
  }

  const MultiVector x{*this * scale};
  MultiVector result{1.0f};
  MultiVector term{1.0f};
  for (int n = 1; n <= 12; ++n) {
    term = (term * x) / static_cast<float>(n);
    result += term;
  }
  for (int i = 0; i < squarings; ++i) {
    result = result * result;
  }
  return result;
}

// e0123 squares to zero, so the series terminates: exp(p*I) = 1 + p*I.
constexpr Motor PseudoScalar::Gexp() const {
  return {1, 0, 0, 0, 0, 0, 0, data[0]};
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
  requires(!std::is_arithmetic_v<T>) && requires(const T &value) { !value; }
[[nodiscard]] constexpr auto HodgeDual(const T &value) {
  return !value;
}
// The dual of a scalar is the pseudoscalar (the inverse of PseudoScalar's !).
[[nodiscard]] constexpr PseudoScalar HodgeDual(float scalar) {
  return PseudoScalar{scalar};
}
} // namespace GA

} // namespace flyfish

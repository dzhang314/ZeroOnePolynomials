import Mathlib.Algebra.Polynomial.Div
import Mathlib.Data.Real.Basic

open scoped Polynomial
open Polynomial (coeff Monic natTrailingDegree X)

namespace ZeroOnePolynomials

def IsZeroOnePolynomial (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n = 0) ∨ (coeff P n = 1)

def HasNonnegativeCoefficients (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n ≥ 0)

def IsRestrictedPolynomial (P : ℝ[X]) : Prop :=
  (Monic P) ∧ (HasNonnegativeCoefficients P) ∧ (coeff P 0 = 1)

def ZeroOnePolynomialConjecture (P Q : ℝ[X]) : Prop :=
  (Monic P) →
  (Monic Q) →
  (HasNonnegativeCoefficients P) →
  (HasNonnegativeCoefficients Q) →
  (IsZeroOnePolynomial (P * Q)) →
  ((IsZeroOnePolynomial P) ∧ (IsZeroOnePolynomial Q))

def RestrictedConjecture (P Q : ℝ[X]) : Prop :=
  (IsRestrictedPolynomial P) →
  (IsRestrictedPolynomial Q) →
  (IsZeroOnePolynomial (P * Q)) →
  ((IsZeroOnePolynomial P) ∧ (IsZeroOnePolynomial Q))

theorem trailing_degree_divides (P : ℝ[X]) : X^(natTrailingDegree P) ∣ P := by
  rw [Polynomial.X_pow_dvd_iff]
  intro d Hd
  exact Polynomial.coeff_eq_zero_of_lt_natTrailingDegree Hd

section

variable {P : ℝ[X]} {k : ℕ}

theorem mul_pow_monic (H : Monic (X^k * P)) : Monic P := by
  simpa! [Monic] using H

theorem mul_pow_nonnegative (H : HasNonnegativeCoefficients (X^k * P)) :
    HasNonnegativeCoefficients P := by
  intro n
  simpa using H (n + k)

theorem mul_pow_zero_one (H : IsZeroOnePolynomial (X^k * P)) :
    IsZeroOnePolynomial P := by
  intro n
  simpa using H (n + k)

theorem zero_one_mul_pow (H : IsZeroOnePolynomial P) :
    IsZeroOnePolynomial (X^k * P) := by
  intro n
  rw [Polynomial.coeff_X_pow_mul']
  split
  · exact H (n - k)
  · left; rfl

end

end ZeroOnePolynomials

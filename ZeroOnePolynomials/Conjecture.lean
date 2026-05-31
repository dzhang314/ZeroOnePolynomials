import Mathlib.Algebra.Polynomial.Degree.Defs
import Mathlib.Data.Real.Basic

open scoped Polynomial
open Polynomial (coeff Monic X)

namespace ZeroOnePolynomials

def IsZeroOnePolynomial (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n = 0) ∨ (coeff P n = 1)

def HasNonnegativeCoefficients (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, coeff P n ≥ 0

def IsRestrictedPolynomial (P : ℝ[X]) : Prop :=
  (Monic P) ∧ (HasNonnegativeCoefficients P) ∧ (coeff P 0 = 1)

def ZeroOnePolynomialConjecture (P Q : ℝ[X]) : Prop :=
  (Monic P) →
  (Monic Q) →
  (HasNonnegativeCoefficients P) →
  (HasNonnegativeCoefficients Q) →
  (IsZeroOnePolynomial (P * Q)) →
  (IsZeroOnePolynomial P ∧ IsZeroOnePolynomial Q)

def RestrictedConjecture (P Q : ℝ[X]) : Prop :=
  (IsRestrictedPolynomial P) →
  (IsRestrictedPolynomial Q) →
  (IsZeroOnePolynomial (P * Q)) →
  (IsZeroOnePolynomial P ∧ IsZeroOnePolynomial Q)

end ZeroOnePolynomials

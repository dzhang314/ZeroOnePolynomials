import Mathlib.Algebra.Polynomial.Degree.Defs
import Mathlib.Data.Real.Basic

namespace ZeroOnePolynomials

def IsZeroOnePolynomial (P : Polynomial ℝ) : Prop :=
  ∀ n : ℕ, (P.coeff n = 0) ∨ (P.coeff n = 1)

def HasNonnegativeCoefficients (P : Polynomial ℝ) : Prop :=
  ∀ n : ℕ, P.coeff n ≥ 0

def ZeroOnePolynomialConjecture (P Q : Polynomial ℝ) : Prop :=
  P.Monic →
  Q.Monic →
  HasNonnegativeCoefficients P →
  HasNonnegativeCoefficients Q →
  IsZeroOnePolynomial (P * Q) →
  IsZeroOnePolynomial P ∧ IsZeroOnePolynomial Q

end ZeroOnePolynomials

import Mathlib.Algebra.Polynomial.Div
import Mathlib.Data.Real.Basic

/-!
# 0-1 Polynomial Conjecture

In this file, we formally state the 0-1 Polynomial Conjecture and prove some
basic observations. Namely, we show that it suffices to consider polynomials
with constant term $`1`.
-/

open scoped Polynomial
open Polynomial (coeff Monic natTrailingDegree X)

namespace ZeroOnePolynomials

/-- **Definition:** $`P ∈ ℝ[X]` is a **0-1 polynomial** if all of its
    coefficients are either $`0` or $`1`. -/
def IsZeroOnePolynomial (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n = 0) ∨ (coeff P n = 1)

def HasNonnegativeCoefficients (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n ≥ 0)

/-- **0-1 Polynomial Conjecture:** Let $`P, Q ∈ ℝ[X]` be monic polynomials
    with nonnegative coefficients. If their product $`R(X) := P(X) Q(X)` is
    a 0-1 polynomial, then $`P` and $`Q` are also 0-1 polynomials. -/
def ZeroOnePolynomialConjecture (P Q : ℝ[X]) : Prop :=
  (Monic P) →
  (Monic Q) →
  (HasNonnegativeCoefficients P) →
  (HasNonnegativeCoefficients Q) →
  (IsZeroOnePolynomial (P * Q)) →
  ((IsZeroOnePolynomial P) ∧ (IsZeroOnePolynomial Q))

/-!
This conjecture, first formulated by [Emmanuel Amiot in 2019][1] and
[posted to MathOverflow][2] by [user Sil][3] a week later, is a notoriously
difficult open problem. The MathOverflow posting has received more than a dozen
solution attempts, [all of which have been retracted][4].

[1]: https://math.stackexchange.com/questions/3325163/the-coefficients-of-a-product-of-monic-polynomials-are-0-and-1-if-the-polyn
[2]: https://mathoverflow.net/questions/339137/why-do-polynomials-with-coefficients-0-1-like-to-have-only-factors-with-0-1
[3]: https://mathoverflow.net/users/136794/sil
[4]: https://mathoverflow.net/questions/339137/why-do-polynomials-with-coefficients-0-1-like-to-have-only-factors-with-0-1#comment1296767_339137
-/

/-- To prove the 0-1 Polynomial Conjecture, it suffices to consider monic
    polynomials with nonnegative coefficients and constant term $`1`. We call
    these **restricted polynomials** and formulate a restricted conjecture over
    this domain. We will later show that the restricted conjecture implies the
    full conjecture. -/
def IsRestrictedPolynomial (P : ℝ[X]) : Prop :=
  (Monic P) ∧ (HasNonnegativeCoefficients P) ∧ (coeff P 0 = 1)

def RestrictedConjecture (P Q : ℝ[X]) : Prop :=
  (IsRestrictedPolynomial P) →
  (IsRestrictedPolynomial Q) →
  (IsZeroOnePolynomial (P * Q)) →
  ((IsZeroOnePolynomial P) ∧ (IsZeroOnePolynomial Q))

/-- Mathlib defines the _trailing degree_ of a polynomial to be the index of
    its lowest-degree nonzero coefficient. This is precisely the largest
    natural number $`k ∈ ℕ` such that $`X^k` divides $`P(X)`. -/
theorem trailing_degree_divides (P : ℝ[X]) : X^(natTrailingDegree P) ∣ P := by
  rw [Polynomial.X_pow_dvd_iff]
  intro d Hd
  exact Polynomial.coeff_eq_zero_of_lt_natTrailingDegree Hd

/-!
To show that the restricted conjecture implies the full conjecture, we will
repeatedly divide a polynomial $`P ∈ ℝ[X]` by $`X` until its constant term is
nonzero. To make this argument formal, we must prove that the properties of
being monic, having nonnegative coefficients, and being a 0-1 polynomial are
preserved by this process.
-/

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

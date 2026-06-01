import Mathlib.Algebra.Polynomial.Div
import Mathlib.Data.Real.Basic
set_option pp.fieldNotation false

/-!
# 0-1 Polynomial Conjecture

In this file, we formally state the 0-1 Polynomial Conjecture and prove some
basic observations. Namely, we show that it suffices to consider polynomials
with constant term $`1`.
-/

open scoped Polynomial
open Polynomial (coeff Monic natDegree natTrailingDegree X)

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

theorem constant_term_upper_bound {P Q : ℝ[X]}
    (HPM : Monic P)
    (HPN : HasNonnegativeCoefficients P)
    (HQN : HasNonnegativeCoefficients Q)
    (HPQ01 : IsZeroOnePolynomial (P * Q)) :
    (coeff Q 0 ≤ 1) := by
  have C1 : coeff (P * Q) (natDegree P) ≤ 1 := by
    cases HPQ01 (natDegree P) with
    | inl h => rw [h]; exact zero_le_one
    | inr h => rw [h]
  have C2 : coeff Q 0 ≤ coeff (P * Q) (natDegree P) := by
    rw [Polynomial.coeff_mul]
    have C3 : (P.natDegree, 0) ∈ Finset.antidiagonal P.natDegree := by simp
    have C4 : ∀ x ∈ Finset.antidiagonal P.natDegree,
        0 ≤ (coeff P x.1) * (coeff Q x.2) :=
      fun x _ => mul_nonneg (HPN x.1) (HQN x.2)
    simpa [HPM] using Finset.single_le_sum C4 C3
  exact le_trans C2 C1

theorem constant_term_is_one {P Q : ℝ[X]}
    (HPM : Monic P)
    (HQM : Monic Q)
    (HPN : HasNonnegativeCoefficients P)
    (HQN : HasNonnegativeCoefficients Q)
    (HPQ01 : IsZeroOnePolynomial (P * Q))
    (HP0 : coeff P 0 > 0) (HQ0 : coeff Q 0 > 0) :
    ((coeff P 0 = 1) ∧ (coeff Q 0 = 1)) := by
  have : coeff P 0 ≤ 1 :=
    constant_term_upper_bound HQM HQN HPN (by rwa [mul_comm])
  have : coeff Q 0 ≤ 1 :=
    constant_term_upper_bound HPM HPN HQN HPQ01
  have CPQ : ((coeff P 0) * (coeff Q 0) = 0) ∨
             ((coeff P 0) * (coeff Q 0) = 1) := by
    simpa [Polynomial.mul_coeff_zero] using HPQ01 0
  cases CPQ
  · linarith [mul_pos HP0 HQ0]
  · constructor <;> nlinarith

theorem trailing_coeff_positive {P : ℝ[X]}
    (HPN : HasNonnegativeCoefficients P)
    (HP : P ≠ 0) :
    coeff P (natTrailingDegree P) > 0 :=
  lt_of_le_of_ne (HPN _) (Ne.symm (mt Polynomial.trailingCoeff_eq_zero.mp HP))

theorem restricted_implies_full :
    (∀ P Q : ℝ[X], RestrictedConjecture P Q) ->
    (∀ P Q : ℝ[X], ZeroOnePolynomialConjecture P Q) := by
  intro HRC P Q HPM HQM HPN HQN HPQ01
  let tdP := P.natTrailingDegree
  let tdQ := Q.natTrailingDegree
  have ⟨P', CP⟩ := trailing_degree_divides P
  have ⟨Q', CQ⟩ := trailing_degree_divides Q
  have CPM' : Monic P' := mul_pow_monic (CP ▸ HPM)
  have CQM' : Monic Q' := mul_pow_monic (CQ ▸ HQM)
  have CPN' : HasNonnegativeCoefficients P' := mul_pow_nonnegative (CP ▸ HPN)
  have CQN' : HasNonnegativeCoefficients Q' := mul_pow_nonnegative (CQ ▸ HQN)
  have CPQ : P * Q = X^(tdP + tdQ) * (P' * Q') := by rw [CP, CQ]; ring
  have CPQ01' : IsZeroOnePolynomial (P' * Q') := mul_pow_zero_one (CPQ ▸ HPQ01)
  have CP0' : coeff P' 0 > 0 := by
    have : coeff P tdP = coeff P' 0 := by
      rw [CP, ← zero_add tdP, Polynomial.coeff_X_pow_mul]
    exact this ▸ trailing_coeff_positive HPN HPM.ne_zero
  have CQ0' : coeff Q' 0 > 0 := by
    have : coeff Q tdQ = coeff Q' 0 := by
      rw [CQ, ← zero_add tdQ, Polynomial.coeff_X_pow_mul]
    exact this ▸ trailing_coeff_positive HQN HQM.ne_zero
  have C1 : (coeff P' 0 = 1) ∧ (coeff Q' 0 = 1) :=
    constant_term_is_one CPM' CQM' CPN' CQN' CPQ01' CP0' CQ0'
  have C2 := HRC P' Q' ⟨CPM', CPN', C1.1⟩ ⟨CQM', CQN', C1.2⟩ CPQ01'
  rw [CP, CQ]
  exact ⟨zero_one_mul_pow C2.1, zero_one_mul_pow C2.2⟩

end ZeroOnePolynomials

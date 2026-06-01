import Mathlib.Algebra.Polynomial.Div
import Mathlib.Data.Real.Basic

namespace ZeroOnePolynomials
open scoped Polynomial
open Polynomial (coeff Monic natDegree natTrailingDegree X)
set_option pp.fieldNotation false

/-!
# 0-1 Polynomial Conjecture

In this file, we formally state the 0-1 Polynomial Conjecture and prove some
basic observations. Namely, we show that it suffices to consider polynomials
with constant term $`1`.
-/

/-- **Definition:** A **0-1 polynomial** is a polynomial
    whose coefficients are all either $`0` or $`1`. -/
def IsZeroOnePolynomial (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n = 0) ∨ (coeff P n = 1)

def HasNonnegativeCoefficients (P : ℝ[X]) : Prop :=
  ∀ n : ℕ, (coeff P n ≥ 0)

/-- **0-1 Polynomial Conjecture:** Let $`P, Q ∈ ℝ[X]` be monic polynomials
    with nonnegative coefficients. If their product $`P(X) Q(X)` is a 0-1
    polynomial, then $`P` and $`Q` are also 0-1 polynomials. -/
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
difficult open problem. The MathOverflow posting has received more than a
dozen solution attempts, [all of which have been retracted][4].

[1]: https://math.stackexchange.com/questions/3325163/the-coefficients-of-a-product-of-monic-polynomials-are-0-and-1-if-the-polyn
[2]: https://mathoverflow.net/questions/339137/why-do-polynomials-with-coefficients-0-1-like-to-have-only-factors-with-0-1
[3]: https://mathoverflow.net/users/136794/sil
[4]: https://mathoverflow.net/questions/339137/why-do-polynomials-with-coefficients-0-1-like-to-have-only-factors-with-0-1#comment1296767_339137
-/

/-!
## Reduced Conjecture

The goal of this file is to show that, for the purpose of proving the 0-1
Polynomial Conjecture, we can assume that the polynomials $`P, Q ∈ ℝ[X]`
have constant term $`1` without loss of generality. To formalize this
argument, we state a _reduced conjecture_ pertaining to such polynomials
and prove that the reduced conjecture implies the full conjecture.
-/

/-- **Definition:** A **reduced polynomial** is a monic polynomial
    with nonnegative coefficients and constant term $`1`. -/
def IsReducedPolynomial (P : ℝ[X]) : Prop :=
  (Monic P) ∧ (HasNonnegativeCoefficients P) ∧ (coeff P 0 = 1)

/-- **Reduced Conjecture:** Let $`P, Q ∈ ℝ[X]` be reduced polynomials. If
    their product $`P(X) Q(X)` is a 0-1 polynomial, then $`P` and $`Q` are
    also 0-1 polynomials. -/
def ReducedConjecture (P Q : ℝ[X]) : Prop :=
  (IsReducedPolynomial P) →
  (IsReducedPolynomial Q) →
  (IsZeroOnePolynomial (P * Q)) →
  ((IsZeroOnePolynomial P) ∧ (IsZeroOnePolynomial Q))

/-!
## Preliminaries

To show that the reduced conjecture implies the full conjecture, we first need
to prove some elementary facts about polynomials that Mathlib does not provide.
-/

/-- Mathlib defines the _trailing degree_ of a polynomial $`P ∈ ℝ[X]` to be
    the index of its lowest-degree nonzero coefficient. This is precisely the
    largest natural number $`k ∈ ℕ` such that $`X^k` divides $`P(X)`. -/
theorem trailing_degree_divides (P : ℝ[X]) : X^(natTrailingDegree P) ∣ P :=
  Polynomial.X_pow_dvd_iff.mpr
    (fun _ H => Polynomial.coeff_eq_zero_of_lt_natTrailingDegree H)

/-- The trailing coefficient of a nonzero polynomial
    with nonnegative coefficients is strictly positive. -/
theorem trailing_coeff_positive {P : ℝ[X]}
    (HP : P ≠ 0)
    (HPN : HasNonnegativeCoefficients P) :
    coeff P (natTrailingDegree P) > 0 :=
  lt_of_le_of_ne (HPN (natTrailingDegree P))
    (Ne.symm (mt Polynomial.trailingCoeff_eq_zero.mp HP))

/-!
## Property Preservation

Our reduction strategy involves repeatedly dividing a polynomial $`P ∈ ℝ[X]`
by $`X` until its constant term is nonzero. We therefore must prove that
this procedure preserves the properties of being monic, having nonnegative
coefficients, and being a 0-1 polynomial.

In the following four statements, let $`P ∈ ℝ[X]` and $`k ∈ ℕ` be arbitrary.
-/

section
variable {P : ℝ[X]} {k : ℕ}

/-- If $`X^k P(X)` is monic, then so is $`P`. -/
theorem mul_pow_monic (H : Monic (X^k * P)) : Monic P := by
  simpa! [Monic] using H

/-- If $`X^k P(X)` has nonnegative coefficients, then so does $`P`. -/
theorem mul_pow_nonnegative (H : HasNonnegativeCoefficients (X^k * P)) :
    HasNonnegativeCoefficients P :=
  fun n => Polynomial.coeff_X_pow_mul P k n ▸ H (n + k)

/-- If $`X^k P(X)` is a 0-1 polynomial, then so is $`P`. -/
theorem mul_pow_zero_one (H : IsZeroOnePolynomial (X^k * P)) :
    IsZeroOnePolynomial P :=
  fun n => Polynomial.coeff_X_pow_mul P k n ▸ H (n + k)

/-- If $`P` is a 0-1 polynomial, then so is $`X^k P(X)`. -/
theorem zero_one_mul_pow (H : IsZeroOnePolynomial P) :
    IsZeroOnePolynomial (X^k * P) := by
  intro n
  rw [Polynomial.coeff_X_pow_mul']
  split
  · exact H (n - k)
  · left; rfl

end

/-!
## Bounding the Constant Term

We are now prepared to show that any polynomials $`P, Q ∈ ℝ[X]` satisfying the
hypotheses of the 0-1 Polynomial Conjecture must have trailing coefficient
$`1`. The first step is to show that the constant term is _at most_ $`1`.

To see this, write $`P(X) = X^m + P_{m-1} X^{m-1} + ⋯ + P_1 X + P_0` and
$`Q(X) = X^n + Q_{n-1} X^{n-1} + ⋯ + Q_1 X + Q_0`. If $`P(X) Q(X)` is a 0-1
polynomial, then in particular, the coefficient of $`X^m` in $`P(X) Q(X)` is
$`0` or $`1`. Because $`P` is monic, this coefficient is
$`Q_0 + P_{m-1} Q_1 + P_{m-2} Q_2 + ⋯ + P_1 Q_{m-1}`.
Every term in this sum is nonnegative, so we must have $`Q_0 ≤ 1`.

In the following proof, the Mathlib theorem {name}`Polynomial.coeff_mul`
expresses the coefficient of $`X^m` in $`P(X) Q(X)` as a sum over the index set
{lit}`Finset.antidiagonal (natDegree P)`. We show that every term in this sum
is nonnegative (claim {lit}`C2`) and that $`Q_0`, which corresponds to the
ordered pair {lit}`(natDegree P, 0)`, is a term in this sum (claim {lit}`C3`).

The Mathlib theorem {name}`Finset.single_le_sum`, which takes {lit}`C2` and
{lit}`C3` as hypotheses, now proves that $`Q_0` is bounded above by the
coefficient of $`X^m` in $`P(X) Q(X)` (claim {lit}`C1`). By hypothesis
{lit}`HPQ01`, we know that this coefficient is $`0` or $`1`. In either case
({name}`Or.elim`), this coefficient is bounded above by $`1`, since $`0 ≤ 1`
({name}`zero_le_one`) and $`1 ≤ 1` ({name}`le_rfl`). Thus, by the transitivity
of the $`≤` relation ({name}`le_trans`), we conclude that $`Q_0 ≤ 1`.

Note that this argument does not use all hypotheses of the 0-1 Polynomial
Conjecture. In particular, $`Q` is not required to be monic. Thus, we have
proven the following statement:
-/

/-- Let $`P, Q ∈ ℝ[X]` have nonnegative coefficients. If $`P` is monic and
    $`P(X) Q(X)` is a 0-1 polynomial, then $`Q_0 ≤ 1`. -/
theorem constant_term_upper_bound {P Q : ℝ[X]}
    (HPM : Monic P)
    (HPN : HasNonnegativeCoefficients P)
    (HQN : HasNonnegativeCoefficients Q)
    (HPQ01 : IsZeroOnePolynomial (P * Q)) :
    (coeff Q 0 ≤ 1) := by
  have C1 : coeff Q 0 ≤ coeff (P * Q) (natDegree P) := by
    rw [Polynomial.coeff_mul]
    let slice := Finset.antidiagonal (natDegree P)
    have C2 : ∀ x ∈ slice, 0 ≤ (coeff P x.1) * (coeff Q x.2) :=
      fun x _ => mul_nonneg (HPN x.1) (HQN x.2)
    have C3 : (natDegree P, 0) ∈ slice := Finset.mem_antidiagonal.mpr rfl
    simpa [HPM] using Finset.single_le_sum C2 C3
  exact le_trans C1 (Or.elim
    (HPQ01 (natDegree P)) (· ▸ zero_le_one) (· ▸ le_rfl))

/-!
Now, if $`P` and $`Q` are both monic, then we can apply the preceding argument
again, with the roles of $`P` and $`Q` reversed, to conclude $`P_0 ≤ 1` and
$`Q_0 ≤ 1`. If $`P_0` and $`Q_0` are both nonzero, then $`P_0 Q_0` cannot be
$`0` ({name}`mul_pos`). Hence, $`P_0 Q_0 = 1`, and it follows by elementary
algebra ({name (full := Mathlib.Tactic.nlinarith)}`nlinarith`) that the only
solution of $`P_0 Q_0 = 1` satisfying $`P_0 ≤ 1` and $`Q_0 ≤ 1` is
$`P_0 = Q_0 = 1`. Thus, we have proven the following:
-/

/-- If $`P, Q ∈ ℝ[X]` satisfy the hypotheses of the 0-1 Polynomial Conjecture
    and their constant terms are nonzero, then both constant terms are $`1`. -/
theorem constant_term_is_one {P Q : ℝ[X]}
    (HPM : Monic P)
    (HQM : Monic Q)
    (HPN : HasNonnegativeCoefficients P)
    (HQN : HasNonnegativeCoefficients Q)
    (HPQ01 : IsZeroOnePolynomial (P * Q))
    (HP0 : coeff P 0 > 0) (HQ0 : coeff Q 0 > 0) :
    ((coeff P 0 = 1) ∧ (coeff Q 0 = 1)) := by
  have : coeff P 0 ≤ 1 :=
    constant_term_upper_bound HQM HQN HPN (mul_comm P Q ▸ HPQ01)
  have : coeff Q 0 ≤ 1 :=
    constant_term_upper_bound HPM HPN HQN HPQ01
  have : (coeff P 0) * (coeff Q 0) = 1 :=
    Or.resolve_left (Polynomial.mul_coeff_zero P Q ▸ HPQ01 0)
      (ne_of_gt (mul_pos HP0 HQ0))
  constructor <;> nlinarith

/-!
## Main Result

We have now assembled all of the necessary tools to prove that the reduced
conjecture implies the full conjecture. Given $`P, Q ∈ ℝ[X]` satisfying the
hypotheses of the 0-1 Polynomial Conjecture, we repeatedly divide $`P` and $`Q`
by $`X` until their constant terms are nonzero, obtaining $`P', Q' ∈ ℝ[X]` and
$`k_P, k_Q ∈ ℕ` satisfying $`P(X) = X^{k_P} P'(X)` and $`Q(X) = X^{k_Q} Q'(X)`.

We use {name}`mul_pow_monic`, {name}`mul_pow_nonnegative`, and
{name}`mul_pow_zero_one` to show that $`P'`and $`Q'` also satisfy the
hypotheses of the 0-1 Polynomial Conjecture. This implies, via
{name}`constant_term_is_one`, that $`P'` and $`Q'` are both reduced
polynomials. Hence, $`P'` and $`Q'` satisfy the hypotheses of the
reduced conjecture (hypothesis {lit}`HRC`), from which we conclude
that $`P'` and $`Q'` are 0-1 polynomials. Finally, we use
{name}`zero_one_mul_pow` to lift this fact to $`P` and $`Q`.

Thus, we have shown:
-/

/-- The reduced conjecture implies the full 0-1 Polynomial Conjecture. -/
theorem reduced_implies_full :
    (∀ P Q : ℝ[X], ReducedConjecture P Q) ->
    (∀ P Q : ℝ[X], ZeroOnePolynomialConjecture P Q) := by
  intro HRC P Q HPM HQM HPN HQN HPQ01
  let kP := natTrailingDegree P
  let kQ := natTrailingDegree Q
  have ⟨P', CP⟩ := trailing_degree_divides P
  have ⟨Q', CQ⟩ := trailing_degree_divides Q
  have CPM' : Monic P' := mul_pow_monic (CP ▸ HPM)
  have CQM' : Monic Q' := mul_pow_monic (CQ ▸ HQM)
  have CPN' : HasNonnegativeCoefficients P' := mul_pow_nonnegative (CP ▸ HPN)
  have CQN' : HasNonnegativeCoefficients Q' := mul_pow_nonnegative (CQ ▸ HQN)
  have CPQ : P * Q = X^(kP + kQ) * (P' * Q') := by rw [CP, CQ]; ring
  have CPQ01' : IsZeroOnePolynomial (P' * Q') := mul_pow_zero_one (CPQ ▸ HPQ01)
  have CP0' : coeff P' 0 > 0 :=
    have : coeff P kP = coeff P' 0 :=
      Eq.symm CP ▸ (zero_add kP) ▸ Polynomial.coeff_X_pow_mul P' kP 0
    this ▸ trailing_coeff_positive (Monic.ne_zero HPM) HPN
  have CQ0' : coeff Q' 0 > 0 :=
    have : coeff Q kQ = coeff Q' 0 :=
      Eq.symm CQ ▸ (zero_add kQ) ▸ Polynomial.coeff_X_pow_mul Q' kQ 0
    this ▸ trailing_coeff_positive (Monic.ne_zero HQM) HQN
  have C1 : (coeff P' 0 = 1) ∧ (coeff Q' 0 = 1) :=
    constant_term_is_one CPM' CQM' CPN' CQN' CPQ01' CP0' CQ0'
  have C2 := HRC P' Q' ⟨CPM', CPN', C1.1⟩ ⟨CQM', CQN', C1.2⟩ CPQ01'
  exact CP ▸ CQ ▸ ⟨zero_one_mul_pow C2.1, zero_one_mul_pow C2.2⟩

end ZeroOnePolynomials

# ZeroOnePolynomials

**Definition:** Let $R$ be a commutative unital ring. A polynomial $P \in R[x]$ is a ___0-1 polynomial___ if every coefficient of $P$ is either $0_R$ or $1_R$.

**[0-1 Polynomial Conjecture](https://mathoverflow.net/questions/339137/why-do-polynomials-with-coefficients-0-1-like-to-have-only-factors-with-0-1):** Let $P, Q \in \mathbb{R}[x]$ be monic polynomials with nonnegative coefficients. If their product $R(x) \coloneqq P(x) Q(x)$ is a 0-1 polynomial, then $P$ and $Q$ are 0-1 polynomials.

This repository contains high-performance computer programs that test the 0-1 Polynomial Conjecture for small values of $(\deg P, \deg Q)$. Using these programs, I have independently verified that the 0-1 Polynomial Conjecture holds whenever $\deg R \le 45$.

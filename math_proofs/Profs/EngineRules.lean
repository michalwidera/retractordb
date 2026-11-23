import Profs.CppArith
import Profs.ShiftMatching
import Profs.StartupBoundaries

namespace Profs

/- Funkcje z src/include/SOperations.hpp jako przepisania w arytmetyce sprawdzanej z CppArith.lean
   (#634, warstwa A, krok A2): funkcje pomocnicze i mapy odczytu.

   Kazda funkcja C++ ma tu przepisanie w monadzie Option: ta sama kolejnosc dzialan, te same typy,
   a przy kazdym kroku komentarz z odpowiadajacym fragmentem C++. Wynik none oznacza
   niezdefiniowane zachowanie w miejscu, w ktorym przepisanie go zwraca. Twierdzenie <nazwa>_eq
   mowi, ze przy hipotezach przepisanie daje some i wartosc rowna definicji modelu; jest wiec
   zarazem dowodem braku UB.

   Hipotezy twierdzen to assert() z kodu oraz zalozenia kompilatora (interwaly dodatnie
   i znormalizowane, indeks rekordu z [0, INT_MAX]). W buildzie Release asercji nie ma, wiec
   niczego nie gwarantuja: wymusza je kompilator planu, nie funkcja.

   Parametry modelu sa nieskroconymi iloczynami krzyzowymi (decyzja D6 z #634): dla interwalow
   r = r1/r2 i s = s1/s2 iloczyn cross r s = r1*s2. -/

namespace Cpp

/- std::numeric_limits<std::int64_t>::max() -/
def int64Max : ℤ := 2 ^ 63 - 1

/- ===== Lematy pomocnicze: zakres ===== -/

theorem inRange64_of_nonneg {x : ℤ} (h0 : 0 ≤ x) (h1 : x ≤ int64Max) : inRange wInt64 x := by
  unfold int64Max at h1
  constructor <;> norm_num [wInt64] <;> omega

theorem inRange128_of_nonneg {x : ℤ} (h0 : -1 ≤ x) (h1 : x ≤ 2 ^ 126) : inRange wInt128 x := by
  constructor <;> norm_num [wInt128] <;> omega

/- Skladowe dodatniego interwalu rational<int> leza w (0, 2^31). -/
theorem ratInt_pos_bounds {r : RatInt} (hr : r.Valid) (h : 0 < r.val) :
    0 < r.num ∧ r.num < 2 ^ 31 ∧ 0 < r.den ∧ r.den < 2 ^ 31 := by
  have hn := ratInt_num_pos hr h
  have h1 := hr.2.2.1.2
  have h2 := hr.2.2.2.2
  norm_num [wInt] at h1 h2
  exact ⟨hn, h1, hr.1, h2⟩

/- Iloraz obciety miesci sie w int64 poza przypadkiem INT64_MIN / -1. -/
theorem tdiv_inRange64 {a b : ℤ} (ha : inRange wInt64 a) (hb : b ≠ 0)
    (hmin : ¬ (a = -2 ^ 63 ∧ b = -1)) : inRange wInt64 (a.tdiv b) := by
  obtain ⟨h1, h2⟩ := ha
  norm_num [wInt64] at h1 h2
  have hq : (a.tdiv b).natAbs = a.natAbs / b.natAbs := Int.natAbs_tdiv a b
  rcases (show b = 1 ∨ b = -1 ∨ 2 ≤ b.natAbs by omega) with rfl | rfl | h2b
  · rw [Int.tdiv_one]; exact ⟨by norm_num [wInt64]; omega, by norm_num [wInt64]; omega⟩
  · rw [Int.tdiv_neg, Int.tdiv_one]; exact ⟨by norm_num [wInt64]; omega, by norm_num [wInt64]; omega⟩
  · have : a.natAbs / b.natAbs ≤ a.natAbs / 2 := Nat.div_le_div_left h2b (by norm_num)
    constructor <;> norm_num [wInt64] <;> omega

/- Przy niezerowej reszcie |b| >= 2, wiec |a.tdiv b| <= 2^62 i iloraz plus minus jeden nie
   przepelnia int64. -/
theorem tdiv_small_of_tmod_ne_zero {a b : ℤ} (ha : inRange wInt64 a) (hr : a.tmod b ≠ 0)
    (hb : b ≠ 0) : -(2 : ℤ) ^ 62 ≤ a.tdiv b ∧ a.tdiv b ≤ 2 ^ 62 := by
  obtain ⟨h1, h2⟩ := ha
  norm_num [wInt64] at h1 h2
  have hq : (a.tdiv b).natAbs = a.natAbs / b.natAbs := Int.natAbs_tdiv a b
  have hm : (a.tmod b).natAbs = a.natAbs % b.natAbs := Int.natAbs_tmod a b
  have h2b : 2 ≤ b.natAbs := by
    rcases (show b.natAbs = 1 ∨ 2 ≤ b.natAbs by omega) with h | h
    · rw [h, Nat.mod_one] at hm; omega
    · exact h
  have : a.natAbs / b.natAbs ≤ a.natAbs / 2 := Nat.div_le_div_left h2b (by norm_num)
  omega

/- Sufit ilorazu w Z przy dodatnim mianowniku: ceil(N/D) = (N + D - 1) / D. -/
theorem intCeil_div (N D : ℤ) (hD : 0 < D) : ⌈(N : ℚ) / D⌉ = (N + D - 1) / D := by
  have hDq : (0 : ℚ) < D := by exact_mod_cast hD
  have hdm := Int.mul_ediv_add_emod (N + D - 1) D
  have hr0 := Int.emod_nonneg (N + D - 1) hD.ne'
  have hr1 := Int.emod_lt_of_pos (N + D - 1) hD
  rw [Int.ceil_eq_iff]
  constructor
  · have h1 : ((N + D - 1) / D - 1) * D < N := by nlinarith
    have h2 : (((N + D - 1) / D - 1 : ℤ) : ℚ) * D < N := by exact_mod_cast h1
    have h3 : (((N + D - 1) / D - 1 : ℤ) : ℚ) < N / D := by rw [lt_div_iff₀ hDq]; exact h2
    push_cast at h3
    exact h3
  · rw [div_le_iff₀ hDq]
    exact_mod_cast (show N ≤ (N + D - 1) / D * D by nlinarith)

/- ===== floorDiv ===== -/

-- ANCHOR: floor_div_cpp
def floorDivCpp (a b : ℤ) : Option ℤ := do
  -- const std::int64_t quotient = numerator / denominator;
  let quotient ← tdivC wInt64 a b
  -- numerator % denominator
  let r ← tmodC wInt64 a b
  -- (r != 0 && ((numerator < 0) != (denominator < 0))) ? quotient - 1 : quotient
  if r ≠ 0 ∧ (decide (a < 0) != decide (b < 0)) = true then subC wInt64 quotient 1
  else some quotient
-- ANCHOR_END: floor_div_cpp

theorem floorDivCpp_eq {a b : ℤ} (ha : inRange wInt64 a) (hb : b ≠ 0)
    (hmin : ¬ (a = -2 ^ 63 ∧ b = -1)) : floorDivCpp a b = some (a.fdiv b) := by
  have hq := tdiv_inRange64 ha hb hmin
  unfold floorDivCpp
  rw [tdivC_of_inRange hb hq, tmodC_of_inRange hb hq]
  simp only [Option.bind_eq_bind, Option.bind_some]
  rw [Int.fdiv_eq_tdiv]
  by_cases hd : b ∣ a
  · have h0 : a.tmod b = 0 := Int.dvd_iff_tmod_eq_zero.mp hd
    simp [h0, hd]
  · have h0 : a.tmod b ≠ 0 := fun h => hd (Int.dvd_iff_tmod_eq_zero.mpr h)
    have hs := tdiv_small_of_tmod_ne_zero ha h0 hb
    have ha0 : a ≠ 0 := by rintro rfl; exact hd (dvd_zero b)
    have hsub : subC wInt64 (a.tdiv b) 1 = some (a.tdiv b - 1) :=
      subC_of_inRange (by constructor <;> norm_num [wInt64] <;> omega)
    rcases lt_or_gt_of_ne hb with hbn | hbp
    · rw [Int.sign_eq_neg_one_of_neg hbn]
      by_cases ha' : a < 0
      · simp [hd, h0, ha', hbn, show ¬ 0 ≤ a by omega, show ¬ 0 ≤ b by omega]
      · simp [hd, h0, ha', hbn, hsub, show 0 ≤ a by omega, show ¬ 0 ≤ b by omega]
    · rw [Int.sign_eq_one_of_pos hbp]
      by_cases ha' : a < 0
      · simp [hd, h0, ha', hsub, show ¬ b < 0 by omega, show ¬ 0 ≤ a by omega, show 0 ≤ b by omega]
      · simp [hd, h0, ha', show ¬ b < 0 by omega, show 0 ≤ a by omega, show 0 ≤ b by omega]

/- ===== roundedAwayFromZero ===== -/

/- Zaokraglenie od zera liczby wymiernej: sufit dla x >= 0, podloga dla x < 0. -/
def awayFromZero (x : ℚ) : ℤ := if 0 ≤ x then ⌈x⌉ else ⌊x⌋

-- ANCHOR: rounded_away_cpp
def roundedAwayFromZeroCpp (a b : ℤ) : Option ℤ := do
  -- const std::int64_t quotient = numerator / denominator;
  let quotient ← tdivC wInt64 a b
  -- if (numerator % denominator == 0) return quotient;
  let r ← tmodC wInt64 a b
  if r = 0 then some quotient
  -- return ((numerator < 0) == (denominator < 0)) ? quotient + 1 : quotient - 1;
  else if (decide (a < 0) == decide (b < 0)) = true then addC wInt64 quotient 1
  else subC wInt64 quotient 1
-- ANCHOR_END: rounded_away_cpp

/- a/b = q + r/b z q = a.tdiv b, r = a.tmod b, przy czym r/b lezy w (0, 1), gdy r != 0 i znaki
   a i b sa zgodne, a w (-1, 0), gdy sa rozne. -/
theorem awayFromZero_div {a b : ℤ} (hb : b ≠ 0) :
    awayFromZero ((a : ℚ) / b) =
      if a.tmod b = 0 then a.tdiv b
      else if (decide (a < 0) == decide (b < 0)) = true then a.tdiv b + 1 else a.tdiv b - 1 := by
  set q := a.tdiv b with hqdef
  set r := a.tmod b with hrdef
  have hdef : a = b * q + r := by rw [hrdef, Int.tmod_def]; ring
  have hm : r.natAbs = a.natAbs % b.natAbs := Int.natAbs_tmod a b
  have hmlt : a.natAbs % b.natAbs < b.natAbs := Nat.mod_lt _ (by omega)
  have hsgn1 : 0 ≤ a → 0 ≤ r := fun h => Int.tmod_nonneg b h
  have hsgn2 : a ≤ 0 → r ≤ 0 := by
    intro h
    have : 0 ≤ (-a).tmod b := Int.tmod_nonneg b (by omega)
    rw [Int.neg_tmod] at this
    omega
  have hbq : (b : ℚ) ≠ 0 := by exact_mod_cast hb
  have hx : (a : ℚ) / b = q + (r : ℚ) / b := by
    rw [hdef]; push_cast; field_simp
  unfold awayFromZero
  by_cases hr : r = 0
  · rw [if_pos hr, hx, hr]
    simp
  · rw [if_neg hr]
    have hrb : r.natAbs < b.natAbs := by omega
    rcases lt_or_gt_of_ne hb with hbn | hbp
    · have hbq' : (b : ℚ) < 0 := by exact_mod_cast hbn
      by_cases ha : a < 0
      · -- r < 0, b < r: r/b w (0, 1)
        have hr' : r < 0 := by have := hsgn2 ha.le; omega
        have hbr : b < r := by omega
        have h0 : (0 : ℚ) < (r : ℚ) / b := div_pos_of_neg_of_neg (by exact_mod_cast hr') hbq'
        have h1 : (r : ℚ) / b < 1 := by
          rw [div_lt_one_of_neg hbq']; exact_mod_cast hbr
        have hq0 : (0 : ℚ) ≤ q := by
          have := Int.tdiv_nonneg (show 0 ≤ -a by omega) (show (0 : ℤ) ≤ -b by omega)
          rw [Int.neg_tdiv, Int.tdiv_neg, neg_neg] at this
          exact_mod_cast this
        rw [if_pos (by rw [hx]; linarith)]
        simp only [ha, hbn, decide_true, beq_self_eq_true, if_true]
        rw [Int.ceil_eq_iff, hx]; push_cast; constructor <;> linarith
      · -- r > 0, r < -b: r/b w (-1, 0)
        have hr' : 0 < r := by have := hsgn1 (by omega); omega
        have hbr : r < -b := by omega
        have h0 : (r : ℚ) / b < 0 := div_neg_of_pos_of_neg (by exact_mod_cast hr') hbq'
        have h1 : -1 < (r : ℚ) / b := by
          rw [lt_div_iff_of_neg hbq']; have : (r : ℚ) < -b := by exact_mod_cast hbr
          linarith
        have hx0 : ¬ 0 ≤ (a : ℚ) / b := by
          rw [hx, not_le]
          have : (q : ℚ) ≤ 0 := by
            have hq0 : q ≤ 0 := by
              have := Int.tdiv_nonneg (show 0 ≤ a by omega) (show (0 : ℤ) ≤ -b by omega)
              rw [Int.tdiv_neg] at this; omega
            exact_mod_cast hq0
          linarith
        rw [if_neg hx0]
        simp only [ha, hbn, decide_false, decide_true, Bool.false_beq, Bool.not_true,
          Bool.false_eq_true, if_false]
        rw [Int.floor_eq_iff, hx]; push_cast; constructor <;> linarith
    · have hbq' : (0 : ℚ) < b := by exact_mod_cast hbp
      by_cases ha : a < 0
      · -- r < 0, -b < r: r/b w (-1, 0)
        have hr' : r < 0 := by have := hsgn2 ha.le; omega
        have hbr : -b < r := by omega
        have h0 : (r : ℚ) / b < 0 := div_neg_of_neg_of_pos (by exact_mod_cast hr') hbq'
        have h1 : -1 < (r : ℚ) / b := by
          rw [lt_div_iff₀ hbq']; have : (-b : ℚ) < r := by exact_mod_cast hbr
          linarith
        have hx0 : ¬ 0 ≤ (a : ℚ) / b := by
          rw [hx, not_le]
          have : (q : ℚ) ≤ 0 := by
            have hq0 : q ≤ 0 := by
              have := Int.tdiv_nonneg (show 0 ≤ -a by omega) hbp.le
              rw [Int.neg_tdiv] at this; omega
            exact_mod_cast hq0
          linarith
        rw [if_neg hx0]
        simp only [ha, show ¬ b < 0 by omega, decide_false, decide_true, Bool.true_beq,
          Bool.false_eq_true, if_false]
        rw [Int.floor_eq_iff, hx]; push_cast; constructor <;> linarith
      · -- r > 0, r < b: r/b w (0, 1)
        have hr' : 0 < r := by have := hsgn1 (by omega); omega
        have hbr : r < b := by omega
        have h0 : (0 : ℚ) < (r : ℚ) / b := div_pos (by exact_mod_cast hr') hbq'
        have h1 : (r : ℚ) / b < 1 := by
          rw [div_lt_one hbq']; exact_mod_cast hbr
        have hq0 : (0 : ℚ) ≤ q := by
          have := Int.tdiv_nonneg (show 0 ≤ a by omega) hbp.le
          exact_mod_cast this
        rw [if_pos (by rw [hx]; linarith)]
        simp only [ha, show ¬ b < 0 by omega, decide_false, beq_self_eq_true, if_true]
        rw [Int.ceil_eq_iff, hx]; push_cast; constructor <;> linarith

theorem roundedAwayFromZeroCpp_eq {a b : ℤ} (ha : inRange wInt64 a) (hb : b ≠ 0)
    (hmin : ¬ (a = -2 ^ 63 ∧ b = -1)) :
    roundedAwayFromZeroCpp a b = some (awayFromZero ((a : ℚ) / b)) := by
  have hq := tdiv_inRange64 ha hb hmin
  unfold roundedAwayFromZeroCpp
  rw [tdivC_of_inRange hb hq, tmodC_of_inRange hb hq, awayFromZero_div hb]
  simp only [Option.bind_eq_bind, Option.bind_some]
  by_cases h0 : a.tmod b = 0
  · simp [h0]
  · have hs := tdiv_small_of_tmod_ne_zero ha h0 hb
    rw [if_neg h0, if_neg h0]
    split_ifs
    · exact addC_of_inRange (by constructor <;> norm_num [wInt64] <;> omega)
    · exact subC_of_inRange (by constructor <;> norm_num [wInt64] <;> omega)

/- Dla nieujemnego licznika i dodatniego mianownika zaokraglenie od zera to sufit, w postaci
   natCeil_div ze StartupBoundaries.lean. -/
theorem roundedAwayFromZeroCpp_natCast (m n : ℕ) (hn : 0 < n) (hm : inRange wInt64 m) :
    roundedAwayFromZeroCpp m n = some (((m + n - 1) / n : ℕ) : ℤ) := by
  rw [roundedAwayFromZeroCpp_eq hm (by exact_mod_cast hn.ne') (by omega)]
  unfold awayFromZero
  have h0 : (0 : ℚ) ≤ ((m : ℤ) : ℚ) / ((n : ℤ) : ℚ) := by positivity
  rw [if_pos h0]
  congr 1
  rw [← natCeil_div m n hn, Int.natCast_ceil_eq_ceil (by positivity)]
  simp only [Int.cast_natCast]

/- ===== saturatedIndex ===== -/

-- ANCHOR: saturated_index_cpp
def saturatedIndexCpp (index : ℤ) : Option ℤ :=
  -- return index > limit ? limit : static_cast<std::int64_t>(index);
  if index > int64Max then some int64Max else castC wInt64 index
-- ANCHOR_END: saturated_index_cpp

theorem saturatedIndexCpp_eq {x : ℤ} (h0 : 0 ≤ x) : saturatedIndexCpp x = some (min x int64Max) := by
  unfold saturatedIndexCpp
  split_ifs with h
  · rw [min_eq_right h.le]
  · rw [min_eq_left (not_lt.mp h)]
    exact checked_of_inRange (inRange64_of_nonneg h0 (not_lt.mp h))

/- ===== saturatedTail ===== -/

-- ANCHOR: saturated_tail_cpp
def saturatedTailCpp (numerator denominator : ℤ) : Option ℤ := do
  -- numerator + denominator
  let s ← addC wInt128 numerator denominator
  -- ... - 1
  let s1 ← subC wInt128 s 1
  -- (...) / denominator
  let qt ← tdivC wInt128 s1 denominator
  -- const __int128 tail = (...) - 1;
  let tail ← subC wInt128 qt 1
  -- if (tail < 0) return 0;
  if tail < 0 then some 0
  -- return tail > limit ? limit : static_cast<std::int64_t>(tail);
  else if tail > int64Max then some int64Max
  else castC wInt64 tail
-- ANCHOR_END: saturated_tail_cpp

/- C++ liczy (numerator + denominator) - 1, wiec wyrazem posrednim jest N + D; jego zakres jest
   hipoteza, ktora A3 rozladowuje dla kazdej reguly. -/
theorem saturatedTailCpp_eq {N D : ℤ} (hN : 0 ≤ N) (hD : 0 < D) (hND : inRange wInt128 (N + D)) :
    saturatedTailCpp N D = some (min (max 0 (⌈(N : ℚ) / D⌉ - 1)) int64Max) := by
  obtain ⟨h1, h2⟩ := hND
  norm_num [wInt128] at h1 h2
  have hs1 : inRange wInt128 (N + D - 1) := by constructor <;> norm_num [wInt128] <;> omega
  have hq0 : 0 ≤ (N + D - 1) / D := Int.ediv_nonneg (by omega) hD.le
  have hq1 : (N + D - 1) / D ≤ N + D - 1 := Int.ediv_le_self _ (by omega)
  unfold saturatedTailCpp
  rw [addC_of_inRange ⟨by norm_num [wInt128]; omega, by norm_num [wInt128]; omega⟩]
  simp only [Option.bind_eq_bind, Option.bind_some]
  rw [subC_of_inRange hs1]
  simp only [Option.bind_some]
  rw [tdivC_of_nonneg (by omega) hD hs1]
  simp only [Option.bind_some]
  rw [subC_of_inRange (by constructor <;> norm_num [wInt128] <;> omega)]
  simp only [Option.bind_some]
  rw [intCeil_div N D hD]
  split_ifs with ht hl
  · rw [max_eq_left ht.le, min_eq_left (by unfold int64Max; norm_num)]
  · rw [max_eq_right (not_lt.mp ht), min_eq_right hl.le]
  · rw [max_eq_right (not_lt.mp ht), min_eq_left (not_lt.mp hl)]
    exact checked_of_inRange (inRange64_of_nonneg (not_lt.mp ht) (not_lt.mp hl))

/- ===== Mapy odczytu: parametry D6 ===== -/

/- Iloczyn krzyzowy r1*s2 interwalow r = r1/r2 i s = s1/s2 jako liczba naturalna. -/
def cross (r s : RatInt) : ℕ := (r.num * s.den).toNat

theorem cross_spec {r s : RatInt} (hr : r.Valid) (hr0 : 0 < r.val) (hs : s.Valid) :
    ((cross r s : ℕ) : ℤ) = r.num * s.den ∧ 0 < r.num * s.den := by
  obtain ⟨h1, -, -⟩ := crossProduct_bound (ratInt_num_pos hr hr0) hs.1 hr.2.2.1 hs.2.2.2
  exact ⟨Int.toNat_of_nonneg h1.le, h1⟩

theorem affineRead_cast (p q c n : ℕ) : ((affineRead p q c n : ℕ) : ℤ) = ((n : ℤ) * p + c) / q := by
  unfold affineRead; push_cast; rfl

theorem inRange128_of_le {x B : ℤ} (h0 : -1 ≤ x) (h1 : x ≤ B) (hB : B ≤ 2 ^ 126) :
    inRange wInt128 x :=
  inRange128_of_nonneg h0 (h1.trans hB)

theorem index_lt {i : ℤ} (hi : inRange wInt i) : i < 2 ^ 31 := by
  have := hi.2; norm_num [wInt] at this; exact this

/- ===== hashLeftCount ===== -/

-- ANCHOR: hash_left_count_cpp
def hashLeftCountCpp (a b : RatInt) (n : ℤ) : Option ℤ := do
  -- const std::int64_t right = std::int64_t{deltaB.numerator()} * deltaA.denominator();
  let right ← mulC wInt64 b.num a.den
  -- const std::int64_t left = std::int64_t{deltaA.numerator()} * deltaB.denominator();
  let left ← mulC wInt64 a.num b.den
  -- __int128{n} * right
  let t ← mulC wInt128 n right
  -- __int128{left} + right
  let s ← addC wInt128 left right
  -- (...) / (...)
  let qt ← tdivC wInt128 t s
  -- static_cast<std::int64_t>(...)
  castC wInt64 qt
-- ANCHOR_END: hash_left_count_cpp

theorem progress_le (a b n : ℕ) : progress a b n ≤ n := by
  unfold progress
  rcases Nat.eq_zero_or_pos (a + b) with h | h
  · rw [h, Nat.div_zero]; exact Nat.zero_le n
  · exact Nat.div_le_of_le_mul (by nlinarith)

/- L = a1*b2 i R = b1*a2: hashLeftCount to progress L R n, bez nasycenia, bo wynik <= n. -/
theorem hashLeftCountCpp_eq {a b : RatInt} (ha : a.Valid) (ha0 : 0 < a.val) (hb : b.Valid)
    (hb0 : 0 < b.val) {n : ℤ} (hn0 : 0 ≤ n) (hn : n ≤ 2 ^ 31) :
    hashLeftCountCpp a b n = some (progress (cross a b) (cross b a) n.toNat : ℤ) := by
  obtain ⟨a1p, a1l, a2p, a2l⟩ := ratInt_pos_bounds ha ha0
  obtain ⟨b1p, b1l, b2p, b2l⟩ := ratInt_pos_bounds hb hb0
  obtain ⟨hL, -⟩ := cross_spec ha ha0 hb
  obtain ⟨hR, -⟩ := cross_spec hb hb0 ha
  have pR : 0 < b.num * a.den := mul_pos b1p a2p
  have pL : 0 < a.num * b.den := mul_pos a1p b2p
  have lR : b.num * a.den ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul b1l.le a2l.le a2p.le (by norm_num)
  have lL : a.num * b.den ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul a1l.le b2l.le b2p.le (by norm_num)
  have lT : n * (b.num * a.den) ≤ 2 ^ 31 * (2 ^ 31 * 2 ^ 31) := mul_le_mul hn lR pR.le (by norm_num)
  have pT : 0 ≤ n * (b.num * a.den) := mul_nonneg hn0 pR.le
  have hq1 : n * (b.num * a.den) / (a.num * b.den + b.num * a.den) ≤ n := by
    calc n * (b.num * a.den) / (a.num * b.den + b.num * a.den)
        ≤ n * (a.num * b.den + b.num * a.den) / (a.num * b.den + b.num * a.den) :=
          Int.ediv_le_ediv (by linarith) (mul_le_mul_of_nonneg_left (by linarith) hn0)
      _ = n := Int.mul_ediv_cancel _ (by linarith)
  have hq0 : 0 ≤ n * (b.num * a.den) / (a.num * b.den + b.num * a.den) :=
    Int.ediv_nonneg pT (by linarith)
  have s1 : mulC wInt64 b.num a.den = some (b.num * a.den) :=
    mulC_of_inRange (inRange64_of_nonneg pR.le (by unfold int64Max; linarith))
  have s2 : mulC wInt64 a.num b.den = some (a.num * b.den) :=
    mulC_of_inRange (inRange64_of_nonneg pL.le (by unfold int64Max; linarith))
  have s3 : mulC wInt128 n (b.num * a.den) = some (n * (b.num * a.den)) :=
    mulC_of_inRange (inRange128_of_le (by linarith) lT (by norm_num))
  have s4 : addC wInt128 (a.num * b.den) (b.num * a.den) = some (a.num * b.den + b.num * a.den) :=
    addC_of_inRange (inRange128_of_le (by linarith) (add_le_add lL lR) (by norm_num))
  have s5 : tdivC wInt128 (n * (b.num * a.den)) (a.num * b.den + b.num * a.den) =
      some (n * (b.num * a.den) / (a.num * b.den + b.num * a.den)) :=
    tdivC_of_nonneg pT (by linarith) (inRange128_of_le (by linarith) lT (by norm_num))
  have s6 : castC wInt64 (n * (b.num * a.den) / (a.num * b.den + b.num * a.den)) =
      some (n * (b.num * a.den) / (a.num * b.den + b.num * a.den)) :=
    checked_of_inRange (inRange64_of_nonneg hq0 (by unfold int64Max; linarith))
  simp only [hashLeftCountCpp, s1, s2, s3, s4, s5, s6, Option.bind_eq_bind, Option.bind_some]
  unfold progress
  rw [Int.natCast_div]
  push_cast
  rw [hL, hR, Int.toNat_of_nonneg hn0]

/- ===== Hash ===== -/

-- ANCHOR: hash_cpp
def hashCpp (a b : RatInt) (i : ℤ) : Option (Bool × ℤ) := do
  -- const std::int64_t leftCount = hashLeftCount(deltaA, deltaB, i);
  let leftCount ← hashLeftCountCpp a b i
  -- std::int64_t{i} + 1
  let i1 ← addC wInt64 i 1
  -- const bool ret = leftCount == hashLeftCount(deltaA, deltaB, std::int64_t{i} + 1);
  let nextCount ← hashLeftCountCpp a b i1
  let ret := leftCount == nextCount
  -- retPos = static_cast<int>(ret ? i - leftCount : leftCount);  // B : A
  let pos ← if ret then subC wInt64 i leftCount else some leftCount
  let retPos ← castC wInt pos
  -- return ret;
  some (ret, retPos)
-- ANCHOR_END: hash_cpp

/- Hash zwraca !selectsA L R i i retPos = selectedIndex L R i; static_cast<int> jest bezpieczne,
   bo retPos <= i. -/
theorem hashCpp_eq {a b : RatInt} (ha : a.Valid) (ha0 : 0 < a.val) (hb : b.Valid)
    (hb0 : 0 < b.val) {i : ℤ} (hi0 : 0 ≤ i) (hi : inRange wInt i) :
    hashCpp a b i = some (!selectsA (cross a b) (cross b a) i.toNat,
      (selectedIndex (cross a b) (cross b a) i.toNat : ℤ)) := by
  have hil := index_lt hi
  have h1 := hashLeftCountCpp_eq ha ha0 hb hb0 hi0 (by linarith)
  have h2 := hashLeftCountCpp_eq ha ha0 hb hb0 (n := i + 1) (by linarith) (by linarith)
  have s1 : addC wInt64 i 1 = some (i + 1) :=
    addC_of_inRange (inRange64_of_nonneg (by linarith) (by unfold int64Max; linarith))
  have ht : (i + 1).toNat = i.toNat + 1 := by omega
  rw [ht] at h2
  have hp := progress_le (cross a b) (cross b a) i.toNat
  simp only [hashCpp, h1, s1, h2, Option.bind_eq_bind, Option.bind_some]
  generalize hP : progress (cross a b) (cross b a) i.toNat = P at hp ⊢
  generalize hP1 : progress (cross a b) (cross b a) (i.toNat + 1) = P1
  have hsel : selectsA (cross a b) (cross b a) i.toNat = decide (P ≠ P1) := by
    simp only [selectsA, hP, hP1]
  have hidx : selectedIndex (cross a b) (cross b a) i.toNat =
      if decide (P ≠ P1) = true then P else i.toNat - P := by
    simp only [selectedIndex, hsel, hP]
  rw [hidx, hsel]
  by_cases h : P = P1
  · subst h
    have hsub : subC wInt64 i (P : ℤ) = some (i - P) :=
      subC_of_inRange (inRange64_of_nonneg (by omega) (by unfold int64Max; omega))
    have hcast : castC wInt (i - P) = some (i - P) :=
      checked_of_inRange (by constructor <;> norm_num [wInt] <;> omega)
    simp only [beq_self_eq_true, if_true, hsub, hcast, Option.bind_some, ne_eq, not_true_eq_false,
      decide_false, Bool.not_false, Bool.false_eq_true, if_false, Option.some.injEq,
      Prod.mk.injEq, true_and]
    omega
  · have hne : ((P : ℤ) == (P1 : ℤ)) = false := by
      rw [beq_eq_false_iff_ne]; exact_mod_cast h
    have hcast : castC wInt (P : ℤ) = some (P : ℤ) :=
      checked_of_inRange (by (constructor <;> norm_num [wInt]); omega)
    simp [hne, hcast, h]

/- ===== Div (Theta) ===== -/

-- ANCHOR: div_cpp
def divCpp (a b : RatInt) (i : ℤ) : Option ℤ := do
  -- const __int128 numerator = (__int128{i} + 1) * deltaA.numerator() * deltaB.denominator();
  let i1 ← addC wInt128 i 1
  let t1 ← mulC wInt128 i1 a.num
  let numerator ← mulC wInt128 t1 b.den
  -- const __int128 denominator = __int128{deltaA.denominator()} * deltaB.numerator();
  let denominator ← mulC wInt128 a.den b.num
  -- numerator + denominator - 1
  let s ← addC wInt128 numerator denominator
  let s1 ← subC wInt128 s 1
  -- (...) / denominator
  let c ← tdivC wInt128 s1 denominator
  -- i + (...)   (int + __int128 -> __int128)
  let idx ← addC wInt128 i c
  -- return saturatedIndex(...);
  saturatedIndexCpp idx
-- ANCHOR_END: div_cpp

/- u = a1*b2, v = a2*b1: Div = affineRead (u+v) v (u+v-1) i, a z thetaRead_eq
   i + ceil((i+1)*u/v). Nasycenie na INT64_MAX jak w saturatedIndex. -/
theorem divCpp_eq {a b : RatInt} (ha : a.Valid) (ha0 : 0 < a.val) (hb : b.Valid)
    (hb0 : 0 < b.val) {i : ℤ} (hi0 : 0 ≤ i) (hi : inRange wInt i) :
    divCpp a b i = some (min (affineRead (cross a b + cross b a) (cross b a)
      (cross a b + cross b a - 1) i.toNat : ℤ) int64Max) := by
  obtain ⟨a1p, a1l, a2p, a2l⟩ := ratInt_pos_bounds ha ha0
  obtain ⟨b1p, b1l, b2p, b2l⟩ := ratInt_pos_bounds hb hb0
  have hil := index_lt hi
  obtain ⟨hu, pu⟩ := cross_spec ha ha0 hb
  obtain ⟨hv, -⟩ := cross_spec hb hb0 ha
  have p1 : 0 < (i + 1) * a.num := mul_pos (by linarith) a1p
  have p2 : 0 < (i + 1) * a.num * b.den := mul_pos p1 b2p
  have p3 : 0 < a.den * b.num := mul_pos a2p b1p
  have l1 : (i + 1) * a.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul (by linarith) a1l.le a1p.le (by norm_num)
  have l2 : (i + 1) * a.num * b.den ≤ 2 ^ 31 * 2 ^ 31 * 2 ^ 31 :=
    mul_le_mul l1 b2l.le b2p.le (by norm_num)
  have l3 : a.den * b.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul a2l.le b1l.le b1p.le (by norm_num)
  have hc0 : 0 ≤ ((i + 1) * a.num * b.den + a.den * b.num - 1) / (a.den * b.num) :=
    Int.ediv_nonneg (by linarith) p3.le
  have hc1 : ((i + 1) * a.num * b.den + a.den * b.num - 1) / (a.den * b.num) ≤
      (i + 1) * a.num * b.den + a.den * b.num - 1 := Int.ediv_le_self _ (by linarith)
  have s1 : addC wInt128 i 1 = some (i + 1) :=
    addC_of_inRange (inRange128_of_le (by linarith) (show i + 1 ≤ 2 ^ 31 by linarith) (by norm_num))
  have s2 : mulC wInt128 (i + 1) a.num = some ((i + 1) * a.num) :=
    mulC_of_inRange (inRange128_of_le (by linarith) l1 (by norm_num))
  have s3 : mulC wInt128 ((i + 1) * a.num) b.den = some ((i + 1) * a.num * b.den) :=
    mulC_of_inRange (inRange128_of_le (by linarith) l2 (by norm_num))
  have s4 : mulC wInt128 a.den b.num = some (a.den * b.num) :=
    mulC_of_inRange (inRange128_of_le (by linarith) l3 (by norm_num))
  have s5 : addC wInt128 ((i + 1) * a.num * b.den) (a.den * b.num) =
      some ((i + 1) * a.num * b.den + a.den * b.num) :=
    addC_of_inRange (inRange128_of_le (by linarith) (add_le_add l2 l3) (by norm_num))
  have r6 : inRange wInt128 ((i + 1) * a.num * b.den + a.den * b.num - 1) :=
    inRange128_of_le (by linarith) (show _ ≤ (2 : ℤ) ^ 31 * 2 ^ 31 * 2 ^ 31 + 2 ^ 31 * 2 ^ 31 by
      linarith) (by norm_num)
  have s6 : subC wInt128 ((i + 1) * a.num * b.den + a.den * b.num) 1 =
      some ((i + 1) * a.num * b.den + a.den * b.num - 1) := subC_of_inRange r6
  have s7 := tdivC_of_nonneg (by linarith) p3 r6
  have s8 : addC wInt128 i (((i + 1) * a.num * b.den + a.den * b.num - 1) / (a.den * b.num)) =
      some (i + ((i + 1) * a.num * b.den + a.den * b.num - 1) / (a.den * b.num)) :=
    addC_of_inRange (inRange128_of_le (by linarith)
      (show _ ≤ (2 : ℤ) ^ 31 + (2 ^ 31 * 2 ^ 31 * 2 ^ 31 + 2 ^ 31 * 2 ^ 31) by linarith) (by norm_num))
  simp only [divCpp, s1, s2, s3, s4, s5, s6, s7, s8, Option.bind_eq_bind, Option.bind_some]
  rw [saturatedIndexCpp_eq (by linarith), affineRead_cast]
  have hsub : ((cross a b + cross b a - 1 : ℕ) : ℤ) = (cross a b : ℤ) + cross b a - 1 := by omega
  rw [hsub]
  push_cast
  rw [hu, hv, Int.toNat_of_nonneg hi0]
  congr 2
  rw [show i * (a.num * b.den + b.num * a.den) + (a.num * b.den + b.num * a.den - 1) =
      ((i + 1) * a.num * b.den + a.den * b.num - 1) + i * (a.den * b.num) by ring,
    show b.num * a.den = a.den * b.num by ring, Int.add_mul_ediv_right _ _ p3.ne']
  ring

/- ===== Mod (~Theta) ===== -/

-- ANCHOR: mod_cpp
def modCpp (a b : RatInt) (i : ℤ) : Option ℤ := do
  -- const __int128 numerator = __int128{i} * deltaB.numerator() * deltaA.denominator();
  let t1 ← mulC wInt128 i b.num
  let numerator ← mulC wInt128 t1 a.den
  -- const __int128 denominator = __int128{deltaB.denominator()} * deltaA.numerator();
  let denominator ← mulC wInt128 b.den a.num
  -- numerator / denominator
  let c ← tdivC wInt128 numerator denominator
  -- i + (...)   (int + __int128 -> __int128)
  let idx ← addC wInt128 i c
  -- return saturatedIndex(...);
  saturatedIndexCpp idx
-- ANCHOR_END: mod_cpp

/- u = b1*a2, v = b2*a1: Mod = affineRead (u+v) v 0 i, a z nthetaRead_eq i + floor(i*u/v). -/
theorem modCpp_eq {a b : RatInt} (ha : a.Valid) (ha0 : 0 < a.val) (hb : b.Valid)
    (hb0 : 0 < b.val) {i : ℤ} (hi0 : 0 ≤ i) (hi : inRange wInt i) :
    modCpp a b i = some (min (affineRead (cross b a + cross a b) (cross a b) 0 i.toNat : ℤ)
      int64Max) := by
  obtain ⟨a1p, a1l, a2p, a2l⟩ := ratInt_pos_bounds ha ha0
  obtain ⟨b1p, b1l, b2p, b2l⟩ := ratInt_pos_bounds hb hb0
  have hil := index_lt hi
  obtain ⟨hu, -⟩ := cross_spec hb hb0 ha
  obtain ⟨hv, -⟩ := cross_spec ha ha0 hb
  have p1 : 0 ≤ i * b.num := mul_nonneg hi0 b1p.le
  have p2 : 0 ≤ i * b.num * a.den := mul_nonneg p1 a2p.le
  have p3 : 0 < b.den * a.num := mul_pos b2p a1p
  have l1 : i * b.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul hil.le b1l.le b1p.le (by norm_num)
  have l2 : i * b.num * a.den ≤ 2 ^ 31 * 2 ^ 31 * 2 ^ 31 := mul_le_mul l1 a2l.le a2p.le (by norm_num)
  have l3 : b.den * a.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul b2l.le a1l.le a1p.le (by norm_num)
  have hc1 : i * b.num * a.den / (b.den * a.num) ≤ i * b.num * a.den := Int.ediv_le_self _ p2
  have hc0 : 0 ≤ i * b.num * a.den / (b.den * a.num) := Int.ediv_nonneg p2 p3.le
  have s1 : mulC wInt128 i b.num = some (i * b.num) :=
    mulC_of_inRange (inRange128_of_le (by linarith) l1 (by norm_num))
  have r2 : inRange wInt128 (i * b.num * a.den) := inRange128_of_le (by linarith) l2 (by norm_num)
  have s2 : mulC wInt128 (i * b.num) a.den = some (i * b.num * a.den) := mulC_of_inRange r2
  have s3 : mulC wInt128 b.den a.num = some (b.den * a.num) :=
    mulC_of_inRange (inRange128_of_le (by linarith) l3 (by norm_num))
  have s4 := tdivC_of_nonneg p2 p3 r2
  have s5 : addC wInt128 i (i * b.num * a.den / (b.den * a.num)) =
      some (i + i * b.num * a.den / (b.den * a.num)) :=
    addC_of_inRange (inRange128_of_le (by linarith)
      (show _ ≤ (2 : ℤ) ^ 31 + 2 ^ 31 * 2 ^ 31 * 2 ^ 31 by linarith) (by norm_num))
  simp only [modCpp, s1, s2, s3, s4, s5, Option.bind_eq_bind, Option.bind_some]
  rw [saturatedIndexCpp_eq (by linarith), affineRead_cast]
  push_cast
  rw [hu, hv, Int.toNat_of_nonneg hi0]
  congr 2
  rw [show i * (b.num * a.den + a.num * b.den) + 0 = i * b.num * a.den + i * (b.den * a.num) by ring,
    show a.num * b.den = b.den * a.num by ring, Int.add_mul_ediv_right _ _ p3.ne']
  ring

/- ===== Subtract ===== -/

-- ANCHOR: subtract_cpp
def subtractCpp (s t : RatInt) (i : ℤ) : Option ℤ :=
  -- if (deltaSource == deltaTarget) return i;
  if s.beq t then some i
  else do
    -- const __int128 numerator = __int128{i} * deltaTarget.numerator() * deltaSource.denominator();
    let t1 ← mulC wInt128 i t.num
    let numerator ← mulC wInt128 t1 s.den
    -- const __int128 denominator = __int128{deltaTarget.denominator()} * deltaSource.numerator();
    let denominator ← mulC wInt128 t.den s.num
    -- numerator + denominator - 1
    let s0 ← addC wInt128 numerator denominator
    let s1 ← subC wInt128 s0 1
    -- (...) / denominator
    let c ← tdivC wInt128 s1 denominator
    -- return saturatedIndex(...);
    saturatedIndexCpp c
-- ANCHOR_END: subtract_cpp

/- Przy rownych skladowych iloczyny krzyzowe sa rowne, a affineRead p p c n = n dla c < p. -/
theorem affineRead_self (p c n : ℕ) (hc : c < p) : affineRead p p c n = n := by
  unfold affineRead
  rw [show n * p + c = c + n * p by ring, Nat.add_mul_div_right _ _ (by omega),
    Nat.div_eq_of_lt hc, zero_add]

/- p = t1*s2, q = t2*s1: Subtract = affineRead p q (q-1) i, a z subtractRead_eq ceil(i*p/q);
   galaz == daje i, tak samo jak wzor przy p = q. -/
theorem subtractCpp_eq {s t : RatInt} (hs : s.Valid) (hs0 : 0 < s.val) (ht : t.Valid)
    (ht0 : 0 < t.val) {i : ℤ} (hi0 : 0 ≤ i) (hi : inRange wInt i) :
    subtractCpp s t i = some (min (affineRead (cross t s) (cross s t) (cross s t - 1) i.toNat : ℤ)
      int64Max) := by
  obtain ⟨s1p, s1l, s2p, s2l⟩ := ratInt_pos_bounds hs hs0
  obtain ⟨t1p, t1l, t2p, t2l⟩ := ratInt_pos_bounds ht ht0
  have hil := index_lt hi
  obtain ⟨hp, -⟩ := cross_spec ht ht0 hs
  obtain ⟨hq, pq⟩ := cross_spec hs hs0 ht
  unfold subtractCpp
  split_ifs with heq
  · have e : s.num = t.num ∧ s.den = t.den := by
      simpa [RatInt.beq] using heq
    have hpq : cross t s = cross s t := by unfold cross; rw [e.1, e.2]
    rw [hpq, affineRead_self _ _ _ (by omega), Int.toNat_of_nonneg hi0,
      min_eq_left (by unfold int64Max; linarith)]
  · have p1 : 0 ≤ i * t.num := mul_nonneg hi0 t1p.le
    have p2 : 0 ≤ i * t.num * s.den := mul_nonneg p1 s2p.le
    have p3 : 0 < t.den * s.num := mul_pos t2p s1p
    have l1 : i * t.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul hil.le t1l.le t1p.le (by norm_num)
    have l2 : i * t.num * s.den ≤ 2 ^ 31 * 2 ^ 31 * 2 ^ 31 :=
      mul_le_mul l1 s2l.le s2p.le (by norm_num)
    have l3 : t.den * s.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul t2l.le s1l.le s1p.le (by norm_num)
    have hc0 : 0 ≤ (i * t.num * s.den + t.den * s.num - 1) / (t.den * s.num) :=
      Int.ediv_nonneg (by linarith) p3.le
    have s1 : mulC wInt128 i t.num = some (i * t.num) :=
      mulC_of_inRange (inRange128_of_le (by linarith) l1 (by norm_num))
    have s2 : mulC wInt128 (i * t.num) s.den = some (i * t.num * s.den) :=
      mulC_of_inRange (inRange128_of_le (by linarith) l2 (by norm_num))
    have s3 : mulC wInt128 t.den s.num = some (t.den * s.num) :=
      mulC_of_inRange (inRange128_of_le (by linarith) l3 (by norm_num))
    have s4 : addC wInt128 (i * t.num * s.den) (t.den * s.num) =
        some (i * t.num * s.den + t.den * s.num) :=
      addC_of_inRange (inRange128_of_le (by linarith) (add_le_add l2 l3) (by norm_num))
    have r5 : inRange wInt128 (i * t.num * s.den + t.den * s.num - 1) :=
      inRange128_of_le (by linarith) (show _ ≤ (2 : ℤ) ^ 31 * 2 ^ 31 * 2 ^ 31 + 2 ^ 31 * 2 ^ 31 by
        linarith) (by norm_num)
    have s5 : subC wInt128 (i * t.num * s.den + t.den * s.num) 1 =
        some (i * t.num * s.den + t.den * s.num - 1) := subC_of_inRange r5
    have s6 := tdivC_of_nonneg (by linarith) p3 r5
    simp only [s1, s2, s3, s4, s5, s6, Option.bind_eq_bind, Option.bind_some]
    rw [saturatedIndexCpp_eq hc0, affineRead_cast]
    have hsub : ((cross s t - 1 : ℕ) : ℤ) = (cross s t : ℤ) - 1 := by omega
    rw [hsub, hp, hq, Int.toNat_of_nonneg hi0]
    congr 2
    rw [show i * (t.num * s.den) + (s.num * t.den - 1) = i * t.num * s.den + t.den * s.num - 1 by
      ring, show s.num * t.den = t.den * s.num by ring]

/- ===== Add ===== -/

-- ANCHOR: add_cpp
def addCpp (o src : RatInt) (n : ℤ) : Option ℤ :=
  -- if (deltaOut == deltaSrc) return n;
  if o.beq src then some n
  else do
    -- const __int128 numerator = __int128{n} * deltaOut.numerator() * deltaSrc.denominator();
    let t1 ← mulC wInt128 n o.num
    let numerator ← mulC wInt128 t1 src.den
    -- const __int128 denominator = __int128{deltaOut.denominator()} * deltaSrc.numerator();
    let denominator ← mulC wInt128 o.den src.num
    -- numerator / denominator
    let c ← tdivC wInt128 numerator denominator
    -- return saturatedIndex(...);
    saturatedIndexCpp c
-- ANCHOR_END: add_cpp

/- p = o1*s2, q = o2*s1: Add = affineRead p q 0 n, a z addRead_eq floor(n*p/q); galaz == daje n,
   tak samo jak wzor przy p = q. -/
theorem addCpp_eq {o src : RatInt} (ho : o.Valid) (ho0 : 0 < o.val) (hs : src.Valid)
    (hs0 : 0 < src.val) {n : ℤ} (hn0 : 0 ≤ n) (hn : inRange wInt n) :
    addCpp o src n = some (min (affineRead (cross o src) (cross src o) 0 n.toNat : ℤ) int64Max) := by
  obtain ⟨o1p, o1l, o2p, o2l⟩ := ratInt_pos_bounds ho ho0
  obtain ⟨s1p, s1l, s2p, s2l⟩ := ratInt_pos_bounds hs hs0
  have hnl := index_lt hn
  obtain ⟨hp, pp⟩ := cross_spec ho ho0 hs
  obtain ⟨hq, -⟩ := cross_spec hs hs0 ho
  unfold addCpp
  split_ifs with heq
  · have e : o.num = src.num ∧ o.den = src.den := by
      simpa [RatInt.beq] using heq
    have hpq : cross o src = cross src o := by unfold cross; rw [e.1, e.2]
    rw [← hpq, affineRead_self _ _ _ (by omega), Int.toNat_of_nonneg hn0,
      min_eq_left (by unfold int64Max; linarith)]
  · have p1 : 0 ≤ n * o.num := mul_nonneg hn0 o1p.le
    have p2 : 0 ≤ n * o.num * src.den := mul_nonneg p1 s2p.le
    have p3 : 0 < o.den * src.num := mul_pos o2p s1p
    have l1 : n * o.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul hnl.le o1l.le o1p.le (by norm_num)
    have l2 : n * o.num * src.den ≤ 2 ^ 31 * 2 ^ 31 * 2 ^ 31 :=
      mul_le_mul l1 s2l.le s2p.le (by norm_num)
    have l3 : o.den * src.num ≤ 2 ^ 31 * 2 ^ 31 := mul_le_mul o2l.le s1l.le s1p.le (by norm_num)
    have s1 : mulC wInt128 n o.num = some (n * o.num) :=
      mulC_of_inRange (inRange128_of_le (by linarith) l1 (by norm_num))
    have r2 : inRange wInt128 (n * o.num * src.den) := inRange128_of_le (by linarith) l2 (by norm_num)
    have s2 : mulC wInt128 (n * o.num) src.den = some (n * o.num * src.den) := mulC_of_inRange r2
    have s3 : mulC wInt128 o.den src.num = some (o.den * src.num) :=
      mulC_of_inRange (inRange128_of_le (by linarith) l3 (by norm_num))
    have s4 := tdivC_of_nonneg p2 p3 r2
    simp only [s1, s2, s3, s4, Option.bind_eq_bind, Option.bind_some]
    rw [saturatedIndexCpp_eq (Int.ediv_nonneg p2 p3.le), affineRead_cast]
    push_cast
    rw [hp, hq, Int.toNat_of_nonneg hn0]
    congr 2
    rw [show n * (o.num * src.den) + 0 = n * o.num * src.den by ring,
      show src.num * o.den = o.den * src.num by ring]

end Cpp

end Profs

module

public import Mathlib.Data.Rat.Lemmas
public import Mathlib.Tactic

@[expose] public section

namespace Profs

/- Arytmetyka C++ funkcji z src/include/SOperations.hpp (#634, warstwa A, krok A1): int, int64
   i __int128 jako arytmetyka sprawdzana oraz boost::rational<int> jako para (num, den)
   z niezmiennikiem Boost.

   Operacja na typie ze znakiem o szerokosci w bitow jest funkcja Z -> Z -> Option Z. Wynik none
   oznacza wartosc spoza [-2^(w-1), 2^(w-1)) albo dzielenie przez zero, czyli niezdefiniowane
   zachowanie w C++. Typy Int32 i Int64 z rdzenia Lean licza modulo 2^w, wiec opisywalyby
   zachowanie, ktorego C++ dla typow ze znakiem nie gwarantuje. Dzielenie i reszta obcinaja
   w strone zera (C++11), czyli Int.tdiv i Int.tmod.

   Argumenty sa wartosciami swojego typu; model nie sprawdza ich ponownie. Wyrazenie mieszane,
   np. int razy __int128, liczy sie w szerokosci szerszego argumentu po konwersji, ktora nie
   traci wartosci (inRange_mono). Rzutowanie zawezajace daje none poza zakresem; C++20 okresla
   je modulo 2^w, ale kod silnika rzutuje wylacznie wartosci juz sprawdzone. -/

namespace Cpp

def inRange (w : ℕ) (x : ℤ) : Prop := -(2 : ℤ) ^ (w - 1) ≤ x ∧ x < 2 ^ (w - 1)

instance (w : ℕ) (x : ℤ) : Decidable (inRange w x) :=
  inferInstanceAs (Decidable (_ ∧ _))

/- Szerokosci typow silnika: int, std::int64_t, __int128. -/
abbrev wInt : ℕ := 32
abbrev wInt64 : ℕ := 64
abbrev wInt128 : ℕ := 128

def checked (w : ℕ) (x : ℤ) : Option ℤ := if inRange w x then some x else none

def addC (w : ℕ) (a b : ℤ) : Option ℤ := checked w (a + b)
def subC (w : ℕ) (a b : ℤ) : Option ℤ := checked w (a - b)
def mulC (w : ℕ) (a b : ℤ) : Option ℤ := checked w (a * b)

/- a / b w C++: UB przy b = 0 i przy ilorazie spoza zakresu (MIN / -1). -/
def tdivC (w : ℕ) (a b : ℤ) : Option ℤ := if b = 0 then none else checked w (a.tdiv b)

/- a % b w C++: UB dokladnie wtedy, gdy UB jest a / b. -/
def tmodC (w : ℕ) (a b : ℤ) : Option ℤ :=
  if b = 0 then none else if inRange w (a.tdiv b) then some (a.tmod b) else none

/- static_cast do typu o szerokosci w. -/
def castC (w : ℕ) (x : ℤ) : Option ℤ := checked w x

/- std::gcd na nieujemnych int64 - jedyne uzycie w SOperations.hpp. Petla Euklidesa nie jest
   przepisana: gcd jest funkcja matematyczna, co jest jawna granica modelu. -/
def gcd64 (a b : ℤ) : Option ℤ :=
  if 0 ≤ a ∧ 0 ≤ b ∧ inRange wInt64 a ∧ inRange wInt64 b then some (Int.gcd a b) else none

/- ===== Operacje sprawdzane ===== -/

/- Przepisanie daje none dokladnie w miejscu przepelnienia. -/
theorem checked_eq_none_iff (w : ℕ) (x : ℤ) : checked w x = none ↔ ¬ inRange w x := by
  unfold checked
  split_ifs with h <;> simp [h]

theorem checked_of_inRange {w : ℕ} {x : ℤ} (h : inRange w x) : checked w x = some x := by
  simp [checked, h]

theorem addC_of_inRange {w : ℕ} {a b : ℤ} (h : inRange w (a + b)) : addC w a b = some (a + b) :=
  checked_of_inRange h

theorem subC_of_inRange {w : ℕ} {a b : ℤ} (h : inRange w (a - b)) : subC w a b = some (a - b) :=
  checked_of_inRange h

theorem mulC_of_inRange {w : ℕ} {a b : ℤ} (h : inRange w (a * b)) : mulC w a b = some (a * b) :=
  checked_of_inRange h

theorem tdivC_of_inRange {w : ℕ} {a b : ℤ} (hb : b ≠ 0) (h : inRange w (a.tdiv b)) :
    tdivC w a b = some (a.tdiv b) := by
  simp [tdivC, hb, checked_of_inRange h]

theorem tmodC_of_inRange {w : ℕ} {a b : ℤ} (hb : b ≠ 0) (h : inRange w (a.tdiv b)) :
    tmodC w a b = some (a.tmod b) := by
  simp [tmodC, hb, h]

/- Konwersja do typu nie wezszego nie traci wartosci. -/
theorem inRange_mono {w w' : ℕ} (hw : w ≤ w') {x : ℤ} (h : inRange w x) : inRange w' x := by
  obtain ⟨h1, h2⟩ := h
  have hp : (2 : ℤ) ^ (w - 1) ≤ 2 ^ (w' - 1) := pow_le_pow_right₀ (by norm_num) (by omega)
  exact ⟨by linarith, by linarith⟩

theorem castC_widen {w w' : ℕ} (hw : w ≤ w') {x : ℤ} (h : inRange w x) : castC w' x = some x :=
  checked_of_inRange (inRange_mono hw h)

/- Dla a >= 0 i b > 0 iloraz obciety jest ilorazem euklidesowym i lezy w [0, a], wiec nie
   wychodzi poza zakres argumentu; reszta obcieta jest reszta euklidesowa. -/
theorem tdiv_inRange_of_nonneg {w : ℕ} {a b : ℤ} (ha : 0 ≤ a) (hb : 0 < b) (h : inRange w a) :
    inRange w (a.tdiv b) := by
  have hq0 : 0 ≤ a.tdiv b := Int.tdiv_nonneg ha hb.le
  have hq1 : a.tdiv b ≤ a := Int.tdiv_le_self b ha
  have hp : (0 : ℤ) ≤ 2 ^ (w - 1) := by positivity
  exact ⟨by linarith, by linarith [h.2]⟩

theorem tdivC_of_nonneg {w : ℕ} {a b : ℤ} (ha : 0 ≤ a) (hb : 0 < b) (h : inRange w a) :
    tdivC w a b = some (a / b) := by
  rw [tdivC_of_inRange hb.ne' (tdiv_inRange_of_nonneg ha hb h), Int.tdiv_eq_ediv_of_nonneg ha]

theorem tmodC_of_nonneg {w : ℕ} {a b : ℤ} (ha : 0 ≤ a) (hb : 0 < b) (h : inRange w a) :
    tmodC w a b = some (a % b) := by
  rw [tmodC_of_inRange hb.ne' (tdiv_inRange_of_nonneg ha hb h), Int.tmod_eq_emod_of_nonneg ha]

/- Ta sama zgodnosc dla wartosci naturalnych: / i % w C++ to / i % na N. -/
theorem tdivC_natCast {w : ℕ} (m n : ℕ) (hn : 0 < n) (h : inRange w m) :
    tdivC w m n = some ((m / n : ℕ) : ℤ) := by
  rw [tdivC_of_nonneg (by positivity) (by exact_mod_cast hn) h]
  norm_cast

theorem tmodC_natCast {w : ℕ} (m n : ℕ) (hn : 0 < n) (h : inRange w m) :
    tmodC w m n = some ((m % n : ℕ) : ℤ) := by
  rw [tmodC_of_nonneg (by positivity) (by exact_mod_cast hn) h]
  norm_cast

theorem gcd64_of_nonneg {a b : ℤ} (ha : 0 ≤ a) (hb : 0 ≤ b) (hra : inRange wInt64 a)
    (hrb : inRange wInt64 b) : gcd64 a b = some (Int.gcd a b) := by
  simp [gcd64, ha, hb, hra, hrb]

/- ===== boost::rational<int> ===== -/

/- Para (num, den) z numerator() i denominator(). Po #644 funkcje z SOperations.hpp nie licza
   na rational: uzywaja tylko obu skladowych i porownania ==. -/
structure RatInt where
  num : ℤ
  den : ℤ

/- Niezmiennik Boost 1.91 (konstruktor normalizuje): den > 0, gcd(num, den) = 1, skladowe w int.
   Dodatniosc interwalow jest warunkiem wstepnym funkcji, nie czescia niezmiennika. -/
def RatInt.Valid (r : RatInt) : Prop :=
  0 < r.den ∧ Int.gcd r.num r.den = 1 ∧ inRange wInt r.num ∧ inRange wInt r.den

def RatInt.val (r : RatInt) : ℚ := (r.num : ℚ) / r.den

/- operator== z boost/rational.hpp porownuje skladowe. -/
def RatInt.beq (r s : RatInt) : Bool := r.num == s.num && r.den == s.den

/- Przy niezmienniku rownosc par to rownosc wartosci. -/
theorem ratInt_beq_iff_val {r s : RatInt} (hr : r.Valid) (hs : s.Valid) :
    r.beq s = true ↔ r.val = s.val := by
  unfold RatInt.beq RatInt.val
  simp only [Bool.and_eq_true, beq_iff_eq]
  constructor
  · rintro ⟨h1, h2⟩
    rw [h1, h2]
  · intro h
    have cr : Nat.Coprime r.num.natAbs r.den.natAbs := hr.2.1
    have cs : Nat.Coprime s.num.natAbs s.den.natAbs := hs.2.1
    have n1 := Rat.num_div_eq_of_coprime hr.1 cr
    have n2 := Rat.num_div_eq_of_coprime hs.1 cs
    have d1 := Rat.den_div_eq_of_coprime hr.1 cr
    have d2 := Rat.den_div_eq_of_coprime hs.1 cs
    rw [h] at n1 d1
    exact ⟨n1.symm.trans n2, d1.symm.trans d2⟩

/- Interwal dodatni ma dodatni licznik. -/
theorem ratInt_num_pos {r : RatInt} (hr : r.Valid) (h : 0 < r.val) : 0 < r.num := by
  unfold RatInt.val at h
  have hd : (0 : ℚ) < r.den := by exact_mod_cast hr.1
  have e : (r.num : ℚ) = (r.num : ℚ) / r.den * r.den := by field_simp
  have : (0 : ℚ) < r.num := by
    rw [e]
    exact mul_pos h hd
  exact_mod_cast this

/- Iloczyn krzyzowy dwoch dodatnich skladowych int jest mniejszy niz 2^62, wiec miesci sie
   w int64 (D6: parametry modelu to nieskrocone iloczyny krzyzowe). -/
theorem crossProduct_bound {a b : ℤ} (ha : 0 < a) (hb : 0 < b) (ha' : inRange wInt a)
    (hb' : inRange wInt b) : 0 < a * b ∧ a * b < 2 ^ 62 ∧ inRange wInt64 (a * b) := by
  have h1 : a < 2 ^ 31 := by
    have := ha'.2
    norm_num [wInt] at this ⊢
    linarith
  have h2 : b < 2 ^ 31 := by
    have := hb'.2
    norm_num [wInt] at this ⊢
    linarith
  have hab : a * b < 2 ^ 62 := by
    calc a * b < 2 ^ 31 * 2 ^ 31 := mul_lt_mul'' h1 h2 ha.le hb.le
      _ = 2 ^ 62 := by norm_num
  have hpos : 0 < a * b := mul_pos ha hb
  refine ⟨hpos, hab, ?_, ?_⟩ <;> norm_num [wInt64] <;> linarith

/- Iloraz dwoch dodatnich interwalow to iloraz iloczynow krzyzowych. -/
theorem ratInt_val_div {r s : RatInt} (hr : r.Valid) (hs : s.Valid) (hs0 : 0 < s.val) :
    r.val / s.val = ((r.num * s.den : ℤ) : ℚ) / ((r.den * s.num : ℤ) : ℚ) := by
  unfold RatInt.val
  have h1 : (r.den : ℚ) ≠ 0 := by exact_mod_cast hr.1.ne'
  have h2 : (s.den : ℚ) ≠ 0 := by exact_mod_cast hs.1.ne'
  have h3 : (s.num : ℚ) ≠ 0 := by exact_mod_cast (ratInt_num_pos hs hs0).ne'
  push_cast
  field_simp

end Cpp

end Profs

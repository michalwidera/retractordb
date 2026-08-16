// Bramka kroku B: kanoniczny serializer po stronie Flinka musi dawac te sama liczbe,
// co `rdb::probe::canonicalRecordBytes` w retractordb.
//
// Test jest podwojnie zamkniety:
//   1. wobec kolumny oczekiwanej w canonical_vectors.tsv (przypadki o ZNANEJ odpowiedzi,
//      przepisane z test/UnitTest/test_probe.cpp — bramki tej metryki po stronie C++),
//   2. wobec wyjscia oracle/canonical_oracle, ktory nie ma wlasnej implementacji, tylko
//      linkuje funkcje silnika.
//
// Punkt 2 jest wazniejszy: wyklucza sytuacje, w ktorej oba przepisania specyfikacji zgadzaja
// sie ze soba i oba rozjezdzaja sie z kodem.
//
// Uzycie: java CanonTest <canonical_vectors.tsv> [<oracle_out.tsv>]
// Kod wyjscia: 0 = wszystkie wektory zgodne, 1 = rozbieznosc.
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class CanonTest {

  static Canon.Descriptor parseDescriptor(String spec) {
    if (spec.equals("-")) {
      return new Canon.Descriptor();
    }
    List<Canon.Field> fields = new ArrayList<>();
    for (String fieldSpec : spec.split(";")) {
      String[] p = fieldSpec.split(":");
      if (p.length != 4) {
        throw new IllegalArgumentException("zle pole: " + fieldSpec);
      }
      fields.add(new Canon.Field(p[0], Integer.parseInt(p[1]), Integer.parseInt(p[2]), Canon.Type.valueOf(p[3])));
    }
    return new Canon.Descriptor(fields.toArray(new Canon.Field[0]));
  }

  public static void main(String[] args) throws Exception {
    if (args.length < 1) {
      System.err.println("uzycie: java CanonTest <canonical_vectors.tsv> [<oracle_out.tsv>]");
      System.exit(2);
    }

    Map<String, Long> oracle = new LinkedHashMap<>();
    if (args.length >= 2) {
      for (String line : Files.readAllLines(Paths.get(args[1]), StandardCharsets.UTF_8)) {
        if (line.isBlank()) {
          continue;
        }
        String[] c = line.split("\t");
        oracle.put(c[0], Long.parseLong(c[1].trim()));
      }
    }

    int checked = 0;
    int failed = 0;
    int againstOracle = 0;
    for (String line : Files.readAllLines(Paths.get(args[0]), StandardCharsets.UTF_8)) {
      if (line.isBlank() || line.startsWith("#")) {
        continue;
      }
      String[] c = line.split("\t");
      if (c.length < 3) {
        continue;
      }
      String label = c[0];
      long expected = Long.parseLong(c[2].trim());
      long actual = Canon.recordBytes(parseDescriptor(c[1]));
      checked++;

      if (actual != expected) {
        System.out.println("BLAD  " + label + ": Java=" + actual + " oczekiwano=" + expected);
        failed++;
      }
      if (oracle.containsKey(label)) {
        againstOracle++;
        long fromEngine = oracle.get(label);
        if (actual != fromEngine) {
          System.out.println("BLAD  " + label + ": Java=" + actual + " oracle(C++)=" + fromEngine);
          failed++;
        }
      } else if (!oracle.isEmpty()) {
        System.out.println("BLAD  " + label + ": brak wektora w wyjsciu oracle");
        failed++;
      }
    }

    if (oracle.size() != againstOracle) {
      System.out.println("BLAD  oracle zwrocil " + oracle.size() + " wektorow, dopasowano " + againstOracle);
      failed++;
    }

    System.out.println("wektory=" + checked + " porownane_z_oracle=" + againstOracle + " bledy=" + failed);
    if (failed > 0) {
      System.out.println("WYNIK: ROZBIEZNOSC — metryka miedzysystemowa niewazna");
      System.exit(1);
    }
    System.out.println("WYNIK: OK — serializer Flinka zgodny z rdb::probe::canonicalRecordBytes");
  }
}

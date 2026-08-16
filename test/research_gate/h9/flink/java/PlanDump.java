// Krok D: zapis planu LOGICZNEGO i FIZYCZNEGO oraz zliczenie instancji operatorow badanego
// podplanu.
//
// Granica podplanu jest ta sama, co po stronie RetractorDB (SZKIC_RODZIN.md §3.1): operatory
// miedzy zrodlem a per-monitorowym sinkiem. Zrodla (ingress), transport, checkpointy i
// per-monitorowy sink sa POZA metryka; publiczne rekordy monitorow sa MIANOWNIKIEM.
//
// Konwencja nazw operatorow, po ktorej liczy ten plik:
//   SUB:<monitor>:<wezel>   — wezel BADANEGO PODPLANU (licznik metryki),
//   PUB:<monitor>           — etap publiczny monitora (mianownik),
//   SINK:<monitor>          — zapis wyniku publicznego (poza metryka),
//   SRC:<strumien>          — zadeklarowane zrodlo (ingress, poza metryka).
//
// Waga wezla to liczba jego zapisow w przebiegu wyrazona w JEDNOSTKACH `n_h` — tak samo, jak
// w RAPORT_PILOTA.md §2, gdzie 1 jednostka = liczba slotow strumienia przeplecionego (150 Hz)
// razy kanoniczna szerokosc rekordu. Wezel 150 Hz ma wage 1, wezel 100 Hz — 2/3,
// wezel 50 Hz — 1/3. Jednostki sa arytmetyka predeklaracyjna (rate x szerokosc), nie odczytem
// licznika: ten krok nie uruchamia pomiaru, dokladnie jak pilot compile-only.
//
// Rejestr wag jest weryfikowany wobec ZBUDOWANEGO grafu: kazdy wezel `SUB:` musi wystapic
// w rejestrze i odwrotnie. Rozbieznosc konczy program kodem 1.
import org.apache.flink.api.java.tuple.Tuple3;
import org.apache.flink.runtime.jobgraph.JobGraph;
import org.apache.flink.runtime.jobgraph.JobVertex;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.datastream.SingleOutputStreamOperator;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;
import org.apache.flink.streaming.api.graph.StreamEdge;
import org.apache.flink.streaming.api.graph.StreamGraph;
import org.apache.flink.streaming.api.graph.StreamNode;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

public final class PlanDump {

  private PlanDump() {}

  /** Waga w jednostkach `n_h`: 150 Hz = 1, 100 Hz = 2/3, 50 Hz = 1/3. */
  public static final double UNIT_150 = 1.0;
  public static final double UNIT_100 = 2.0 / 3.0;
  public static final double UNIT_50 = 1.0 / 3.0;

  /** Rodzaj operatora — decyduje, ktory licznik pracy on obsluguje. */
  public enum Kind {
    /** Jedno wejscie, jeden rekord na rekord: przesuniecie albo przejscie monitora. */
    MAP,
    /** Przeplot `#`: jeden wybor skladowej na rekord wyjscia (`hashPicks`). */
    INTERLEAVE,
    /** Suma strumieni `+`: jedno scalenie payloadow na rekord wyjscia (`addMerges`). */
    ADD
  }

  /** Wpis rejestru: rola w metryce, waga w jednostkach `n_h`, dlugosc programu, rodzaj. */
  private record Node(boolean substrate, double unitWeight, int programTokens, Kind kind) {}

  private static final Map<String, Node> registry = new LinkedHashMap<>();
  private static int costlyProgramTokens = 0;

  /**
   * Deklaruje, ktory program rodziny jest KOSZTOWNY — jego liczba wykonan na slot jest
   * wielkoscia odporna na ciecie obliczenia na operatory, wiec to ona niesie twierdzenie
   * o zduplikowanej pracy (§10, metryki mechanizmu). Wartosc w tokenach, odczytana ze zrzutu
   * planu pilota (`K26Ops.TOKENS_*`).
   */
  public static void costlyProgram(int tokens) {
    costlyProgramTokens = tokens;
  }

  /** Zglasza operator jako wezel badanego podplanu i nadaje mu nazwe wg konwencji. */
  public static DataStream<Tuple3<Long, Long, Integer>> sub(
      SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stream, String name, double unitWeight,
      int programTokens, Kind kind) {
    return register("SUB:" + name, stream, new Node(true, unitWeight, programTokens, kind));
  }

  /** Etap publiczny monitora — mianownik metryki bajtowej, ale JEGO PRACA sie liczy. */
  public static DataStream<Tuple3<Long, Long, Integer>> pub(
      SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stream, String monitor, double unitWeight,
      int programTokens, Kind kind) {
    return register("PUB:" + monitor, stream, new Node(false, unitWeight, programTokens, kind));
  }

  private static DataStream<Tuple3<Long, Long, Integer>> register(
      String full, SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stream, Node node) {
    if (registry.put(full, node) != null) {
      throw new IllegalStateException("duplikat nazwy operatora: " + full);
    }
    return stream.name(full).uid(full);
  }

  public static void reset() {
    registry.clear();
    costlyProgramTokens = 0;
  }

  /**
   * Zapisuje oba plany i wiersz zestawienia. Nie wywoluje `env.execute()` — plan powstaje
   * z transformacji, bez uruchomienia joba i bez jakiegokolwiek pomiaru.
   */
  public static void dump(StreamExecutionEnvironment env, String family, String variant, int q, String outDir)
      throws IOException {
    StreamGraph streamGraph = env.getStreamGraph(false);

    Path plans = Paths.get(outDir, "plans");
    Path results = Paths.get(outDir, "results");
    Files.createDirectories(plans);
    Files.createDirectories(results);
    String stem = family + "_" + variant + "_Q" + q;

    // 1. Plan LOGICZNY — graf strumieniowy w postaci, w ktorej podaje go sam Flink.
    Files.writeString(plans.resolve(stem + "_logical.json"), streamGraph.getStreamingPlanAsJSON() + "\n",
        StandardCharsets.UTF_8);

    // 2. Plan logiczny w postaci tabelarycznej — wezel, rownoleglosc, krawedzie.
    List<StreamNode> nodes = new ArrayList<>(streamGraph.getStreamNodes());
    nodes.sort((x, y) -> Integer.compare(x.getId(), y.getId()));
    StringBuilder logical = new StringBuilder("node_id\toperator\tparallelism\trole\tinputs\n");
    for (StreamNode node : nodes) {
      StringBuilder inputs = new StringBuilder();
      for (StreamEdge edge : node.getInEdges()) {
        if (inputs.length() > 0) {
          inputs.append(',');
        }
        inputs.append(edge.getSourceId());
      }
      logical.append(node.getId()).append('\t').append(node.getOperatorName()).append('\t')
          .append(node.getParallelism()).append('\t').append(role(node.getOperatorName())).append('\t')
          .append(inputs.length() == 0 ? "-" : inputs).append('\n');
    }
    Files.writeString(plans.resolve(stem + "_logical.tsv"), logical.toString(), StandardCharsets.UTF_8);

    // 3. Plan FIZYCZNY — wierzcholki JobGraphu po scaleniu lancuchow operatorow.
    //    Lancuchowanie NIE jest wylaczane: §10 zabrania blokowania optymalizacji Flinka.
    JobGraph jobGraph = streamGraph.getJobGraph();
    StringBuilder physical = new StringBuilder("vertex_id\tvertex_name\tparallelism\tchained_operators\n");
    int vertices = 0;
    for (JobVertex vertex : jobGraph.getVerticesSortedTopologicallyFromSources()) {
      vertices++;
      physical.append(vertex.getID()).append('\t').append(vertex.getName().replace('\n', ' ')).append('\t')
          .append(vertex.getParallelism()).append('\t').append(vertex.getOperatorIDs().size()).append('\n');
    }
    Files.writeString(plans.resolve(stem + "_physical.tsv"), physical.toString(), StandardCharsets.UTF_8);

    // 4. Zliczenie instancji badanego podplanu + kontrola rejestru wobec grafu.
    Map<String, Node> fromGraph = new TreeMap<>();
    int sinks = 0;
    int sources = 0;
    for (StreamNode node : nodes) {
      String name = stripFlinkPrefix(node.getOperatorName());
      if (name.startsWith("SUB:") || name.startsWith("PUB:")) {
        Node entry = registry.get(name);
        if (entry == null) {
          throw new IllegalStateException("operator poza rejestrem: " + name);
        }
        fromGraph.put(name, entry);
      } else if (name.startsWith("SINK:")) {
        sinks++;
      } else if (name.startsWith("SRC:")) {
        sources++;
      } else {
        throw new IllegalStateException("operator poza konwencja nazw: " + name);
      }
    }
    if (fromGraph.size() != registry.size()) {
      throw new IllegalStateException("rejestr ma " + registry.size() + " operatorow, graf " + fromGraph.size());
    }
    if (costlyProgramTokens == 0) {
      throw new IllegalStateException("job nie zadeklarowal kosztownego programu (PlanDump.costlyProgram)");
    }

    // 5. Praca przewidziana z planu. Wielkosci wazone rate'em, w jednostkach `n_h`, tak samo
    //    jak bajty: `evalCalls` na jednostke slotow przeplotu. To jest arytmetyka planu —
    //    licznik runtime (Canon.workReport) czyta te same liczby w P6.
    int subplanNodes = 0;
    int publicStages = 0;
    double units = 0;
    double evalCalls = 0;
    double evalTokens = 0;
    double costlyEvals = 0;
    double hashPicks = 0;
    double addMerges = 0;
    StringBuilder breakdown = new StringBuilder("family\tvariant\tq\tnode\trole\tunit_weight\tprogram_tokens\tkind\n");
    for (Map.Entry<String, Node> e : fromGraph.entrySet()) {
      Node n = e.getValue();
      if (n.substrate()) {
        subplanNodes++;
        units += n.unitWeight();
      } else {
        publicStages++;
      }
      evalCalls += n.unitWeight();
      evalTokens += n.unitWeight() * n.programTokens();
      if (n.programTokens() == costlyProgramTokens) {
        costlyEvals += n.unitWeight();
      }
      if (n.kind() == Kind.INTERLEAVE) {
        hashPicks += n.unitWeight();
      }
      if (n.kind() == Kind.ADD) {
        addMerges += n.unitWeight();
      }
      breakdown.append(family).append('\t').append(variant).append('\t').append(q).append('\t').append(e.getKey())
          .append('\t').append(n.substrate() ? "badany_podplan" : "publiczny_monitor").append('\t')
          .append(fmt(n.unitWeight())).append('\t').append(n.programTokens()).append('\t').append(n.kind()).append('\n');
    }
    Files.writeString(plans.resolve(stem + "_subplan_nodes.tsv"), breakdown.toString(), StandardCharsets.UTF_8);

    Path work = results.resolve("flink_work.tsv");
    if (!Files.exists(work)) {
      Files.writeString(work,
          "family\tvariant\tq\tcostly_program_tokens\tcostly_evals_nh\teval_calls_nh\teval_tokens_nh"
              + "\thash_picks_nh\tadd_merges_nh\n",
          StandardCharsets.UTF_8);
    }
    Files.writeString(work,
        String.join("\t", family, variant, String.valueOf(q), String.valueOf(costlyProgramTokens), fmt(costlyEvals),
            fmt(evalCalls), fmt(evalTokens), fmt(hashPicks), fmt(addMerges)) + "\n",
        StandardCharsets.UTF_8, java.nio.file.StandardOpenOption.APPEND);

    String row = String.join("\t", family, variant, String.valueOf(q), String.valueOf(subplanNodes), fmt(units),
        String.valueOf(publicStages), String.valueOf(sinks), String.valueOf(sources), String.valueOf(nodes.size()),
        String.valueOf(vertices), String.valueOf(K26Ops.W)) + "\n";
    Path summary = results.resolve("flink_instances.tsv");
    if (!Files.exists(summary)) {
      Files.writeString(summary,
          "family\tvariant\tq\tsubplan_nodes\tsubplan_units_nh\tpublic_stages\tsinks\tsources"
              + "\tstream_nodes\tjob_vertices\tcanonical_record_bytes\n",
          StandardCharsets.UTF_8);
    }
    Files.writeString(summary, row, StandardCharsets.UTF_8, java.nio.file.StandardOpenOption.APPEND);

    System.out.println(family + " " + variant + " Q=" + q + ": wezly podplanu=" + subplanNodes + " jednostki="
        + fmt(units) + " kosztowny_program/slot=" + fmt(costlyEvals) + " eval_tokeny=" + fmt(evalTokens)
        + " etapy publiczne=" + publicStages + " sinki=" + sinks + " zrodla=" + sources + " wezly grafu="
        + nodes.size() + " wierzcholki JobGraphu=" + vertices);
  }

  private static String fmt(double value) {
    return String.format(java.util.Locale.ROOT, "%.4f", value);
  }

  /**
   * Flink dokleja do nazwy operatora prefiks roli ("Source: ", "Sink: "). Nazwa nadana przez
   * job zostaje bez zmian po tym prefiksie, wiec konwencja nazw czyta sie po jego odjeciu.
   */
  private static String stripFlinkPrefix(String operatorName) {
    for (String prefix : new String[] {"Source: ", "Sink: "}) {
      if (operatorName.startsWith(prefix)) {
        return operatorName.substring(prefix.length());
      }
    }
    return operatorName;
  }

  private static String role(String rawName) {
    String operatorName = stripFlinkPrefix(rawName);
    if (operatorName.startsWith("SUB:")) {
      return "badany_podplan";
    }
    if (operatorName.startsWith("PUB:")) {
      return "publiczny_monitor";
    }
    if (operatorName.startsWith("SINK:")) {
      return "sink_poza_metryka";
    }
    if (operatorName.startsWith("SRC:")) {
      return "ingress_poza_metryka";
    }
    return "NIEZNANA";
  }
}

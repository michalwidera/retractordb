// Rodzina F9-R2 — przemienny multi-sensor feature. Dwa warianty jednego joba.
//
// Postacie monitorow odwzorowuja postacie RQL z SZKIC_RODZIN.md §4.2:
//   P1: SELECT Sqrt(A[0]*A[0]+B[0]*B[0]) STREAM m_i FROM A+B
//   P2: SELECT Sqrt(A[0]*A[0]+B[0]*B[0]) STREAM m_i FROM B+A
// Rownowazne, lecz strukturalnie rozne: DataStream nie ma normalizacji algebraicznej, wiec
// `A.connect(B)` i `B.connect(A)` sa dla niego dwoma roznymi podplanami. Tak samo, jak dla
// dwoch niezaleznych autorow, z ktorych zaden nie wie o pozostalych Q-1 monitorach.
//
// FLINK_NATURAL: Q niezaleznie nazwanych monitorow, kazdy z wlasnym egzemplarzem swojego
//                podplanu; BEZ recznego wydzielania czegokolwiek.
// FLINK_MANUAL:  ten sam job po recznym wydzieleniu wspolnego wezla — kontrola best case,
//                niewchodzaca do progu (§10).
//
// Wspolne dla obu: rownoleglosc 1, te same zrodla, ten sam czas logiczny (indeks slotu),
// te same schematy (jedno pole INTEGER = 9 B kanonicznie) i ta sama funkcja.
import org.apache.flink.api.java.tuple.Tuple3;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.datastream.SingleOutputStreamOperator;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;

public class F9R2Job {

  private static final int F_MAX = 2;

  public static void main(String[] args) throws Exception {
    String variant = K26Ops.arg(args, "--variant", "natural");
    int q = Integer.parseInt(K26Ops.arg(args, "--q", "8"));
    long slots = Long.parseLong(K26Ops.arg(args, "--slots", "1200"));
    String outDir = K26Ops.arg(args, "--out-dir", ".");
    String sinkDir = K26Ops.arg(args, "--sink-dir", "/dev/shm");
    boolean planOnly = K26Ops.flag(args, "--plan-only");

    int[] axisX = K26Ops.loadOrRamp(K26Ops.arg(args, "--a", null));
    int[] axisY = K26Ops.loadOrRamp(K26Ops.arg(args, "--b", null));

    StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
    env.setParallelism(1);
    PlanDump.reset();
    PlanDump.costlyProgram(K26Ops.TOKENS_SQRT_TWO_TERMS);

    // Oba zrodla 100 Hz — os X i os Y tego samego czujnika drgan (SZKIC_RODZIN.md §4.1).
    DataStream<Tuple3<Long, Long, Integer>> srcA =
        env.addSource(new K26Ops.DeclaredSource(axisX, slots, K26Ops.TAG_A)).name("SRC:A").uid("SRC:A");
    DataStream<Tuple3<Long, Long, Integer>> srcB =
        env.addSource(new K26Ops.DeclaredSource(axisY, slots, K26Ops.TAG_B)).name("SRC:B").uid("SRC:B");

    if (variant.equals("manual")) {
      // Inzynier ZAUWAZA rownowaznosc A+B i B+A i wydziela wspolny wezel recznie.
      DataStream<Tuple3<Long, Long, Integer>> shared = PlanDump.sub(
          srcA.connect(srcB).process(new K26Ops.AddFeature(true)),
          "shared:select", PlanDump.UNIT_100, K26Ops.TOKENS_SQRT_TWO_TERMS, PlanDump.Kind.ADD);
      for (int i = 0; i < q; i++) {
        monitorTail(shared, i, sinkDir);
      }
    } else {
      for (int i = 0; i < q; i++) {
        int form = K26Ops.formOf(i, q, F_MAX);
        DataStream<Tuple3<Long, Long, Integer>> left = (form == 0) ? srcA : srcB;
        DataStream<Tuple3<Long, Long, Integer>> right = (form == 0) ? srcB : srcA;
        DataStream<Tuple3<Long, Long, Integer>> own = PlanDump.sub(
            left.connect(right).process(new K26Ops.AddFeature(true)),
            "m" + (i + 1) + ":select_P" + (form + 1), PlanDump.UNIT_100, K26Ops.TOKENS_SQRT_TWO_TERMS,
            PlanDump.Kind.ADD);
        monitorTail(own, i, sinkDir);
      }
    }

    PlanDump.dump(env, "F9-R2", variant, q, outDir);
    if (!planOnly) {
      env.execute("k26-f9r2-" + variant + "-q" + q);
      System.out.println(Canon.logicalReport());
      System.out.println(Canon.workReport());
    }
  }

  /** Etap publiczny monitora + jego wlasny sink. Odpowiednik `m_i :- PUSH_STREAM(substrat)`. */
  private static void monitorTail(DataStream<Tuple3<Long, Long, Integer>> upstream, int i, String sinkDir) {
    String monitor = "m" + (i + 1);
    SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stage = upstream.map(new K26Ops.MonitorOutput());
    PlanDump.pub(stage, monitor, PlanDump.UNIT_100, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP)
        .addSink(new K26Ops.MonitorSink(sinkDir + "/f9r2_" + monitor + ".csv", monitor))
        .name("SINK:" + monitor).uid("SINK:" + monitor);
  }
}

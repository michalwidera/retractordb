// Rodzina F9-R1 — rational-rate delayed fusion. Dwa warianty jednego joba.
//
// Postacie monitorow odwzorowuja postacie RQL z SZKIC_RODZIN.md §5.2, ze stalymi
// rozstrzygnietymi przez czlowieka 2026-08-08 (§9 pkt 4):
//   Delta_A = 1/100, Delta_B = 1/50, i = 2, k = 1, przesuniecie laczne 3.
//   P1: SELECT m_i[0]*m_i[0] STREAM m_i FROM (A>2)#(B>1)   "skompensuj tor, potem przeplataj"
//   P2: SELECT m_i[0]*m_i[0] STREAM m_i FROM (A#B)>3       "przeplataj, potem skompensuj"
// Warunek reguly: i*Delta_A = 2/100 = 1/50 = k*Delta_B — spelniony.
//
// Podzial na wezly odwzorowuje plan RQL (SZKIC_RODZIN.md §5.3):
//   P1 — substraty `A>2` i `B>1`; szczytowy `#` razem z programem pola jest ETAPEM PUBLICZNYM,
//   P2 — substrat `A#B`; szczytowe `>3` razem z programem pola jest ETAPEM PUBLICZNYM
//        (przesuniecie laczne zostaje w programie monitora, wiec nie wnosi zapisow).
import org.apache.flink.api.java.tuple.Tuple3;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.datastream.SingleOutputStreamOperator;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;

public class F9R1Job {

  private static final int F_MAX = 2;
  private static final int SHIFT_A = 2;
  private static final int SHIFT_B = 1;
  private static final int SHIFT_HASH = 3;
  // Kroki wspolnej siatki 1/100: A co 1 jednostke (100 Hz), B co 2 (50 Hz).
  private static final int UNIT_STEP_A = 1;
  private static final int UNIT_STEP_B = 2;

  public static void main(String[] args) throws Exception {
    String variant = K26Ops.arg(args, "--variant", "natural");
    int q = Integer.parseInt(K26Ops.arg(args, "--q", "8"));
    long slots = Long.parseLong(K26Ops.arg(args, "--slots", "1200"));
    String outDir = K26Ops.arg(args, "--out-dir", ".");
    String sinkDir = K26Ops.arg(args, "--sink-dir", "/dev/shm");
    boolean planOnly = K26Ops.flag(args, "--plan-only");

    int[] vibration = K26Ops.loadOrRamp(K26Ops.arg(args, "--a", null));
    int[] current = K26Ops.loadOrRamp(K26Ops.arg(args, "--b", null));

    StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
    env.setParallelism(1);
    PlanDump.reset();
    // W F9-R1 kosztowny program pol to `m[0]*m[0]` (3 tokeny). Uwaga metodyczna: ta rodzina
    // NIE rozdziela sie liczba wykonan programu — kazdy monitor liczy swoj kwadrat w kazdym
    // profilu. Wielkoscia rozdzielajaca jest tu praca przeplotu (`hash_picks_nh`).
    PlanDump.costlyProgram(K26Ops.TOKENS_SQUARE);

    // A = drgania 100 Hz, B = prad 50 Hz — na tym samym czasie logicznym B ma polowe rekordow.
    DataStream<Tuple3<Long, Long, Integer>> srcA =
        env.addSource(new K26Ops.DeclaredSource(vibration, slots, K26Ops.TAG_A)).name("SRC:A").uid("SRC:A");
    DataStream<Tuple3<Long, Long, Integer>> srcB =
        env.addSource(new K26Ops.DeclaredSource(current, slots / 2, K26Ops.TAG_B)).name("SRC:B").uid("SRC:B");

    if (variant.equals("manual")) {
      // Inzynier ZAUWAZA tozsamosc (A>2)#(B>1) = (A#B)>3 i wydziela jeden wspolny przeplot.
      DataStream<Tuple3<Long, Long, Integer>> hash = PlanDump.sub(
          srcA.connect(srcB).process(new K26Ops.Interleave(UNIT_STEP_A, UNIT_STEP_B, true, false)),
          "shared:hash", PlanDump.UNIT_150, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.INTERLEAVE);
      for (int i = 0; i < q; i++) {
        publicShiftSquare(hash, i, sinkDir);
      }
    } else {
      for (int i = 0; i < q; i++) {
        int form = K26Ops.formOf(i, q, F_MAX);
        String monitor = "m" + (i + 1);
        if (form == 0) {
          // P1: wlasne substraty przesuniec, przeplot w etapie publicznym monitora.
          DataStream<Tuple3<Long, Long, Integer>> shiftedA = PlanDump.sub(
              srcA.map(new K26Ops.Shift(SHIFT_A, true, false)), monitor + ":shift_A", PlanDump.UNIT_100,
              K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP);
          DataStream<Tuple3<Long, Long, Integer>> shiftedB = PlanDump.sub(
              srcB.map(new K26Ops.Shift(SHIFT_B, true, false)), monitor + ":shift_B", PlanDump.UNIT_50,
              K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP);
          SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stage = shiftedA.connect(shiftedB)
              .process(new K26Ops.Interleave(UNIT_STEP_A, UNIT_STEP_B, false, true));
          sink(PlanDump.pub(stage, monitor, PlanDump.UNIT_150, K26Ops.TOKENS_SQUARE,
              PlanDump.Kind.INTERLEAVE), monitor, sinkDir);
        } else {
          // P2: wlasny substrat przeplotu, przesuniecie laczne w etapie publicznym.
          DataStream<Tuple3<Long, Long, Integer>> hash = PlanDump.sub(
              srcA.connect(srcB).process(new K26Ops.Interleave(UNIT_STEP_A, UNIT_STEP_B, true, false)),
              monitor + ":hash", PlanDump.UNIT_150, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.INTERLEAVE);
          publicShiftSquare(hash, i, sinkDir);
        }
      }
    }

    PlanDump.dump(env, "F9-R1", variant, q, outDir);
    if (!planOnly) {
      env.execute("k26-f9r1-" + variant + "-q" + q);
      System.out.println(Canon.logicalReport());
      System.out.println(Canon.workReport());
    }
  }

  private static void publicShiftSquare(DataStream<Tuple3<Long, Long, Integer>> hash, int i, String sinkDir) {
    String monitor = "m" + (i + 1);
    SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stage =
        hash.map(new K26Ops.Shift(SHIFT_HASH, false, true));
    sink(PlanDump.pub(stage, monitor, PlanDump.UNIT_150, K26Ops.TOKENS_SQUARE, PlanDump.Kind.MAP), monitor,
        sinkDir);
  }

  private static void sink(DataStream<Tuple3<Long, Long, Integer>> stage, String monitor, String sinkDir) {
    stage.addSink(new K26Ops.MonitorSink(sinkDir + "/f9r1_" + monitor + ".csv", monitor))
        .name("SINK:" + monitor).uid("SINK:" + monitor);
  }
}

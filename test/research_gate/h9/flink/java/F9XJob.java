// Rodzina F9-X — zlozenie R1 -> R2. Dwa warianty jednego joba.
//
// Cztery postacie z SZKIC_RODZIN.md §6.2 powstaja z DWOCH niezaleznych, arbitralnych decyzji
// autora: postac R1 kazdej pary x kolejnosc par w sumie.
//   W1  FROM ((A>2)#(B>1)) + ((C>2)#(D>1))
//   W2  FROM ((C>2)#(D>1)) + ((A>2)#(B>1))
//   W3  FROM ((A#B)>3)     + ((C#D)>3)
//   W4  FROM ((C#D)>3)     + ((A#B)>3)
// Zamrozona kolejnosc postaci przy redukcji F(Q): W1, W4, W2, W3 — pierwsza para rozni sie
// w OBU wymiarach naraz, wiec przy Q=4 rodzina nadal dotyka obu mechanizmow.
//
// Postac zlozenia INLINE jest ZAMROZONA dla wszystkich Q monitorow (D-3, SZKIC_D3.md §3.3):
// nazwanie strumieni posrednich kasuje warstwe R2 i przenosi materializacje do strumieni
// PUBLICZNYCH, czyli z licznika metryki do mianownika (RAPORT_PILOTA.md §6a). Po stronie
// Flinka odpowiada temu to, ze pary zlozone NIE maja wlasnego sinka — sink ma wylacznie
// monitor.
//
// Program pola czyta wspolne schematy WYNIKOW przeplotu: `front` z A#B i `rear` z C#D.
// W szybkim slocie liczy norme (A,C), w wolnym (B,D). Nie zachowuje zatrzaskow A-D.
import org.apache.flink.api.java.tuple.Tuple3;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.datastream.SingleOutputStreamOperator;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;

public class F9XJob {

  private static final int F_MAX = 4;
  private static final int SHIFT_FAST = 2;
  private static final int SHIFT_SLOW = 1;
  private static final int SHIFT_HASH = 3;
  private static final int UNIT_STEP_FAST = 1;
  private static final int UNIT_STEP_SLOW = 2;

  /** Zamrozona kolejnosc postaci: W1, W4, W2, W3 (SZKIC_RODZIN.md §6.2). */
  private static final String[] FORM_ORDER = {"W1", "W4", "W2", "W3"};

  public static void main(String[] args) throws Exception {
    String variant = K26Ops.arg(args, "--variant", "natural");
    int q = Integer.parseInt(K26Ops.arg(args, "--q", "8"));
    long slots = Long.parseLong(K26Ops.arg(args, "--slots", "1200"));
    String outDir = K26Ops.arg(args, "--out-dir", ".");
    String sinkDir = K26Ops.arg(args, "--sink-dir", "/dev/shm");
    boolean planOnly = K26Ops.flag(args, "--plan-only");

    int[] frontVib = K26Ops.loadOrRamp(K26Ops.arg(args, "--a", null));
    int[] frontCur = K26Ops.loadOrRamp(K26Ops.arg(args, "--b", null));
    int[] rearVib = K26Ops.loadOrRamp(K26Ops.arg(args, "--c", null));
    int[] rearCur = K26Ops.loadOrRamp(K26Ops.arg(args, "--d", null));

    StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
    env.setParallelism(1);
    PlanDump.reset();
    PlanDump.costlyProgram(K26Ops.TOKENS_SQRT_TWO_TERMS);

    // Lozysko przednie (A, B) i tylne (C, D); w kazdej parze ten sam uklad taktow co w F9-R1.
    DataStream<Tuple3<Long, Long, Integer>> srcA =
        env.addSource(new K26Ops.DeclaredSource(frontVib, slots, K26Ops.TAG_A)).name("SRC:A").uid("SRC:A");
    DataStream<Tuple3<Long, Long, Integer>> srcB =
        env.addSource(new K26Ops.DeclaredSource(frontCur, slots / 2, K26Ops.TAG_B)).name("SRC:B").uid("SRC:B");
    DataStream<Tuple3<Long, Long, Integer>> srcC =
        env.addSource(new K26Ops.DeclaredSource(rearVib, slots, K26Ops.TAG_C)).name("SRC:C").uid("SRC:C");
    DataStream<Tuple3<Long, Long, Integer>> srcD =
        env.addSource(new K26Ops.DeclaredSource(rearCur, slots / 2, K26Ops.TAG_D)).name("SRC:D").uid("SRC:D");

    if (variant.equals("manual")) {
      // Reczne wydzielenie do postaci, ktora DEFAULT osiaga sam: dwa przeploty, dwa
      // przesuniecia laczne i jeden wspolny wezel cechy (5 wezlow — SZKIC_RODZIN.md §6.3).
      DataStream<Tuple3<Long, Long, Integer>> sharedAB = hashThenShift(srcA, srcB, "shared", "AB");
      DataStream<Tuple3<Long, Long, Integer>> sharedCD = hashThenShift(srcC, srcD, "shared", "CD");
      DataStream<Tuple3<Long, Long, Integer>> select = PlanDump.sub(
          sharedAB.connect(sharedCD).process(new K26Ops.AddFeature(true)),
          "shared:select", PlanDump.UNIT_150, K26Ops.TOKENS_SQRT_TWO_TERMS, PlanDump.Kind.ADD);
      for (int i = 0; i < q; i++) {
        String monitor = "m" + (i + 1);
        SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stage = select.map(new K26Ops.MonitorOutput());
        sink(PlanDump.pub(stage, monitor, PlanDump.UNIT_150, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP),
            monitor, sinkDir);
      }
    } else {
      for (int i = 0; i < q; i++) {
        String monitor = "m" + (i + 1);
        String form = FORM_ORDER[K26Ops.formOf(i, q, F_MAX)];
        boolean shiftFirst = form.equals("W1") || form.equals("W2");
        boolean frontFirst = form.equals("W1") || form.equals("W3");

        DataStream<Tuple3<Long, Long, Integer>> pairAB = shiftFirst
            ? shiftThenHash(srcA, srcB, monitor, "AB")
            : hashThenShift(srcA, srcB, monitor, "AB");
        DataStream<Tuple3<Long, Long, Integer>> pairCD = shiftFirst
            ? shiftThenHash(srcC, srcD, monitor, "CD")
            : hashThenShift(srcC, srcD, monitor, "CD");

        DataStream<Tuple3<Long, Long, Integer>> left = frontFirst ? pairAB : pairCD;
        DataStream<Tuple3<Long, Long, Integer>> right = frontFirst ? pairCD : pairAB;

        // Szczytowy `+` z programem Sqrt(front^2 + rear^2) — etap PUBLICZNY monitora.
        SingleOutputStreamOperator<Tuple3<Long, Long, Integer>> stage = left.connect(right)
            .process(new K26Ops.AddFeature(false));
        sink(PlanDump.pub(stage, monitor + "_" + form, PlanDump.UNIT_150, K26Ops.TOKENS_SQRT_TWO_TERMS,
            PlanDump.Kind.ADD), monitor, sinkDir);
      }
    }

    PlanDump.dump(env, "F9-X", variant, q, outDir);
    if (!planOnly) {
      env.execute("k26-f9x-" + variant + "-q" + q);
      System.out.println(Canon.logicalReport());
      System.out.println(Canon.workReport());
    }
  }

  /** Postac "skompensuj kazdy tor, potem przeplataj": ((X>2)#(Y>1)). Trzy wezly substratu. */
  private static DataStream<Tuple3<Long, Long, Integer>> shiftThenHash(
      DataStream<Tuple3<Long, Long, Integer>> fast, DataStream<Tuple3<Long, Long, Integer>> slow, String owner,
      String pair) {
    DataStream<Tuple3<Long, Long, Integer>> shiftedFast = PlanDump.sub(
        fast.map(new K26Ops.Shift(SHIFT_FAST, true, false)), owner + ":shift_" + pair.charAt(0),
        PlanDump.UNIT_100, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP);
    DataStream<Tuple3<Long, Long, Integer>> shiftedSlow = PlanDump.sub(
        slow.map(new K26Ops.Shift(SHIFT_SLOW, true, false)), owner + ":shift_" + pair.charAt(1),
        PlanDump.UNIT_50, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP);
    return PlanDump.sub(
        shiftedFast.connect(shiftedSlow).process(new K26Ops.Interleave(UNIT_STEP_FAST, UNIT_STEP_SLOW, true, false)),
        owner + ":hash_" + pair, PlanDump.UNIT_150, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.INTERLEAVE);
  }

  /** Postac "przeplataj, potem skompensuj wspolne opoznienie": ((X#Y)>3). Dwa wezly substratu. */
  private static DataStream<Tuple3<Long, Long, Integer>> hashThenShift(
      DataStream<Tuple3<Long, Long, Integer>> fast, DataStream<Tuple3<Long, Long, Integer>> slow, String owner,
      String pair) {
    DataStream<Tuple3<Long, Long, Integer>> hash = PlanDump.sub(
        fast.connect(slow).process(new K26Ops.Interleave(UNIT_STEP_FAST, UNIT_STEP_SLOW, true, false)),
        owner + ":hash_" + pair, PlanDump.UNIT_150, K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.INTERLEAVE);
    return PlanDump.sub(
        hash.map(new K26Ops.Shift(SHIFT_HASH, true, false)), owner + ":shift_" + pair, PlanDump.UNIT_150,
        K26Ops.TOKENS_PASSTHROUGH, PlanDump.Kind.MAP);
  }

  private static void sink(DataStream<Tuple3<Long, Long, Integer>> stage, String monitor, String sinkDir) {
    stage.addSink(new K26Ops.MonitorSink(sinkDir + "/f9x_" + monitor + ".csv", monitor))
        .name("SINK:" + monitor).uid("SINK:" + monitor);
  }
}

// =============================================================================
// PanTompkinsFlinkJob.java -- baseline Flink (krok 2 kampanii baseline-flink)
// (REQUIREMENTS.md, "Plan badawczy -- kampanie baseline"; cel: 7.5(ii)
// main-debs.tex). Rownowazny dataflow Pan-Tompkinsa dla Apache Flink,
// strukturalnie identyczny z config/pan_tompkins_numpy.py --mode slot (te
// same okna: band-pass 25, roznica 5, MWI 30, prog 180; ta sama formula
// detect_out), zeby wynik byl porownywalny z kampania baseline-numpy.
//
// Tryb: local/MiniCluster, jeden JVM (uruchamiany przez
// `bin/flink run -t local`), parallelism=1 na wszystkich operatorach,
// checkpointing wylaczony (nieustawiony execution.checkpointing.interval).
//
// Sonda CSV w formacie zgodnym z RDB_BENCH_CSV (iter,compute_ns,wake_lag_ns,
// e2e_ns) -- analiza tym samym examples/ecg/e1_stats.py. UWAGA
// interpretacyjna (do odnotowania w wynikach/dzienniku): dzieki chainowaniu
// operatorow bez shuffle (map->map->...->sink, ta sama parallelism, brak
// keyBy) caly potok obliczeniowy dziala w JEDNYM wątku Flinka; compute_ns
// mierzy od odebrania rekordu przez zrodlo (tStart, PO pacingu) do konca
// ostatniego etapu obliczen (tEnd, PRZED sinkiem) -- w tym jedno przejscie
// przez kolejke Source->Chain (Flink nie chainuje zrodel), czego NIE ma
// numpy/silnik (tam wszystko w jednym wywolaniu funkcji). Roznica
// metodyczna, nie blad -- flagowana zamiast ukrywana.
// =============================================================================

import org.apache.flink.api.common.functions.OpenContext;
import org.apache.flink.api.common.functions.RichMapFunction;
import org.apache.flink.api.java.tuple.Tuple8;
import org.apache.flink.streaming.api.datastream.DataStream;
import org.apache.flink.streaming.api.environment.StreamExecutionEnvironment;
import org.apache.flink.streaming.api.functions.sink.legacy.RichSinkFunction;
import org.apache.flink.streaming.api.functions.source.legacy.RichSourceFunction;
import org.apache.flink.streaming.api.functions.source.legacy.SourceFunction;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.List;

/**
 * Rec: (iter, deadlineNs, wakeLagNs, tStartNs, mliiRaw, v1Raw, stage, tEndNs).
 * "stage" to biezaca wartosc payloadu przechodzaca przez etapy (jak w
 * pan_tompkins_numpy.py); mliiRaw/v1Raw niesione bez zmian do formatu linii
 * wyjsciowej sinka.
 */
public class PanTompkinsFlinkJob {

  static double[] loadCoef(String path) throws Exception {
    List<Double> vals = new ArrayList<>();
    java.io.BufferedReader r = new java.io.BufferedReader(new java.io.FileReader(path));
    String line;
    while ((line = r.readLine()) != null) {
      line = line.trim();
      if (!line.isEmpty()) {
        vals.add(Double.parseDouble(line));
      }
    }
    r.close();
    double[] out = new double[vals.size()];
    for (int i = 0; i < out.length; i++) {
      out[i] = vals.get(i);
    }
    return out;
  }

  /** Wczytuje rec205 (int32 LE, pary MLII,V1) -- ten sam format co load_inputs() w numpy. */
  static double[][] loadRec(String path) throws Exception {
    RandomAccessFile raf = new RandomAccessFile(path, "r");
    long len = raf.length();
    int nRec = (int) (len / 8); // 2 x int32 (4B) per rekord
    ByteBuffer buf = ByteBuffer.allocate((int) len).order(ByteOrder.LITTLE_ENDIAN);
    FileChannel ch = raf.getChannel();
    while (buf.hasRemaining() && ch.read(buf) >= 0) {
      // dopoki nie wczytamy calego pliku
    }
    buf.flip();
    double[] mlii = new double[nRec];
    double[] v1 = new double[nRec];
    for (int i = 0; i < nRec; i++) {
      mlii[i] = buf.getInt();
      v1[i] = buf.getInt();
    }
    raf.close();
    return new double[][] {mlii, v1};
  }

  static class PacedSource extends RichSourceFunction<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> {
    private final double[] mlii;
    private final double[] v1;
    private final double rateHz;
    private final long samples;
    private volatile boolean running = true;

    PacedSource(double[] mlii, double[] v1, double rateHz, long samples) {
      this.mlii = mlii;
      this.v1 = v1;
      this.rateHz = rateHz;
      this.samples = samples;
    }

    @Override
    public void run(SourceContext<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> ctx) throws Exception {
      long periodNs = Math.round(1e9 / rateHz);
      long t0 = System.nanoTime();
      int nRec = mlii.length;
      for (long n = 0; n < samples && running; n++) {
        long deadline = t0 + n * periodNs;
        long now = System.nanoTime();
        if (now < deadline) {
          long sleepNs = deadline - now;
          Thread.sleep(sleepNs / 1_000_000L, (int) (sleepNs % 1_000_000L));
          now = System.nanoTime();
        }
        long wakeLag = now - deadline;
        long tStart = now;
        int idx = (int) (n % nRec);
        double x = mlii[idx];
        double xv = v1[idx];
        ctx.collect(Tuple8.of(n, deadline, wakeLag, tStart, x, xv, x, 0L));
      }
    }

    @Override
    public void cancel() {
      running = false;
    }
  }

  /** Band-pass: okno len(bp), dot/1000.0 -- identyczne z run_slot (win_bp). */
  static class BandPass extends RichMapFunction<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>,
      Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> {
    private final double[] coef;
    private double[] win;

    BandPass(double[] coef) {
      this.coef = coef;
    }

    @Override
    public void open(OpenContext ctx) {
      win = new double[coef.length];
    }

    @Override
    public Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> map(
        Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> in) {
      System.arraycopy(win, 1, win, 0, win.length - 1);
      win[win.length - 1] = in.f4; // mliiRaw
      double acc = 0.0;
      for (int i = 0; i < win.length; i++) {
        acc += win[i] * coef[i];
      }
      double bpOut = acc / 1000.0;
      return Tuple8.of(in.f0, in.f1, in.f2, in.f3, in.f4, in.f5, bpOut, in.f7);
    }
  }

  /** Roznica: okno len(d), dot -- identyczne z run_slot (win_d), na wyjsciu band-passu. */
  static class Derivative extends RichMapFunction<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>,
      Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> {
    private final double[] coef;
    private double[] win;

    Derivative(double[] coef) {
      this.coef = coef;
    }

    @Override
    public void open(OpenContext ctx) {
      win = new double[coef.length];
    }

    @Override
    public Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> map(
        Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> in) {
      System.arraycopy(win, 1, win, 0, win.length - 1);
      win[win.length - 1] = in.f6; // bp_out
      double acc = 0.0;
      for (int i = 0; i < win.length; i++) {
        acc += win[i] * coef[i];
      }
      double dOut = acc;
      return Tuple8.of(in.f0, in.f1, in.f2, in.f3, in.f4, in.f5, dOut, in.f7);
    }
  }

  /** Kwadrat/1000 + MWI (srednia z 30) -- identyczne z run_slot (sq, win_mwi). */
  static class SquareMwi extends RichMapFunction<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>,
      Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> {
    private static final int WIN = 30;
    private double[] win;

    @Override
    public void open(OpenContext ctx) {
      win = new double[WIN];
    }

    @Override
    public Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> map(
        Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> in) {
      double dOut = in.f6;
      double sq = dOut * dOut / 1000.0;
      System.arraycopy(win, 1, win, 0, win.length - 1);
      win[win.length - 1] = sq;
      double sum = 0.0;
      for (double v : win) {
        sum += v;
      }
      double mwi = sum / WIN;
      return Tuple8.of(in.f0, in.f1, in.f2, in.f3, in.f4, in.f5, mwi, in.f7);
    }
  }

  /** Prog (srednia z 180) + detect_out; ustawia tEnd -- koniec obliczen, przed sinkiem. */
  static class Threshold extends RichMapFunction<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>,
      Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> {
    private static final int WIN = 180;
    private double[] win;

    @Override
    public void open(OpenContext ctx) {
      win = new double[WIN];
    }

    @Override
    public Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> map(
        Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> in) {
      double mwi = in.f6;
      System.arraycopy(win, 1, win, 0, win.length - 1);
      win[win.length - 1] = mwi;
      double sum = 0.0;
      for (double v : win) {
        sum += v;
      }
      double thr = sum / WIN;
      double detectOut = (mwi - 2.0 * thr) * 5.0;
      long tEnd = System.nanoTime();
      return Tuple8.of(in.f0, in.f1, in.f2, in.f3, in.f4, in.f5, detectOut, tEnd);
    }
  }

  /** Emisja: dopisuje linie do sink pliku (jak numpy sink.write) + wiersz sondy. */
  static class ProbeSink extends RichSinkFunction<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> {
    private final String sinkPath;
    private final String probePath;
    private transient BufferedWriter sinkWriter;
    private transient BufferedWriter probeWriter;

    ProbeSink(String sinkPath, String probePath) {
      this.sinkPath = sinkPath;
      this.probePath = probePath;
    }

    @Override
    public void open(OpenContext ctx) throws Exception {
      sinkWriter = new BufferedWriter(new FileWriter(sinkPath));
      probeWriter = new BufferedWriter(new FileWriter(probePath));
      probeWriter.write("iter,compute_ns,wake_lag_ns,e2e_ns\n");
    }

    @Override
    public void invoke(Tuple8<Long, Long, Long, Long, Double, Double, Double, Long> rec) throws Exception {
      long iter = rec.f0;
      long deadline = rec.f1;
      long wakeLag = rec.f2;
      long tStart = rec.f3;
      double mliiRaw = rec.f4;
      double v1Raw = rec.f5;
      double detectOut = rec.f6;
      long tEnd = rec.f7;

      sinkWriter.write((mliiRaw - 900.0) + " " + (v1Raw - 900.0) + " " + detectOut);
      sinkWriter.newLine();
      long tEmit = System.nanoTime();

      long computeNs = tEnd - tStart;
      long e2eNs = tEmit - deadline;
      probeWriter.write(iter + "," + computeNs + "," + wakeLag + "," + e2eNs);
      probeWriter.newLine();
    }

    @Override
    public void close() throws Exception {
      if (sinkWriter != null) sinkWriter.close();
      if (probeWriter != null) probeWriter.close();
    }
  }

  public static void main(String[] args) throws Exception {
    String rec = argVal(args, "--rec", null);
    String bpPath = argVal(args, "--bp", null);
    String dPath = argVal(args, "--d", null);
    double rateHz = Double.parseDouble(argVal(args, "--rate", "360"));
    long samples = Long.parseLong(argVal(args, "--samples", "20000"));
    String probePath = argVal(args, "--probe", "probe_flink.csv");
    String sinkPath = argVal(args, "--sink", "/dev/null");

    if (rec == null || bpPath == null || dPath == null) {
      throw new IllegalArgumentException("Wymagane: --rec --bp --d");
    }

    double[][] recData = loadRec(rec);
    double[] mlii = recData[0];
    double[] v1 = recData[1];
    double[] bpCoef = loadCoef(bpPath);
    double[] dCoef = loadCoef(dPath);

    System.out.println("META rec_len=" + mlii.length + " bp_len=" + bpCoef.length
        + " d_len=" + dCoef.length + " rate_hz=" + rateHz + " samples=" + samples);

    StreamExecutionEnvironment env = StreamExecutionEnvironment.getExecutionEnvironment();
    env.setParallelism(1);
    // Domyslne execution.buffer-timeout=100ms opoznia flush kanalu miedzy
    // operatorami do wypelnienia bufora lub uplywu timeoutu -- przy rzadkim
    // ruchu (360 Hz = 1 rekord/2,8ms, bufor sie nie wypelnia) kazdy rekord
    // moglby czekac do 100ms na flush. 0 = flush natychmiast po kazdym
    // rekordzie (rekomendacja Flinka dla low-latency; patrz JOURNAL.md).
    env.setBufferTimeout(0);

    DataStream<Tuple8<Long, Long, Long, Long, Double, Double, Double, Long>> src =
        env.addSource(new PacedSource(mlii, v1, rateHz, samples));

    src.map(new BandPass(bpCoef))
        .map(new Derivative(dCoef))
        .map(new SquareMwi())
        .map(new Threshold())
        .addSink(new ProbeSink(sinkPath, probePath));

    env.execute("pan-tompkins-flink-baseline");
    System.out.println("PROBE " + probePath);
  }

  static String argVal(String[] args, String key, String def) {
    for (int i = 0; i < args.length - 1; i++) {
      if (args[i].equals(key)) {
        return args[i + 1];
      }
    }
    return def;
  }
}

// solve.cpp — Hackathon Squad MWIS solver (interactive, 5-minute budget)
//
// Reads a graph from a file or stdin, runs the hybrid MWIS heuristic with
// progress reporting to stderr, prints a summary to the terminal, and writes
// the full answer (incl. selected coders) to an output file or stdout.
//
// Build:
//   g++ -O2 -std=c++17 -DNDEBUG -static -static-libgcc -static-libstdc++ \
//       solve.cpp -o build/solve
//
// Usage:
//   ./build/solve -i input.in                       # 5-min budget, answer to stdout
//   ./build/solve -i input.in -t 60                 # 60-second budget
//   ./build/solve -i input.in -o answer.out         # write answer to file
//   ./build/solve -i input.in -t 300 -o answer.out  # all options together
//   ./build/solve < input.in                        # pipe from stdin
//   ./build/solve                                   # interactive (Ctrl+D / Ctrl+Z to end)

#include <bits/stdc++.h>
#include <chrono>
using namespace std;
using namespace std::chrono;

// ---------------------------------------------------------------------------
// Solver — GWMIN + GWMIN2 multi-start, then ILS with (1,2)-swap local search
// Progress is printed to stderr every ~5 seconds.
// ---------------------------------------------------------------------------
class MWISSolver {
private:
    int N;
    vector<long long> weight;
    vector<vector<int>> adj;

    vector<char> inSet;
    vector<int>  tightness;
    long long    currentWeight = 0;
    int          currentSize   = 0;

    vector<int> bestSet;
    long long   bestWeight = 0;

    vector<char> adjMark;
    vector<long long> neighWeightSum;
    bool neighSumReady = false;

    mt19937 rng;
    steady_clock::time_point startTime;
    double timeLimit;

    // Progress reporting
    double lastReportTime = -1e9;
    const double reportInterval = 5.0;
    long long lastReportBest = -1;

    inline double elapsed() const {
        return duration<double>(steady_clock::now() - startTime).count();
    }
    inline bool timeOut() const { return elapsed() > timeLimit; }

    void report(const string& phase = "") {
        double e = elapsed();
        if (e - lastReportTime < reportInterval && bestWeight == lastReportBest) return;
        lastReportTime = e;
        lastReportBest = bestWeight;
        cerr << "  [" << setw(4) << (int)e << "s / " << (int)timeLimit << "s] "
             << "best = " << bestWeight
             << "  team_size = " << bestSet.size();
        if (!phase.empty()) cerr << "  (" << phase << ")";
        cerr << "\n";
        cerr.flush();
    }

    void addVertex(int v) {
        inSet[v] = 1;
        currentWeight += weight[v];
        ++currentSize;
        for (int u : adj[v]) ++tightness[u];
    }
    void removeVertex(int v) {
        inSet[v] = 0;
        currentWeight -= weight[v];
        --currentSize;
        for (int u : adj[v]) --tightness[u];
    }

    void updateBest() {
        if (currentWeight > bestWeight) {
            bestWeight = currentWeight;
            bestSet.clear();
            bestSet.reserve(currentSize);
            for (int v = 1; v <= N; ++v)
                if (inSet[v]) bestSet.push_back(v);
        }
    }

    void resetState() {
        fill(inSet.begin(), inSet.end(), 0);
        fill(tightness.begin(), tightness.end(), 0);
        currentWeight = 0;
        currentSize = 0;
    }

    void restoreBest() {
        resetState();
        for (int v : bestSet) addVertex(v);
    }

    // GWMIN: score = w(v) / (1 + deg(v))
    void greedyConstruct(double noise) {
        resetState();
        vector<pair<double,int>> scored;
        scored.reserve(N);
        for (int v = 1; v <= N; ++v) {
            double s = (double)weight[v] / (1.0 + (double)adj[v].size());
            if (noise > 0.0) {
                double r = (double)rng() / (double)mt19937::max();
                s *= (1.0 + noise * (r - 0.5));
            }
            scored.emplace_back(-s, v);
        }
        sort(scored.begin(), scored.end());
        vector<char> excluded(N + 1, 0);
        for (auto& p : scored) {
            int v = p.second;
            if (!excluded[v]) {
                addVertex(v);
                excluded[v] = 1;
                for (int u : adj[v]) excluded[u] = 1;
            }
        }
    }

    void computeNeighWeightSum() {
        if (neighSumReady) return;
        neighWeightSum.assign(N + 1, 0);
        for (int v = 1; v <= N; ++v) {
            long long s = 0;
            for (int u : adj[v]) s += weight[u];
            neighWeightSum[v] = s;
        }
        neighSumReady = true;
    }

    // GWMIN2: score = w(v) / (w(v) + sum w(N(v)))
    void greedyConstructGWMIN2(double noise) {
        computeNeighWeightSum();
        resetState();
        vector<pair<double,int>> scored;
        scored.reserve(N);
        for (int v = 1; v <= N; ++v) {
            double denom = (double)weight[v] + (double)neighWeightSum[v];
            double s = (double)weight[v] / denom;
            if (noise > 0.0) {
                double r = (double)rng() / (double)mt19937::max();
                s *= (1.0 + noise * (r - 0.5));
            }
            scored.emplace_back(-s, v);
        }
        sort(scored.begin(), scored.end());
        vector<char> excluded(N + 1, 0);
        for (auto& p : scored) {
            int v = p.second;
            if (!excluded[v]) {
                addVertex(v);
                excluded[v] = 1;
                for (int u : adj[v]) excluded[u] = 1;
            }
        }
    }

    bool oneImprove() {
        bool improved = false;
        for (int v = 1; v <= N; ++v) {
            if (!inSet[v] && tightness[v] == 0) {
                addVertex(v);
                improved = true;
            }
        }
        return improved;
    }

    bool twoImprove() {
        bool improved = false;
        vector<int> order;
        order.reserve(currentSize);
        for (int v = 1; v <= N; ++v) if (inSet[v]) order.push_back(v);
        shuffle(order.begin(), order.end(), rng);

        for (int v : order) {
            if (!inSet[v]) continue;
            vector<int> cand;
            for (int x : adj[v])
                if (!inSet[x] && tightness[x] == 1) cand.push_back(x);
            if ((int)cand.size() < 2) continue;
            sort(cand.begin(), cand.end(),
                 [&](int a, int b) { return weight[a] > weight[b]; });
            int K = (int)cand.size();
            if (K > 256) K = 256;
            long long w_v = weight[v];
            int found_x = -1, found_y = -1;
            for (int i = 0; i < K && found_x < 0; ++i) {
                int x = cand[i];
                if (2 * weight[x] <= w_v) break;
                for (int u : adj[x]) adjMark[u] = 1;
                for (int j = i + 1; j < K; ++j) {
                    int y = cand[j];
                    if (weight[x] + weight[y] <= w_v) break;
                    if (!adjMark[y]) { found_x = x; found_y = y; break; }
                }
                for (int u : adj[x]) adjMark[u] = 0;
            }
            if (found_x >= 0) {
                removeVertex(v);
                addVertex(found_x);
                addVertex(found_y);
                improved = true;
            }
        }
        return improved;
    }

    void localSearch() {
        bool improved = true;
        int rounds = 0;
        while (improved && !timeOut() && rounds < 50) {
            improved = false;
            if (oneImprove()) improved = true;
            if (timeOut()) break;
            if (twoImprove()) improved = true;
            ++rounds;
        }
        updateBest();
    }

    void perturb(int strength) {
        for (int i = 0; i < strength && !timeOut(); ++i) {
            int v = (int)(rng() % (uint32_t)N) + 1;
            if (inSet[v]) continue;
            for (int u : adj[v]) if (inSet[u]) removeVertex(u);
            addVertex(v);
        }
    }

public:
    MWISSolver(int n, vector<long long> w, vector<vector<int>> a, double tl)
        : N(n), weight(std::move(w)), adj(std::move(a)), timeLimit(tl)
    {
        inSet.assign(N + 1, 0);
        tightness.assign(N + 1, 0);
        adjMark.assign(N + 1, 0);
        rng.seed((uint64_t)steady_clock::now().time_since_epoch().count());
        startTime = steady_clock::now();
    }

    void solve() {
        const double noises[] = {0.0, 0.10, 0.25};
        for (double n : noises) {
            if (timeOut()) break;
            greedyConstruct(n);
            localSearch();
            report("GWMIN multi-start");
        }
        for (double n : noises) {
            if (timeOut()) break;
            greedyConstructGWMIN2(n);
            localSearch();
            report("GWMIN2 multi-start");
        }
        if (!bestSet.empty()) restoreBest();

        int strength = 2;
        int sinceImprove = 0;
        long long lastBest = bestWeight;
        vector<char> savedInSet;
        vector<int>  savedTight;
        long long    savedW;
        int          savedSz;

        while (!timeOut()) {
            savedInSet = inSet;
            savedTight = tightness;
            savedW     = currentWeight;
            savedSz    = currentSize;

            perturb(strength);
            localSearch();

            if (currentWeight > savedW) {
                if (currentWeight > lastBest) {
                    lastBest = currentWeight;
                    sinceImprove = 0;
                    strength = 2;
                } else { ++sinceImprove; }
            } else {
                inSet = savedInSet;
                tightness = savedTight;
                currentWeight = savedW;
                currentSize = savedSz;
                ++sinceImprove;
                if (sinceImprove > 20) {
                    strength = min(strength + 2, 100);
                    sinceImprove = 0;
                }
            }
            report();
        }
        // Final report
        lastReportTime = -1e9;
        report("done");
    }

    long long getBestWeight() const { return bestWeight; }
    const vector<int>& getBestSet() const { return bestSet; }
};

// ---------------------------------------------------------------------------
// Input reader that skips lines starting with '#'
// ---------------------------------------------------------------------------
class CommentSkippingReader {
    istream& in;
public:
    CommentSkippingReader(istream& s) : in(s) {}

    bool nextLong(long long& x) {
        while (in.good()) {
            in >> ws;
            int c = in.peek();
            if (c == '#') {
                string dummy;
                getline(in, dummy);
                continue;
            }
            if (c == EOF) return false;
            if (in >> x) return true;
            return false;
        }
        return false;
    }
    bool nextInt(int& x) {
        long long y;
        if (!nextLong(y)) return false;
        x = (int)y;
        return true;
    }
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    string inputPath;
    string outputPath;
    double timeLimit = 300.0;          // 5 minutes by default

    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "-i" && i + 1 < argc) inputPath = argv[++i];
        else if (s == "-t" && i + 1 < argc) timeLimit = stod(argv[++i]);
        else if (s == "-o" && i + 1 < argc) outputPath = argv[++i];
        else if (s == "-h" || s == "--help") {
            cerr << "Usage: " << argv[0] << " [-i INPUT] [-t SECONDS] [-o OUTPUT]\n"
                 << "  -i INPUT    Read graph from FILE (default: stdin)\n"
                 << "  -t SECONDS  Time limit in seconds (default: 300 = 5 min)\n"
                 << "  -o OUTPUT   Write full answer to FILE (default: stdout)\n";
            return 0;
        } else {
            cerr << "Unknown arg: " << s << " (use -h for help)\n";
            return 2;
        }
    }
    if (timeLimit < 1.0) {
        cerr << "Time limit too small (need >= 1 second)\n";
        return 2;
    }

    istream* in = &cin;
    ifstream fin;
    if (!inputPath.empty()) {
        fin.open(inputPath);
        if (!fin) {
            cerr << "Cannot open input file: " << inputPath << "\n";
            return 2;
        }
        in = &fin;
        cerr << "Reading graph from " << inputPath << "...\n";
    } else {
        cerr << "Reading graph from stdin.\n";
        cerr << "Format: N M (line 1), N weights (line 2), then M edges.\n";
        cerr << "Press Ctrl+D (Linux/macOS) or Ctrl+Z then Enter (Windows) when done typing.\n\n";
    }

    CommentSkippingReader r(*in);

    int N;
    long long M;
    if (!r.nextInt(N) || !r.nextLong(M)) {
        cerr << "Bad input: expected 'N M' on first non-comment line\n";
        return 2;
    }
    if (N < 1 || N > 200000) {
        cerr << "N out of range [1, 200000]: " << N << "\n";
        return 2;
    }
    if (M < 0) {
        cerr << "M cannot be negative: " << M << "\n";
        return 2;
    }

    vector<long long> weight(N + 1);
    for (int i = 1; i <= N; ++i) {
        if (!r.nextLong(weight[i])) {
            cerr << "Bad input: expected " << N << " skill ratings, got only " << (i-1) << "\n";
            return 2;
        }
        if (weight[i] < 1) {
            cerr << "Warning: skill rating for coder " << i << " is " << weight[i]
                 << " (should be >= 1)\n";
        }
    }

    vector<vector<int>> adj(N + 1);
    long long actuallyRead = 0;
    for (long long i = 0; i < M; ++i) {
        int u, v;
        if (!r.nextInt(u) || !r.nextInt(v)) {
            cerr << "Bad input: expected " << M << " rivalry pairs, only got " << i << "\n";
            return 2;
        }
        if (u == v) continue;                       // ignore self-loops
        if (u < 1 || u > N || v < 1 || v > N) {
            cerr << "Bad input: edge (" << u << "," << v << ") has vertex out of range\n";
            return 2;
        }
        adj[u].push_back(v);
        adj[v].push_back(u);
        ++actuallyRead;
    }
    // dedupe parallel edges
    long long uniqueEdges = 0;
    for (int i = 1; i <= N; ++i) {
        sort(adj[i].begin(), adj[i].end());
        adj[i].erase(unique(adj[i].begin(), adj[i].end()), adj[i].end());
        uniqueEdges += adj[i].size();
    }
    uniqueEdges /= 2;

    cerr << "\n========== MWIS Solver ==========\n";
    cerr << "  Coders (N):     " << N << "\n";
    cerr << "  Rivalries (M):  " << uniqueEdges
         << (uniqueEdges != M ? " (dedup'd from " + to_string(M) + ")" : "") << "\n";
    cerr << "  Time budget:    " << (int)timeLimit << " seconds\n";
    cerr << "---------------------------------\n";
    cerr << "Solving... (progress every ~5s)\n\n";

    // Reserve ~3 seconds for output / cleanup
    double solverBudget = max(1.0, timeLimit - 3.0);

    auto solveStart = steady_clock::now();
    MWISSolver solver(N, std::move(weight), std::move(adj), solverBudget);
    solver.solve();
    double solveSeconds = duration<double>(steady_clock::now() - solveStart).count();

    // ---------------- final results ----------------
    long long finalWeight = solver.getBestWeight();
    vector<int> finalSet = solver.getBestSet();
    sort(finalSet.begin(), finalSet.end());
    size_t finalSize = finalSet.size();

    // Always print the summary to stderr (terminal) so it shows alongside progress
    cerr << "\n---------------------------------\n";
    cerr << "DONE in " << fixed << setprecision(1) << solveSeconds << "s\n";
    cerr << "========== Answer ==========\n\n";
    cerr << "Total skill rating: " << finalWeight << "\n";
    cerr << "Team size:          " << finalSize << "\n";

    // Write the FULL answer (incl. all selected coder IDs) to file or stdout
    ostream* answer = &cout;
    ofstream foutAns;
    if (!outputPath.empty()) {
        foutAns.open(outputPath);
        if (!foutAns) {
            cerr << "Cannot open output file: " << outputPath << "\n";
            return 2;
        }
        answer = &foutAns;
        cerr << "\nSelected coders written to: " << outputPath << "\n";
    }

    *answer << "Total skill rating: " << finalWeight << "\n";
    *answer << "Team size:          " << finalSize << "\n";
    *answer << "Selected coders:   ";
    for (int v : finalSet) *answer << ' ' << v;
    *answer << "\n";

    return 0;
}
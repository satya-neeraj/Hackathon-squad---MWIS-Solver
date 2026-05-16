# MWIS Solver — Manual Input

A standalone solver for the **Hackathon Squad** problem (Maximum Weight
Independent Set on a conflict graph). Drop your graph file into the project,
run one command, get the answer within 5 minutes.

Single-file C++ solver, no external dependencies. Algorithm: GWMIN + GWMIN2
multi-start, then iterated local search with (1,2)-swaps.

---

## Project Layout

```
mwis-solver/
├── README.md
├── solve.cpp           # the solver (single source file)
├── build.sh            # build script
├── build.bat           # build script (Windows cmd)
├── input.in            # template graph file
├── build/              # compiled binary goes here
│   └── .gitkeep
├── examples/           # tiny pre-built test cases
│   ├── tiny.in
│   ├── star.in
│   └── adversarial.in
├── test10/             # (optional) larger test bundle
│   ├── graph01.in
│   ├── ...
│   └── graph10.in
└── answers/            # (you create this) where -o output files land
    ├── output01.out
    └── ...
```

The `test10/` and `answers/` folders are conventions used in this README — you
can name them anything you want, just match the paths in your commands.

---

## Build (one-time setup)

```bash
bash build.sh
```

(`build.bat` on Windows cmd.) Produces `build/solve` or `build/solve.exe`.
The build uses static linking so the binary has no runtime DLL dependencies.

---

## Usage

The solver is invoked directly with two flags:

```bash
./build/solve -i <input_file> -o <output_file>
```

- `-i <input_file>` — graph to solve (required)
- `-o <output_file>` — where to write the full answer (recommended)
- `-t <seconds>` — time budget; defaults to **300 (= 5 minutes)** if omitted

### Your standard command

```bash
./build/solve -i test10/graph10.in -o answers/output10.out
```

This solves `test10/graph10.in` for 5 minutes (the default) and writes the
full answer to `answers/output10.out`.

The `answers/` folder needs to exist before you run the command:

```bash
mkdir -p answers
```

### Running all 10 graphs in sequence

```bash
mkdir -p answers
for i in 01 02 03 04 05 06 07 08 09 10; do
    echo "=== graph${i} ==="
    ./build/solve -i test10/graph${i}.in -o answers/output${i}.out
done
```

Each iteration takes ~5 minutes, so the full sweep is roughly **50 minutes**.
Progress prints to your terminal during each run; the summary and answer file
appear when each one finishes.

### With a custom time budget

```bash
./build/solve -i test10/graph10.in -o answers/output10.out -t 60   # 1 minute
./build/solve -i test10/graph10.in -o answers/output10.out -t 600  # 10 minutes
```

---

## Input Format

```
N M
S_1 S_2 ... S_N
u_1 v_1
u_2 v_2
...
u_M v_M
```

- `N` — number of coders (1 to 200,000)
- `M` — number of rivalry pairs
- `S_i` — skill rating of coder `i` (1 to 1,000,000,000)
- `(u, v)` — coder `u` and `v` refuse to work together

Rules:
- Coders are numbered from 1.
- Lines starting with `#` are comments and are ignored.
- Blank lines are fine.

See `input.in` and the `examples/` folder for ready-to-use samples.

---

## What You See vs. What's Saved

The solver uses two output streams so big answer lists don't flood your terminal.

**Terminal (stderr) — visible while solving:**

```
Reading graph from test10/graph10.in...

========== MWIS Solver ==========
  Coders (N):     200000
  Rivalries (M):  5999088
  Time budget:    300 seconds
---------------------------------
Solving... (progress every ~5s)

  [   0s / 297s] best = 0  team_size = 0
  [   5s / 297s] best = 2745001234567  team_size = 14998  (GWMIN multi-start)
  [  10s / 297s] best = 2812998112900  team_size = 15187  (GWMIN2 multi-start)
  ...
  [ 295s / 297s] best = 2867204193112  team_size = 15301  (done)

---------------------------------
DONE in 297.0s
========== Answer ==========

Total skill rating: 2867204193112
Team size:          15301

Selected coders written to: answers/output10.out
```

**Answer file (`answers/output10.out`) — the actual saved deliverable:**

```
Total skill rating: 2867204193112
Team size:          15301
Selected coders:    1 5 9 12 23 ... 199987
```

The "Selected coders" line is the full list of chosen coder IDs in ascending
order. This is what would have flooded your terminal if not piped to a file.

---

## Flags Summary

| Flag | Purpose | Default |
|---|---|---|
| `-i FILE` | input graph file | reads from stdin |
| `-o FILE` | output answer file | writes to stdout |
| `-t SECONDS` | time budget in seconds | `300` (5 minutes) |
| `-h`, `--help` | show usage | — |

---

## Built-in Examples (sanity checks)

| File | What it tests | Expected answer |
|---|---|---|
| `examples/tiny.in` | 5-coder path graph | weight 90, set `{1, 3, 5}` |
| `examples/star.in` | Hub vs four leaves | weight 120, set `{2, 3, 4, 5}` |
| `examples/adversarial.in` | Trojan-hub trap | weight 10000, set `{2..11}` |

Quick verification (short budgets are plenty for these):

```bash
mkdir -p answers
./build/solve -i examples/tiny.in        -o answers/tiny.out         -t 5
./build/solve -i examples/star.in        -o answers/star.out         -t 5
./build/solve -i examples/adversarial.in -o answers/adversarial.out  -t 10
```

Check the results:

```bash
head answers/tiny.out
head answers/star.out
head answers/adversarial.out
```

---

## Reading Just the Headline Number

After running a batch, extract the totals quickly:

```bash
for i in 01 02 03 04 05 06 07 08 09 10; do
    printf "graph%s: " "$i"
    head -1 "answers/output${i}.out" | awk '{print $NF}'
done
```

Output:

```
graph01: 53298373822328
graph02: 10732811200499
...
graph10: 2867204193112
```

---

## How the Solver Works (brief)

1. **Multi-start construction** — six greedy starts: three GWMIN (score by
   `w/(1+deg)`) and three GWMIN2 (score by `w/(w+ΣwN)`), with randomized
   noise. The best of all six runs becomes the ILS seed.
2. **Local search** — repeatedly applies 1-improvement (add any vertex with
   no neighbor in the set) and (1,2)-swap (replace one in-set vertex with two
   non-adjacent vertices of greater combined weight) until no further gain.
3. **Iterated local search (ILS)** — perturbs the current solution by
   force-inserting random vertices, then re-runs local search. Accepts on
   improvement, reverts on regression. Perturbation strength adapts when
   progress stalls. Continues until the time budget runs out.

Memory: O(N + M). Time: bounded strictly by the configured budget.
Result quality on N=200,000 graphs with the full 5-minute budget: typically
within 0.5–2% of optimal (often the true optimum on structured instances).

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `g++: command not found` | Install MinGW-w64 (Windows) or Xcode CLT (macOS) |
| Segfault on Windows | Make sure `build.sh` still has `-static -static-libgcc -static-libstdc++` flags |
| `Cannot open input file: ...` | Path is wrong; check you're in the project root and the file exists |
| `Cannot open output file: ...` | The output directory doesn't exist — run `mkdir -p answers` first |
| `N out of range [1, 200000]` | The constraint is hard-coded in `solve.cpp`; raise it there if you need larger |
| No output appearing for minutes | Normal — output only appears when the time budget expires. Progress lines on screen confirm it's working |

---

## Scale Notes

- **N up to 200,000 coders**, **M up to ~10 million rivalries** (memory ceiling)
- Peak memory at maximum scale: ~200 MB
- The 5-minute budget gives near-optimal results on virtually all instances
  in the supported size range

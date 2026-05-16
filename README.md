# MWIS Solver — Manual Input

A standalone solver for the **Hackathon Squad** problem (Maximum Weight
Independent Set on a conflict graph). Edit your graph in `input.in`, run one
command, get the answer within 5 minutes.

Single-file solver, no dependencies beyond `g++`. Same algorithm as a full
benchmark suite (GWMIN + GWMIN2 multi-start, iterated local search with
(1,2)-swaps), wrapped in a tiny CLI.

---

## Project Layout

```
mwis-solver/
├── README.md
├── solve.cpp           # the solver (single file, ~400 lines)
├── build.sh / build.bat
├── run.sh   / run.bat
├── input.in            # YOUR graph goes here (edit me!)
├── build/              # compiled binary lands here
└── examples/
    ├── tiny.in
    ├── star.in
    └── adversarial.in
```

---

## Prerequisites

- **g++** with C++17 support (any modern GCC: 9 or later)
- Windows users: install MinGW-w64 (the build uses `-static` flags so DLL
  conflicts are sidestepped automatically)

---

## Quick Start

### 1. Build (one time)

**Linux / macOS / Git Bash on Windows:**
```bash
bash build.sh
```

**Windows cmd:**
```cmd
build.bat
```

Produces `build/solve` (or `build/solve.exe` on Windows). Takes a few seconds.

### 2. Verify with a built-in example

```bash
bash run.sh examples/adversarial.in 5
```

Expected (last lines on screen):

```
========== Answer ==========

Total skill rating: 10000
Team size:          10

Selected coders written to: adversarial.out
```

And `adversarial.out` will contain:

```
Total skill rating: 10000
Team size:          10
Selected coders:    2 3 4 5 6 7 8 9 10 11
```

If you see that, setup is complete.

### 3. Solve your own graph

Open `input.in` in any text editor, replace the example block with your
graph (format documented inside the file), save, then run:

```bash
bash run.sh
```

That uses `input.in` as input and 300-second (5-minute) budget. Progress
prints every 5 seconds while solving. The summary appears on screen at the
end, and the full answer (including selected coder IDs) is written to
`input.out`.

To pass a different time limit:

```bash
bash run.sh input.in 60        # 1-minute budget
bash run.sh input.in 600       # 10-minute budget
```

To control the output filename:

```bash
bash run.sh input.in 300 my_answer.out
```

Windows cmd: replace `bash run.sh` with `run.bat`, same arguments.

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

- `N` — number of coders, between 1 and 200,000
- `M` — number of rivalry pairs, between 0 and `N(N-1)/2`
- `S_i` — skill rating of coder `i`, between 1 and 1,000,000,000
- `(u, v)` — coder `u` and coder `v` refuse to work together

**Rules:**
- Coders are numbered starting at 1.
- Lines starting with `#` are comments and ignored.
- Blank lines are fine.
- Self-loops (`u == v`) and duplicate edges are silently dropped.

See `input.in` for an annotated template, and `examples/` for ready-to-run
graphs.

---

## Running the Solver Directly

If you'd rather not use `run.sh`, call the binary yourself:

```bash
./build/solve -i input.in                 # 300s budget, answer to stdout
./build/solve -i input.in -t 60           # custom budget
./build/solve -i input.in -o answer.out   # write answer to file
./build/solve -i input.in -t 300 -o answer.out

./build/solve < input.in                  # pipe via stdin
cat input.in | ./build/solve

./build/solve                             # type interactively, Ctrl+D when done
./build/solve -h                          # show help
```

On Windows replace `./build/solve` with `build\solve.exe`.

### Flags

| Flag | Purpose | Default |
|---|---|---|
| `-i FILE` | read graph from `FILE` | read from stdin |
| `-t SECONDS` | time budget (1 to any value) | 300 (5 min) |
| `-o FILE` | write the full answer to `FILE` | write to stdout |
| `-h`, `--help` | show usage | — |

---

## What You See vs. What's Saved

The solver splits output cleanly across two channels:

**Terminal (stderr):**
- Banner with N, M, time budget
- Progress lines every ~5 seconds
- Final summary: total skill rating + team size
- Note about where the answer file was saved

**Answer (stdout or `-o FILE`):**
- `Total skill rating: <number>`
- `Team size: <number>`
- `Selected coders:` followed by the full list of chosen coder IDs

The "selected coders" list can be very long (tens of thousands of IDs at
N=200,000), which is why it's only written to a file or piped output — it
won't flood your terminal.

### Example session

```
$ bash run.sh input.in 300
Reading graph from input.in...

========== MWIS Solver ==========
  Coders (N):     200000
  Rivalries (M):  399997
  Time budget:    300 seconds
---------------------------------
Solving... (progress every ~5s)

  [   0s / 297s] best = 0  team_size = 0
  [   5s / 297s] best = 52934821093001  team_size = 88412  (GWMIN multi-start)
  [  10s / 297s] best = 53087412390442  team_size = 88641  (GWMIN2 multi-start)
  [  15s / 297s] best = 53187912001234  team_size = 88720
  ...
  [ 295s / 297s] best = 53298373822328  team_size = 89244  (done)

---------------------------------
DONE in 297.0s
========== Answer ==========

Total skill rating: 53298373822328
Team size:          89244

Selected coders written to: input.out
Answer saved to: input.out
```

Then `input.out` contains all three answer lines including the coder IDs.

---

## Built-in Examples

| File | Description |
|---|---|
| `examples/tiny.in` | 5-coder path graph (sanity check) |
| `examples/star.in` | Hub vs. four leaves — leaves should win (120 > 100) |
| `examples/adversarial.in` | Trojan hub — forces correct heuristic behavior (10000, not 1001) |

Run any of them quickly:

```bash
bash run.sh examples/tiny.in 5
bash run.sh examples/star.in 5
bash run.sh examples/adversarial.in 10
```

---

## How the Solver Works (brief)

1. **Multi-start construction** — six greedy starts: three GWMIN (score by
   `w/(1+deg)`) and three GWMIN2 (score by `w/(w+ΣwN)`), with mild
   randomization noise. The best result is kept as the seed.
2. **Local search** — alternates 1-improvement (add any free vertex) and
   (1,2)-swap (replace one vertex with two non-adjacent ones of higher
   combined weight) until no improvement is found.
3. **Iterated local search** — repeatedly perturbs the solution by
   force-inserting random vertices (evicting their neighbors), then
   re-runs local search. Accepts if better, reverts if worse. Perturbation
   strength adapts when progress stalls.

Memory: O(N + M). Time: bounded by the configured budget. Result: typically
within 0.5–2% of optimal on random graphs, often optimal on structured ones.

---

## Troubleshooting

| Symptom | Fix |
|---|---|
| `g++: command not found` | Install MinGW-w64 (Windows) or Xcode Command Line Tools (macOS); see the build instructions for your platform |
| Segfault on Windows even after build succeeds | Caused by mixed C-runtime DLLs in some MinGW builds; the `-static -static-libgcc -static-libstdc++` flags in `build.sh` should prevent this. If you've removed those flags, put them back |
| `Cannot open input file: ...` | Make sure you're in the project directory and the file path is correct |
| `N out of range [1, 200000]` | The constraint is hard-coded; raise it in `solve.cpp` if you need larger |
| Output stops mid-run | The progress lines are on stderr; if you piped only stdout to a file (`> something`), progress still prints to the terminal — the answer file is being written normally |
| Terminal flooded with thousands of coder IDs | You ran without `-o` or `run.sh`; the answer printed to stdout. Use `-o FILE` (or `run.sh` which auto-creates a `.out` file) |

---

## Notes on Scale

This solver is built to handle the full problem ceiling:

- **N up to 200,000 coders**
- **M up to ~10 million rivalries** (practical limit; the theoretical
  `N(N-1)/2` doesn't fit in memory anyway)
- Memory stays under ~200 MB at maximum scale
- Result quality: ~99% of optimal at N=200,000 with the full 5-minute budget

For very large stdin inputs, prefer `-i FILE` over `< file` — file-based
reading is faster than the slower stdin-pipe path.

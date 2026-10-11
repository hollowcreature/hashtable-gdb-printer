# Hashtable GDB printer

[![CI](https://github.com/hollowcreature/hashtable-gdb-printer/actions/workflows/ci.yml/badge.svg)](https://github.com/hollowcreature/hashtable-gdb-printer/actions/workflows/ci.yml)

A small hash table in C with a GDB Python pretty-printer and a custom `dump-table` command

## Before / after

Plain GDB shows only pointers:

```gdb
(gdb) print *t
$1 = {size = 8, count = 6, buckets = 0x5555555596d0}

(gdb) p *t->buckets@8
$2 = {0x0, 0x0, 0x0, 0x555555559760, 0x555555559860, 0x5555555597e0, 0x0, 0x0}

(gdb) p *t->buckets[3]->next
$3 = {key = 0x555555559740 "abrams", value = 67, next = 0x0}
```

With `printer.py` loaded:

```gdb
(gdb) p *t
$1 = table(count=6, size=8) = {["baba"] = 1, ["abrams"] = 67, ["solomon"] = 5, ["paradox"] = 4, ["dynamo"] = 3, ["drifter"] = 2}

(gdb) dump-table t
bucket 0: (empty)
bucket 1: (empty)
bucket 2: (empty)
bucket 3, length: 2, list: baba=1 -> abrams=67
bucket 4, length: 2, list: solomon=5 -> paradox=4
bucket 5, length: 2, list: dynamo=3 -> drifter=2
bucket 6: (empty)
bucket 7: (empty)
```

## Directory

- **`main.c`**: hash table with separate chaining (djb2 hash). `table_new`, `table_insert`, `table_get`, `table_remove`, `table_free`, built on `find` helper function. `main()` performs a test run.
- **`printer.py`**:
  - `TablePrinter`: a pretty-printer for `struct table` using the `map` display hint
  - `dump-table`: a GDB command that shows the bucket layout and chain lengths
  - `hashtable-max-size`: a GDB setting (default 1,000,000) that bounds what the printer trusts. Tables larger than this are reported as invalid
- **`test.gdb`** + **`expected.txt`**: a batch GDB script whose output is diffed against an expected result, once on a populated table and once after all keys are removed

## Build and run

Requires Linux or WSL with `gcc`, `make`, `gdb` and `valgrind`

```sh
make
make memcheck
make test
./main
```

## Using the printer

```sh
gdb ./main
(gdb) source printer.py
(gdb) break table_remove
(gdb) run
(gdb) p *t
(gdb) dump-table t
```

## Robustness

The printer validates the table before walking it, so inspecting a half-built or corrupted table won't hang GDB or crash the printer.
Lowering the cap shows the check in action:

```gdb
(gdb) set hashtable-max-size 4
(gdb) p *t
$1 = table(invalid: size=8, count=6)
```

- sizes above `hashtable-max-size` are treated as garbage (`set hashtable-max-size N` to adjust)
- chain walks are capped by `count`, so a cycle in a chain can't loop forever

## Tests and CI

`make test` runs `test.gdb` in batch mode, keeps only the printer and `dump-table` output, and diffs it against `expected.txt`.

GitHub Actions runs on every push: build, `make memcheck`, then `make test`.

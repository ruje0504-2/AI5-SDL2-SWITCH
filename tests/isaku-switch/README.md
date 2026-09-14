# Isaku Switch regressions

These fixtures link the real runtime objects with a replacement test main.
They never ship game data. The runner writes separate dependency/object files
and does not overwrite normal build dependencies.

```sh
meson setup build-default -Dmovies=disabled
ninja -C build-default
python3 tests/isaku-switch/run.py --build build-default scope

meson setup build-isaku-test -Dmovies=disabled \
  '-Dc_args=-DAI5_ISAKU_SWITCH_BUILD -DAI5_ISAKU_SWITCH_TEST'
ninja -C build-isaku-test
python3 tests/isaku-switch/run.py --build build-isaku-test \
  --chs-font /absolute/path/to/chinese-font.ttf \
  scope isaku_save_init chs_text fidelity refresh_benchmark
```

The cursor/effect/menu suite additionally needs the user's original Japanese
EXE and extracted cursor file:

```sh
python3 tests/isaku-switch/run.py --build build-isaku-test \
  --data /absolute/path/to/original-japanese-data runtime_effects
```

`scope` checks the activation gate for every game ID, the original non-target
GBK punctuation behavior, absence of a new cursor-file requirement, immediate
screen submission and the original non-target empty-save behavior. Run it in
both builds. Other fixtures check initialization migration without file writes,
valid mute-state preservation, mixed text/control boundaries, intermediate
blend pixels, aliased surfaces, final rows, original cursor AND/XOR and menu
callbacks. `slow_present.c` is an optional macOS interposer for modeling a
blocking display; modeled timings are not hardware benchmarks.

The private host macro is for testing only. Ordinary desktop builds must not
be given `AI5_ISAKU_SWITCH_TEST`. Linux CI runs the data-free fixtures; actual
Switch/Windows/macOS/Linux game playthrough coverage is a separate task.

<div align="center">

# oranj

[![License][license-badge]][license-link]  
[![GitHub release (latest by date)][release-badge]][release-link]
[![Commits since latest release][commits-badge]][commits-link]

![LGBTQ+ friendly][lgbtqp-badge]
![trans rights][trans-rights-badge]

</div>

a work-in-progress UCI [shatranj] engine based on [Stormphrax], with NNUE evaluation trained from zero knowledge starting with random weights

## Strength
??

## Features
- standard PVS with quiescence search and iterative deepening
  - aspiration windows
  - futility pruning
  - history
    - capture history
    - 1-ply continuation history (countermove history)
    - 2-ply continuation history (follow-up history)
    - 4-ply continuation history
  - correction history
    - pawn
    - non-pawn
    - major
    - continuation
  - history pruning
  - internal iterative reduction
  - killers (1 per ply)
  - late move reductions
  - late move pruning
  - mate distance pruning
  - multicut
  - nullmove pruning
  - reverse futility pruning
  - probcut
  - SEE move ordering and pruning
  - singular extensions
    - double extensions
    - triple extensions
    - various negative extensions
  - Syzygy tablebase support
- NNUE
  - _
  - trained from zero knowledge with reinforcement learning from a randomly-initialised network
- BMI2 attacks in the `bmi2` build and up, otherwise fancy black magic
  - `pext`/`pdep` for rooks
  - `pext` for bishops
- lazy SMP
- static contempt

## To-do
- make it stronger uwu

## UCI options
| Name                          |  Type   | Default value |       Valid values        | Description                                                                                                                                                                                                                         |
|:------------------------------|:-------:|:-------------:|:-------------------------:|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Hash`                        | integer |      64       |        [1, 131072]        | Memory allocated to the transposition table (in MiB).                                                                                                                                                                               |
| `Clear Hash`                  | button  |      N/A      |            N/A            | Clears the transposition table.                                                                                                                                                                                                     |
| `Threads`                     | integer |       1       |         [1, 2048]         | Number of threads used to search.                                                                                                                                                                                                   |
| `UCI_ShowWDL`                 |  check  |    `true`     |      `false`, `true`      | Whether oranj displays predicted win/draw/loss probabilities in UCI output.                                                                                                                                                         |
| `ShowCurrMove`                |  check  |    `false`    |      `false`, `true`      | Whether oranj starts printing the move currently being searched after a short delay.                                                                                                                                                |
| `Move Overhead`               | integer |      10       |        [0, 50000]         | Amount of time oranj assumes to be lost to overhead when making a move (in ms).                                                                                                                                                     |
| `SoftNodes`                   |  check  |    `false`    |      `false`, `true`      | Whether oranj will finish the current depth after hitting the node limit when sent `go nodes`.                                                                                                                                      |
| `SoftNodeHardLimitMultiplier` | integer |     1678      |         [1, 5000]         | With `SoftNodes` enabled, the multiplier applied to the `go nodes` limit after which oranj will abort the search anyway.                                                                                                            |
| `EnableWeirdTCs`              |  check  |    `false`    |      `false`, `true`      | Whether unusual time controls (movestogo != 0, or increment = 0) are enabled. Enabling this option means you recognise that oranj is neither designed for nor tested with these TCs, and is likely to perform worse than under X+Y. |
| `EvalFile`                    | string  | `<internal>`  | any path, or `<internal>` | NNUE file to use for evaluation.                                                                                                                                                                                                    |

## Builds
`vnni512`: requires BMI2, AVX-512 and VNNI (Zen 4/Cascade Lake-SP/Rocket Lake and up)  
`avx512`: requires BMI2 and AVX-512 (Skylake-X, Cannon Lake)  
`avx2-bmi2`: requires BMI2 and AVX2 and assumes fast `pext` and `pdep` (i.e. no Bulldozer, Piledriver, Steamroller, Excavator, Zen 1, Zen+ or Zen 2)  
`avx2`: requires BMI and AVX2 - primarily useful for pre-Zen 3 AMD CPUs back to Excavator  
`sse41-popcnt`: needs SSE 4.1 and `popcnt` - for older x64 CPUs

If in doubt, compare the `avx2-bmi2` and `avx2` binaries and pick the one that's faster. BMI2 will always be faster on Intel CPUs.

Alternatively, build the makefile target `native` for a binary tuned for your specific CPU (see below)  

### Note:  
- If you have an AMD Zen 1 (Ryzen x 1xxx), Zen+ (Ryzen x 2xxx) or Zen 2 (Ryzen x 3xxx) CPU, use the `avx2` build even though your CPU supports BMI2. These CPUs implement the BMI2 instructions `pext` and `pdep` in microcode, which makes them unusably slow for oranj's purposes.

## Building
Requires Make and a competent C++20 compiler that supports LTO. GCC is not currently supported, so the usual compiler is Clang. MSVC explicitly does not work.
```bash
> make <BUILD> CXX=<COMPILER>
```
- replace `<COMPILER>` with your preferred compiler - for example, `clang++` or `icpx`
  - if not specified, the compiler defaults to `clang++`
- replace `<BUILD>` with the binary you wish to build - `native`/`vnni512`/`avx512`/`avx2-bmi2`/`avx2`/`sse41-popcnt`
  - if not specified, the default build is `native`
- if you wish, you can have oranj include the current git commit hash in its UCI version string - pass `COMMIT_HASH=on`

By default, the makefile builds binaries with profile-guided optimisation (PGO). To disable this, pass `PGO=off`. When using Clang with PGO enabled, `llvm-profdata` must be in your PATH.

## Credit
oranj makes use of the following libraries:
- a slightly modified version of [incbin] for embedding neural network files, under the Unlicense
- [Zstandard] for decompressing NNUE files, under GPLv2 (see [COPYING](src/3rdparty/zstd/COPYING))

In no particular order, these engines have been notable sources of ideas or inspiration:
- [Viridithas]
- [Svart]
- [Stockfish]
- [Ethereal]
- [Berserk]
- [Weiss]
- [Koivisto]

oranj's networks are trained with [bullet].

[license-badge]: https://img.shields.io/github/license/Ciekce/oranj?style=for-the-badge
[release-badge]: https://img.shields.io/github/v/release/Ciekce/oranj?style=for-the-badge
[commits-badge]: https://img.shields.io/github/commits-since/Ciekce/oranj/latest?style=for-the-badge

[license-link]: https://github.com/Ciekce/oranj/blob/main/LICENSE
[release-link]: https://github.com/Ciekce/oranj/releases/latest
[commits-link]: https://github.com/Ciekce/oranj/commits/main

[lgbtqp-badge]: https://pride-badges.pony.workers.dev/static/v1?label=lgbtq%2B%20friendly&stripeWidth=6&stripeColors=E40303,FF8C00,FFED00,008026,24408E,732982
[trans-rights-badge]: https://pride-badges.pony.workers.dev/static/v1?label=trans%20rights&stripeWidth=6&stripeColors=5BCEFA,F5A9B8,FFFFFF,F5A9B8,5BCEFA

[shatranj]: https://en.wikipedia.org/wiki/Shatranj

[stormphrax]: https://github.com/Ciekce/Stormphrax

[incbin]: https://github.com/graphitemaster/incbin
[zstandard]: https://github.com/facebook/zstd

[viridithas]: https://github.com/cosmobobak/viridithas
[svart]: https://github.com/crippa1337/svart
[stockfish]: https://github.com/official-stockfish/Stockfish
[ethereal]: https://github.com/AndyGrant/Ethereal
[berserk]: https://github.com/jhonnold/Berserk
[weiss]: https://github.com/TerjeKir/weiss
[koivisto]: https://github.com/Luecx/Koivisto

[bullet]: https://github.com/jw1912/bullet

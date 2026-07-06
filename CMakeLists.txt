cmake_minimum_required(VERSION 3.15)

file(STRINGS version.txt OJ_VERSION LIMIT_COUNT 1)
project(oranj VERSION ${OJ_VERSION})

set(CMAKE_CXX_STANDARD 20)

file(STRINGS network.txt OJ_DEFAULT_NET_NAME LIMIT_COUNT 1)

set(OJ_CLANG CMAKE_CXX_COMPILER_ID STREQUAL "Clang")

if(MSVC AND NOT OJ_CLANG)
	message(FATAL_ERROR oranj does not support building with MSVC)
endif()

option(OJ_EMBED_COMMIT_HASH "whether to include the current git commit in the UCI version string" ON)

if(OJ_EMBED_COMMIT_HASH)
	set(OJ_COMMIT_HASH "unknown")
	find_package(Git QUIET)
	if(GIT_FOUND)
		execute_process(
			COMMAND git log -1 --pretty=format:%h
			OUTPUT_VARIABLE OJ_COMMIT_HASH
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET
		)
	endif()
endif()

# disable -Wunused-function and -Wunused-const-variable for zstd
if(MSVC)
	add_compile_options(/EHsc /W4 /clang:-Wno-sign-compare /clang:-Wno-unused-function /clang:-Wno-unused-const-variable /WX /clang:-fconstexpr-steps=2097152)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
else()
	add_compile_options(-Wall -Wextra -Wno-sign-compare -Wno-unused-function -Wno-unused-const-variable -Werror -fconstexpr-steps=2097152)
endif()

find_package(Threads REQUIRED)

option(OJ_FAST_PEXT "whether pext and pdep are usably fast on this architecture, for building native binaries" ON)
option(OJ_DISABLE_NEON_DOTPROD "whether to disable NEON dotprod on ARM machines" OFF)

add_executable(oranj-native src/types.h src/main.cpp src/uci/uci.h src/uci/uci.cpp src/core.h src/core.cpp src/util/bitfield.h
	src/util/bits.h src/util/parse.h src/util/split.h src/util/split.cpp src/util/rng.h src/util/static_vector.h
	src/bitboard.h src/move.h src/move.cpp src/keys.h src/position.h src/position.cpp src/search.h
	src/search.cpp src/movegen.h src/movegen.cpp src/attacks/util.h src/attacks/attacks.h src/util/timer.h
	src/util/timer.cpp src/rays.h src/ttable.h src/ttable.cpp
	src/util/cemath.h src/eval/nnue.h src/eval/nnue.cpp src/util/range.h src/arch.h src/perft.h
	src/perft.cpp src/thread.h src/see.h src/bench.h src/bench.cpp src/tunable.h src/tunable.cpp src/opts.h
	src/opts.cpp src/datagen/datagen.h src/datagen/datagen.cpp
	src/util/u4array.h src/eval/eval.h src/util/barrier.h src/util/simd.h src/wdl.h src/wdl.cpp src/eval/arch.h
	src/cuckoo.h src/cuckoo.cpp src/eval/nnue/network.h src/eval/nnue/activation.h src/eval/nnue/output.h
	src/eval/nnue/input.h src/util/memstream.h src/util/aligned_array.h src/eval/nnue/loader.h src/eval/nnue/features/psq.h
	src/datagen/format.h src/datagen/common.h src/datagen/marlinformat.h src/datagen/marlinformat.cpp
	src/datagen/oranjformat.h src/datagen/oranjformat.cpp src/movepick.h src/history.h
	src/util/multi_array.h src/correction.h src/util/simd/avx512.h src/util/simd/avx2.h
	src/util/simd/neon.h src/util/align.h 3rdparty/zstd/zstddeclib.c
	src/eval/nnue/loader.cpp src/datagen/fen.h src/datagen/fen.cpp src/util/ctrlc.h src/util/ctrlc.cpp
	src/eval/nnue/arch/singlelayer.h src/eval/nnue/arch/multilayer.h src/stats.h src/stats.cpp
	3rdparty/fmt/src/format.cc src/eval/nnue/arch/util/sparse.h src/thread.cpp src/root_move.h src/pv.h src/limit.h
	src/limit.cpp src/util/numa/numa.h src/util/numa/numa_libnuma.cpp src/util/numa/numa_fallback.cpp
	src/eval/nnue/features/threats.h src/eval/nnue/features/threats.cpp src/attacks/bmi2/data.h
	src/attacks/bmi2/attacks.h src/attacks/bmi2/attacks.cpp src/attacks/black_magic/data.h
	src/attacks/black_magic/attacks.h src/attacks/black_magic/attacks.cpp
	src/eval/header.h src/correction.cpp src/movepick.cpp src/pv.cpp src/see.cpp src/eval/nnue_state.h
	src/eval/nnue_state.cpp src/eval/eval.cpp src/uci/option.h src/uci/option.cpp)

target_include_directories(oranj-native PUBLIC 3rdparty/fmt/include)
target_compile_options(oranj-native PUBLIC -march=native $<$<CONFIG:Release>:-flto>)
target_link_options(oranj-native PUBLIC -fuse-ld=lld)
target_link_libraries(oranj-native PUBLIC Threads::Threads)
target_compile_definitions(oranj-native PUBLIC OJ_NATIVE OJ_VERSION=${CMAKE_PROJECT_VERSION})

if(OJ_EMBED_COMMIT_HASH)
	target_compile_definitions(oranj-native PUBLIC OJ_COMMIT_HASH=${OJ_COMMIT_HASH})
endif()

if(OJ_FAST_PEXT)
	target_compile_definitions(oranj-native PUBLIC OJ_FAST_PEXT)
endif()

if(OJ_DISABLE_NEON_DOTPROD)
	target_compile_definitions(oranj-native PUBLIC OJ_DISABLE_NEON_DOTPROD)
endif()

add_executable(permute-native preprocess/permute.cpp 3rdparty/fmt/src/format.cc)
target_include_directories(permute-native PUBLIC 3rdparty/fmt/include)
target_compile_definitions(permute-native PUBLIC OJ_NATIVE)
target_compile_options(permute-native PUBLIC -march=native)

add_custom_command(
	COMMAND permute-native "${PROJECT_SOURCE_DIR}/${OJ_DEFAULT_NET_NAME}.nnue" "${OJ_DEFAULT_NET_NAME}.nnue_permuted"
	OUTPUT ${OJ_DEFAULT_NET_NAME}.nnue_permuted
	DEPENDS permute-native
	COMMENT "Prepermuting network"
)

add_custom_target(permuted_net DEPENDS ${OJ_DEFAULT_NET_NAME}.nnue_permuted)
add_dependencies(oranj-native permuted_net)
target_compile_definitions(oranj-native PUBLIC OJ_NETWORK_FILE="${OJ_DEFAULT_NET_NAME}.nnue_permuted")

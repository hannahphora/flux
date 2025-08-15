## notes:

### general
 - for hot reloading:
	- jet-live on linux/mac (https://github.com/ddovod/jet-live)
	- blink on windows (https://github.com/crosire/blink)
 - 
##### references
 - https://vkguide.dev/docs/ascendant/from_tutorial_to_engine/

### c++ tools
##### general
 - flags: -Wall -Wextra -Wpedantic -Wconversion -Werror (possibly -Weverything)
 - sanitizers:
	-fsanitize=array-bounds,null,alignment,unreachable,leak,undefined,(address/memory/thread)
	- asan (address)
	- tsan (thread)
	- memsan (memory)
 - clang-tidy, clang-format
 - valgrind
 - clang-include-fixer/include-what-you-use
 - libstdc++ debug mode
##### profiling
 - tracy
 - perf + hotspot
 - gprof (with visualisation tool)
 - pprof
 - easy profiler (https://github.com/yse/easy_profiler)
 - likwid (https://github.com/RRZE-HPC/likwid)
 - cachegrind
 - valgrind
 - lcov (for coverage)
 - heaptrack (for tracking allocations)
 - renderdoc (for gpu profiling)
##### references
 - https://www.brendangregg.com/linuxperf.html


### deps
 - glfw (windowing)
 - glm (math)
 - meshoptimizer
 - nuklear (ui)
 - stb (assorted)
 - vma (vulkan memory allocator)
 - vkb (vulkan bootstrap)
 - tracy (profiling)
 - soloud/fmod (audio)
 - jolt/bullet (physics)

### systems
 - flux is comprised of systems
 - systems can have subsystems
 - each system and its subsystems get compiled to a single translation unit
 - each system has its own state struct and namespace
	e.g. flux::renderer::init(flux::renderer::RendererState* state);
 - each system has an init, deinit, and update function

##### engine
 - core system, ties all other systems together
 - also provides implementations for common subsystems used by other systems
 - subsystems:
	- log
	- profiling
	- unit tests
	- ecs
	- math
	- allocators
	- utility
	- 

##### renderer
 - subsystems:
	- ui
 - 

##### input

##### audio

##### physics

##### networking

##### ai

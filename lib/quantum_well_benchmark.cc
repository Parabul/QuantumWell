#include <benchmark/benchmark.h>

#include "quantum_well.h"

// ---------------------------------------------------------
// Benchmark 1: The Initialization Math
// Measures how fast we can find Bessel roots and normalize
// ---------------------------------------------------------
static void BM_AddState(benchmark::State& state) {
  for (auto _ : state) {
    CircularQuantumWell well(200, 1.0);

    // Add a state to trigger the root-finding and spatial grid calculation
    well.addState(2, 2, 0.707, 0.0);

    // Prevent the compiler from optimizing away the well object
    benchmark::DoNotOptimize(well);
  }
}
// Register the benchmark
BENCHMARK(BM_AddState);

// ---------------------------------------------------------
// Benchmark 2: The Rendering Loop (The Bottleneck)
// Measures the speed of the 200x200 pixel physics updates
// ---------------------------------------------------------
static void BM_RenderFrame(benchmark::State& state) {
  // Setup happens OUTSIDE the for-loop so we don't measure it
  CircularQuantumWell well(200, 1.0);
  well.addState(0, 1, 0.5, 0.0);
  well.addState(1, 1, 0.5, 0.0);
  well.addState(2, 2, 0.5, 0.0);

  for (auto _ : state) {
    // This is what we actually want to measure!
    well.renderFrame(0.015);

    // Force the compiler to actually generate the buffer in memory
    benchmark::DoNotOptimize(well.getBufferPointer());
  }
}
// Register the benchmark
BENCHMARK(BM_RenderFrame);

// ---------------------------------------------------------
// Benchmark: Full Browser Replication
// Simulates the exact load of the JS frontend: 136 states,
// a localized wavepacket, and the 200x200 pixel loop.
// ---------------------------------------------------------
static void BM_BrowserSimulation(benchmark::State& state) {
  // 1. Setup Phase (Outside the timer loop)
  CircularQuantumWell well(200, 1.0);

  // Load the large basis set of empty states exactly like JS
  for (int l = -8; l <= 8; ++l) {
    for (int n = 1; n <= 8; ++n) {
      well.addState(l, n, 0.0, 0.0);
    }
  }

  // Simulate the user clicking on the canvas to inject a wavepacket.
  // We use arbitrary coordinates (e.g., right side of the circle)
  well.createWavepacket(0.5, 0.2, 0.15);

  // 2. Measurement Phase
  for (auto _ : state) {
    // Run the physics and rendering update exactly like JS
    well.renderFrame(0.001);

    // Force the compiler to write to the memory buffer
    benchmark::DoNotOptimize(well.getBufferPointer());
  }
}
// Register the benchmark and force the output to be in milliseconds
BENCHMARK(BM_BrowserSimulation)->Unit(benchmark::kMillisecond);
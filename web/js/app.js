/**
 * Memory Engine Diagnostics Suite - Main Application
 */

import { Visualizer } from './visualizer.js';
import { MetricsDisplay } from './metrics.js';
import { Controls } from './controls.js';

class MemoryEngineApp {
    constructor() {
        this.visualizer = null;
        this.metrics = null;
        this.controls = null;
        this.wasmModule = null;
        this.isRunning = false;
        this.currentAllocator = 1;
        this.currentTest = 1;
    }

    async init() {
        this.log('system', '[System] Initializing Memory Engine...');

        // Initialize components
        this.visualizer = new Visualizer('memory-grid', 'contention-graph');
        this.metrics = new MetricsDisplay();
        this.controls = new Controls(this);

        // Try to load WASM module
        await this.loadWasmModule();

        // Initialize visualization
        this.visualizer.init();
        this.visualizer.generateDemoGrid();

        // Setup pressure bars
        this.metrics.initPressureBars();

        this.log('init', '[Init] Memory Engine ready.');
        this.updateStatus('NOMINAL');
    }

    async loadWasmModule() {
        try {
            // Check if WASM is available
            if (typeof Module !== 'undefined') {
                this.wasmModule = Module;
                this.log('init', '[Init] WebAssembly module loaded.');
            } else {
                this.log('warmup', '[System] Running in demo mode (WASM not available).');
            }
        } catch (error) {
            this.log('system', '[Error] Failed to load WASM: ' + error.message);
        }
    }

    setAllocator(type) {
        this.currentAllocator = type;

        if (this.wasmModule && this.wasmModule.setAllocator) {
            this.wasmModule.setAllocator(type);
        }

        const allocatorNames = ['Standard (new/delete)', 'Pool Allocator', 'Stack Allocator', 'Free List Allocator'];
        this.log('config', `[Config] Allocator: ${allocatorNames[type]}`);
    }

    setConcurrencyTest(type) {
        this.currentTest = type;
        const testNames = ['Mutex Contention', 'Atomic Performance', 'Producer-Consumer', 'Thread Creation'];
        this.log('config', `[Config] Concurrency test: ${testNames[type]}`);
    }

    async runSuite() {
        if (this.isRunning) return;

        this.isRunning = true;
        this.updateStatus('RUNNING');
        document.body.classList.add('running');

        const config = this.controls.getConfig();

        this.log('warmup', `[Warmup] Commencing ${config.iterations} averaged iterations...`);
        this.log('config', `[Config] Alignment: ${config.alignment}-byte.`);
        this.log('init', `[Init] Allocated ${config.objectCount.toLocaleString()} objects, size ${this.getObjectSizeName(config.objectSize)}.`);

        // Simulate benchmark if no WASM
        if (!this.wasmModule || !this.wasmModule.runBenchmark) {
            await this.runDemoBenchmark(config);
        } else {
            await this.runRealBenchmark(config);
        }

        this.isRunning = false;
        this.updateStatus('NOMINAL');
        document.body.classList.remove('running');
    }

    async runDemoBenchmark(config) {
        const totalSteps = 100;
        const stepDelay = 50;

        for (let step = 0; step < totalSteps; step++) {
            await this.delay(stepDelay);

            // Update visualization
            const progress = step / totalSteps;
            this.visualizer.updateGridAnimation(progress);

            // Generate demo metrics
            const metrics = this.generateDemoMetrics(progress, config);
            this.metrics.update(metrics);

            // Update contention graph
            this.visualizer.updateContentionGraph(step, metrics.contention);
        }

        // Final results
        const finalMetrics = this.generateFinalMetrics(config);
        this.metrics.update(finalMetrics);
        this.log('init', `[Complete] Benchmark finished. Latency: ${finalMetrics.latency.toFixed(1)}ns`);
    }

    async runRealBenchmark(config) {
        const result = this.wasmModule.runBenchmark(
            config.objectSize,
            config.objectCount,
            config.iterations,
            config.alignment
        );

        this.metrics.update({
            latency: result.meanAllocTime,
            throughput: result.throughput / 1000,
            contention: 0,
            pressure: Math.min(100, (result.peakMemory / (1024 * 1024 * 100)) * 100),
            allocations: Math.floor(result.throughput * config.iterations),
            fragmentation: result.fragmentation,
            peakDepth: 0
        });

        // Run concurrency test
        const concurrencyResult = this.wasmModule.runConcurrencyTest(
            this.currentTest,
            config.threads,
            config.iterations,
            100
        );

        this.metrics.updateConcurrency(concurrencyResult);
    }

    generateDemoMetrics(progress, config) {
        const baseLatency = 20 + Math.random() * 30;
        const allocatorMultiplier = [1.5, 0.3, 0.2, 0.8][this.currentAllocator];

        return {
            latency: baseLatency * allocatorMultiplier * (1 + progress * 0.5),
            throughput: (8 + Math.random() * 6) * (1 / allocatorMultiplier),
            contention: 0.3 + Math.random() * 0.8,
            pressure: Math.min(100, progress * 100 * (1 + Math.random() * 0.2)),
            allocations: Math.floor(config.objectCount * progress * config.iterations),
            fragmentation: 1 + Math.random() * 3,
            peakDepth: Math.floor(progress * 10)
        };
    }

    generateFinalMetrics(config) {
        const allocatorMultiplier = [1.5, 0.3, 0.2, 0.8][this.currentAllocator];
        const baseLatency = 41.4;

        return {
            latency: baseLatency * allocatorMultiplier,
            throughput: 11.6 * (1 / allocatorMultiplier),
            contention: 0.7,
            pressure: 1,
            allocations: 5338399,
            fragmentation: 2.0,
            peakDepth: 0
        };
    }

    getObjectSizeName(size) {
        if (size <= 64) return 'Small Object (64B)';
        if (size <= 256) return 'Medium Object (256B)';
        if (size <= 4096) return 'Large Object (4KB)';
        return 'Huge Object (64KB)';
    }

    log(type, message) {
        const logContainer = document.getElementById('event-log');
        const entry = document.createElement('div');
        entry.className = `log-entry ${type}`;
        entry.textContent = message;

        logContainer.appendChild(entry);
        logContainer.scrollTop = logContainer.scrollHeight;

        // Keep only last 50 entries
        while (logContainer.children.length > 50) {
            logContainer.removeChild(logContainer.firstChild);
        }
    }

    updateStatus(status) {
        const badge = document.getElementById('status-badge');
        badge.textContent = status;
        badge.style.color = status === 'NOMINAL' ? 'var(--accent-green)' : 'var(--accent-yellow)';
    }

    delay(ms) {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
}

// Initialize application
document.addEventListener('DOMContentLoaded', () => {
    const app = new MemoryEngineApp();
    app.init();
    window.memoryEngine = app;
});

export { MemoryEngineApp };

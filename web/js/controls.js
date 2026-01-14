/**
 * UI Controls Manager
 */

class Controls {
    constructor(app) {
        this.app = app;
        this.init();
    }

    init() {
        this.setupAllocatorButtons();
        this.setupConcurrencyButtons();
        this.setupSliders();
        this.setupRunButton();
    }

    setupAllocatorButtons() {
        const container = document.getElementById('allocator-buttons');
        if (!container) return;

        container.querySelectorAll('.btn').forEach(btn => {
            btn.addEventListener('click', () => {
                container.querySelectorAll('.btn').forEach(b => b.classList.remove('active'));
                btn.classList.add('active');

                const allocatorType = parseInt(btn.dataset.allocator);
                this.app.setAllocator(allocatorType);
            });
        });
    }

    setupConcurrencyButtons() {
        const container = document.getElementById('concurrency-buttons');
        if (!container) return;

        container.querySelectorAll('.btn').forEach(btn => {
            btn.addEventListener('click', () => {
                container.querySelectorAll('.btn').forEach(b => b.classList.remove('active'));
                btn.classList.add('active');

                const testType = parseInt(btn.dataset.test);
                this.app.setConcurrencyTest(testType);
            });
        });
    }

    setupSliders() {
        // Object Count Slider
        const objectCountSlider = document.getElementById('object-count');
        const objectCountValue = document.getElementById('object-count-value');

        if (objectCountSlider && objectCountValue) {
            objectCountSlider.addEventListener('input', () => {
                objectCountValue.textContent = parseInt(objectCountSlider.value).toLocaleString();
            });
        }

        // Iterations Slider
        const iterationsSlider = document.getElementById('iterations');
        const iterationsValue = document.getElementById('iterations-value');

        if (iterationsSlider && iterationsValue) {
            iterationsSlider.addEventListener('input', () => {
                iterationsValue.textContent = `${iterationsSlider.value} ITERATIONS`;
            });
        }

        // Threads Slider
        const threadsSlider = document.getElementById('threads');
        const threadsValue = document.getElementById('threads-value');

        if (threadsSlider && threadsValue) {
            threadsSlider.addEventListener('input', () => {
                threadsValue.textContent = `${threadsSlider.value} CORES`;
            });
        }
    }

    setupRunButton() {
        const runBtn = document.getElementById('run-btn');
        if (!runBtn) return;

        runBtn.addEventListener('click', () => {
            this.app.runSuite();
        });
    }

    getConfig() {
        return {
            objectSize: parseInt(document.getElementById('object-size')?.value || 4096),
            objectCount: parseInt(document.getElementById('object-count')?.value || 85500),
            iterations: parseInt(document.getElementById('iterations')?.value || 10),
            alignment: parseInt(document.getElementById('alignment')?.value || 32),
            threads: parseInt(document.getElementById('threads')?.value || 56)
        };
    }
}

export { Controls };

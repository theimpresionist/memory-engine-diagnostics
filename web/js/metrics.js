/**
 * Metrics Display Manager
 */

class MetricsDisplay {
    constructor() {
        this.elements = {
            latency: document.getElementById('latency-value'),
            throughput: document.getElementById('throughput-value'),
            contention: document.getElementById('contention-value'),
            pressure: document.getElementById('pressure-value'),
            allocations: document.getElementById('allocations-value'),
            fragmentation: document.getElementById('fragmentation-value'),
            peakDepth: document.getElementById('peak-depth-value'),
            driftFill: document.getElementById('drift-fill'),
            pressureBars: document.getElementById('pressure-bars')
        };
    }

    initPressureBars() {
        if (!this.elements.pressureBars) return;

        // Create pressure indicator bars
        const barCount = 20;
        this.elements.pressureBars.innerHTML = '';

        for (let i = 0; i < barCount; i++) {
            const bar = document.createElement('div');
            bar.style.cssText = `
                width: 4px;
                height: 12px;
                background: ${i < 5 ? 'var(--accent-green)' : 'var(--bg-secondary)'};
                border-radius: 1px;
            `;
            bar.dataset.index = i;
            this.elements.pressureBars.appendChild(bar);
        }
    }

    update(metrics) {
        if (this.elements.latency) {
            this.elements.latency.textContent = `${metrics.latency.toFixed(1)}ns`;
        }

        if (this.elements.throughput) {
            this.elements.throughput.textContent = `${metrics.throughput.toFixed(1)}k`;
        }

        if (this.elements.contention) {
            this.elements.contention.textContent = `${metrics.contention.toFixed(1)}MS`;
        }

        if (this.elements.pressure) {
            this.elements.pressure.textContent = `${Math.round(metrics.pressure)}%`;
            this.updatePressureBars(metrics.pressure);
            this.updatePressureColor(metrics.pressure);
        }

        if (this.elements.allocations) {
            this.elements.allocations.textContent = metrics.allocations.toLocaleString();
        }

        if (this.elements.fragmentation) {
            this.elements.fragmentation.textContent = `${metrics.fragmentation.toFixed(1)}%`;
        }

        if (this.elements.peakDepth) {
            this.elements.peakDepth.textContent = metrics.peakDepth;
        }

        if (this.elements.driftFill) {
            const driftPercent = Math.min(100, Math.random() * 30);
            this.elements.driftFill.style.width = `${driftPercent}%`;
        }
    }

    updatePressureBars(pressure) {
        if (!this.elements.pressureBars) return;

        const bars = this.elements.pressureBars.children;
        const activeCount = Math.ceil((pressure / 100) * bars.length);

        for (let i = 0; i < bars.length; i++) {
            if (i < activeCount) {
                if (i < bars.length * 0.5) {
                    bars[i].style.background = 'var(--accent-green)';
                } else if (i < bars.length * 0.75) {
                    bars[i].style.background = 'var(--accent-yellow)';
                } else {
                    bars[i].style.background = 'var(--accent-red)';
                }
            } else {
                bars[i].style.background = 'var(--bg-secondary)';
            }
        }
    }

    updatePressureColor(pressure) {
        if (!this.elements.pressure) return;

        if (pressure < 30) {
            this.elements.pressure.style.color = 'var(--accent-green)';
        } else if (pressure < 70) {
            this.elements.pressure.style.color = 'var(--accent-yellow)';
        } else {
            this.elements.pressure.style.color = 'var(--accent-red)';
        }
    }

    updateConcurrency(result) {
        if (this.elements.contention) {
            this.elements.contention.textContent = `${result.contentionTimeMs.toFixed(1)}MS`;
        }
    }

    formatNumber(num) {
        if (num >= 1000000) {
            return (num / 1000000).toFixed(1) + 'M';
        } else if (num >= 1000) {
            return (num / 1000).toFixed(1) + 'K';
        }
        return num.toString();
    }
}

export { MetricsDisplay };

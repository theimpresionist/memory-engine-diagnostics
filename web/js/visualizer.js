/**
 * Memory Grid Visualizer
 */

class Visualizer {
    constructor(gridCanvasId, graphCanvasId) {
        this.gridCanvas = document.getElementById(gridCanvasId);
        this.graphCanvas = document.getElementById(graphCanvasId);
        this.gridCtx = this.gridCanvas?.getContext('2d');
        this.graphCtx = this.graphCanvas?.getContext('2d');

        this.gridData = [];
        this.contentionHistory = [];
        this.cellSize = 12;
        this.cellGap = 2;
        this.columns = 0;
        this.rows = 0;
    }

    init() {
        this.resizeCanvases();
        window.addEventListener('resize', () => this.resizeCanvases());
    }

    resizeCanvases() {
        if (this.gridCanvas) {
            const rect = this.gridCanvas.getBoundingClientRect();
            this.gridCanvas.width = rect.width;
            this.gridCanvas.height = rect.height;

            this.columns = Math.floor(rect.width / (this.cellSize + this.cellGap));
            this.rows = Math.floor(rect.height / (this.cellSize + this.cellGap));

            this.generateDemoGrid();
        }

        if (this.graphCanvas) {
            const rect = this.graphCanvas.getBoundingClientRect();
            this.graphCanvas.width = rect.width;
            this.graphCanvas.height = rect.height;
        }
    }

    generateDemoGrid() {
        const totalCells = this.columns * this.rows;
        this.gridData = [];

        for (let i = 0; i < totalCells; i++) {
            // Random allocation pattern with clusters
            const x = i % this.columns;
            const y = Math.floor(i / this.columns);

            // Create natural-looking clusters
            const noise = this.simplerNoise(x * 0.1, y * 0.1);
            const allocated = noise > 0.3 || Math.random() > 0.7;
            const active = allocated && Math.random() > 0.85;

            this.gridData.push({
                allocated,
                active,
                intensity: Math.random()
            });
        }

        this.drawGrid();
    }

    simplerNoise(x, y) {
        const n = Math.sin(x * 12.9898 + y * 78.233) * 43758.5453;
        return n - Math.floor(n);
    }

    drawGrid() {
        if (!this.gridCtx) return;

        const ctx = this.gridCtx;
        ctx.clearRect(0, 0, this.gridCanvas.width, this.gridCanvas.height);

        const offsetX = (this.gridCanvas.width - this.columns * (this.cellSize + this.cellGap)) / 2;
        const offsetY = (this.gridCanvas.height - this.rows * (this.cellSize + this.cellGap)) / 2;

        for (let i = 0; i < this.gridData.length; i++) {
            const x = i % this.columns;
            const y = Math.floor(i / this.columns);
            const cell = this.gridData[i];

            const posX = offsetX + x * (this.cellSize + this.cellGap);
            const posY = offsetY + y * (this.cellSize + this.cellGap);

            // Cell background
            if (cell.allocated) {
                const intensity = 0.5 + cell.intensity * 0.5;
                ctx.fillStyle = `rgba(35, 134, 54, ${intensity})`;
            } else {
                ctx.fillStyle = '#1a2332';
            }
            ctx.fillRect(posX, posY, this.cellSize, this.cellSize);

            // Active cell border
            if (cell.active) {
                ctx.strokeStyle = '#7ee787';
                ctx.lineWidth = 2;
                ctx.strokeRect(posX, posY, this.cellSize, this.cellSize);
            } else if (cell.allocated) {
                ctx.strokeStyle = '#2d4a3e';
                ctx.lineWidth = 1;
                ctx.strokeRect(posX + 0.5, posY + 0.5, this.cellSize - 1, this.cellSize - 1);
            }
        }
    }

    updateGridAnimation(progress) {
        // Animate allocation pattern
        const waveFront = progress * this.columns * 1.5;

        for (let i = 0; i < this.gridData.length; i++) {
            const x = i % this.columns;
            const y = Math.floor(i / this.columns);
            const distance = x + y * 0.5;

            if (distance < waveFront) {
                const intensity = Math.max(0, 1 - (waveFront - distance) / 10);
                this.gridData[i].allocated = true;
                this.gridData[i].active = intensity > 0.8;
                this.gridData[i].intensity = 0.5 + intensity * 0.5;
            }
        }

        this.drawGrid();
    }

    updateContentionGraph(step, value) {
        if (!this.graphCtx) return;

        this.contentionHistory.push(value);
        if (this.contentionHistory.length > 50) {
            this.contentionHistory.shift();
        }

        const ctx = this.graphCtx;
        const width = this.graphCanvas.width;
        const height = this.graphCanvas.height;

        ctx.clearRect(0, 0, width, height);

        // Draw grid
        ctx.strokeStyle = '#1e2a38';
        ctx.lineWidth = 1;
        for (let i = 0; i < 5; i++) {
            const y = (height / 5) * i;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(width, y);
            ctx.stroke();
        }

        // Draw line
        ctx.strokeStyle = '#f85149';
        ctx.lineWidth = 2;
        ctx.beginPath();

        const stepWidth = width / 50;
        this.contentionHistory.forEach((val, idx) => {
            const x = idx * stepWidth;
            const y = height - (val / 2) * height;

            if (idx === 0) {
                ctx.moveTo(x, y);
            } else {
                ctx.lineTo(x, y);
            }
        });

        ctx.stroke();
    }

    setGridFromData(boolArray) {
        this.gridData = boolArray.map(allocated => ({
            allocated,
            active: allocated && Math.random() > 0.9,
            intensity: 0.5 + Math.random() * 0.5
        }));
        this.drawGrid();
    }
}

export { Visualizer };

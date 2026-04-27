(() => {
    try {
        if (window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches) return;
    } catch (e) {
        // ignore
    }

    const existing = document.getElementById('glitterCanvas');
    const canvas = existing || document.createElement('canvas');
    canvas.id = 'glitterCanvas';
    if (!existing) document.body.appendChild(canvas);

    const ctx = canvas.getContext('2d', { alpha: true });
    if (!ctx) return;

    const dpr = () => Math.max(1, Math.min(2, window.devicePixelRatio || 1));
    let width = 0;
    let height = 0;
    let ratio = dpr();

    const resize = () => {
        ratio = dpr();
        width = Math.floor(window.innerWidth);
        height = Math.floor(window.innerHeight);
        canvas.width = Math.floor(width * ratio);
        canvas.height = Math.floor(height * ratio);
        canvas.style.width = width + 'px';
        canvas.style.height = height + 'px';
        ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    };

    const rand = (min, max) => min + Math.random() * (max - min);
    const clamp = (v, a, b) => Math.max(a, Math.min(b, v));

    const sparkles = [];
    const maxSparkles = 80;
    const palette = ['#67e8f9', '#f6c967', '#a78bfa', '#34d399'];

    const makeSparkle = () => {
        const size = rand(0.8, 2.6);
        const speed = rand(0.15, 0.65);
        return {
            x: rand(0, width),
            y: rand(0, height),
            r: size,
            vx: rand(-0.25, 0.25) * speed,
            vy: rand(-0.6, -0.1) * speed,
            a: rand(0.25, 0.9),
            life: rand(60, 220),
            hue: palette[(Math.random() * palette.length) | 0]
        };
    };

    const ensurePopulation = () => {
        while (sparkles.length < maxSparkles) sparkles.push(makeSparkle());
    };

    const drawSparkle = (s) => {
        const alpha = clamp(s.a, 0, 1);
        const g = ctx.createRadialGradient(s.x, s.y, 0, s.x, s.y, s.r * 6);
        g.addColorStop(0, `rgba(255,255,255,${alpha})`);
        g.addColorStop(0.35, `${hexToRgba(s.hue, alpha * 0.55)}`);
        g.addColorStop(1, 'rgba(0,0,0,0)');
        ctx.fillStyle = g;
        ctx.beginPath();
        ctx.arc(s.x, s.y, s.r * 6, 0, Math.PI * 2);
        ctx.fill();

        // tiny star cross
        ctx.strokeStyle = `rgba(255,255,255,${alpha * 0.55})`;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(s.x - s.r * 1.8, s.y);
        ctx.lineTo(s.x + s.r * 1.8, s.y);
        ctx.moveTo(s.x, s.y - s.r * 1.8);
        ctx.lineTo(s.x, s.y + s.r * 1.8);
        ctx.stroke();
    };

    const hexToRgba = (hex, a) => {
        const h = String(hex || '').replace('#', '');
        if (h.length !== 6) return `rgba(255,255,255,${a})`;
        const r = parseInt(h.slice(0, 2), 16);
        const g = parseInt(h.slice(2, 4), 16);
        const b = parseInt(h.slice(4, 6), 16);
        return `rgba(${r},${g},${b},${a})`;
    };

    let tick = 0;
    const loop = () => {
        tick++;
        ensurePopulation();

        ctx.clearRect(0, 0, width, height);
        ctx.globalCompositeOperation = 'lighter';

        for (let i = sparkles.length - 1; i >= 0; i--) {
            const s = sparkles[i];

            s.x += s.vx;
            s.y += s.vy;
            s.life -= 1;

            // soft twinkle
            s.a += Math.sin((tick + i) * 0.06) * 0.004;

            drawSparkle(s);

            if (s.life <= 0 || s.y < -20 || s.x < -40 || s.x > width + 40) {
                sparkles[i] = makeSparkle();
                sparkles[i].y = rand(height * 0.4, height + 20);
            }
        }

        ctx.globalCompositeOperation = 'source-over';
        requestAnimationFrame(loop);
    };

    resize();
    window.addEventListener('resize', resize, { passive: true });
    requestAnimationFrame(loop);
})();


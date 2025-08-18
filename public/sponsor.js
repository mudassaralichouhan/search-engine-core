/* Header hide/show on scroll */
(function() {
    const header = document.getElementById('site-header');
    let prevY = window.scrollY;
    let ticking = false;
    const onScroll = () => {
        const y = window.scrollY;
        const goingDown = y > prevY && y > 24;
        header.classList.toggle('header--hidden', goingDown);
        prevY = y;
        ticking = false;
    };
    window.addEventListener('scroll', () => {
        if (!ticking) { requestAnimationFrame(onScroll); ticking = true; }
    }, { passive: true });
})();

/* Theme: dark by default, respect system preference, allow toggle */
(function() {
    const root = document.documentElement;
    const btn = document.getElementById('themeToggle');
    const storageKey = 'hatef-theme';

    function getSystemTheme() {
        return window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark';
    }

    function applyTheme(theme) {
        root.setAttribute('data-theme', theme);
        // Update button icon and labels
        if (btn) {
            const isDark = theme !== 'light';
            btn.textContent = isDark ? '🌙' : '☀️';
            btn.setAttribute('aria-label', isDark ? 'Switch to light mode' : 'Switch to dark mode');
            btn.setAttribute('aria-pressed', isDark ? 'true' : 'false');
        }
    }

    const saved = localStorage.getItem(storageKey);
    const initial = saved || getSystemTheme(); // default dark unless prefers light
    applyTheme(initial);

    btn?.addEventListener('click', () => {
        const current = root.getAttribute('data-theme') || initial;
        const next = current === 'light' ? 'dark' : 'light';
        applyTheme(next);
        localStorage.setItem(storageKey, next);
    });

    // React to system changes if user hasn't set a manual preference
    const mq = window.matchMedia('(prefers-color-scheme: light)');
    mq.addEventListener?.('change', (e) => {
        const manual = localStorage.getItem(storageKey);
        if (!manual) applyTheme(e.matches ? 'light' : 'dark');
    });
})();

/* Tier data: loaded from translations when present */
/* Utilities */
const money0 = new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD', maximumFractionDigits: 0 });
function formatUSD(n) { return money0.format(n); }

/* Tiers are now rendered server-side in the template. We only wire up the reveal interaction. */
(function wireSeeAllReveal(){
    const grid = document.getElementById('tiersGrid');
    if (!grid) return;
    const btn = document.getElementById('seeAllTiers');
    if (!btn) return;
    const hiddenCards = Array.from(grid.querySelectorAll('.extra-tier.hide'));
    btn.addEventListener('click', (e) => {
        e.preventDefault();
        hiddenCards.forEach((c, idx) => {
            c.classList.remove('hide');
            c.classList.add('reveal');
            c.style.animationDelay = (idx * 60) + 'ms';
            c.addEventListener('animationend', () => c.classList.remove('reveal'), { once: true });
        });
        btn.remove();
    });
})();

/* Build IRR tier select: now server-rendered; no JS population needed */

function preselectTier(tierId) {
    const sel = document.getElementById('tierSelect');
    if (!sel) return;
    sel.value = tierId;
}

/* Payment modals */
const irrModal = document.getElementById('irr-modal');
const btcModal = document.getElementById('btc-modal');
let lastFocus = null;
function openModal(id) {
    const dlg = document.getElementById(id);
    if (!dlg) return;
    lastFocus = document.activeElement;
    if (typeof dlg.showModal === 'function') dlg.showModal(); else dlg.setAttribute('open', '');
    const focusable = dlg.querySelector('button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])');
    (focusable || dlg).focus();
}
function closeModal(dlg) {
    if (!dlg) return;
    if (typeof dlg.close === 'function') dlg.close(); else dlg.removeAttribute('open');
    if (lastFocus) lastFocus.focus();
}
document.addEventListener('click', (e) => {
    const openId = e.target.closest?.('[data-open]')?.getAttribute('data-open');
    if (openId) {
        e.preventDefault();
        openModal(openId);
    }
    if (e.target.matches?.('[data-close]')) {
        const dlg = e.target.closest('.modal');
        closeModal(dlg);
    }
});
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        if (irrModal?.open) closeModal(irrModal);
        if (btcModal?.open) closeModal(btcModal);
    }
});

/* Copy BTC address */
(function() {
    const btn = document.getElementById('copyBtc');
    const addr = document.getElementById('btcAddr');
    btn?.addEventListener('click', async () => {
        try {
            await navigator.clipboard.writeText(addr.textContent.trim());
            const grid = document.getElementById('tiersGrid');
            btn.textContent = (grid?.dataset?.copied) || 'Copied';
            setTimeout(() => (btn.textContent = (grid?.dataset?.copy || 'Copy')), 1200);
        } catch {
            const grid = document.getElementById('tiersGrid');
            btn.textContent = (grid?.dataset?.copyFailed) || 'Copy failed';
        }
    });
})();

/* Accordion */
(function() {
    const acc = document.getElementById('faqAcc');
    acc.querySelectorAll('.acc-item').forEach(item => {
        const btn = item.querySelector('.acc-btn');
        const panel = document.getElementById(btn.getAttribute('aria-controls'));
        btn.addEventListener('click', () => {
            const expanded = item.getAttribute('aria-expanded') === 'true';
            // Close any other open items (optional: keep multiple open? comment next 4 lines to allow multiple)
            // acc.querySelectorAll('.acc-item[aria-expanded="true"]').forEach(openItem => {
            //     if (openItem !== item) {
            //         openItem.setAttribute('aria-expanded', 'false');
            //         const openPanel = document.getElementById(openItem.querySelector('.acc-btn').getAttribute('aria-controls'));
            //         if (openPanel) openPanel.style.maxHeight = '0px';
            //     }
            // });

            item.setAttribute('aria-expanded', String(!expanded));
            btn.setAttribute('aria-expanded', String(!expanded));
            if (!expanded) {
                // opening
                panel.style.maxHeight = panel.scrollHeight + 'px';
            } else {
                // closing
                panel.style.maxHeight = '0px';
            }
        });
    });
})();

/* Footer year */
document.getElementById('year').textContent = new Date().getFullYear();


// TIER_DATA removed; tiers are rendered server-side
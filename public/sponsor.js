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

/* Tier data (editable) */
const TIER_DATA = [
    {
        id: "individual",
        name: "Individual Backer",
        priceUsdYear: 99,
        priceUsdMonth: 9,
        benefits: [
            "Backer badge on profile",
            "Early access to new features",
            "Community voting on public RFCs (non-ranking)"
        ]
    },
    {
        id: "startup",
        name: "Startup",
        priceUsdYear: 1000,
        benefits: [
            "Logo on sponsor wall",
            "Webmaster Pro (limited quotas)",
            "Quarterly roadmap call invite"
        ]
    },
    {
        id: "silver",
        name: "Silver",
        priceUsdYear: 20000,
        benefits: [
            "Expanded API quota",
            "Co-marketing (blog/webinar slot)",
            "Access to working groups"
        ]
    },
    {
        id: "gold",
        name: "Gold",
        priceUsdYear: 100000,
        benefits: [
            "Engineering office hours (priority)",
            "Feature requests (non-ranking) consideration",
            "Major brand placement on site"
        ]
    },
    {
        id: "platinum",
        name: "Platinum / Strategic",
        priceUsdYear: 250000,
        priceNote: "minimum",
        benefits: [
            "Advisory Council seat (non-governance over ranking)",
            "Joint research initiatives (Privacy/Crawling)",
            "Highest API quota & co-marketing"
        ]
    },
    {
        id: "infrastructure",
        name: "Infrastructure Sponsor",
        priceUsdYear: 0,
        priceNote: "In-kind hardware/bandwidth (USD equivalent agreed)",
        benefits: [
            "“Powered by …” attribution",
            "Sponsor wall with usage reports",
            "Public acknowledgment in release notes"
        ]
    }
];

/* Utilities */
const money0 = new Intl.NumberFormat('en-US', { style: 'currency', currency: 'USD', maximumFractionDigits: 0 });
function formatUSD(n) { return money0.format(n); }

/* Render tiers */
(function renderTiers() {
    const grid = document.getElementById('tiersGrid');
    const frag = document.createDocumentFragment();
    const INITIAL_VISIBLE_TIERS = new Set(['individual','silver','platinum']);
    const hiddenCards = [];
    TIER_DATA.forEach(t => {
        const card = document.createElement('article');
        card.className = 'card';
        card.id = t.id;

        const anchor = document.createElement('div');
        anchor.className = 'tier-anchor';
        anchor.innerHTML = `<a href="#${t.id}" aria-label="Link to ${t.name}">#${t.id}</a>`;

        const title = document.createElement('h3');
        title.textContent = t.name;

        const price = document.createElement('div');
        price.className = 'price';
        if (t.priceUsdYear > 0) {
            const yr = `${formatUSD(t.priceUsdYear)}<span class="per">/yr</span>`;
            const mo = t.priceUsdMonth ? ` <span class="note">(${formatUSD(t.priceUsdMonth)}/mo)</span>` : '';
            price.innerHTML = yr + mo + (t.priceNote ? ` <span class="note">(${t.priceNote})</span>` : '');
        } else {
            price.innerHTML = `<span class="note">${t.priceNote || 'In-kind support'}</span>`;
        }

        const ul = document.createElement('ul');
        ul.className = 'benefits';
        t.benefits.forEach(b => {
            const li = document.createElement('li');
            li.innerHTML = `<svg class="icon" viewBox="0 0 24 24" aria-hidden="true" style="vertical-align: -3px; margin-right:.35rem"><path fill="currentColor" d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z"/></svg>${b}`;
            ul.appendChild(li);
        });

        const actions = document.createElement('div');
        const btn = document.createElement('a');
        btn.href = '#payment';
        btn.className = 'btn btn-primary';
        btn.textContent = 'Sponsor now';
        btn.setAttribute('data-tier', t.id);
        btn.addEventListener('click', (e) => {
            e.preventDefault();
            document.getElementById('payment').scrollIntoView({ behavior: 'smooth', block: 'start' });
            setTimeout(() => { openModal('irr-modal'); preselectTier(t.id); }, 300);
        });
        actions.appendChild(btn);

        card.append(anchor, title, price, ul, actions);
        // Hide non-initial tiers
        if (!INITIAL_VISIBLE_TIERS.has(t.id)) {
            card.classList.add('hide', 'extra-tier');
            hiddenCards.push(card);
        }
        frag.appendChild(card);
    });
    grid.appendChild(frag);
    grid.setAttribute('aria-busy', 'false');

    // Add "See all tiers" reveal link if there are hidden tiers
    if (hiddenCards.length > 0) {
        const revealWrap = document.createElement('div');
        revealWrap.className = 'see-all';
        const a = document.createElement('a');
        a.href = '#tiers';
        a.className = 'btn btn-outline';
        a.textContent = 'See all tiers';
        a.setAttribute('aria-controls', 'tiersGrid');
        a.setAttribute('aria-expanded', 'false');
        a.addEventListener('click', (e) => {
            e.preventDefault();
            hiddenCards.forEach((c, idx) => {
                c.classList.remove('hide');
                // Staggered reveal animation
                c.classList.add('reveal');
                c.style.animationDelay = (idx * 60) + 'ms';
                // Clean up the class after animation ends
                c.addEventListener('animationend', () => c.classList.remove('reveal'), { once: true });
            });
            a.setAttribute('aria-expanded', 'true');
            revealWrap.remove();
        });
        revealWrap.appendChild(a);
        grid.parentNode.appendChild(revealWrap);
    }
})();

/* Build IRR tier select */
(function buildTierSelect() {
    const sel = document.getElementById('tierSelect');
    const def = document.createElement('option');
    def.value = '';
    def.textContent = 'Select a tier…';
    def.disabled = true; def.selected = true;
    sel.appendChild(def);
    TIER_DATA.forEach(t => {
        const opt = document.createElement('option');
        opt.value = t.id;
        const yr = t.priceUsdYear > 0 ? `${formatUSD(t.priceUsdYear)}/yr` : 'In-kind';
        opt.textContent = `${t.name} — ${yr}`;
        sel.appendChild(opt);
    });
})();

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
            btn.textContent = 'Copied';
            setTimeout(() => (btn.textContent = 'Copy'), 1200);
        } catch { btn.textContent = 'Copy failed'; }
    });
})();

/* Accordion */
(function() {
    const acc = document.getElementById('faqAcc');
    acc.querySelectorAll('.acc-item').forEach(item => {
        const btn = item.querySelector('.acc-btn');
        btn.addEventListener('click', () => {
            const expanded = item.getAttribute('aria-expanded') === 'true';
            item.setAttribute('aria-expanded', String(!expanded));
            btn.setAttribute('aria-expanded', String(!expanded));
        });
    });
})();

/* Footer year */
document.getElementById('year').textContent = new Date().getFullYear();



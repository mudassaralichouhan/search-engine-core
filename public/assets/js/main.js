// ===================================
// Search Engine Core JavaScript
// ===================================

// Global configuration
const SearchConfig = {
    API_BASE_URL: '/api', // Update this with your actual API endpoint
    DEBOUNCE_DELAY: 300,
    MIN_QUERY_LENGTH: 2,
    MAX_SUGGESTIONS: 10,
    STORAGE_PREFIX: 'search_engine_'
};

// ===================================
// Utility Functions
// ===================================

// Debounce function for search input
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

// Storage utilities
const Storage = {
    set(key, value) {
        try {
            localStorage.setItem(SearchConfig.STORAGE_PREFIX + key, JSON.stringify(value));
        } catch (e) {
            console.error('Storage error:', e);
        }
    },
    
    get(key) {
        try {
            const item = localStorage.getItem(SearchConfig.STORAGE_PREFIX + key);
            return item ? JSON.parse(item) : null;
        } catch (e) {
            console.error('Storage error:', e);
            return null;
        }
    },
    
    remove(key) {
        localStorage.removeItem(SearchConfig.STORAGE_PREFIX + key);
    }
};

// ===================================
// Search Functionality
// ===================================

class SearchEngine {
    constructor() {
        this.searchInput = null;
        this.searchButton = null;
        this.suggestionsContainer = null;
        this.recentSearches = Storage.get('recent_searches') || [];
    }

    init() {
        this.bindElements();
        this.attachEventListeners();
        this.setupKeyboardShortcuts();
    }

    bindElements() {
        this.searchInput = document.getElementById('searchInput');
        this.searchButton = document.getElementById('searchButton');
        this.suggestionsContainer = document.getElementById('searchSuggestions');
    }

    attachEventListeners() {
        if (this.searchInput) {
            this.searchInput.addEventListener('input', debounce((e) => {
                this.handleSearchInput(e.target.value);
            }, SearchConfig.DEBOUNCE_DELAY));

            this.searchInput.addEventListener('keydown', (e) => {
                if (e.key === 'Enter') {
                    e.preventDefault();
                    this.performSearch(e.target.value);
                }
            });
        }

        if (this.searchButton) {
            this.searchButton.addEventListener('click', () => {
                this.performSearch(this.searchInput.value);
            });
        }
    }

    setupKeyboardShortcuts() {
        document.addEventListener('keydown', (e) => {
            // Ctrl/Cmd + K to focus search
            if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
                e.preventDefault();
                if (this.searchInput) {
                    this.searchInput.focus();
                    this.searchInput.select();
                }
            }
            
            // Escape to clear search
            if (e.key === 'Escape' && this.searchInput === document.activeElement) {
                this.searchInput.value = '';
                this.hideSuggestions();
            }
        });
    }

    async handleSearchInput(query) {
        if (query.length < SearchConfig.MIN_QUERY_LENGTH) {
            this.hideSuggestions();
            return;
        }

        try {
            const suggestions = await this.fetchSuggestions(query);
            this.displaySuggestions(suggestions);
        } catch (error) {
            console.error('Error fetching suggestions:', error);
        }
    }

    async fetchSuggestions(query) {
        // In a real application, this would make an API call
        // For now, returning mock suggestions
        return new Promise((resolve) => {
            setTimeout(() => {
                const mockSuggestions = [
                    `${query} tutorial`,
                    `${query} examples`,
                    `${query} documentation`,
                    `${query} best practices`,
                    `${query} vs`,
                ];
                resolve(mockSuggestions.slice(0, SearchConfig.MAX_SUGGESTIONS));
            }, 100);
        });
    }

    displaySuggestions(suggestions) {
        if (!this.suggestionsContainer || suggestions.length === 0) {
            this.hideSuggestions();
            return;
        }

        const html = suggestions.map((suggestion, index) => `
            <div class="suggestion-item" data-index="${index}" tabindex="0">
                <svg class="suggestion-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="11" cy="11" r="8"></circle>
                    <path d="m21 21-4.35-4.35"></path>
                </svg>
                <span>${this.highlightMatch(suggestion, this.searchInput.value)}</span>
            </div>
        `).join('');

        this.suggestionsContainer.innerHTML = html;
        this.suggestionsContainer.classList.remove('hidden');

        // Add click handlers to suggestions
        this.suggestionsContainer.querySelectorAll('.suggestion-item').forEach((item) => {
            item.addEventListener('click', () => {
                const text = item.textContent.trim();
                this.searchInput.value = text;
                this.performSearch(text);
            });
        });
    }

    highlightMatch(text, query) {
        const regex = new RegExp(`(${query})`, 'gi');
        return text.replace(regex, '<strong>$1</strong>');
    }

    hideSuggestions() {
        if (this.suggestionsContainer) {
            this.suggestionsContainer.classList.add('hidden');
        }
    }

    performSearch(query) {
        const trimmedQuery = query.trim();
        
        if (!trimmedQuery) {
            return;
        }

        // Show loading state
        if (this.searchButton) {
            this.searchButton.classList.add('loading');
        }

        // Save to recent searches
        this.saveRecentSearch(trimmedQuery);

        // In a real application, redirect to results page or perform search
        setTimeout(() => {
            if (this.searchButton) {
                this.searchButton.classList.remove('loading');
            }
            
            // Redirect to results page with query parameter
            window.location.href = `/search.html?q=${encodeURIComponent(trimmedQuery)}`;
        }, 500);
    }

    saveRecentSearch(query) {
        // Remove duplicates and limit to 10 recent searches
        this.recentSearches = [query, ...this.recentSearches.filter(q => q !== query)].slice(0, 10);
        Storage.set('recent_searches', this.recentSearches);
    }

    getRecentSearches() {
        return this.recentSearches;
    }

    clearRecentSearches() {
        this.recentSearches = [];
        Storage.remove('recent_searches');
    }
}

// ===================================
// Page Navigation
// ===================================

class Navigation {
    constructor() {
        this.mobileMenuButton = null;
        this.navLinks = null;
    }

    init() {
        this.bindElements();
        this.attachEventListeners();
        this.highlightActiveLink();
    }

    bindElements() {
        this.mobileMenuButton = document.getElementById('mobileMenuButton');
        this.navLinks = document.querySelector('.nav-links');
    }

    attachEventListeners() {
        if (this.mobileMenuButton && this.navLinks) {
            this.mobileMenuButton.addEventListener('click', () => {
                this.toggleMobileMenu();
            });
        }
    }

    toggleMobileMenu() {
        this.navLinks.classList.toggle('nav-links--mobile-active');
        this.mobileMenuButton.classList.toggle('active');
    }

    highlightActiveLink() {
        const currentPath = window.location.pathname;
        document.querySelectorAll('.nav-link').forEach(link => {
            if (link.getAttribute('href') === currentPath) {
                link.classList.add('active');
            }
        });
    }
}

// ===================================
// Theme Management
// ===================================

class ThemeManager {
    constructor() {
        this.theme = Storage.get('theme') || 'light';
    }

    init() {
        this.applyTheme();
        this.bindThemeToggle();
    }

    applyTheme() {
        document.documentElement.setAttribute('data-theme', this.theme);
    }

    bindThemeToggle() {
        const themeToggle = document.getElementById('themeToggle');
        if (themeToggle) {
            themeToggle.addEventListener('click', () => {
                this.toggleTheme();
            });
        }
    }

    toggleTheme() {
        this.theme = this.theme === 'light' ? 'dark' : 'light';
        Storage.set('theme', this.theme);
        this.applyTheme();
    }
}

// ===================================
// Initialize on DOM Ready
// ===================================

document.addEventListener('DOMContentLoaded', () => {
    // Initialize search engine
    const searchEngine = new SearchEngine();
    searchEngine.init();

    // Initialize navigation
    const navigation = new Navigation();
    navigation.init();

    // Initialize theme manager
    const themeManager = new ThemeManager();
    themeManager.init();

    // Make search engine available globally for other scripts
    window.searchEngine = searchEngine;
});

// ===================================
// Export for use in other modules
// ===================================

// If using modules
if (typeof module !== 'undefined' && module.exports) {
    module.exports = {
        SearchEngine,
        Navigation,
        ThemeManager,
        Storage,
        debounce
    };
} 
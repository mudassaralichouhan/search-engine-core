// Sample JavaScript file for testing minification
// This file contains various JavaScript features to test different minification levels

/**
 * Calculate total price with tax
 * @param {Array} items - Array of items with price and quantity
 * @param {number} taxRate - Tax rate as decimal (default: 0.08)
 * @returns {Object} Object containing subtotal, tax, and total
 */
function calculateTotalPrice(items, taxRate = 0.08) {
    let subtotal = 0;
    
    // Calculate subtotal for all items
    for (const item of items) {
        if (item.price && item.quantity) {
            subtotal += item.price * item.quantity;
            console.log('Processing item:', item.name);
        }
    }
    
    // Calculate tax amount
    const taxAmount = subtotal * taxRate;
    const total = subtotal + taxAmount;
    
    // Debug information (will be removed in aggressive mode)
    const debugInfo = 'Debug mode enabled';
    debugger;
    
    console.log('Subtotal:', subtotal);
    console.log('Tax:', taxAmount);
    console.log('Total:', total);
    
    return {
        subtotal: subtotal,
        tax: taxAmount,
        total: total
    };
}

// Utility functions
const utils = {
    /**
     * Format currency
     * @param {number} amount - Amount to format
     * @param {string} currency - Currency code (default: 'USD')
     * @returns {string} Formatted currency string
     */
    formatCurrency: function(amount, currency = 'USD') {
        return new Intl.NumberFormat('en-US', {
            style: 'currency',
            currency: currency
        }).format(amount);
    },
    
    /**
     * Debounce function
     * @param {Function} func - Function to debounce
     * @param {number} wait - Wait time in milliseconds
     * @returns {Function} Debounced function
     */
    debounce: function(func, wait) {
        let timeout;
        return function executedFunction(...args) {
            const later = () => {
                clearTimeout(timeout);
                func(...args);
            };
            clearTimeout(timeout);
            timeout = setTimeout(later, wait);
        };
    },
    
    /**
     * Validate email address
     * @param {string} email - Email to validate
     * @returns {boolean} True if valid email
     */
    validateEmail: function(email) {
        const regex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return regex.test(email);
    }
};

// Unused function (will be removed in aggressive mode)
function unusedFunction() {
    console.log('This function is never called');
    return 'unused';
}

// Class for handling shopping cart
class ShoppingCart {
    constructor() {
        this.items = [];
        this.taxRate = 0.08;
    }
    
    addItem(item) {
        this.items.push(item);
        console.log('Added item to cart:', item.name);
    }
    
    removeItem(itemId) {
        this.items = this.items.filter(item => item.id !== itemId);
        console.log('Removed item from cart:', itemId);
    }
    
    getTotal() {
        return calculateTotalPrice(this.items, this.taxRate);
    }
    
    clear() {
        this.items = [];
        console.log('Cart cleared');
    }
}

// Export functions and classes
module.exports = {
    calculateTotalPrice,
    utils,
    ShoppingCart
};

// Example usage (will be removed in aggressive mode)
if (typeof window !== 'undefined') {
    // Browser environment
    window.calculator = {
        calculateTotalPrice,
        utils,
        ShoppingCart
    };
}

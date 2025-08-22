// Sample JavaScript code for testing minification
function calculateTotal(items) {
    let total = 0;
    console.log('Starting calculation for', items.length, 'items');
    
    // Loop through each item
    for (let i = 0; i < items.length; i++) {
        const item = items[i];
        if (item.price && item.quantity) {
            total += item.price * item.quantity;
            console.log('Added item:', item.name, 'Total so far:', total);
        } else {
            console.warn('Invalid item:', item);
        }
    }
    
    // Unused variable for testing dead code elimination
    const unusedVariable = 'This should be removed in advanced minification';
    debugger; // This should be removed in production
    
    console.log('Final total:', total);
    return total;
}

/* Multi-line comment that should be removed
   This comment spans multiple lines
   and contains various information */

// Create some test data
const testItems = [
    { name: 'Widget A', price: 10.50, quantity: 2 },
    { name: 'Widget B', price: 25.00, quantity: 1 },
    { name: 'Widget C', price: 15.75, quantity: 3 }
];

// Calculate the result
const result = calculateTotal(testItems);

// Another function for more complex testing
function processUserData(userData) {
    if (!userData || typeof userData !== 'object') {
        throw new Error('Invalid user data provided');
    }
    
    const processed = {
        id: userData.id || Math.random().toString(36),
        name: userData.name ? userData.name.trim().toLowerCase() : 'anonymous',
        email: userData.email ? userData.email.toLowerCase() : null,
        createdAt: new Date().toISOString(),
        isActive: userData.isActive !== false // default to true
    };
    
    // Validation
    if (processed.email && !processed.email.includes('@')) {
        console.warn('Invalid email format:', processed.email);
        processed.email = null;
    }
    
    return processed;
}

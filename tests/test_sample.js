// Sample JavaScript file for testing minification
var userName = "John Doe";
var userAge = 25;

/* 
 * Multi-line comment
 * This function calculates something
 */
function calculateUserInfo(name, age) {
    // Check if name is valid
    if (name && name.length > 0) {
        console.log("User name: " + name);
    }
    
    // Check age
    if (age > 0) {
        console.log("User age: " + age);
    }
    
    return {
        name: name,
        age: age,
        isAdult: age >= 18
    };
}

// Call the function
var result = calculateUserInfo(userName, userAge);
console.log("Result:", result);

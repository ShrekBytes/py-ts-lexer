// This is a sample TypeScript file
// Testing the lexical analyzer

/*
 * This is a multiline comment
 * for testing purposes
 */

function calculate(x: number, y: number): number {
    // Add two numbers
    let result: number = x + y;
    return result;
}

// Variable with type annotations
let count: number = "hello";    // Type mismatch: number declared, string assigned
let name: string = 42;          // Type mismatch: string declared, number assigned
const isValid: boolean = "yes"; // Type mismatch: boolean declared, string assigned

// Misspelled keywords
funtion doSomething() {         // Should be 'function'
    cosnt x = 5;                // Should be 'const'
}

// Undeclared variable usage
let total = countr + 5;         // 'countr' is undeclared

// Invalid operators
if (x =< 10) {                  // Should be <=
    console.log("test");
}

/* 
   Another multiline
   comment block
*/


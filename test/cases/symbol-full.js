console.log("--- Symbol Full Test ---");

// Constructor
const s1 = Symbol("foo");
const s2 = Symbol("foo");
console.log(s1 === s2); 
console.log(s1.toString()); 
console.log(s1.description); 

const s3 = Symbol();
console.log(s3.toString()); 
console.log(s3.description); 

// Symbol.for / keyFor
const sFor1 = Symbol.for("bar");
const sFor2 = Symbol.for("bar");
console.log(sFor1 === sFor2); 
console.log(Symbol.keyFor(sFor1)); 
console.log(Symbol.keyFor(s1)); 

// Well-known symbols
console.log(typeof Symbol.iterator); 
console.log(typeof Symbol.asyncIterator); 
console.log(Symbol.iterator.toString()); 

// valueOf
console.log(s1.valueOf() === s1); 

// Check type
console.log(typeof s1); 

// Check property access on symbol
console.log(s1["description"]);

// Check new Symbol() throws TypeError
try {
    new Symbol();
} catch (e) {
    console.log("Caught expected error:", e.name);
}
 

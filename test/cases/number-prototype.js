console.log("--- Number Prototype ---");

const n = 123.456;

// toFixed
console.log("toFixed(0):", n.toFixed(0));
console.log("toFixed(2):", n.toFixed(2));
console.log("toFixed(5):", n.toFixed(5));

// toExponential
console.log("toExponential(1):", n.toExponential(1));
console.log("toExponential():", (123456).toExponential());

// toPrecision
console.log("toPrecision(2):", n.toPrecision(2));
console.log("toPrecision(5):", n.toPrecision(5));

// toString with radix
const num = 255;
console.log("toString(16):", num.toString(16));
console.log("toString(2):", num.toString(2));
console.log("toString(10):", num.toString(10));

// valueOf
const prim = n.valueOf();
console.log("valueOf:", prim, typeof prim === "number");

// toLocaleString (basic check)
console.log("toLocaleString:", n.toLocaleString());

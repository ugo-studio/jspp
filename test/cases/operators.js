console.log("--- Operators ---");

let a = 10;
let b = 5;

console.log("a:", a, "b:", b);

// Arithmetic Operators
console.log("a + b =", a + b);
console.log("a - b =", a - b);
console.log("a * b =", a * b);
console.log("a / b =", a / b);
console.log("a % b =", a % b);
console.log("a ** 2 =", a ** 2);

// Increment and Decrement
a++;
console.log("a++:", a);
b--;
console.log("b--:", b);
console.log("++a:", ++a);
console.log("--b:", --b);

// Assignment Operators
a += 5;
console.log("a += 5:", a);
a -= 5;
console.log("a -= 5:", a);
a *= 2;
console.log("a *= 2:", a);
a /= 2;
console.log("a /= 2:", a);
a %= 3;
console.log("a %= 3:", a);

// Comparison Operators
console.log("a > b:", a > b);
console.log("a < b:", a < b);
console.log("a >= b:", a >= b);
console.log("a <= b:", a <= b);
console.log("a != b:", a != b);

// Bitwise Operators
let c = 5; // 0101
let d = 3; // 0011
console.log("c & d:", c & d); // 0001 (1)
console.log("c | d:", c | d); // 0111 (7)
console.log("c ^ d:", c ^ d); // 0110 (6)
console.log("~c:", ~c);
console.log("c << 1:", c << 1); // 1010 (10)
console.log("c >> 1:", c >> 1); // 0010 (2)

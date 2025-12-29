console.log("--- Math Object ---");

console.log("Math.PI:", Math.PI);
console.log("Math.E:", Math.E);
console.log("Math.SQRT2:", Math.SQRT2);

console.log("Math.abs(-5):", Math.abs(-5));
console.log("Math.ceil(4.2):", Math.ceil(4.2));
console.log("Math.floor(4.8):", Math.floor(4.8));
console.log("Math.round(4.5):", Math.round(4.5));
console.log("Math.round(4.4):", Math.round(4.4));
console.log("Math.round(-4.5):", Math.round(-4.5)); // Expect -4 (ties towards +Infinity)
console.log("Math.round(-4.51):", Math.round(-4.51)); // Expect -5

console.log("Math.max(1, 5, 2):", Math.max(1, 5, 2));
console.log("Math.min(1, 5, 2):", Math.min(1, 5, 2));

console.log("Math.pow(2, 3):", Math.pow(2, 3));
console.log("Math.sqrt(9):", Math.sqrt(9));

console.log("Math.sign(-10):", Math.sign(-10));
console.log("Math.sign(0):", Math.sign(0));
console.log("Math.sign(10):", Math.sign(10));

console.log("Math.imul(5, 5):", Math.imul(5, 5));
console.log("Math.clz32(1):", Math.clz32(1)); // 31

console.log("Math.hypot(3, 4):", Math.hypot(3, 4)); // 5

console.log("Math.f16round(1.5):", Math.f16round(1.5));
console.log("Math.f16round(1.337):", Math.f16round(1.337));

console.log("Math.sumPrecise([1, 2, 3]):", Math.sumPrecise([1, 2, 3]));
console.log("Math.sumPrecise([0.1, 0.2]):", Math.sumPrecise([0.1, 0.2]));

// Random check (just checking it returns a number between 0 and 1)
const r = Math.random();
console.log("Math.random() is number:", typeof r === "number");
console.log("Math.random() >= 0:", r >= 0);
console.log("Math.random() < 1:", r < 1);

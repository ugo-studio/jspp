console.log("--- Spread Syntax Optimization ---");

// Array Spread Optimization
const arr1 = [1, ...[2, 3], 4];
console.log("arr1 (optimized):", arr1.join(","));

const arr2 = [...[...[1]]];
console.log("arr2 (deep optimized):", arr2.join(","));

const arr3 = [..."abc"];
console.log("arr3 (string optimization):", arr3.join(","));

// Object Spread Optimization
const obj1 = { a: 1, ...{ b: 2, c: 3 }, d: 4 };
console.log("obj1.b:", obj1.b, "obj1.c:", obj1.c);

const obj2 = { ...{ ...{ a: 1 } }, b: 2 };
console.log("obj2.a:", obj2.a, "obj2.b:", obj2.b);

// Function Call Spread Optimization
function sum4(a, b, c, d) {
    return a + b + c + d;
}
console.log("sum4(...[1, 2], ...[3, 4]):", sum4(...[1, 2], ...[3, 4]));
console.log("sum4(1, ...[2, 3], 4):", sum4(1, ...[2, 3], 4));

// New Expression Spread Optimization
class Vec2 {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
}
const v = new Vec2(...[10, 20]);
console.log("v.x:", v.x, "v.y:", v.y);

// Mix of static and dynamic (should still use runtime spreading for dynamic parts)
const dynamic = [2, 3];
const mixed = [1, ...dynamic, ...[4, 5]];
console.log("mixed:", mixed.join(","));

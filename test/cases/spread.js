console.log("--- Spread Syntax ---");

console.log("--- Array Spread ---");
const arr1 = [1, 2, 3];
const arr2 = [...arr1, 4, 5];
console.log("arr2:", arr2.join(","));

const str = "abc";
const arr3 = [...str, "d"];
console.log("arr3:", arr3.join(","));

function* gen() {
    yield 10;
    yield 20;
}
const arr4 = [...gen(), 30];
console.log("arr4:", arr4.join(","));

console.log("--- Object Spread ---");
const obj1 = { a: 1, b: 2 };
const obj2 = { ...obj1, c: 3 };
console.log("obj2.a:", obj2.a, "obj2.b:", obj2.b, "obj2.c:", obj2.c);

const obj3 = { a: 0, ...obj1 };
console.log("obj3.a:", obj3.a); // Should be 1 (overwritten by spread)

const obj4 = { ...obj1, a: 5 };
console.log("obj4.a:", obj4.a); // Should be 5 (overwritten by explicit prop)

console.log("--- Function Call Spread ---");
function sum(x, y, z) {
    return x + y + z;
}
const numbers = [1, 2, 3];
console.log("sum(...numbers):", sum(...numbers));
console.log("sum(0, ...[1, 2]):", sum(0, ...[1, 2]));

console.log("--- New Expression Spread ---");
class Point {
    constructor(x, y) {
        this.x = x;
        this.y = y;
    }
}
const coords = [10, 20];
const p = new Point(...coords);
console.log("p.x:", p.x, "p.y:", p.y);

console.log("--- Spread with non-iterable (Object) ---");
const obj5 = { ...null, ...undefined, a: 1 };
console.log("obj5.a:", obj5.a);

console.log("--- Spread with non-iterable (Array) - should throw ---");
try {
    const arr5 = [...null];
} catch (e) {
    console.log("Caught expected error for spreading null in array");
}

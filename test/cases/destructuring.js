console.log("--- Destructuring ---");

// Array destructuring
const [a, b, ...c] = [1, 2, 3, 4, 5];
console.log("Array:", a, b, c.join(","));

// Object destructuring
const { x, y: renamedY, z = 10 } = { x: 100, y: 200 };
console.log("Object:", x, renamedY, z);

// Nested destructuring
const { p: [q, r] } = { p: [10, 20] };
console.log("Nested:", q, r);

// Iterator destructuring (using a generator)
function* gen() {
    yield "one";
    yield "two";
    yield "three";
}
const [i1, i2] = gen();
console.log("Iterator:", i1, i2);

// Iterator destructuring (using an array iterator)
const [v1, v2] = [10, 20][Symbol.iterator]();
console.log("Array Iterator:", v1, v2);

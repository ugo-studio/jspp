console.log("--- Array Holes and Methods ---");

// new Array(n) creates holes. map() should skip them.
const holes = new Array(10);
const mapped = holes.map((_, i) => i);
console.log("Mapped (should be empty):", mapped);
console.log("Mapped length:", mapped.length);

// reverse() should preserve holes
const reversed = mapped.reverse();
console.log("Reversed:", reversed);

// Adding an element at the end
reversed[10] = "added";
console.log("With added:", reversed);

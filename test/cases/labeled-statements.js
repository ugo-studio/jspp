console.log("--- Labeled Statements ---");

console.log("--- Labeled break ---");
let x = 0;
labelCancelLoops: while (x < 3) {
  console.log("Outer loop:", x);
  x += 1;
  let z = 0;
  while (z < 3) {
    console.log("Inner loop:", z);
    z += 1;
    if (x === 2 && z === 2) {
      console.log("Breaking outer loop");
      break labelCancelLoops;
    }
  }
}
console.log("After labeled break. x is", x);


console.log("--- Labeled continue ---");
outer:
for (let i = 0; i < 3; i++) {
  for (let j = 0; j < 3; j++) {
    if (i === 1 && j === 1) {
      console.log(`continuing outer loop at i=${i}, j=${j}`);
      continue outer;
    }
    console.log(`i=${i}, j=${j}`);
  }
}
console.log("After labeled continue");

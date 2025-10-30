console.log("--- For Loops ---");

console.log("- C-style for loop -");
for (let i = 0; i < 5; i = i + 1) {
  console.log(i);
}

console.log("- for...in loop -");
const obj = { a: 1, b: 2 };
for (const key in obj) {
  console.log(key, obj[key]);
}

console.log("- for...of loop -");
const arr = [10, 20, 30];
for (const val of arr) {
    console.log(val);
}

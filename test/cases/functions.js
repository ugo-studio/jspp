console.log("--- Functions ---");

function main() {
  hoisted();
}

function hoisted() {
  console.log("hoisted");
}

main();

function counter() {
  let count = 0;
  function increment() {
    console.log("adding count");
    count = count + 1;
    console.log("added count");
    return count;
  }
  return increment;
}

const myCounter = counter();
console.log(myCounter());
console.log(myCounter());

function returnsUndefined() {
  // no return
}

console.log(returnsUndefined());

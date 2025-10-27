console.log("--- Functions ---");

function main() {
  local0();
  local1();
  function local0() {
    console.log("local0");
  }
  function local1() {
    console.log("local1");
  }
  hoisted();
}

main();

function hoisted() {
  console.log("hoisted");
}

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

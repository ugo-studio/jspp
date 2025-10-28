console.log("--- Functions ---");

function first() {
  local0();
  local1();
  function local0() {
    local1();
    console.log("local0");
  }
  function local1() {
    console.log("local1");
  }
  hoisted();
}

first();

function hoisted() {
  console.log("hoisted");
}

function counter() {
  let count = 0;
  function increment() {
    count = count + 1;
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

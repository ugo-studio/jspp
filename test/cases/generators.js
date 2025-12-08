console.log("--- Generators ---");

function* simpleGenerator() {
  yield 1;
  yield 2;
  yield 3;
}

console.log("- for...of on generator call -");
const gen = simpleGenerator();
for (const val of gen) {
  console.log(val);
}

console.log("- Iterator protocol -");
const iter = simpleGenerator();
let res = iter.next();
console.log(res.value);
res = iter.next();
console.log(res.value);
res = iter.next();
console.log(res.value);
res = iter.next();
console.log(res.done);

console.log("- Generator with return -");
function* genWithReturn() {
  yield "a";
  return "b";
}
const iter2 = genWithReturn();
console.log(iter2.next().value);
const res2 = iter2.next();
console.log(res2.value);
console.log(res2.done);

console.log("- Iterable Iterator -");
// This tests that an iterator (result of generator) is itself iterable
const iter3 = simpleGenerator();
for (const val of iter3) {
    console.log("from iter:", val);
}

console.log("- Array Iterator -");
const arr = [10, 20];
const arrIter = arr[Symbol.iterator]();
console.log(arrIter.next().value);
console.log(arrIter.next().value);
console.log(arrIter.next().done);

console.log("- Generator with input -");
function* genWithInput() {
    console.log("start");
    const a = yield 1;
    console.log("received a:", a);
    const b = yield 2;
    console.log("received b:", b);
    return 3;
}

const gInput = genWithInput();
console.log(gInput.next().value);
console.log(gInput.next("value-for-a").value);
console.log(gInput.next("value-for-b").value);
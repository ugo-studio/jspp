console.log("--- Array Methods Full ---");

const arr = [1, 2, 3, 4, 5];

// at
console.log("at(-1):", arr.at(-1));

// findLast
console.log("findLast > 3:", arr.findLast(x => x > 3));

// toReversed
const reversed = arr.toReversed();
console.log("original:", arr.join(","));
console.log("reversed:", reversed.join(","));

// toSpliced
const spliced = arr.toSpliced(1, 2, 99, 100);
console.log("spliced:", spliced.join(","));

// flatMap
const flatMapped = arr.flatMap(x => [x, x * 2]);
console.log("flatMapped:", flatMapped.join(","));

// with
const withArr = arr.with(2, 999);
console.log("with:", withArr.join(","));

// reduceRight
const reduced = arr.reduceRight((acc, cur) => acc + ("" + cur), "");
console.log("reduceRight:", reduced);

// copyWithin
const copyWithinArr = [1, 2, 3, 4, 5];
copyWithinArr.copyWithin(0, 3);
console.log("copyWithin:", copyWithinArr.join(","));

// keys iterator
const iter = arr.keys();
console.log("keys next:", iter.next().value);

// entries
const entriesIter = arr.entries();
const firstEntry = entriesIter.next().value;
console.log("entries next:", firstEntry[0], firstEntry[1]);

// Symbol.species
class MyArray extends Array {}
console.log("MyArray species:", MyArray[Symbol.species] === MyArray);
console.log("Array species:", Array[Symbol.species] === Array);
console.log("Array[Symbol.species]:", Array[Symbol.species]);

// fromAsync
(async () => {
    const asyncIterable = {
        [Symbol.asyncIterator]() {
            let i = 0;
            return {
                next() {
                    if (i < 3) return Promise.resolve({ value: i++, done: false });
                    return Promise.resolve({ done: true });
                }
            };
        }
    };
    try {
        const fromAsyncArr = await Array.fromAsync(asyncIterable);
        console.log("fromAsync:", fromAsyncArr.join(","));
    } catch (e) {
        console.log("fromAsync error:", e);
    }
})();

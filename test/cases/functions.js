function hoisted() {
    console.log("hoisted");
}

function main() {
    hoisted();
}

main();

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

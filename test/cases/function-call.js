function greet(greeting, punctuation) {
  console.log(greeting + ", " + this.name + punctuation);
}

const person = { name: "Alice" };

greet.call(person, "Hello", "!");

function sum(a, b) {
    return a + b;
}

console.log(sum.call(null, 1, 2));
console.log(sum.call(undefined, 3, 4));

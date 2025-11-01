console.log("--- Recursion ---");

function factorial(n) {
  if (n <= 1) {
    return 1;
  }
  return n * factorial(n - 1);
}

console.log("Factorial of 6:", factorial(6));

function isEven(n) {
  if (n === 0) return true;
  return isOdd(n - 1);
}

function isOdd(n) {
  if (n === 0) return false;
  return isEven(n - 1);
}

console.log("Is 10 even?", isEven(10));
console.log("Is 10 odd?", isOdd(10));
console.log("Is 7 even?", isEven(7));
console.log("Is 7 odd?", isOdd(7));

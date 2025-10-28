console.log("--- IIFE ---");

(() => {
  console.log("Arrow IIFE");
})();

(function() {
  console.log("Anon function IIFE");
})();

(function named() {
  console.log("Named function IIFE");
})();

const result = (function(a, b) {
  return a + b;
})(1, 2);
console.log("IIFE with args and return:", result);

(function countdown(n) {
  if (n > 0) {
    console.log(n);
    countdown(n - 1);
  } else {
    console.log("Blast off!");
  }
})(3);

// Test that named IIFE doesn't leak its name
try {
    named();
} catch (e) {
    console.log("Caught expected error for named IIFE leak");
}

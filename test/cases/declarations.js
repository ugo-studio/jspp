console.log("--- Declarations ---");

console.log("--- var ---");
var a = 1;
console.log(a);
a = 2;
console.log(a);

console.log("--- let ---");
let b = 3;
console.log(b);
b = 4;
console.log(b);

console.log("--- const ---");
const c = 5;
console.log(c);
try {
    c = 6;
} catch (e) {
    console.log("Caught expected error");
}
const d = c + 1;
console.log(d);
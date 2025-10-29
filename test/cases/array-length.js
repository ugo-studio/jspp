console.log("--- Array Length ---");
const arr = [1, 2, 3];
console.log(arr.length);
arr.length = 5;
console.log(arr.length);
console.log(arr[3]);
console.log(arr[4]);
arr.length = 2;
console.log(arr.length);
console.log(arr[2]);
try {
    arr.length = -1;
} catch (e) {
    console.log("Caught expected error:", e);
}

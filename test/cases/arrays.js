console.log("--- Arrays ---");

const arr = [1, "hello", true];
console.log(arr[0]);
console.log(arr[1]);
console.log(arr[2]);
console.log(arr.length);

arr[1] = "world";
console.log(arr[1]);

const nested = [[1, 2], [3, 4]];
console.log(nested[0][1]);

const empty = [];
console.log(empty.length);
empty[0] = "first";
console.log(empty[0]);
console.log(empty.length);

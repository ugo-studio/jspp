console.log("--- Objects ---");

const obj = { a: 1, b: "hello" };
console.log(obj.a);
console.log(obj["b"]);
obj.c = true;
console.log(obj["c"]);
obj.a = obj.a + 1;
console.log(obj.a);

const nested = { d: { e: "nested" } };
console.log(nested.d.e);
console.log(nested["d"].e);
nested.d.f = "new";
console.log(nested.d.f);
console.log(nested["d"]["f"]);

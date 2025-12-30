console.log("--- New Operators Test ---");

console.log("--- Bitwise ---");
console.log("5 ^ 3:", 5 ^ 3);
console.log("5 & 3:", 5 & 3);
console.log("5 | 3:", 5 | 3);
console.log("5 << 1:", 5 << 1);
console.log("5 >> 1:", 5 >> 1);
console.log("5 >>> 1:", 5 >>> 1);
console.log("-5 >>> 1:", -5 >>> 1);

console.log("--- Logical Assignment ---");
let a = 1;
a &&= 2;
console.log("a &&= 2:", a);
a &&= 0;
console.log("a &&= 0:", a);
a ||= 3;
console.log("a ||= 3:", a);
let b = null;
b ??= 4;
console.log("b ??= 4:", b);
b ??= 5;
console.log("b ??= 5:", b);

console.log("--- Optional Chaining ---");
const obj = {
    prop: {
        val: 10
    },
    method() {
        return "method result";
    }
};

console.log("obj?.prop?.val:", obj?.prop?.val);
console.log("obj?.nonExistent?.val:", obj?.nonExistent?.val);
console.log("obj?.method?.():", obj?.method?.());
console.log("obj?.nonExistentMethod?.():", obj?.nonExistentMethod?.());

const arr = [1, 2, 3];
console.log("arr?.[1]:", arr?.[1]);
console.log("arr?.[10]:", arr?.[10]);

console.log("--- In Operator ---");
console.log("'prop' in obj:", "prop" in obj);
console.log("'nonExistent' in obj:", "nonExistent" in obj);
console.log("'length' in arr:", "length" in arr);
console.log("'0' in arr:", "0" in arr);

console.log("--- Instanceof ---");
class MyClass {}
const myInstance = new MyClass();
console.log("myInstance instanceof MyClass:", myInstance instanceof MyClass);
console.log("myInstance instanceof Object:", myInstance instanceof Object);
console.log("[] instanceof Array:", [] instanceof Array);
console.log("[] instanceof Object:", [] instanceof Object);

console.log("--- Unary Plus and Not ---");
console.log("+'5':", +'5');
console.log("+true:", +true);
console.log("!true:", !true);
console.log("!false:", !false);
console.log("!0:", !0);
console.log("!1:", !1);

console.log("--- Nullish Coalescing ---");
console.log("null ?? 'default':", null ?? 'default');
console.log("undefined ?? 'default':", undefined ?? 'default');
console.log("false ?? 'default':", false ?? 'default');
console.log("0 ?? 'default':", 0 ?? 'default');

console.log("--- Delete ---");
const delObj = { a: 1, b: 2 };
console.log("delete delObj.a:", delete delObj.a);
console.log("'a' in delObj:", "a" in delObj);
const delArr = [1, 2, 3];
console.log("delete delArr[1]:", delete delArr[1]);
console.log("delArr[1]:", delArr[1]);
console.log("delArr.length:", delArr.length);

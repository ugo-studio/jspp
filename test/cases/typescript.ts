console.log("--- TypeScript Support ---");

// Type alias (should be ignored)
type MyNumber = number;

// Interface (should be ignored)
interface Person {
    name: string;
    age: MyNumber;
}

// Function with types
function greet(p: Person): string {
    return "Hello, " + p.name;
}

// Variable with type
const alice: Person = { name: "Alice", age: 30 };

console.log(greet(alice));

// Type assertion
const x: any = "hello";
const l = (x as string).length;
console.log("Length:", l);

const y = <string>x;
console.log("y:", y);

// Non-null assertion
function len(s: string | null) {
    return s!.length;
}
console.log("len:", len("abc"));

// Satisfies (TS 4.9+)
const config = { width: 100 } satisfies Record<string, number>;
console.log("width:", config.width);

// Enums (should be ignored for now, or compiled if we supported them, but visitor returns "")
// Note: Enums in TS emit code. Our current implementation ignores them (visitor returns "").
// So using an enum value would fail at runtime in C++ if we referenced it.
// We just test that the declaration doesn't crash the compiler.
enum Colors { Red, Green, Blue }
// console.log(Colors.Red); // This would fail because Colors is not emitted.

// Declare
declare const externalLib: any;
// externalLib.doSomething(); // Should compile to access but runtime fail if not present (unless stripped)
// actually `declare const` emits nothing with our change.
// `externalLib` usage would generate `jspp::Access::deref_stack(..., "externalLib")`.
// This is fine.

console.log("TS Test Complete");

console.log("--- Enums ---");

enum Direction {
    Up,
    Down,
    Left,
    Right
}

console.log("Direction.Up:", Direction.Up);
console.log("Direction.Down:", Direction.Down);
console.log("Direction[0]:", Direction[0]);
console.log("Direction[1]:", Direction[1]);

enum Initialized {
    A = 5,
    B,
    C = 10,
    D
}

console.log("Initialized.A:", Initialized.A);
console.log("Initialized.B:", Initialized.B);
console.log("Initialized.C:", Initialized.C);
console.log("Initialized.D:", Initialized.D);
console.log("Initialized[6]:", Initialized[6]);

enum StringEnum {
    Success = "SUCCESS",
    Failure = "FAILURE"
}

console.log("StringEnum.Success:", StringEnum.Success);
console.log("StringEnum.Failure:", StringEnum.Failure);
// Reverse mapping doesn't exist for string enums in TS, but our implementation checks if value is number.
// So checking StringEnum["SUCCESS"] should be undefined or not set.
// Standard TS generates an object where keys are names and values are strings. No reverse map.
console.log("StringEnum['SUCCESS']:", StringEnum["SUCCESS"]); // Should be undefined

enum Mixed {
    No = 0,
    Yes = "YES"
}
console.log("Mixed.No:", Mixed.No);
console.log("Mixed.Yes:", Mixed.Yes);
console.log("Mixed[0]:", Mixed[0]);

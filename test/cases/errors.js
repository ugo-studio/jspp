console.log("--- Errors ---");

try {
    throw new Error("my message");
} catch (e) {
    console.log(e);
}

try {
    throw "a string error";
} catch (e) {
    console.log(e);
}

try {
    throw 123;
} catch (e) {
    console.log(e);
}

try {
    const e = new Error("m");
    e.name = "CustomError";
    throw e;
} catch (e) {
    console.log(e);
}

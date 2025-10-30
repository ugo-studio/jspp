console.log("--- Unresolved Property Access ---");
try {
    console.log(a.b);
} catch (e) {
    console.log("Caught expected error:", e);
}
try {
    const x = a.b;
} catch (e) {
    console.log("Caught expected error:", e);
}

console.log("--- Unresolved Property Access ---");
try {
    console.log(non_existent_var.b);
} catch (e) {
    console.log("Caught expected error:", e);
}
try {
    const x = non_existent_var.b;
} catch (e) {
    console.log("Caught expected error:", e);
}

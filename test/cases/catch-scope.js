console.log("--- Catch Scope ---");

try {
  throw "This is an error";
} catch (e) {
  console.log("Caught error:", e);
  let ex = "This is a new variable";
  console.log("Inner variable:", ex);

  // This should resolve to undefined
  console.log("Leaked variable:", __caught_exception);

  // This should be a normal variable declaration
  let __caught_exception = "This is another new variable";
  console.log("Assigned leaked variable:", __caught_exception);
}

try {
  throw "This is an error";
} catch (__caught_exception) {
  console.log("Caught error:", e);
  let ex = "This is a new variable";
  console.log("Inner variable:", ex);

  // This should resolve to undefined
  console.log("Leaked variable:", __caught_exception);

  //   // This should be a normal variable declaration
  //   let __caught_exception = "This is another new variable";
  //   console.log("Assigned leaked variable:", __caught_exception);
}

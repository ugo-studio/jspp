console.log("This should be undefined:", varVal);
var varVal = true;

try {
  console.log("Should not log:", letVal);
  let letVal = true;
} catch (e) {
  console.log("Caught expected error for let:", e);
}

try {
  console.log("Should not log:", constVal);
  const constVal = true;
} catch (e) {
  console.log("Caught expected error for const:", e);
}

var varVal = 5;
console.log("This should be 5:", varVal);

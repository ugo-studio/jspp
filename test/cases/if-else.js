console.log("--- If-ElseIf-Else ---");

function testNumber(n) {
  if (n > 0) {
    console.log("Positive");
  } else if (n < 0) {
    console.log("Negative");
  } else {
    console.log("Zero");
  }
}

testNumber(5);
testNumber(-3);
testNumber(0);

let a = 10;
if (a === 10) {
    a = 20;
}
console.log(a);

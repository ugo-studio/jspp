console.log("--- Invalid Labeled Statements ---");

// Case 1: break to a non-iteration statement
try {
  mylabel: {
    console.log("Inside labeled block");
    break mylabel; // This should be a SyntaxError in JavaScript
  }
} catch (e) {
  console.log("Caught expected error for labeled block break:", e);
}

// Case 2: continue to a non-iteration statement
try {
  mylabel2: console.log("Single statement labeled");
  continue mylabel2; // This should be a SyntaxError
} catch (e) {
  console.log("Caught expected error for labeled non-loop continue:", e);
}

// Case 3: break to an undefined label
try {
  outer: for(let i = 0; i < 1; i++) {
    inner: for(let j = 0; j < 1; j++) {
      break unknownLabel; // This should be a SyntaxError (undefined label)
    }
  }
} catch (e) {
  console.log("Caught expected error for undefined break label:", e);
}

// Case 4: continue to an undefined label
try {
  outer2: for(let i = 0; i < 1; i++) {
    inner2: for(let j = 0; j < 1; j++) {
      continue unknownLabel2; // This should be a SyntaxError (undefined label)
    }
  }
} catch (e) {
  console.log("Caught expected error for undefined continue label:", e);
}

// Case 5: break without a label in an invalid context (global scope)
try {
  break; // Should be SyntaxError
} catch (e) {
  console.log("Caught expected error for unlabeled break in global scope:", e);
}

// Case 6: continue without a label in an invalid context (global scope)
try {
  continue; // Should be SyntaxError
} catch (e) {
  console.log("Caught expected error for unlabeled continue in global scope:", e);
}

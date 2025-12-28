console.log("--- Var in For Loop ---");

var arr = ["undefined", "NaN", "Infinity"];

for (var i = 0; i < arr.length; ++i) {
  var propertyName = arr[i];
  console.log(propertyName);
}

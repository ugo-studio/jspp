console.log("--- Default-parameters ---");

function greet(name, message) {
  console.log(name, message);
}
greet("John");

function def(p = "defaultValue", p1 = "defaultValue1") {
  console.log(p, p1);
}
def("value");

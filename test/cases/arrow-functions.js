console.log("--- Arrow functions ---");

const func1 = () => {
  console.log("func1");
};
const func2 = () => console.log("func2");
func1();
func2();

let func3;
console.log(func3);
func3 = () => console.log("func3");
func3();

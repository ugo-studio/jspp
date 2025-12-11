console.log("--- New Operator ---");

function Person(name) {
    this.name = name;
}
Person.prototype.greet = function() {
    console.log("Hello, " + this.name);
};

const p = new Person("World");
p.greet();
console.log(p.name);

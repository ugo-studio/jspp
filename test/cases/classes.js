console.log("--- Classes ---");

class Animal {
    constructor(name) {
        this.name = name;
    }

    speak() {
        console.log(this.name + " makes a noise.");
    }
}

const a = new Animal("Dog");
a.speak();

class Dog extends Animal {
    constructor(name, breed) {
        super(name);
        this.breed = breed;
    }

    speak() {
        console.log(this.name + " barks.");
    }
    
    getDescription() {
        return this.name + " is a " + this.breed;
    }
}

const d = new Dog("Rex", "German Shepherd");
d.speak();
console.log(d.getDescription());

console.log("--- Static Methods ---");
class MathUtils {
    static add(a, b) {
        return a + b;
    }
}
console.log(MathUtils.add(5, 3));

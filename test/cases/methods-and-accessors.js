console.log("--- Object Methods & Computed Keys ---");
const cVar = "c";
const dVar = "d";
const o = {
  a: () => {
    console.log("a");
  },
  b() {
    console.log("b");
  },
  [cVar]() {
    console.log("c");
  },
  [dVar]: function () {
    console.log("d");
  },
};
o.a();
o.b();
o[cVar]();
o[dVar]();

console.log("--- Object Accessors & Computed Keys ---");
const bKey = "b";
const o2 = {
  aVal: 1,
  bVal: 2,

  get a() {
    return this.aVal;
  },
  get [bKey]() {
    return this.bVal;
  },

  set a(v) {
    this.aVal = v;
  },
  set [bKey](v) {
    this.bVal = v;
  },
};

console.log(o2.a, o2[bKey]);
o2.a = 20;
o2[bKey] = 30;
console.log(o2.a, o2[bKey]);
console.log(o2.aVal, o2.bVal);

console.log("--- Class Computed Methods ---");
const method = "dynamicMethod";
class A {
  static [method]() {
    console.log("static dynamic");
  }
  [method]() {
    console.log("instance dynamic");
  }
}

A[method]();
new A()[method]();

console.log("--- Class Accessors ---");
class C {
    constructor() {
        this._val = 100;
    }
    get val() { return this._val; }
    set val(v) { this._val = v; }

    static get staticVal() { return "static"; }
}

const c = new C();
console.log("Class getter:", c.val);
c.val = 200;
console.log("Class getter after set:", c.val);
console.log("Static getter:", C.staticVal);

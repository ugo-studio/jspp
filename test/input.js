// // TEST 1
// function counter() {
//   let count = 0;

//   function increment() {
//     count = count + 1;
//     return count;
//   }

//   return increment;
// }

// const myCounter = counter();
// console.log(myCounter());
// console.log(myCounter());

// // TEST 2
// const user = {
//   id: 1,
//   name: "John Doe",
//   active: true,
// };

// const data = [10, "hello", 25.5];

// const users = [
//   { id: 2, name: "Jane Doe", active: false },
//   { id: 3, name: "Peter Pan", active: true },
// ];

// for (const u of users) {
//   console.log(u.name);
// }

function hello() {
  function world() {
    console.log("world");
  }
  console.log("hello");
  world();
}

hello();

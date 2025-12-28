console.log("--- Timers ---");
console.log("start");

setTimeout(() => {
    console.log("timeout 100ms");
}, 100);

setTimeout(() => {
    console.log("timeout 50ms");
}, 50);

const id = setInterval(() => {
    console.log("interval 75ms");
    clearInterval(id);
}, 75);

console.log("end");

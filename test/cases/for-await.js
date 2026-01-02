console.log("--- For Await Of ---");

async function* asyncGen() {
    yield 10;
    yield 20;
    return 30;
}

(async () => {
    try {
        console.log("start loop");
        for await (const x of asyncGen()) {
            console.log("x:", x);
        }
        console.log("end loop");
    } catch (e) {
        console.log("Error:", e);
    }
})();

(async () => {
    try {
        console.log("start sync iterable loop");
        const syncIter = [1, 2, 3];
        for await (const y of syncIter) {
            console.log("y:", y);
        }
        console.log("end sync iterable loop");
    } catch (e) {
        console.log("Error:", e);
    }
})();

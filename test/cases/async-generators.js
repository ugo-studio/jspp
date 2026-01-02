console.log("--- Async Generators ---");

async function* asyncGen() {
    console.log("start async gen");
    yield 1;
    console.log("after 1");
    yield await Promise.resolve(2);
    console.log("after 2");
    return 3;
}

(async () => {
    try {
        const gen = asyncGen();
        console.log("Gen created");
        
        let res = await gen.next();
        console.log("res 1:", res.value, res.done);
        
        res = await gen.next();
        console.log("res 2:", res.value, res.done);
        
        res = await gen.next();
        console.log("res 3:", res.value, res.done);
    } catch (e) {
        console.log("Error in async gen test:", e);
    }
})();

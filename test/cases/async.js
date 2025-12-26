
async function delay(ms) {
    return new Promise(resolve => {
        // We don't have setTimeout yet, so we just resolve immediately or use a dummy wait
        resolve("done");
    });
}

async function foo() {
    console.log("start foo");
    const res = await delay(100);
    console.log("delay result:", res);
    return "foo result";
}

async function main() {
    console.log("before foo");
    const val = await foo();
    console.log("after foo:", val);
    
    const p1 = Promise.resolve(10);
    const p2 = Promise.resolve(20);
    const all = await Promise.all([p1, p2]);
    console.log("Promise.all:", all.join(","));
}

main();
console.log("end script (async main continues)");

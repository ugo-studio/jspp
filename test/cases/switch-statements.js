console.log("--- Switch Statements ---");

function testSwitch(value) {
    switch (value) {
        case 1:
            console.log("Case 1");
            break;
        case 2:
            console.log("Case 2");
            break;
        case "hello":
            console.log("Case 'hello'");
            break;
        case true:
            console.log("Case true");
            break;
        default:
            console.log("Default case");
            break;
    }
}

testSwitch(1);
testSwitch(2);
testSwitch(3);
testSwitch("hello");
testSwitch(true);
testSwitch(false);
testSwitch(null);
testSwitch(undefined);

console.log("--- Switch with Fallthrough ---");

function testFallthrough(value) {
    switch (value) {
        case 1:
            console.log("Fallthrough 1");
        case 2:
            console.log("Fallthrough 2");
            break;
        case 3:
            console.log("Fallthrough 3");
        default:
            console.log("Fallthrough Default");
            break;
    }
}

testFallthrough(1);
testFallthrough(2);
testFallthrough(3);
testFallthrough(4);

console.log("--- Switch with no matches and no default ---");
function testNoMatch(value) {
    switch(value) {
        case 1:
            console.log("Should not reach here");
            break;
    }
}
testNoMatch(0);

console.log("--- Switch in a loop ---");
for (let i = 0; i < 3; i++) {
    switch (i) {
        case 0:
            console.log("Loop Case 0");
            break;
        case 1:
            console.log("Loop Case 1");
            continue; // Should continue the for loop
        case 2:
            console.log("Loop Case 2");
            break;
    }
    console.log("After switch, loop i:", i);
}

console.log("--- Labeled Switch ---");
outerSwitch: switch (1) {
    case 1:
        console.log("Outer Switch Case 1");
        innerSwitch: switch ("a") {
            case "a":
                console.log("Inner Switch Case 'a'");
                break outerSwitch; // Breaks out of outerSwitch
            case "b":
                console.log("Inner Switch Case 'b'");
                break;
        }
        console.log("Should not be reached after labeled break");
    case 2:
        console.log("Should not be reached after labeled break");
}
console.log("After labeled outer switch");

console.log("--- Switch with complex expressions ---");
let x = 10;
let y = 20;
switch (x + y) {
    case 30:
        console.log("Sum is 30");
        break;
    case 40:
        console.log("Sum is 40");
        break;
}

const obj = { prop: "value" };
switch (obj.prop) {
    case "value":
        console.log("Object property matched");
        break;
    default:
        console.log("Object property not matched");
        break;
}

console.log("--- Global Return ---");

try {
  return "should fail";
} catch (e) {
  console.log(e);
}

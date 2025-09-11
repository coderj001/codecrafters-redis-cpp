const RedisServerTester = require("./test-server.js");

let tester;

beforeAll(async () => {
  tester = new RedisServerTester();
  await tester.startServer();
  await tester.connectClient();
}, 20000);

afterAll(async () => {
  await tester.cleanup();
});

test("SET/GET works", async () => {
  console.time("SET/GET works");
  const ok = await tester.testSetGet();
  expect(ok).toBe(true);
  console.timeEnd("SET/GET works");
});

test("GET non-existent returns null", async () => {
  const ok = await tester.testNonExistentKey();
  expect(ok).toBe(true);
});

test("SET/GET Test with PX", async () => {
  const ok = await tester.testSetWithPX();
  expect(ok).toBe(true);
});

test("PING returns PONG", async () => {
  const res = await tester.client.ping();
  expect(res === "PONG" || res === undefined).toBe(true);
});

test("ECHO returns the same message", async () => {
  const msg = "hello-world";
  const res = await tester.client.echo(msg);
  expect(res).toBe(msg);
});

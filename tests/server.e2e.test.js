const RedisServerTester = require('./test-server.js');

let tester;

beforeAll(async () => {
  tester = new RedisServerTester();
  await tester.startServer();
  await tester.connectClient();
}, 20000);

afterAll(async () => {
  await tester.cleanup();
});

test('SET/GET works', async () => {
  const ok = await tester.testSetGet();
  expect(ok).toBe(true);
});

test('GET non-existent returns null', async () => {
  const ok = await tester.testNonExistentKey();
  expect(ok).toBe(true);
});

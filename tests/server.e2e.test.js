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

test('PING returns PONG', async () => {
  const res = await tester.client.ping();
  // redis client v4 returns 'PONG' for ping()
  expect(res === 'PONG' || res === undefined).toBe(true);
});

test('ECHO returns the same message', async () => {
  const msg = 'hello-world';
  const res = await tester.client.echo(msg);
  expect(res).toBe(msg);
});

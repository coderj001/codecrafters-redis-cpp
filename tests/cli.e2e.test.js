const RedisCliTester = require('./test-redis-cli.js');

jest.setTimeout(30000);

test('redis-cli based e2e tests', async () => {
  const tester = new RedisCliTester();
  const success = await tester.runTests();
  expect(success).toBe(true);
});

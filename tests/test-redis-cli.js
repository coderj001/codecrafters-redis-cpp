const { spawn } = require('child_process');
const RedisServerTester = require('./test-server');

class RedisCliTester {
  constructor() {
    this.serverTester = new RedisServerTester();
  }

  async runRedisCliCommand(args) {
    return new Promise((resolve, reject) => {
      const process = spawn('npx', ['redis-cli', '-p', '6379', ...args], {
        stdio: 'pipe'
      });

      let stdout = '';
      let stderr = '';

      const timeoutMs = 5000;
      const t = setTimeout(() => {
        try { process.kill('SIGKILL'); } catch (e) {}
        reject(new Error('redis-cli command timed out'));
      }, timeoutMs);

      process.stdout.on('data', (data) => {
        stdout += data.toString();
      });

      process.stderr.on('data', (data) => {
        stderr += data.toString();
      });

      process.on('exit', (code) => {
        clearTimeout(t);
        // remove listeners
        try { process.stdout.removeAllListeners(); } catch (e) {}
        try { process.stderr.removeAllListeners(); } catch (e) {}

        if (code === 0) {
          resolve(stdout.trim());
        } else {
          reject(new Error(`redis-cli exited with code ${code}: ${stderr}`));
        }
      });

      process.on('error', (err) => {
        clearTimeout(t);
        reject(err);
      });
    });
  }

  async testBasicSetGet() {
    try {
      console.log('Testing basic SET/GET...');
      
      // SET command
      const setResult = await this.runRedisCliCommand(['SET', 'test_key', 'test_value']);
      console.log('SET result:', setResult);
      
      // GET command
      const getResult = await this.runRedisCliCommand(['GET', 'test_key']);
      console.log('GET result:', getResult);
      
      const success = getResult === 'test_value';
      console.log(success ? '✓ Basic SET/GET test passed' : '✗ Basic SET/GET test failed');
      return success;
    } catch (err) {
      console.log('✗ Basic SET/GET test failed:', err.message);
      return false;
    }
  }

  async testNonExistentKey() {
    try {
      console.log('Testing GET for non-existent key...');
      
      const result = await this.runRedisCliCommand(['GET', 'nonexistent_key']);
      console.log('GET nonexistent result:', result);
      
      // Redis returns (nil) for non-existent keys
      const success = result === '(nil)' || result === '';
      console.log(success ? '✓ Non-existent key test passed' : '✗ Non-existent key test failed');
      return success;
    } catch (err) {
      console.log('✗ Non-existent key test failed:', err.message);
      return false;
    }
  }

  async testOverwriteValue() {
    try {
      console.log('Testing value overwrite...');
      
      // Set initial value
      await this.runRedisCliCommand(['SET', 'overwrite_key', 'value1']);
      
      // Overwrite with new value
      await this.runRedisCliCommand(['SET', 'overwrite_key', 'value2']);
      
      // Get the new value
      const result = await this.runRedisCliCommand(['GET', 'overwrite_key']);
      
      const success = result === 'value2';
      console.log(success ? '✓ Overwrite test passed' : '✗ Overwrite test failed');
      return success;
    } catch (err) {
      console.log('✗ Overwrite test failed:', err.message);
      return false;
    }
  }

  async testMultipleKeys() {
    try {
      console.log('Testing multiple keys...');
      
      // Set multiple keys
      await this.runRedisCliCommand(['SET', 'key1', 'value1']);
      await this.runRedisCliCommand(['SET', 'key2', 'value2']);
      await this.runRedisCliCommand(['SET', 'key3', 'value3']);
      
      // Get all values
      const val1 = await this.runRedisCliCommand(['GET', 'key1']);
      const val2 = await this.runRedisCliCommand(['GET', 'key2']);
      const val3 = await this.runRedisCliCommand(['GET', 'key3']);
      
      const success = val1 === 'value1' && val2 === 'value2' && val3 === 'value3';
      console.log(success ? '✓ Multiple keys test passed' : '✗ Multiple keys test failed');
      return success;
    } catch (err) {
      console.log('✗ Multiple keys test failed:', err.message);
      return false;
    }
  }

  async testPingEcho() {
    try {
      console.log('Testing PING and ECHO...');

      const pingRes = await this.runRedisCliCommand(['PING']);
      // redis-cli prints PONG
      if (pingRes.trim() !== 'PONG') {
        console.log('✗ PING test failed, got:', pingRes);
        return false;
      }

      const echoMsg = 'hello-redis-cli';
      const echoRes = await this.runRedisCliCommand(['ECHO', echoMsg]);
      if (echoRes.trim() !== echoMsg) {
        console.log('✗ ECHO test failed, got:', echoRes);
        return false;
      }

      console.log('✓ PING/ECHO tests passed');
      return true;
    } catch (err) {
      console.log('✗ PING/ECHO test failed:', err.message);
      return false;
    }
  }

  async testErrorHandling() {
    try {
      console.log('Testing error handling...');
      
      // Test GET without key (should return error)
      try {
        await this.runRedisCliCommand(['GET']);
        console.log('✗ Error handling test failed: GET without key should error');
        return false;
      } catch (err) {
        console.log('✓ GET without key properly errored');
      }
      
      // Test SET without value (should return error)
      try {
        await this.runRedisCliCommand(['SET', 'test_key']);
        console.log('✗ Error handling test failed: SET without value should error');
        return false;
      } catch (err) {
        console.log('✓ SET without value properly errored');
      }
      
      console.log('✓ Error handling test passed');
      return true;
    } catch (err) {
      console.log('✗ Error handling test failed:', err.message);
      return false;
    }
  }

  async runAllTests() {
    const results = [];

    results.push(await this.testBasicSetGet());
    results.push(await this.testNonExistentKey());
    results.push(await this.testOverwriteValue());
    results.push(await this.testMultipleKeys());
    results.push(await this.testErrorHandling());
  results.push(await this.testPingEcho());

    return results.every(result => result);
  }

  async runTests() {
    try {
      console.log('Starting Redis server for redis-cli tests...');
      await this.serverTester.startServer();

      console.log('Running redis-cli tests...');
      const success = await this.runAllTests();

      console.log(success ? '\n🎉 All redis-cli tests passed!' : '\n❌ Some redis-cli tests failed');
      return success;

    } catch (err) {
      console.error('redis-cli test setup failed:', err.message);
      return false;
    } finally {
      await this.serverTester.cleanup();
    }
  }
}

async function runRedisCliTest() {
  const tester = new RedisCliTester();
  return await tester.runTests();
}

if (require.main === module) {
  runRedisCliTest().then((success) => {
    process.exit(success ? 0 : 1);
  });
}

module.exports = RedisCliTester;

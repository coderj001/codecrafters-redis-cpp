const { spawn } = require('child_process');
const path = require('path');
const redis = require('redis');

class RedisServerTester {
  constructor() {
    this.serverProcess = null;
    this.client = null;
  }

  async startServer() {
    const serverPath = path.join(__dirname, '..', 'your_program.sh');
    
    return new Promise((resolve, reject) => {
      this.serverProcess = spawn('bash', [serverPath], {
        cwd: path.dirname(serverPath),
        stdio: ['pipe', 'pipe', 'pipe']
      });

      let started = false;
      const timeout = setTimeout(() => {
        if (!started) {
          reject(new Error('Server failed to start within 10 seconds'));
        }
      }, 10000);

      this.serverProcess.stdout.on('data', (data) => {
        const output = data.toString();
        console.log('Server stdout:', output);
        if (output.includes('Server started') || output.includes('listening')) {
          started = true;
          clearTimeout(timeout);
          // Wait a bit more for server to be ready
          setTimeout(resolve, 1000);
        }
      });

      this.serverProcess.stderr.on('data', (data) => {
        console.log('Server stderr:', data.toString());
      });

      this.serverProcess.on('error', (err) => {
        clearTimeout(timeout);
        reject(err);
      });
    });
  }

  async connectClient() {
    this.client = redis.createClient({
      host: 'localhost',
      port: 6379,
      retry_strategy: (options) => {
        if (options.error && options.error.code === 'ECONNREFUSED') {
          console.log('Connection refused, retrying...');
          return Math.min(options.attempt * 100, 3000);
        }
        return false;
      }
    });

    return new Promise((resolve, reject) => {
      this.client.on('connect', () => {
        console.log('Connected to Redis server');
        resolve();
      });

      this.client.on('error', (err) => {
        console.log('Redis client error:', err.message);
        reject(err);
      });

      this.client.connect();
    });
  }

  async testSetGet() {
    try {
      // Test SET
      const setResult = await this.client.set('testkey', 'testvalue');
      console.log('SET result:', setResult);

      // Test GET
      const getResult = await this.client.get('testkey');
      console.log('GET result:', getResult);

      if (getResult === 'testvalue') {
        console.log('✓ SET/GET test passed');
        return true;
      } else {
        console.log('✗ SET/GET test failed');
        return false;
      }
    } catch (err) {
      console.log('Error in SET/GET test:', err.message);
      return false;
    }
  }

  async testNonExistentKey() {
    try {
      const result = await this.client.get('nonexistent');
      console.log('GET nonexistent result:', result);

      if (result === null) {
        console.log('✓ Non-existent key test passed');
        return true;
      } else {
        console.log('✗ Non-existent key test failed');
        return false;
      }
    } catch (err) {
      console.log('Error in non-existent key test:', err.message);
      return false;
    }
  }

  async runTests() {
    const results = [];

    results.push(await this.testSetGet());
    results.push(await this.testNonExistentKey());

    return results.every(result => result);
  }

  async cleanup() {
    if (this.client) {
      await this.client.quit();
    }

    if (this.serverProcess) {
      this.serverProcess.kill('SIGTERM');
      
      // Wait for process to exit
      await new Promise((resolve) => {
        this.serverProcess.on('exit', resolve);
        setTimeout(resolve, 5000); // Force kill after 5 seconds
      });
    }
  }
}

async function runE2ETests() {
  const tester = new RedisServerTester();

  try {
    console.log('Starting Redis server...');
    await tester.startServer();

    console.log('Connecting to server...');
    await tester.connectClient();

    console.log('Running tests...');
    const success = await tester.runTests();

    console.log(success ? 'All tests passed!' : 'Some tests failed');

  } catch (err) {
    console.error('Test setup failed:', err.message);
  } finally {
    await tester.cleanup();
  }
}

if (require.main === module) {
  runE2ETests();
}

module.exports = RedisServerTester;

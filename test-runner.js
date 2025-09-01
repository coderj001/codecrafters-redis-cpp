#!/usr/bin/env node

const { spawn } = require('child_process');
const path = require('path');

class TestRunner {
  constructor() {
    this.projectRoot = path.join(__dirname, '..');
    this.testsDir = path.join(__dirname);
  }

  async runCommand(command, args = [], cwd = this.projectRoot) {
    return new Promise((resolve, reject) => {
      const process = spawn(command, args, {
        cwd,
        stdio: 'inherit',
        shell: true
      });

      process.on('exit', (code) => {
        if (code === 0) {
          resolve();
        } else {
          reject(new Error(`Command failed with code ${code}`));
        }
      });

      process.on('error', reject);
    });
  }

  async checkPrerequisites() {
    console.log('🔍 Checking prerequisites...');

    // Check if redis-cli is available
    try {
      await this.runCommand('redis-cli', ['--version']);
      console.log('✓ redis-cli is available');
    } catch (err) {
      console.log('✗ redis-cli not found. Please install Redis CLI.');
      console.log('  macOS: brew install redis');
      console.log('  Ubuntu: sudo apt-get install redis-tools');
      return false;
    }

    // Check if build exists
    const fs = require('fs');
    if (!fs.existsSync(path.join(this.projectRoot, 'build', 'server'))) {
      console.log('✗ Server binary not found. Building...');
      try {
        await this.buildServer();
        console.log('✓ Server built successfully');
      } catch (err) {
        console.log('✗ Failed to build server:', err.message);
        return false;
      }
    } else {
      console.log('✓ Server binary found');
    }

    return true;
  }

  async buildServer() {
    console.log('🔨 Building Redis server...');
    await this.runCommand('cmake', ['-B', 'build', '-S', '.', '-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake']);
    await this.runCommand('cmake', ['--build', './build']);
  }

  async runTestSuite() {
    console.log('🧪 Running test suite...');

    // Run Node.js client tests
    console.log('\n📋 Running Node.js Redis client tests...');
    try {
      await this.runCommand('npm', ['run', 'test'], this.testsDir);
      console.log('✓ Node.js client tests passed');
    } catch (err) {
      console.log('✗ Node.js client tests failed');
      throw err;
    }

    // Run redis-cli tests
    console.log('\n📋 Running redis-cli tests...');
    try {
      await this.runCommand('npm', ['run', 'test:cli'], this.testsDir);
      console.log('✓ redis-cli tests passed');
    } catch (err) {
      console.log('✗ redis-cli tests failed');
      throw err;
    }
  }

  async run() {
    try {
      console.log('🚀 Starting Redis Server E2E Test Runner\n');

      const prerequisitesOk = await this.checkPrerequisites();
      if (!prerequisitesOk) {
        process.exit(1);
      }

      await this.runTestSuite();

      console.log('\n🎉 All tests completed successfully!');

    } catch (err) {
      console.error('\n❌ Test runner failed:', err.message);
      process.exit(1);
    }
  }
}

// Run if called directly
if (require.main === module) {
  const runner = new TestRunner();
  runner.run();
}

module.exports = TestRunner;

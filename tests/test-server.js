const { spawn } = require("child_process");
const path = require("path");
const net = require("net");
const redis = require("redis");

class RedisServerTester {
  constructor() {
    this.serverProcess = null;
    this.client = null;
  }

  async startServer() {
    // Spawn the built server binary directly (avoid relying on your_program.sh output)
    const serverBinary = path.join(__dirname, "..", "build", "server");

    return new Promise((resolve, reject) => {
      try {
        this.serverProcess = spawn(serverBinary, [], {
          cwd: path.dirname(serverBinary),
          stdio: ["ignore", "pipe", "pipe"],
        });
      } catch (err) {
        return reject(err);
      }

      this.serverProcess.stdout.on("data", (data) => {
        console.log("Server stdout:", data.toString());
      });

      this.serverProcess.stderr.on("data", (data) => {
        console.error("Server stderr:", data.toString());
      });

      this.serverProcess.on("error", (err) => {
        reject(err);
      });

      // Wait for TCP port 6379 to accept connections
      const start = Date.now();
      const timeoutMs = 10000; // 10s timeout

      const tryConnect = () => {
        const socket = net.createConnection(
          { host: "127.0.0.1", port: 6379 },
          () => {
            socket.end();
            // give server a tiny moment to settle
            setTimeout(resolve, 100);
          },
        );

        socket.on("error", (err) => {
          socket.destroy();
          if (Date.now() - start >= timeoutMs) {
            reject(
              new Error(
                "Server did not accept connections on port 6379 within timeout",
              ),
            );
          } else {
            setTimeout(tryConnect, 150);
          }
        });
      };

      tryConnect();
    });
  }

  async connectClient() {
    // Use redis client to run tests; createClient syntax varies between redis versions,
    // so first try modern API, fall back to legacy options if needed.
    try {
      // Modern (v4+) API
      this.client = redis.createClient({
        socket: { host: "127.0.0.1", port: 6379 },
      });
    } catch (err) {
      // Fallback for older redis clients
      this.client = redis.createClient({ host: "127.0.0.1", port: 6379 });
    }

    return new Promise((resolve, reject) => {
      // redis >=4 uses connect() promise, but also emits events; use both defensively.
      const onError = (err) => {
        cleanupListeners();
        reject(err);
      };

      const onConnect = () => {
        cleanupListeners();
        resolve();
      };

      const cleanupListeners = () => {
        if (this.client && this.client.off) {
          this.client.off("error", onError);
          this.client.off("connect", onConnect);
        }
      };

      if (this.client.on) {
        this.client.on("error", onError);
        this.client.on("connect", onConnect);
      }

      // Attempt to connect; for redis@4 this returns a promise
      try {
        const maybePromise = this.client.connect();
        if (maybePromise && typeof maybePromise.then === "function") {
          maybePromise
            .then(() => {
              // connect() resolved — ensure events cleaned
              cleanupListeners();
              resolve();
            })
            .catch(onError);
        }
      } catch (err) {
        // some clients throw synchronously; handle via events instead
      }

      // As a safety, set a timeout
      setTimeout(() => {
        reject(new Error("Client failed to connect within 10s"));
      }, 10000);
    });
  }

  async testSetGet() {
    try {
      // Test SET
      const setResult = await this.client.set("testkey", "testvalue");
      console.log("SET result:", setResult);

      // Test GET
      const getResult = await this.client.get("testkey");
      console.log("GET result:", getResult);

      if (getResult === "testvalue") {
        console.log("✓ SET/GET test passed");
        return true;
      } else {
        console.log("✗ SET/GET test failed");
        return false;
      }
    } catch (err) {
      console.log("Error in SET/GET test:", err.message);
      return false;
    }
  }

  async testNonExistentKey() {
    try {
      const result = await this.client.get("nonexistent");
      console.log("GET nonexistent result:", result);

      if (result === null) {
        console.log("✓ Non-existent key test passed");
        return true;
      } else {
        console.log("✗ Non-existent key test failed");
        return false;
      }
    } catch (err) {
      console.log("Error in non-existent key test:", err.message);
      return false;
    }
  }

  async testSetWithPX() {
    try {
      // Test SET with PX (1 second = 1000 ms)
      const setResult = await this.client.set("testkey_px", "tempvalue", {
        PX: 1000,
      });
      console.log("SET with PX result:", setResult);

      // Immediately GET -> should exist
      let getResult = await this.client.get("testkey_px");
      console.log("GET immediately:", getResult);

      if (getResult !== "tempvalue") {
        console.log("✗ PX test failed (value missing right after set)");
        return false;
      }

      // Wait 1.2s -> should expire
      await new Promise((resolve) => setTimeout(resolve, 1200));
      getResult = await this.client.get("testkey_px");
      console.log("GET after expiry:", getResult);

      if (getResult === null) {
        console.log("✓ PX expiry test passed");
        return true;
      } else {
        console.log("✗ PX expiry test failed (key still exists)");
        return false;
      }
    } catch (err) {
      console.log("Error in PX test:", err.message);
      return false;
    }
  }

  async runTests() {
    const results = [];

    results.push(await this.testSetGet());
    results.push(await this.testNonExistentKey());

    return results.every((result) => result);
  }

  async cleanup() {
    if (this.client) {
      try {
        if (this.client.quit) {
          await this.client.quit();
        } else if (this.client.disconnect) {
          await this.client.disconnect();
        }
      } catch (err) {
        // ignore errors during cleanup
      }
    }

    if (this.serverProcess) {
      try {
        // remove listeners on stdio so they don't keep the event loop alive
        try {
          if (this.serverProcess.stdout)
            this.serverProcess.stdout.removeAllListeners();
        } catch (e) {}
        try {
          if (this.serverProcess.stderr)
            this.serverProcess.stderr.removeAllListeners();
        } catch (e) {}

        // Try graceful shutdown
        this.serverProcess.kill("SIGTERM");

        // Wait up to 5s for exit, then force kill
        const exited = await new Promise((resolve) => {
          let finished = false;
          const onExit = () => {
            if (!finished) {
              finished = true;
              resolve(true);
            }
          };
          this.serverProcess.on("exit", onExit);
          setTimeout(() => {
            if (!finished) {
              finished = true;
              resolve(false);
            }
          }, 5000);
        });

        if (!exited) {
          try {
            this.serverProcess.kill("SIGKILL");
          } catch (e) {}
        }
      } catch (err) {
        // ignore
      }

      // give Node a moment to let handles close
      await new Promise((r) => setTimeout(r, 50));
    }
  }
}

async function runE2ETests() {
  const tester = new RedisServerTester();

  try {
    console.log("Starting Redis server...");
    await tester.startServer();

    console.log("Connecting to server...");
    await tester.connectClient();

    console.log("Running tests...");
    const success = await tester.runTests();

    console.log(success ? "All tests passed!" : "Some tests failed");
  } catch (err) {
    console.error("Test setup failed:", err.message);
  } finally {
    await tester.cleanup();
  }
}

if (require.main === module) {
  runE2ETests();
}

module.exports = RedisServerTester;

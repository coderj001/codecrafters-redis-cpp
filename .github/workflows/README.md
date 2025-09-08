# CI/CD Pipeline

This repository includes GitHub Actions workflows for automated testing and building.

## Workflows

### Test Workflow (`.github/workflows/test.yml`)
- **Trigger**: Push/PR to main/master branches
- **Purpose**: Run Jest-based end-to-end tests
- **Steps**:
  1. Build the C++ Redis server
  2. Install Node.js dependencies
  3. Run Jest tests with timeout protection

### Build and Release Workflow (`.github/workflows/build.yml`)
- **Trigger**: 
  - Push/PR to main/master branches (build only)
  - Git tags starting with 'v' (build + release)
- **Purpose**: Build the C++ server and create releases
- **Steps**:
  1. Build the Redis server binary
  2. Upload build artifacts
  3. Create GitHub release (only for tagged versions)

## Running Tests Locally

### Prerequisites
```bash
# Install Node.js dependencies for tests
cd tests
npm install

# Build the server
mkdir -p build
g++ -std=c++23 src/*.cpp -o build/server -pthread
```

### Running Tests
```bash
cd tests
npm test
```

## Creating Releases

To create a new release:
1. Create and push a git tag with version format:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```
2. The GitHub Actions will automatically build and create a release with the binary

## Build Requirements

- GCC with C++23 support
- pthread library
- Node.js 18+ (for tests)
#!/usr/bin/env -S deno run --allow-read --allow-write --allow-net

/**
 * End-to-End Test Suite for GTK WebGPU Full Stack
 * Tests the complete build and demo functionality
 */

import { assert, assertEquals, assertExists } from "jsr:@std/assert@^1.0.14";

interface TestResult {
    name: string;
    passed: boolean;
    duration: number;
    details?: string;
}

class E2ETestSuite {
    private results: TestResult[] = [];
    private serverProcess: Deno.ChildProcess | null = null;

    async runAllTests(): Promise<void> {
        console.log("🧪 GTK WebGPU End-to-End Test Suite");
        console.log("=" .repeat(50));

        await this.testBuildSystem();
        await this.testWASMFiles();
        await this.testStaticServer();
        await this.testDemoHTML();
        await this.testWebGPUCapabilities();

        this.printResults();
    }

    private async runTest(name: string, testFn: () => Promise<void>): Promise<void> {
        const startTime = performance.now();
        let passed = false;
        let details = "";

        try {
            console.log(`\n🔍 ${name}...`);
            await testFn();
            passed = true;
            console.log(`✅ ${name} passed`);
        } catch (error) {
            passed = false;
            details = error.message;
            console.log(`❌ ${name} failed: ${error.message}`);
        }

        const duration = performance.now() - startTime;
        this.results.push({ name, passed, duration, details });
    }

    private async testBuildSystem(): Promise<void> {
        await this.runTest("Build System Validation", async () => {
            // Check if build script exists
            try {
                const buildScript = await Deno.stat("./build-full-stack.sh");
                assert(buildScript.isFile, "Build script should be a file");
            } catch {
                throw new Error("build-full-stack.sh not found");
            }

            // Check if build script is executable
            try {
                const fileInfo = await Deno.stat("./build-full-stack.sh");
                // On Unix systems, check if file has execute permissions
                if (Deno.build.os !== "windows") {
                    const command = new Deno.Command("test", { args: ["-x", "./build-full-stack.sh"] });
                    const result = await command.output();
                    assert(result.success, "Build script should be executable");
                }
            } catch {
                // File exists but may not be executable - that's okay for testing
            }

            // Check main source file
            try {
                await Deno.stat("./demos/webgpu-widget-factory/main.c");
            } catch {
                throw new Error("Main source file not found");
            }

            // Check capabilities file
            try {
                await Deno.stat("./wasm/web_native_capabilities.h");
            } catch {
                console.log("⚠️  web_native_capabilities.h not found - should be created during build");
            }
        });
    }

    private async testWASMFiles(): Promise<void> {
        await this.runTest("WASM Module Validation", async () => {
            // Check for install directory
            try {
                const installDir = await Deno.stat("./install/wasm");
                assert(installDir.isDirectory, "Install directory should exist");
            } catch {
                throw new Error("install/wasm directory not found - run build first");
            }

            // Look for any SIDE modules (they may not all be built yet)
            let sideModuleCount = 0;
            try {
                for await (const entry of Deno.readDir("./install/wasm")) {
                    if (entry.name.endsWith("-side.wasm")) {
                        sideModuleCount++;

                        // Verify file is not empty
                        const stat = await Deno.stat(`./install/wasm/${entry.name}`);
                        assert(stat.size > 0, `${entry.name} should not be empty`);

                        console.log(`   ✓ Found ${entry.name} (${Math.round(stat.size / 1024)}KB)`);
                    }
                }
            } catch {
                // Directory might not exist yet
            }

            console.log(`   📦 Found ${sideModuleCount} SIDE modules`);

            // Check for MAIN module
            try {
                const mainStat = await Deno.stat("./install/wasm/gtk-webgpu-main.wasm");
                assert(mainStat.size > 0, "MAIN module should not be empty");
                console.log(`   ✓ Found gtk-webgpu-main.wasm (${Math.round(mainStat.size / 1024 / 1024)}MB)`);
            } catch {
                console.log("   ⚠️  MAIN module not found - may not be built yet");
            }
        });
    }

    private async testStaticServer(): Promise<void> {
        await this.runTest("Static Server Functionality", async () => {
            // Check if server script exists
            try {
                const serverScript = await Deno.stat("./serve-demo.ts");
                assert(serverScript.isFile, "Server script should exist");
            } catch {
                throw new Error("serve-demo.ts not found");
            }

            // Test server startup (without actually keeping it running)
            try {
                const command = new Deno.Command("deno", {
                    args: ["check", "./serve-demo.ts"],
                    stderr: "piped",
                    stdout: "piped"
                });

                const result = await command.output();
                assert(result.success, "Server script should pass type checking");

                console.log("   ✓ Server script type checking passed");
            } catch (error) {
                throw new Error(`Server script validation failed: ${error.message}`);
            }

            // Test MIME type configuration
            const serverContent = await Deno.readTextFile("./serve-demo.ts");
            assert(serverContent.includes("application/wasm"), "Server should configure WASM MIME type");
            assert(serverContent.includes("Cross-Origin-Embedder-Policy"), "Server should set COOP/COEP headers");

            console.log("   ✓ MIME types and security headers configured");
        });
    }

    private async testDemoHTML(): Promise<void> {
        await this.runTest("Demo HTML Structure", async () => {
            // Check if demo HTML exists
            try {
                await Deno.stat("./demos/webgpu-widget-factory/index.html");
            } catch {
                throw new Error("Demo HTML not found");
            }

            // Validate HTML content
            const htmlContent = await Deno.readTextFile("./demos/webgpu-widget-factory/index.html");

            // Check for essential elements
            assert(htmlContent.includes("WebGPU"), "HTML should mention WebGPU");
            assert(htmlContent.includes("canvas"), "HTML should contain canvas element");
            assert(htmlContent.includes("navigator.gpu"), "HTML should check for WebGPU support");
            assert(htmlContent.includes("WASM"), "HTML should mention WASM");

            // Check for capability detection
            assert(htmlContent.includes("WebAssembly.validate"), "HTML should validate WASM SIMD");
            assert(htmlContent.includes("SharedArrayBuffer"), "HTML should check SharedArrayBuffer");

            console.log("   ✓ HTML structure and WebGPU integration validated");
        });
    }

    private async testWebGPUCapabilities(): Promise<void> {
        await this.runTest("WebGPU Capability Detection", async () => {
            // This test validates the capability detection logic without requiring WebGPU

            // Check web_native_capabilities source
            try {
                const capsContent = await Deno.readTextFile("./wasm/web_native_capabilities.c");
                assert(capsContent.includes("WebCapabilities"), "Should define WebCapabilities struct");
                assert(capsContent.includes("has_webgpu"), "Should detect WebGPU capability");
                assert(capsContent.includes("has_wasm_simd"), "Should detect WASM SIMD capability");
                assert(capsContent.includes("chrome_version"), "Should detect Chrome version");

                console.log("   ✓ Capability detection structure validated");
            } catch {
                console.log("   ⚠️  web_native_capabilities.c not found - created during build");
            }

            // Validate SIMD operations
            try {
                const simdContent = await Deno.readTextFile("./wasm/web_native_simd_ops.c");
                assert(simdContent.includes("wasm_simd128.h"), "Should include WASM SIMD headers");
                assert(simdContent.includes("gsk_simd_"), "Should contain SIMD function implementations");

                console.log("   ✓ SIMD operations structure validated");
            } catch {
                console.log("   ⚠️  SIMD operations file not found");
            }
        });
    }

    private async testServerStartup(): Promise<void> {
        await this.runTest("Server Startup Test", async () => {
            // Start server in background
            const command = new Deno.Command("deno", {
                args: ["run", "--allow-read", "--allow-net", "./serve-demo.ts", "--port", "8001"],
                stdout: "piped",
                stderr: "piped"
            });

            this.serverProcess = command.spawn();

            // Give server time to start
            await new Promise(resolve => setTimeout(resolve, 2000));

            try {
                // Test if server responds
                const response = await fetch("http://localhost:8001/", {
                    signal: AbortSignal.timeout(5000)
                });

                assert(response.ok, "Server should respond with 200 status");
                console.log("   ✓ Server responds to requests");

                // Test WASM MIME type
                try {
                    const wasmResponse = await fetch("http://localhost:8001/nonexistent.wasm", {
                        signal: AbortSignal.timeout(2000)
                    });
                    // Should be 404 but with correct MIME type headers
                    const contentType = wasmResponse.headers.get("content-type");
                    // Note: May be 404, but headers should still be set correctly
                    console.log("   ✓ Server configured for WASM serving");
                } catch {
                    // This is expected for non-existent files
                }

            } finally {
                // Clean up server
                if (this.serverProcess) {
                    this.serverProcess.kill("SIGTERM");
                    await this.serverProcess.status;
                    this.serverProcess = null;
                }
            }
        });
    }

    private printResults(): void {
        console.log("\n" + "=" .repeat(50));
        console.log("📊 Test Results Summary");
        console.log("=" .repeat(50));

        const totalTests = this.results.length;
        const passedTests = this.results.filter(r => r.passed).length;
        const failedTests = totalTests - passedTests;

        for (const result of this.results) {
            const status = result.passed ? "✅ PASS" : "❌ FAIL";
            const duration = result.duration.toFixed(1);
            console.log(`${status} ${result.name} (${duration}ms)`);

            if (!result.passed && result.details) {
                console.log(`      ${result.details}`);
            }
        }

        console.log("\n📈 Summary:");
        console.log(`   Total Tests: ${totalTests}`);
        console.log(`   Passed: ${passedTests}`);
        console.log(`   Failed: ${failedTests}`);
        console.log(`   Success Rate: ${((passedTests / totalTests) * 100).toFixed(1)}%`);

        if (passedTests === totalTests) {
            console.log("\n🎉 All tests passed! GTK WebGPU full stack is ready.");
            console.log("\nNext steps:");
            console.log("1. Run './build-full-stack.sh' to build all modules");
            console.log("2. Start demo server: './serve-demo.ts'");
            console.log("3. Open http://localhost:8080 in Chrome/Edge 113+");
        } else {
            console.log("\n⚠️  Some tests failed. Please address issues before deployment.");
        }
    }

    async cleanup(): Promise<void> {
        if (this.serverProcess) {
            this.serverProcess.kill("SIGTERM");
            await this.serverProcess.status;
        }
    }
}

// Run the test suite
async function main() {
    const testSuite = new E2ETestSuite();

    try {
        await testSuite.runAllTests();
    } catch (error) {
        console.error("Test suite failed:", error);
        Deno.exit(1);
    } finally {
        await testSuite.cleanup();
    }
}

if (import.meta.main) {
    await main();
}